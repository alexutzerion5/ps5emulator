#include "core/mmu/mmu.h"
#include <cstring>
#include <iostream>

namespace ps5 {

PageTable::PageTable() : physical_memory_size_(0), allocated_pages_(0) {
}

PageTable::~PageTable() {
}

bool PageTable::initialize(uint64_t physical_memory_size) {
    physical_memory_size_ = physical_memory_size;
    physical_memory_ = std::make_unique<uint8_t[]>(physical_memory_size);
    std::memset(physical_memory_.get(), 0, physical_memory_size);
    
    // Initialize page table entries
    pml4_.entries.resize(512);
    pdpt_.entries.resize(512);
    pd_.entries.resize(512);
    pt_.entries.resize(512);
    
    // Clear all entries
    std::memset(pml4_.entries.data(), 0, pml4_.entries.size() * sizeof(PageTableEntry));
    std::memset(pdpt_.entries.data(), 0, pdpt_.entries.size() * sizeof(PageTableEntry));
    std::memset(pd_.entries.data(), 0, pd_.entries.size() * sizeof(PageTableEntry));
    std::memset(pt_.entries.data(), 0, pt_.entries.size() * sizeof(PageTableEntry));
    
    return true;
}

void PageTable::reset() {
    std::memset(pml4_.entries.data(), 0, pml4_.entries.size() * sizeof(PageTableEntry));
    std::memset(pdpt_.entries.data(), 0, pdpt_.entries.size() * sizeof(PageTableEntry));
    std::memset(pd_.entries.data(), 0, pd_.entries.size() * sizeof(PageTableEntry));
    std::memset(pt_.entries.data(), 0, pt_.entries.size() * sizeof(PageTableEntry));
    
    virt_to_phys_.clear();
    phys_to_virt_.clear();
    allocated_pages_ = 0;
}

bool PageTable::translateVirtual(uint64_t virt_addr, uint64_t& phys_addr, bool write) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if address is mapped
    auto it = virt_to_phys_.find(virt_addr & ~PAGE_MASK);
    if (it == virt_to_phys_.end()) {
        return false;
    }
    
    phys_addr = it->second + (virt_addr & PAGE_MASK);
    return true;
}

uint64_t PageTable::virtualToPhysical(uint64_t virt_addr) {
    uint64_t phys_addr;
    if (translateVirtual(virt_addr, phys_addr)) {
        return phys_addr;
    }
    return 0;
}

uint64_t PageTable::allocatePage(uint64_t virt_addr, bool writable, bool user) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find free physical page
    uint64_t phys_addr = allocated_pages_ * PAGE_SIZE;
    if (phys_addr + PAGE_SIZE > physical_memory_size_) {
        return 0;
    }
    
    // Create page table entry
    PageTableEntry entry;
    entry.value = 0;
    entry.setPhysicalAddress(phys_addr);
    entry.setFlags(PageFlags::PRESENT);
    if (writable) {
        entry.setFlags(PageFlags::WRITABLE);
    }
    if (user) {
        entry.setFlags(PageFlags::USER_ACCESSIBLE);
    }
    
    // Map virtual to physical
    virt_to_phys_[virt_addr & ~PAGE_MASK] = phys_addr;
    phys_to_virt_[phys_addr] = virt_addr & ~PAGE_MASK;
    
    allocated_pages_++;
    
    return phys_addr;
}

void PageTable::freePage(uint64_t virt_addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = virt_to_phys_.find(virt_addr & ~PAGE_MASK);
    if (it != virt_to_phys_.end()) {
        phys_to_virt_.erase(it->second);
        virt_to_phys_.erase(it);
        allocated_pages_--;
    }
}

PageTableEntry& PageTable::getEntry(uint64_t virt_addr, bool create) {
    uint64_t pml4_idx = getPML4Index(virt_addr);
    uint64_t pdpt_idx = getPDPTIndex(virt_addr);
    uint64_t pd_idx = getPDIndex(virt_addr);
    uint64_t pt_idx = getPTIndex(virt_addr);
    
    // Simplified: return first entry
    return pt_.entries[0];
}

void PageTable::flushTLB() {
    // Simplified: clear all mappings
    virt_to_phys_.clear();
    phys_to_virt_.clear();
}

void PageTable::flushTLBEntry(uint64_t virt_addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    virt_to_phys_.erase(virt_addr & ~PAGE_MASK);
}

uint8_t PageTable::read8(uint64_t virt_addr) {
    uint64_t phys_addr;
    if (translateVirtual(virt_addr, phys_addr)) {
        return physical_memory_[phys_addr];
    }
    return 0;
}

uint16_t PageTable::read16(uint64_t virt_addr) {
    uint64_t phys_addr;
    if (translateVirtual(virt_addr, phys_addr)) {
        return *reinterpret_cast<uint16_t*>(physical_memory_.get() + phys_addr);
    }
    return 0;
}

uint32_t PageTable::read32(uint64_t virt_addr) {
    uint64_t phys_addr;
    if (translateVirtual(virt_addr, phys_addr)) {
        return *reinterpret_cast<uint32_t*>(physical_memory_.get() + phys_addr);
    }
    return 0;
}

uint64_t PageTable::read64(uint64_t virt_addr) {
    uint64_t phys_addr;
    if (translateVirtual(virt_addr, phys_addr)) {
        return *reinterpret_cast<uint64_t*>(physical_memory_.get() + phys_addr);
    }
    return 0;
}

void PageTable::write8(uint64_t virt_addr, uint8_t val) {
    uint64_t phys_addr;
    if (translateVirtual(virt_addr, phys_addr)) {
        physical_memory_[phys_addr] = val;
    }
}

void PageTable::write16(uint64_t virt_addr, uint16_t val) {
    uint64_t phys_addr;
    if (translateVirtual(virt_addr, phys_addr)) {
        *reinterpret_cast<uint16_t*>(physical_memory_.get() + phys_addr) = val;
    }
}

void PageTable::write32(uint64_t virt_addr, uint32_t val) {
    uint64_t phys_addr;
    if (translateVirtual(virt_addr, phys_addr)) {
        *reinterpret_cast<uint32_t*>(physical_memory_.get() + phys_addr) = val;
    }
}

void PageTable::write64(uint64_t virt_addr, uint64_t val) {
    uint64_t phys_addr;
    if (translateVirtual(virt_addr, phys_addr)) {
        *reinterpret_cast<uint64_t*>(physical_memory_.get() + phys_addr) = val;
    }
}

uint64_t PageTable::getPML4Index(uint64_t virt_addr) const {
    return (virt_addr >> 39) & 0x1FF;
}

uint64_t PageTable::getPDPTIndex(uint64_t virt_addr) const {
    return (virt_addr >> 30) & 0x1FF;
}

uint64_t PageTable::getPDIndex(uint64_t virt_addr) const {
    return (virt_addr >> 21) & 0x1FF;
}

uint64_t PageTable::getPTIndex(uint64_t virt_addr) const {
    return (virt_addr >> 12) & 0x1FF;
}

uint64_t PageTable::getOffset(uint64_t virt_addr) const {
    return virt_addr & 0xFFF;
}

} // namespace ps5