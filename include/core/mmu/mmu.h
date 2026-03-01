#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>

namespace ps5 {

// Page table entry flags
enum class PageFlags : uint64_t {
    PRESENT = 1ULL << 0,
    WRITABLE = 1ULL << 1,
    USER_ACCESSIBLE = 1ULL << 2,
    WRITE_THROUGH = 1ULL << 3,
    CACHE_DISABLED = 1ULL << 4,
    ACCESSED = 1ULL << 5,
    DIRTY = 1ULL << 6,
    HUGE_PAGE = 1ULL << 7,
    GLOBAL = 1ULL << 8,
    NO_EXECUTE = 1ULL << 63
};

// Page table entry
struct PageTableEntry {
    uint64_t value;
    
    bool isPresent() const { return value & static_cast<uint64_t>(PageFlags::PRESENT); }
    bool isWritable() const { return value & static_cast<uint64_t>(PageFlags::WRITABLE); }
    bool isUserAccessible() const { return value & static_cast<uint64_t>(PageFlags::USER_ACCESSIBLE); }
    bool isNoExecute() const { return value & static_cast<uint64_t>(PageFlags::NO_EXECUTE); }
    uint64_t getPhysicalAddress() const { return value & 0x000FFFFFFFFFF000ULL; }
    void setPhysicalAddress(uint64_t addr) { value = (value & ~0x000FFFFFFFFFF000ULL) | (addr & 0x000FFFFFFFFFF000ULL); }
    void setFlags(PageFlags flags) { value |= static_cast<uint64_t>(flags); }
    void clearFlags(PageFlags flags) { value &= ~static_cast<uint64_t>(flags); }
};

// Page table levels (4-level x86-64)
constexpr int PAGE_TABLE_LEVELS = 4;
constexpr int PAGE_SIZE_BITS = 12;
constexpr size_t PAGE_SIZE = 1ULL << PAGE_SIZE_BITS;
constexpr size_t PAGE_MASK = PAGE_SIZE - 1;

// Page table structure
class PageTable {
public:
    PageTable();
    ~PageTable();
    
    // Initialization
    bool initialize(uint64_t physical_memory_size = 16ULL * 1024 * 1024 * 1024); // 16GB
    void reset();
    
    // Address translation
    bool translateVirtual(uint64_t virt_addr, uint64_t& phys_addr, bool write = false);
    uint64_t virtualToPhysical(uint64_t virt_addr);
    
    // Memory allocation
    uint64_t allocatePage(uint64_t virt_addr, bool writable = true, bool user = false);
    void freePage(uint64_t virt_addr);
    
    // Page table manipulation
    PageTableEntry& getEntry(uint64_t virt_addr, bool create = false);
    void flushTLB();
    void flushTLBEntry(uint64_t virt_addr);
    
    // Memory access
    uint8_t read8(uint64_t virt_addr);
    uint16_t read16(uint64_t virt_addr);
    uint32_t read32(uint64_t virt_addr);
    uint64_t read64(uint64_t virt_addr);
    void write8(uint64_t virt_addr, uint8_t val);
    void write16(uint64_t virt_addr, uint16_t val);
    void write32(uint64_t virt_addr, uint32_t val);
    void write64(uint64_t virt_addr, uint64_t val);
    
    // Physical memory access
    uint8_t* getPhysicalMemory() { return physical_memory_.get(); }
    size_t getPhysicalMemorySize() const { return physical_memory_size_; }
    
    // Statistics
    uint64_t getAllocatedPages() const { return allocated_pages_; }
    
private:
    // Page table structures
    struct PageTableLevel {
        std::vector<PageTableEntry> entries;
        uint64_t base_address;
    };
    
    // Page table hierarchy
    PageTableLevel pml4_;
    PageTableLevel pdpt_;
    PageTableLevel pd_;
    PageTableLevel pt_;
    
    // Physical memory
    std::unique_ptr<uint8_t[]> physical_memory_;
    size_t physical_memory_size_;
    
    // Allocated pages
    std::unordered_map<uint64_t, uint64_t> virt_to_phys_;
    std::unordered_map<uint64_t, uint64_t> phys_to_virt_;
    uint64_t allocated_pages_;
    
    // Lock
    std::mutex mutex_;
    
    // Helper functions
    uint64_t getPML4Index(uint64_t virt_addr) const;
    uint64_t getPDPTIndex(uint64_t virt_addr) const;
    uint64_t getPDIndex(uint64_t virt_addr) const;
    uint64_t getPTIndex(uint64_t virt_addr) const;
    uint64_t getOffset(uint64_t virt_addr) const;
};

} // namespace ps5