#include "core/cpu/cpu.h"
#include "core/mmu/mmu.h"
#include "core/cpu/jit/jit.h"
#include <cstring>
#include <iostream>

namespace ps5 {

CPU::CPU() : state_(), running_(false), avx2_supported_(false), mem_(nullptr), jit_(nullptr), page_table_(nullptr) {
}

CPU::~CPU() {
    halt();
    delete jit_;
    delete page_table_;
}

bool CPU::initialize() {
    // Check AVX2 support
    uint32_t eax, ebx, ecx, edx;
    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1), "c"(0)
    );
    avx2_supported_ = (ecx & (1 << 5)) != 0;
    
    // Initialize page table
    page_table_ = new PageTable();
    if (!page_table_->initialize()) {
        delete page_table_;
        page_table_ = nullptr;
        return false;
    }
    
    // Initialize JIT
    jit_ = new JIT();
    if (!jit_->initialize()) {
        delete jit_;
        jit_ = nullptr;
        return false;
    }
    
    // Reset CPU
    reset();
    
    return true;
}

void CPU::reset() {
    // Reset registers
    std::memset(&state_.regs, 0, sizeof(CPURegisters));
    state_.regs.rip = 0x100000; // Start at 1MB
    
    // Reset flags
    state_.eflags = 0x2; // Reserved bit set
    
    // Reset control registers
    state_.cr0 = 0x80010033; // Enable paging, cache
    state_.cr3 = 0; // Page table base
    state_.cr4 = 0x36F0; // Enable SSE, AVX
    
    // Reset MSRs
    state_.msr_star = 0;
    state_.msr_lstar = 0;
    state_.msr_cstar = 0;
    state_.msr_syscall_flag = 0;
    state_.msr_kernel_gs_base = 0;
    
    // Reset page table base
    state_.page_table_base = 0;
    
    running_ = false;
}

void CPU::run() {
    running_ = true;
    while (running_) {
        step();
    }
}

void CPU::step() {
    if (!running_) return;
    
    // Get current instruction
    Instruction instr = decodeInstruction(state_.regs.rip);
    
    // Execute instruction
    executeInstruction(instr);
    
    // Update RIP
    state_.regs.rip += instr.size;
}

void CPU::halt() {
    running_ = false;
}

void CPU::interrupt(CPUException ex, uint32_t code) {
    handleException(ex, code);
}

uint8_t CPU::read8(uint64_t addr) {
    return mem_ ? mem_->read8(addr) : page_table_->read8(addr);
}

uint16_t CPU::read16(uint64_t addr) {
    return mem_ ? mem_->read16(addr) : page_table_->read16(addr);
}

uint32_t CPU::read32(uint64_t addr) {
    return mem_ ? mem_->read32(addr) : page_table_->read32(addr);
}

uint64_t CPU::read64(uint64_t addr) {
    return mem_ ? mem_->read64(addr) : page_table_->read64(addr);
}

void CPU::write8(uint64_t addr, uint8_t val) {
    if (mem_) {
        mem_->write8(addr, val);
    } else {
        page_table_->write8(addr, val);
    }
}

void CPU::write16(uint64_t addr, uint16_t val) {
    if (mem_) {
        mem_->write16(addr, val);
    } else {
        page_table_->write16(addr, val);
    }
}

void CPU::write32(uint64_t addr, uint32_t val) {
    if (mem_) {
        mem_->write32(addr, val);
    } else {
        page_table_->write32(addr, val);
    }
}

void CPU::write64(uint64_t addr, uint64_t val) {
    if (mem_) {
        mem_->write64(addr, val);
    } else {
        page_table_->write64(addr, val);
    }
}

void CPU::compileBlock(uint64_t start, uint64_t end) {
    if (jit_) {
        jit_->compileBlock(start, end);
    }
}

void CPU::executeCompiledBlock(uint64_t start) {
    if (jit_) {
        void* code = jit_->getCompiledBlock(start);
        if (code) {
            jit_->execute(code, start);
        }
    }
}

void CPU::invalidateBlock(uint64_t addr) {
    if (jit_) {
        jit_->invalidateBlock(addr);
    }
}

void CPU::setMemoryCallback(uint64_t addr, uint64_t size,
                           std::function<uint64_t(uint64_t)> read_cb,
                           std::function<void(uint64_t, uint64_t)> write_cb) {
    if (!mem_) {
        mem_ = new MemoryInterface();
    }
    mem_->setCallback(addr, size, read_cb, write_cb);
}

CPU::Instruction CPU::decodeInstruction(uint64_t addr) {
    Instruction instr;
    instr.addr = addr;
    instr.size = 1;
    instr.opcode = read8(addr);
    instr.modrm = 0;
    instr.sib = 0;
    instr.disp = 0;
    instr.imm = 0;
    instr.name = "UNKNOWN";
    
    // Simple instruction decoder
    switch (instr.opcode) {
        case 0x48: // REX.W prefix
            instr.size++;
            instr.opcode = read8(addr + 1);
            // Fall through
        case 0x89: // MOV r64, r64
            instr.name = "MOV";
            if (addr + 2 < 0x100000) {
                instr.modrm = read8(addr + 1);
                instr.size = 2;
            }
            break;
        case 0xB8: // MOV r64, imm64
            instr.name = "MOV";
            instr.size = 10;
            break;
        case 0xFF: // INC/DEC/CALL/JMP
            instr.name = "FF";
            if (addr + 2 < 0x100000) {
                instr.modrm = read8(addr + 1);
                instr.size = 2;
            }
            break;
        case 0xE8: // CALL rel32
            instr.name = "CALL";
            instr.size = 5;
            break;
        case 0xE9: // JMP rel32
            instr.name = "JMP";
            instr.size = 5;
            break;
        case 0xC3: // RET
            instr.name = "RET";
            break;
        case 0x90: // NOP
            instr.name = "NOP";
            break;
        case 0xCC: // INT3
            instr.name = "INT3";
            break;
        case 0xCD: // INT
            instr.name = "INT";
            instr.size = 2;
            break;
        case 0xCF: // IRET
            instr.name = "IRET";
            break;
        case 0xFA: // CLI
            instr.name = "CLI";
            break;
        case 0xFB: // STI
            instr.name = "STI";
            break;
        case 0xFC: // CLD
            instr.name = "CLD";
            break;
        case 0xFD: // STD
            instr.name = "STD";
            break;
        case 0x0F: // Two-byte opcode
            if (addr + 1 < 0x100000) {
                uint8_t opcode2 = read8(addr + 1);
                switch (opcode2) {
                    case 0x80: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x81: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x82: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x83: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x84: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x85: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x86: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x87: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x88: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x89: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x8A: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x8B: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x8C: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x8D: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x8E: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x8F: // JCC rel32
                        instr.name = "JCC";
                        instr.size = 6;
                        break;
                    case 0x31: // XOR r64, r64
                        instr.name = "XOR";
                        instr.size = 2;
                        break;
                    case 0x39: // CMP r64, r64
                        instr.name = "CMP";
                        instr.size = 2;
                        break;
                    case 0x3B: // CMP r64, r64
                        instr.name = "CMP";
                        instr.size = 2;
                        break;
                    case 0x40: // INC r64
                        instr.name = "INC";
                        instr.size = 2;
                        break;
                    case 0x48: // DEC r64
                        instr.name = "DEC";
                        instr.size = 2;
                        break;
                    case 0x63: // MOVSXD r64, r32/m32
                        instr.name = "MOVSXD";
                        instr.size = 2;
                        break;
                    case 0x84: // TEST r64, r64
                        instr.name = "TEST";
                        instr.size = 2;
                        break;
                    case 0x85: // TEST r64, r64
                        instr.name = "TEST";
                        instr.size = 2;
                        break;
                    case 0x8B: // MOV r64, r/m64
                        instr.name = "MOV";
                        instr.size = 2;
                        break;
                    case 0x8D: // LEA r64, m
                        instr.name = "LEA";
                        instr.size = 2;
                        break;
                    case 0x8F: // POP r64
                        instr.name = "POP";
                        instr.size = 2;
                        break;
                    case 0x90: // XCHG r64, r64
                        instr.name = "XCHG";
                        instr.size = 2;
                        break;
                    case 0x99: // CQO
                        instr.name = "CQO";
                        break;
                    case 0xA1: // MOV r64, m64
                        instr.name = "MOV";
                        instr.size = 10;
                        break;
                    case 0xA3: // MOV m64, r64
                        instr.name = "MOV";
                        instr.size = 10;
                        break;
                    case 0xA9: // TEST r64, imm64
                        instr.name = "TEST";
                        instr.size = 10;
                        break;
                    case 0xB8: // MOV r64, imm64
                        instr.name = "MOV";
                        instr.size = 10;
                        break;
                    case 0xC1: // SHL/SHR/SAR r64, imm8
                        instr.name = "SHIFT";
                        instr.size = 4;
                        break;
                    case 0xC3: // RET
                        instr.name = "RET";
                        break;
                    case 0xC7: // MOV m64, imm64
                        instr.name = "MOV";
                        instr.size = 14;
                        break;
                    case 0xC9: // LEAVE
                        instr.name = "LEAVE";
                        break;
                    case 0xD3: // SHL/SHR/SAR r64, CL
                        instr.name = "SHIFT";
                        instr.size = 2;
                        break;
                    case 0xE8: // CALL rel32
                        instr.name = "CALL";
                        instr.size = 5;
                        break;
                    case 0xE9: // JMP rel32
                        instr.name = "JMP";
                        instr.size = 5;
                        break;
                    case 0xEB: // JMP rel8
                        instr.name = "JMP";
                        instr.size = 2;
                        break;
                    case 0xF7: // TEST/NOT/NEG/MUL/IMUL/DIV/IDIV r64
                        instr.name = "MUL";
                        instr.size = 2;
                        break;
                    case 0xFF: // INC/DEC/CALL/JMP r64
                        instr.name = "FF";
                        instr.size = 2;
                        break;
                }
            }
            break;
    }
    
    return instr;
}

void CPU::executeInstruction(const Instruction& instr) {
    switch (instr.opcode) {
        case 0x90: // NOP
            break;
        case 0xC3: // RET
            state_.regs.rsp -= 8;
            state_.regs.rip = read64(state_.regs.rsp);
            break;
        case 0xCC: // INT3
            interrupt(CPUException::BREAKPOINT);
            break;
        case 0xCD: // INT
            interrupt(static_cast<CPUException>(read8(instr.addr + 1)));
            break;
        case 0xCF: // IRET
            state_.regs.rsp -= 8;
            state_.regs.rip = read64(state_.regs.rsp);
            state_.regs.rsp -= 8;
            state_.eflags = read32(state_.regs.rsp);
            break;
        case 0xFA: // CLI
            state_.interrupt_flag = false;
            break;
        case 0xFB: // STI
            state_.interrupt_flag = true;
            break;
        case 0xFC: // CLD
            state_.direction_flag = false;
            break;
        case 0xFD: // STD
            state_.direction_flag = true;
            break;
        case 0x48: // REX.W prefix
        case 0x49:
        case 0x4A:
        case 0x4B:
        case 0x4C:
        case 0x4D:
        case 0x4E:
        case 0x4F: {
            uint8_t opcode = read8(instr.addr + 1);
            switch (opcode) {
                case 0x89: // MOV r64, r64
                    break;
                case 0x8B: // MOV r64, r/m64
                    break;
                case 0x8D: // LEA r64, m
                    break;
                case 0x8F: // POP r64
                    break;
                case 0x31: // XOR r64, r64
                    break;
                case 0x39: // CMP r64, r64
                    break;
                case 0x3B: // CMP r64, r64
                    break;
                case 0xFF: // INC/DEC/CALL/JMP r64
                    break;
            }
            break;
        }
        case 0x89: // MOV r64, r64
            break;
        case 0x8B: // MOV r64, r/m64
            break;
        case 0x8D: // LEA r64, m
            break;
        case 0x8F: // POP r64
            break;
        case 0x31: // XOR r64, r64
            break;
        case 0x39: // CMP r64, r64
            break;
        case 0x3B: // CMP r64, r64
            break;
        case 0xFF: // INC/DEC/CALL/JMP r64
            break;
        case 0xB8: // MOV r64, imm64
            break;
        case 0xE8: // CALL rel32
            break;
        case 0xE9: // JMP rel32
            break;
        case 0xEB: // JMP rel8
            break;
        case 0xF7: // TEST/NOT/NEG/MUL/IMUL/DIV/IDIV r64
            break;
        case 0x0F: {
            uint8_t opcode2 = read8(instr.addr + 1);
            switch (opcode2) {
                case 0x80: // JCC rel32
                    break;
                case 0x84: // TEST r64, r64
                    break;
                case 0x85: // TEST r64, r64
                    break;
                case 0x8B: // MOV r64, r/m64
                    break;
                case 0x8D: // LEA r64, m
                    break;
                case 0x8F: // POP r64
                    break;
                case 0x90: // XCHG r64, r64
                    break;
                case 0x99: // CQO
                    break;
                case 0xA1: // MOV r64, m64
                    break;
                case 0xA3: // MOV m64, r64
                    break;
                case 0xA9: // TEST r64, imm64
                    break;
                case 0xB8: // MOV r64, imm64
                    break;
                case 0xC1: // SHL/SHR/SAR r64, imm8
                    break;
                case 0xC3: // RET
                    break;
                case 0xC7: // MOV m64, imm64
                    break;
                case 0xC9: // LEAVE
                    break;
                case 0xD3: // SHL/SHR/SAR r64, CL
                    break;
                case 0xE8: // CALL rel32
                    break;
                case 0xE9: // JMP rel32
                    break;
                case 0xEB: // JMP rel8
                    break;
                case 0xF7: // TEST/NOT/NEG/MUL/IMUL/DIV/IDIV r64
                    break;
                case 0xFF: // INC/DEC/CALL/JMP r64
                    break;
            }
            break;
        }
    }
}

void CPU::handleException(CPUException ex, uint32_t code) {
    // Save current state
    state_.regs.rsp -= 8;
    write64(state_.regs.rsp, state_.eflags);
    
    state_.regs.rsp -= 8;
    write64(state_.regs.rsp, state_.regs.rip);
    
    // Get exception handler from IDT
    uint64_t idt_base = read64(0); // Simplified
    uint64_t handler = idt_base + static_cast<uint32_t>(ex) * 16;
    
    // Call handler
    state_.regs.rsp -= 8;
    write64(state_.regs.rsp, code);
    state_.regs.rip = handler;
}

} // namespace ps5