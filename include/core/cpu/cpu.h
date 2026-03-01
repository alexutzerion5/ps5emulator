#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <functional>

namespace ps5 {

// CPU Registers (x86-64)
struct CPURegisters {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip;
    uint32_t eflags;
    
    // AVX2 registers (16 x 256-bit)
    alignas(32) uint256_t ymm[16];
};

// CPU State
struct CPUState {
    CPURegisters regs;
    bool interrupt_flag;
    bool trap_flag;
    bool direction_flag;
    bool overflow_flag;
    bool sign_flag;
    bool zero_flag;
    bool carry_flag;
    bool parity_flag;
    
    // Control registers
    uint64_t cr0, cr2, cr3, cr4;
    
    // Model-specific registers
    uint64_t msr_star, msr_lstar, msr_cstar, msr_syscall_flag;
    uint64_t msr_kernel_gs_base;
    
    // Page table base
    uint64_t page_table_base;
};

// CPU Exception types
enum class CPUException : uint32_t {
    NONE = 0,
    DIVIDE_ERROR = 0,
    DEBUG = 1,
    NON_MASKABLE_INTERRUPT = 2,
    BREAKPOINT = 3,
    OVERFLOW = 4,
    BOUND_RANGE = 5,
    INVALID_OPCODE = 6,
    NO_FPU = 7,
    DOUBLE_FAULT = 8,
    COPROC_SEGMENT_OVERRUN = 9,
    INVALID_TSS = 10,
    SEGMENT_NOT_PRESENT = 11,
    STACK_SEGMENT_FAULT = 12,
    GENERAL_PROTECTION = 13,
    PAGE_FAULT = 14,
    FLOATING_POINT_ERROR = 16,
    ALIGNMENT_CHECK = 17,
    MACHINE_CHECK = 18,
    SIMD_FLOATING_POINT = 19,
    VIRTUALIZATION = 20,
    SECURITY_EXCEPTION = 30
};

// CPU Interface
class CPU {
public:
    CPU();
    ~CPU();
    
    // Initialization
    bool initialize();
    void reset();
    
    // Execution
    void run();
    void step();
    void halt();
    void interrupt(CPUException ex, uint32_t code = 0);
    
    // Memory access
    uint8_t read8(uint64_t addr);
    uint16_t read16(uint64_t addr);
    uint32_t read32(uint64_t addr);
    uint64_t read64(uint64_t addr);
    void write8(uint64_t addr, uint8_t val);
    void write16(uint64_t addr, uint16_t val);
    void write32(uint64_t addr, uint32_t val);
    void write64(uint64_t addr, uint64_t val);
    
    // Register access
    const CPURegisters& getRegisters() const { return state_.regs; }
    CPURegisters& getRegisters() { return state_.regs; }
    const CPUState& getState() const { return state_; }
    CPUState& getState() { return state_; }
    
    // JIT compilation
    void compileBlock(uint64_t start, uint64_t end);
    void executeCompiledBlock(uint64_t start);
    void invalidateBlock(uint64_t addr);
    
    // AVX2 support
    bool hasAVX2() const { return avx2_supported_; }
    
    // Callbacks
    void setMemoryCallback(uint64_t addr, uint64_t size,
                          std::function<uint64_t(uint64_t)> read_cb,
                          std::function<void(uint64_t, uint64_t)> write_cb);
    
private:
    CPUState state_;
    bool running_;
    bool avx2_supported_;
    
    // Instruction decoding
    struct Instruction {
        uint64_t addr;
        uint8_t opcode;
        uint8_t modrm;
        uint8_t sib;
        uint64_t disp;
        uint64_t imm;
        int size;
        const char* name;
    };
    
    Instruction decodeInstruction(uint64_t addr);
    void executeInstruction(const Instruction& instr);
    
    // Exception handling
    void handleException(CPUException ex, uint32_t code);
    
    // Memory management
    class MemoryInterface* mem_;
    
    // JIT compiler
    class JIT* jit_;
    
    // Page table
    class PageTable* page_table_;
};

} // namespace ps5