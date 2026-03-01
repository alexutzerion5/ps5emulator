#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>

namespace ps5 {

class JIT {
public:
    JIT();
    ~JIT();
    
    // Compilation
    bool initialize();
    void compileBlock(uint64_t start_addr, uint64_t end_addr);
    void* getCompiledBlock(uint64_t addr);
    void invalidateBlock(uint64_t addr);
    void clearCache();
    
    // Execution
    void execute(void* code, uint64_t start_addr);
    
    // Statistics
    uint64_t getCacheSize() const { return cache_size_; }
    uint64_t getCompilationCount() const { return compilation_count_; }
    
private:
    // Block information
    struct BlockInfo {
        uint64_t start;
        uint64_t end;
        void* code;
        size_t code_size;
        uint64_t checksum;
    };
    
    // AVX2 code generation
    void generatePrologue(uint8_t** ptr);
    void generateEpilogue(uint8_t** ptr);
    void generateMov64(uint8_t** ptr, uint64_t reg, uint64_t val);
    void generateMov64Mem(uint8_t** ptr, uint64_t base, uint64_t disp, uint64_t reg);
    void generateMov64Reg(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateAdd64(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateSub64(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateAnd64(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateOr64(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateXor64(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateCmp64(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateJmp(uint8_t** ptr, uint64_t target);
    void generateJcc(uint8_t** ptr, uint8_t condition, uint64_t target);
    void generateCall(uint8_t** ptr, uint64_t target);
    void generateRet(uint8_t** ptr);
    
    // AVX2 instructions
    void generateVmovdqa(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateVaddq(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateVsubq(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateVandq(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateVorq(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateVxorq(uint8_t** ptr, uint64_t dst, uint64_t src);
    void generateVpermq(uint8_t** ptr, uint64_t dst, uint64_t src, uint64_t shuffle);
    
    // Memory management
    void* allocateCode(size_t size);
    void freeCode(void* ptr, size_t size);
    
    // Cache
    std::unordered_map<uint64_t, BlockInfo> block_cache_;
    std::mutex cache_mutex_;
    uint64_t cache_size_;
    uint64_t compilation_count_;
    
    // Code buffer
    static constexpr size_t CODE_BUFFER_SIZE = 1024 * 1024; // 1MB
    uint8_t* code_buffer_;
    size_t code_buffer_pos_;
};

} // namespace ps5