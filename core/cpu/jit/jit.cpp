#include "core/cpu/jit/jit.h"
#include <cstring>
#include <cstdlib>
#include <iostream>

namespace ps5 {

JIT::JIT() : cache_size_(0), compilation_count_(0), code_buffer_(nullptr), code_buffer_pos_(0) {
}

JIT::~JIT() {
    clearCache();
    if (code_buffer_) {
        freeCode(code_buffer_, CODE_BUFFER_SIZE);
    }
}

bool JIT::initialize() {
    code_buffer_ = static_cast<uint8_t*>(allocateCode(CODE_BUFFER_SIZE));
    if (!code_buffer_) {
        return false;
    }
    code_buffer_pos_ = 0;
    return true;
}

void JIT::compileBlock(uint64_t start_addr, uint64_t end_addr) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Check if already compiled
    if (block_cache_.find(start_addr) != block_cache_.end()) {
        return;
    }
    
    // Allocate code buffer for this block
    uint8_t* code = code_buffer_ + code_buffer_pos_;
    uint8_t* code_start = code;
    
    // Generate prologue
    generatePrologue(&code);
    
    // Generate code for each instruction
    uint64_t addr = start_addr;
    while (addr < end_addr && code - code_start < CODE_BUFFER_SIZE - 1024) {
        // Simple instruction emulation
        switch (read8(addr)) {
            case 0x90: // NOP
                break;
            case 0xC3: // RET
                generateRet(&code);
                break;
            case 0xB8: // MOV rax, imm64
                generateMov64(&code, 0, read64(addr + 1));
                addr += 10;
                continue;
            case 0x48: // REX.W prefix
                if (addr + 1 < end_addr) {
                    uint8_t opcode = read8(addr + 1);
                    if (opcode == 0x89) { // MOV r64, r64
                        generateMov64Reg(&code, 0, 0);
                    }
                }
                addr += 2;
                continue;
        }
        addr++;
    }
    
    // Generate epilogue
    generateEpilogue(&code);
    
    // Store block info
    BlockInfo info;
    info.start = start_addr;
    info.end = end_addr;
    info.code = code_start;
    info.code_size = code - code_start;
    info.checksum = start_addr; // Simplified
    
    block_cache_[start_addr] = info;
    cache_size_ += info.code_size;
    compilation_count_++;
    
    code_buffer_pos_ += info.code_size;
}

void* JIT::getCompiledBlock(uint64_t addr) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = block_cache_.find(addr);
    if (it != block_cache_.end()) {
        return it->second.code;
    }
    return nullptr;
}

void JIT::invalidateBlock(uint64_t addr) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = block_cache_.find(addr);
    if (it != block_cache_.end()) {
        cache_size_ -= it->second.code_size;
        block_cache_.erase(it);
    }
}

void JIT::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    for (auto& pair : block_cache_) {
        // Code is in the shared buffer, no need to free individual blocks
    }
    block_cache_.clear();
    cache_size_ = 0;
}

void JIT::execute(void* code, uint64_t start_addr) {
    if (!code) return;
    
    // Cast to function pointer and execute
    using ExecFunc = void(*)();
    ExecFunc exec = reinterpret_cast<ExecFunc>(code);
    exec();
}

void* JIT::allocateCode(size_t size) {
    // Allocate executable memory
    #ifdef _WIN32
    return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    #else
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return ptr == MAP_FAILED ? nullptr : ptr;
    #endif
}

void JIT::freeCode(void* ptr, size_t size) {
    if (!ptr) return;
    
    #ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
    #else
    munmap(ptr, size);
    #endif
}

void JIT::generatePrologue(uint8_t** ptr) {
    // Push registers
    (*ptr)[0] = 0x50; // push rax
    (*ptr)[1] = 0x53; // push rbx
    (*ptr)[2] = 0x55; // push rbp
    (*ptr)[3] = 0x56; // push rsi
    (*ptr)[4] = 0x57; // push rdi
    (*ptr)[5] = 0x41; // push r8
    (*ptr)[6] = 0x50;
    (*ptr)[7] = 0x41; // push r9
    (*ptr)[8] = 0x51;
    (*ptr)[9] = 0x41; // push r10
    (*ptr)[10] = 0x52;
    (*ptr)[11] = 0x41; // push r11
    (*ptr)[12] = 0x53;
    (*ptr)[13] = 0x41; // push r12
    (*ptr)[14] = 0x54;
    (*ptr)[15] = 0x41; // push r13
    (*ptr)[16] = 0x55;
    (*ptr)[17] = 0x41; // push r14
    (*ptr)[18] = 0x56;
    (*ptr)[19] = 0x41; // push r15
    (*ptr)[20] = 0x57;
    (*ptr)[21] = 0x48; // sub rsp, 0x80
    (*ptr)[22] = 0x83;
    (*ptr)[23] = 0xEC;
    (*ptr)[24] = 0x80;
}

void JIT::generateEpilogue(uint8_t** ptr) {
    // Add rsp, 0x80
    (*ptr)[0] = 0x48; // add rsp, 0x80
    (*ptr)[1] = 0x83;
    (*ptr)[2] = 0xC4;
    (*ptr)[3] = 0x80;
    
    // Pop registers
    (*ptr)[4] = 0x41; // pop r15
    (*ptr)[5] = 0x5F;
    (*ptr)[6] = 0x41; // pop r14
    (*ptr)[7] = 0x5E;
    (*ptr)[8] = 0x41; // pop r13
    (*ptr)[9] = 0x5D;
    (*ptr)[10] = 0x41; // pop r12
    (*ptr)[11] = 0x5C;
    (*ptr)[12] = 0x41; // pop r11
    (*ptr)[13] = 0x5B;
    (*ptr)[14] = 0x41; // pop r10
    (*ptr)[15] = 0x5A;
    (*ptr)[16] = 0x41; // pop r9
    (*ptr)[17] = 0x59;
    (*ptr)[18] = 0x41; // pop r8
    (*ptr)[19] = 0x58;
    (*ptr)[20] = 0x5F; // pop rdi
    (*ptr)[21] = 0x5E; // pop rsi
    (*ptr)[22] = 0x5D; // pop rbp
    (*ptr)[23] = 0x5B; // pop rbx
    (*ptr)[24] = 0x58; // pop rax
    (*ptr)[25] = 0xC3; // ret
}

void JIT::generateMov64(uint8_t** ptr, uint64_t reg, uint64_t val) {
    (*ptr)[0] = 0x48; // MOV rax, imm64
    (*ptr)[1] = 0xB8 | (reg & 7);
    std::memcpy(*ptr + 2, &val, 8);
    *ptr += 10;
}

void JIT::generateMov64Mem(uint8_t** ptr, uint64_t base, uint64_t disp, uint64_t reg) {
    (*ptr)[0] = 0x48; // MOV [base+disp], rax
    (*ptr)[1] = 0x89;
    (*ptr)[2] = 0x04 | ((reg & 7) << 3);
    (*ptr)[3] = 0x25;
    std::memcpy(*ptr + 4, &disp, 8);
    *ptr += 12;
}

void JIT::generateMov64Reg(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0x48; // MOV rax, rbx
    (*ptr)[1] = 0x89;
    (*ptr)[2] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 3;
}

void JIT::generateAdd64(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0x48; // ADD rax, rbx
    (*ptr)[1] = 0x01;
    (*ptr)[2] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 3;
}

void JIT::generateSub64(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0x48; // SUB rax, rbx
    (*ptr)[1] = 0x29;
    (*ptr)[2] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 3;
}

void JIT::generateAnd64(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0x48; // AND rax, rbx
    (*ptr)[1] = 0x21;
    (*ptr)[2] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 3;
}

void JIT::generateOr64(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0x48; // OR rax, rbx
    (*ptr)[1] = 0x09;
    (*ptr)[2] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 3;
}

void JIT::generateXor64(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0x48; // XOR rax, rbx
    (*ptr)[1] = 0x31;
    (*ptr)[2] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 3;
}

void JIT::generateCmp64(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0x48; // CMP rax, rbx
    (*ptr)[1] = 0x39;
    (*ptr)[2] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 3;
}

void JIT::generateJmp(uint8_t** ptr, uint64_t target) {
    int32_t rel = static_cast<int32_t>(target - (reinterpret_cast<uint64_t>(*ptr) + 5));
    (*ptr)[0] = 0xE9; // JMP rel32
    std::memcpy(*ptr + 1, &rel, 4);
    *ptr += 5;
}

void JIT::generateJcc(uint8_t** ptr, uint8_t condition, uint64_t target) {
    (*ptr)[0] = 0x0F; // JCC rel32
    (*ptr)[1] = 0x80 | condition;
    int32_t rel = static_cast<int32_t>(target - (reinterpret_cast<uint64_t>(*ptr) + 6));
    std::memcpy(*ptr + 2, &rel, 4);
    *ptr += 6;
}

void JIT::generateCall(uint8_t** ptr, uint64_t target) {
    int32_t rel = static_cast<int32_t>(target - (reinterpret_cast<uint64_t>(*ptr) + 5));
    (*ptr)[0] = 0xE8; // CALL rel32
    std::memcpy(*ptr + 1, &rel, 4);
    *ptr += 5;
}

void JIT::generateRet(uint8_t** ptr) {
    (*ptr)[0] = 0xC3; // RET
    *ptr += 1;
}

void JIT::generateVmovdqa(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0xC5; // VMODDQA ymm0, ymm1
    (*ptr)[1] = 0xFD;
    (*ptr)[2] = 0x6F;
    (*ptr)[3] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 4;
}

void JIT::generateVaddq(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0xC5; // VADDQ ymm0, ymm1
    (*ptr)[1] = 0xFD;
    (*ptr)[2] = 0xFE;
    (*ptr)[3] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 4;
}

void JIT::generateVsubq(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0xC5; // VSUBQ ymm0, ymm1
    (*ptr)[1] = 0xFD;
    (*ptr)[2] = 0xEB;
    (*ptr)[3] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 4;
}

void JIT::generateVandq(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0xC5; // VANDQ ymm0, ymm1
    (*ptr)[1] = 0xFD;
    (*ptr)[2] = 0xDB;
    (*ptr)[3] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 4;
}

void JIT::generateVorq(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0xC5; // VORQ ymm0, ymm1
    (*ptr)[1] = 0xFD;
    (*ptr)[2] = 0xEB;
    (*ptr)[3] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 4;
}

void JIT::generateVxorq(uint8_t** ptr, uint64_t dst, uint64_t src) {
    (*ptr)[0] = 0xC5; // VXORQ ymm0, ymm1
    (*ptr)[1] = 0xFD;
    (*ptr)[2] = 0xEF;
    (*ptr)[3] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    *ptr += 4;
}

void JIT::generateVpermq(uint8_t** ptr, uint64_t dst, uint64_t src, uint64_t shuffle) {
    (*ptr)[0] = 0xC5; // VPERMQ ymm0, ymm1, imm8
    (*ptr)[1] = 0xFD;
    (*ptr)[2] = 0x00;
    (*ptr)[3] = 0xC0 | ((dst & 7) << 3) | (src & 7);
    (*ptr)[4] = static_cast<uint8_t>(shuffle);
    *ptr += 5;
}

} // namespace ps5