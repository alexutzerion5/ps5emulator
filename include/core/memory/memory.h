#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>

namespace ps5 {

// Memory region types
enum class MemoryType {
    UNMAPPED = 0,
    NORMAL = 1,
    DEVICE = 2,
    CACHE_DISABLED = 3,
    WRITE_COMBINING = 4,
    RESERVED = 5,
    BOOTLOADER = 6,
    KERNEL_CODE = 7,
    KERNEL_DATA = 8,
    GPU_MEMORY = 9,
    IO_MEMORY = 10
};

// Memory region
struct MemoryRegion {
    uint64_t base;
    uint64_t size;
    MemoryType type;
    std::string name;
    bool readable;
    bool writable;
    bool executable;
    std::function<uint64_t(uint64_t)> read_callback;
    std::function<void(uint64_t, uint64_t)> write_callback;
    
    uint64_t end() const { return base + size; }
    bool contains(uint64_t addr) const { return addr >= base && addr < end(); }
};

// Memory manager
class MemoryManager {
public:
    MemoryManager();
    ~MemoryManager();
    
    // Initialization
    bool initialize(uint64_t physical_size = 16ULL * 1024 * 1024 * 1024,
                   uint64_t virtual_size = 1ULL << 48);
    
    // Memory allocation
    uint64_t allocate(uint64_t size, MemoryType type = MemoryType::NORMAL,
                     const std::string& name = "");
    void free(uint64_t addr);
    
    // Memory mapping
    bool map(uint64_t virt_addr, uint64_t phys_addr, uint64_t size,
            MemoryType type = MemoryType::NORMAL, const std::string& name = "");
    bool unmap(uint64_t virt_addr, uint64_t size);
    
    // Memory access
    uint8_t read8(uint64_t addr);
    uint16_t read16(uint64_t addr);
    uint32_t read32(uint64_t addr);
    uint64_t read64(uint64_t addr);
    void write8(uint64_t addr, uint8_t val);
    void write16(uint64_t addr, uint16_t val);
    void write32(uint64_t addr, uint32_t val);
    void write64(uint64_t addr, uint64_t val);
    
    // Region management
    const MemoryRegion* getRegion(uint64_t addr) const;
    std::vector<MemoryRegion> getRegions() const;
    
    // Statistics
    uint64_t getTotalAllocated() const { return total_allocated_; }
    uint64_t getRegionCount() const { return regions_.size(); }
    
private:
    // Physical memory
    std::unique_ptr<uint8_t[]> physical_memory_;
    uint64_t physical_size_;
    
    // Memory regions
    std::vector<MemoryRegion> regions_;
    std::mutex mutex_;
    
    // Allocation tracking
    std::unordered_map<uint64_t, uint64_t> allocations_;
    uint64_t total_allocated_;
    
    // Helper functions
    bool findFreeRegion(uint64_t size, uint64_t& addr);
    void sortRegions();
};

} // namespace ps5