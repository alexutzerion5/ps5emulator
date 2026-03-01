#include "core/memory/memory.h"
#include <cstring>
#include <iostream>

namespace ps5 {

MemoryManager::MemoryManager() : physical_size_(0), total_allocated_(0) {
}

MemoryManager::~MemoryManager() {
}

bool MemoryManager::initialize(uint64_t physical_size, uint64_t virtual_size) {
    physical_size_ = physical_size;
    physical_memory_ = std::make_unique<uint8_t[]>(physical_size);
    std::memset(physical_memory_.get(), 0, physical_size);
    
    // Create default regions
    // Kernel code at 0x100000
    MemoryRegion kernel_code;
    kernel_code.base = 0x100000;
    kernel_code.size = 0x10000000; // 256MB
    kernel_code.type = MemoryType::KERNEL_CODE;
    kernel_code.name = "kernel_code";
    kernel_code.readable = true;
    kernel_code.writable = false;
    kernel_code.executable = true;
    regions_.push_back(kernel_code);
    
    // Kernel data at 0x10100000
    MemoryRegion kernel_data;
    kernel_data.base = 0x10100000;
    kernel_data.size = 0x20000000; // 512MB
    kernel_data.type = MemoryType::KERNEL_DATA;
    kernel_data.name = "kernel_data";
    kernel_data.readable = true;
    kernel_data.writable = true;
    kernel_data.executable = false;
    regions_.push_back(kernel_data);
    
    // GPU memory at 0x8000000000
    MemoryRegion gpu_mem;
    gpu_mem.base = 0x8000000000ULL;
    gpu_mem.size = 0x1000000000ULL; // 64GB
    gpu_mem.type = MemoryType::GPU_MEMORY;
    gpu_mem.name = "gpu_memory";
    gpu_mem.readable = true;
    gpu_mem.writable = true;
    gpu_mem.executable = false;
    regions_.push_back(gpu_mem);
    
    sortRegions();
    
    return true;
}

uint64_t MemoryManager::allocate(uint64_t size, MemoryType type, const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find free region
    uint64_t addr;
    if (!findFreeRegion(size, addr)) {
        return 0;
    }
    
    // Create region
    MemoryRegion region;
    region.base = addr;
    region.size = size;
    region.type = type;
    region.name = name;
    region.readable = true;
    region.writable = true;
    region.executable = false;
    
    regions_.push_back(region);
    sortRegions();
    
    total_allocated_ += size;
    allocations_[addr] = size;
    
    return addr;
}

void MemoryManager::free(uint64_t addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = allocations_.find(addr);
    if (it != allocations_.end()) {
        total_allocated_ -= it->second;
        
        // Remove region
        regions_.erase(std::remove_if(regions_.begin(), regions_.end(),
            [addr](const MemoryRegion& r) { return r.base == addr; }),
            regions_.end());
        
        allocations_.erase(it);
    }
}

bool MemoryManager::map(uint64_t virt_addr, uint64_t phys_addr, uint64_t size,
                       MemoryType type, const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create region
    MemoryRegion region;
    region.base = virt_addr;
    region.size = size;
    region.type = type;
    region.name = name;
    region.readable = true;
    region.writable = true;
    region.executable = false;
    
    regions_.push_back(region);
    sortRegions();
    
    return true;
}

bool MemoryManager::unmap(uint64_t virt_addr, uint64_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove region
    regions_.erase(std::remove_if(regions_.begin(), regions_.end(),
        [virt_addr, size](const MemoryRegion& r) {
            return r.base == virt_addr && r.size == size;
        }),
        regions_.end());
    
    return true;
}

uint8_t MemoryManager::read8(uint64_t addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const MemoryRegion* region = getRegion(addr);
    if (!region || !region->readable) {
        return 0;
    }
    
    if (region->read_callback) {
        return static_cast<uint8_t>(region->read_callback(addr));
    }
    
    return physical_memory_[addr];
}

uint16_t MemoryManager::read16(uint64_t addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const MemoryRegion* region = getRegion(addr);
    if (!region || !region->readable) {
        return 0;
    }
    
    if (region->read_callback) {
        return static_cast<uint16_t>(region->read_callback(addr));
    }
    
    return *reinterpret_cast<uint16_t*>(physical_memory_.get() + addr);
}

uint32_t MemoryManager::read32(uint64_t addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const MemoryRegion* region = getRegion(addr);
    if (!region || !region->readable) {
        return 0;
    }
    
    if (region->read_callback) {
        return static_cast<uint32_t>(region->read_callback(addr));
    }
    
    return *reinterpret_cast<uint32_t*>(physical_memory_.get() + addr);
}

uint64_t MemoryManager::read64(uint64_t addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const MemoryRegion* region = getRegion(addr);
    if (!region || !region->readable) {
        return 0;
    }
    
    if (region->read_callback) {
        return region->read_callback(addr);
    }
    
    return *reinterpret_cast<uint64_t*>(physical_memory_.get() + addr);
}

void MemoryManager::write8(uint64_t addr, uint8_t val) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const MemoryRegion* region = getRegion(addr);
    if (!region || !region->writable) {
        return;
    }
    
    if (region->write_callback) {
        region->write_callback(addr, val);
        return;
    }
    
    physical_memory_[addr] = val;
}

void MemoryManager::write16(uint64_t addr, uint16_t val) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const MemoryRegion* region = getRegion(addr);
    if (!region || !region->writable) {
        return;
    }
    
    if (region->write_callback) {
        region->write_callback(addr, val);
        return;
    }
    
    *reinterpret_cast<uint16_t*>(physical_memory_.get() + addr) = val;
}

void MemoryManager::write32(uint64_t addr, uint32_t val) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const MemoryRegion* region = getRegion(addr);
    if (!region || !region->writable) {
        return;
    }
    
    if (region->write_callback) {
        region->write_callback(addr, val);
        return;
    }
    
    *reinterpret_cast<uint32_t*>(physical_memory_.get() + addr) = val;
}

void MemoryManager::write64(uint64_t addr, uint64_t val) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const MemoryRegion* region = getRegion(addr);
    if (!region || !region->writable) {
        return;
    }
    
    if (region->write_callback) {
        region->write_callback(addr, val);
        return;
    }
    
    *reinterpret_cast<uint64_t*>(physical_memory_.get() + addr) = val;
}

const MemoryRegion* MemoryManager::getRegion(uint64_t addr) const {
    for (const auto& region : regions_) {
        if (region.contains(addr)) {
            return &region;
        }
    }
    return nullptr;
}

std::vector<MemoryRegion> MemoryManager::getRegions() const {
    return regions_;
}

bool MemoryManager::findFreeRegion(uint64_t size, uint64_t& addr) {
    // Find first free region
    uint64_t start = 0x100000000; // Start at 4GB
    while (start + size < physical_size_) {
        bool free = true;
        for (const auto& region : regions_) {
            if (region.base < start + size && region.end() > start) {
                free = false;
                start = region.end();
                break;
            }
        }
        if (free) {
            addr = start;
            return true;
        }
    }
    return false;
}

void MemoryManager::sortRegions() {
    std::sort(regions_.begin(), regions_.end(),
        [](const MemoryRegion& a, const MemoryRegion& b) {
            return a.base < b.base;
        });
}

} // namespace ps5