#include "loader/elf_loader.h"
#include "core/memory/memory.h"
#include <fstream>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace ps5 {

ELFLoader::ELFLoader() {
    std::memset(&header_, 0, sizeof(header_));
}

ELFLoader::~ELFLoader() {
}

bool ELFLoader::load(const std::string& path) {
    // Read ELF file
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    data_.resize(size);
    if (!file.read(reinterpret_cast<char*>(data_.data()), size)) {
        return false;
    }
    
    // Read header
    if (!readHeader()) {
        return false;
    }
    
    // Read program headers
    if (!readProgramHeaders()) {
        return false;
    }
    
    // Read section headers
    if (!readSectionHeaders()) {
        return false;
    }
    
    // Read symbols
    if (!readSymbols()) {
        return false;
    }
    
    // Read interpreter
    if (!readInterpreter()) {
        return false;
    }
    
    // Read data
    if (!readData()) {
        return false;
    }
    
    return true;
}

bool ELFLoader::load(const std::vector<uint8_t>& data) {
    data_ = data;
    
    // Read header
    if (!readHeader()) {
        return false;
    }
    
    // Read program headers
    if (!readProgramHeaders()) {
        return false;
    }
    
    // Read section headers
    if (!readSectionHeaders()) {
        return false;
    }
    
    // Read symbols
    if (!readSymbols()) {
        return false;
    }
    
    // Read interpreter
    if (!readInterpreter()) {
        return false;
    }
    
    // Read data
    if (!readData()) {
        return false;
    }
    
    return true;
}

bool ELFLoader::loadPKG(const std::string& pkg_path, const std::string& elf_name) {
    // Simplified: load from PKG
    // In real implementation, this would extract from PKG
    
    return load(elf_name);
}

bool ELFLoader::prepare() {
    // Prepare for execution
    // Simplified: just return true
    // In real implementation, this would set up memory mappings
    
    return true;
}

bool ELFLoader::relocate() {
    // Perform relocations
    // Simplified: just return true
    // In real implementation, this would process relocation entries
    
    return true;
}

bool ELFLoader::resolveSymbols() {
    // Resolve symbols
    // Simplified: just return true
    // In real implementation, this would resolve external symbols
    
    return true;
}

bool ELFLoader::mapToMemory(MemoryManager* mem) {
    if (!mem) {
        return false;
    }
    
    // Map segments to memory
    for (const auto& phdr : phdrs_) {
        if (phdr.p_type == static_cast<uint32_t>(SegmentType::LOAD)) {
            uint64_t addr = phdr.p_vaddr;
            uint64_t size = phdr.p_memsz;
            
            // Map memory
            mem->map(addr, addr, size);
            
            // Copy data
            if (phdr.p_filesz > 0) {
                std::memcpy(mem->getPhysicalMemory() + addr,
                           data_.data() + phdr.p_offset, phdr.p_filesz);
            }
            
            // Zero bss
            if (phdr.p_memsz > phdr.p_filesz) {
                std::memset(mem->getPhysicalMemory() + addr + phdr.p_filesz, 0,
                           phdr.p_memsz - phdr.p_filesz);
            }
        }
    }
    
    return true;
}

const ELFHeader& ELFLoader::getHeader() const {
    return header_;
}

const std::vector<ProgramHeader>& ELFLoader::getProgramHeaders() const {
    return phdrs_;
}

const std::vector<SectionHeader>& ELFLoader::getSectionHeaders() const {
    return shdrs_;
}

const std::vector<ELFSymbol>& ELFLoader::getSymbols() const {
    return symbols_;
}

const std::string& ELFLoader::getInterpreter() const {
    return interpreter_;
}

const uint8_t* ELFLoader::getSectionData(const std::string& name) const {
    auto it = sections_.find(name);
    if (it != sections_.end()) {
        return data_.data() + it->second.sh_offset;
    }
    return nullptr;
}

size_t ELFLoader::getSectionSize(const std::string& name) const {
    auto it = sections_.find(name);
    if (it != sections_.end()) {
        return it->second.sh_size;
    }
    return 0;
}

bool ELFLoader::readHeader() {
    if (data_.size() < sizeof(header_)) {
        return false;
    }
    
    std::memcpy(&header_, data_.data(), sizeof(header_));
    
    // Verify magic
    if (header_.e_ident[0] != 0x7F ||
        header_.e_ident[1] != 'E' ||
        header_.e_ident[2] != 'L' ||
        header_.e_ident[3] != 'F') {
        return false;
    }
    
    // Verify 64-bit
    if (header_.e_ident[4] != 2) {
        return false;
    }
    
    // Verify little endian
    if (header_.e_ident[5] != 1) {
        return false;
    }
    
    return true;
}

bool ELFLoader::readProgramHeaders() {
    if (header_.e_phoff == 0 || header_.e_phnum == 0) {
        return true;
    }
    
    phdrs_.resize(header_.e_phnum);
    
    for (uint16_t i = 0; i < header_.e_phnum; i++) {
        uint64_t offset = header_.e_phoff + i * header_.e_phentsize;
        if (offset + header_.e_phentsize > data_.size()) {
            return false;
        }
        
        std::memcpy(&phdrs_[i], data_.data() + offset, header_.e_phentsize);
    }
    
    return true;
}

bool ELFLoader::readSectionHeaders() {
    if (header_.e_shoff == 0 || header_.e_shnum == 0) {
        return true;
    }
    
    shdrs_.resize(header_.e_shnum);
    
    for (uint16_t i = 0; i < header_.e_shnum; i++) {
        uint64_t offset = header_.e_shoff + i * header_.e_shentsize;
        if (offset + header_.e_shentsize > data_.size()) {
            return false;
        }
        
        std::memcpy(&shdrs_[i], data_.data() + offset, header_.e_shentsize);
    }
    
    // Build section name map
    if (header_.e_shstrndx < shdrs_.size()) {
        const SectionHeader& shstrtab = shdrs_[header_.e_shstrndx];
        const char* shstrtab_data = reinterpret_cast<const char*>(data_.data() + shstrtab.sh_offset);
        
        for (const auto& shdr : shdrs_) {
            if (shdr.sh_name < shstrtab.sh_size) {
                std::string name = shstrtab_data + shdr.sh_name;
                sections_[name] = shdr;
            }
        }
    }
    
    return true;
}

bool ELFLoader::readSymbols() {
    // Read symbol tables
    // Simplified: just return true
    // In real implementation, this would read symbol tables
    
    return true;
}

bool ELFLoader::readInterpreter() {
    // Read interpreter
    // Simplified: just return true
    // In real implementation, this would read interpreter path
    
    interpreter_ = "/lib/ld-linux.so.2";
    
    return true;
}

bool ELFLoader::readData() {
    // Read data
    // Simplified: already read in load()
    
    return true;
}

} // namespace ps5