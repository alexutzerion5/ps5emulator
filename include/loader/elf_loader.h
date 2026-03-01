#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace ps5 {

// ELF header
struct ELFHeader {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

// Program header
struct ProgramHeader {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

// Section header
struct SectionHeader {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
};

// ELF segment types
enum class SegmentType : uint32_t {
    NULL_ = 0,
    LOAD = 1,
    DYNAMIC = 2,
    INTERP = 3,
    NOTE = 4,
    PHDR = 6,
    TLS = 7,
    GNU_EH_FRAME = 0x6474e550,
    GNU_STACK = 0x6474e551,
    GNU_RELRO = 0x6474e552,
    GNU_PROPERTY = 0x6474e553
};

// ELF symbol
struct ELFSymbol {
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
};

// ELF loader
class ELFLoader {
public:
    ELFLoader();
    ~ELFLoader();
    
    // Loading
    bool load(const std::string& path);
    bool load(const std::vector<uint8_t>& data);
    bool loadPKG(const std::string& pkg_path, const std::string& elf_name);
    
    // Execution
    bool prepare();
    bool relocate();
    bool resolveSymbols();
    uint64_t getEntryPoint() const { return header_.e_entry; }
    
    // Memory mapping
    bool mapToMemory(class MemoryManager* mem);
    
    // Information
    const ELFHeader& getHeader() const { return header_; }
    const std::vector<ProgramHeader>& getProgramHeaders() const { return phdrs_; }
    const std::vector<SectionHeader>& getSectionHeaders() const { return shdrs_; }
    const std::vector<ELFSymbol>& getSymbols() const { return symbols_; }
    const std::string& getInterpreter() const { return interpreter_; }
    
    // Sections
    const uint8_t* getSectionData(const std::string& name) const;
    size_t getSectionSize(const std::string& name) const;
    
private:
    ELFHeader header_;
    std::vector<ProgramHeader> phdrs_;
    std::vector<SectionHeader> shdrs_;
    std::vector<ELFSymbol> symbols_;
    std::string interpreter_;
    
    std::vector<uint8_t> data_;
    std::unordered_map<std::string, SectionHeader> sections_;
    
    // Helper functions
    bool readHeader();
    bool readProgramHeaders();
    bool readSectionHeaders();
    bool readSymbols();
    bool readInterpreter();
    bool readData();
};

} // namespace ps5