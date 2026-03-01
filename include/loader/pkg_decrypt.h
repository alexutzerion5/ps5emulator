#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

namespace ps5 {

// PKG file structure
struct PKGHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t data_offset;
    uint64_t data_size;
    uint64_t total_size;
    uint32_t content_id_offset;
    uint32_t content_id_size;
    uint32_t metadata_offset;
    uint32_t metadata_size;
    uint32_t cert_offset;
    uint32_t cert_size;
    uint32_t signature_offset;
    uint32_t signature_size;
    uint32_t file_count;
    uint32_t file_info_offset;
    uint32_t file_info_size;
    uint32_t string_table_offset;
    uint32_t string_table_size;
    uint32_t flags;
    uint32_t key_index;
    uint64_t package_hash;
    uint8_t padding[128];
};

// PKG file entry
struct PKGFileEntry {
    uint64_t offset;
    uint64_t size;
    uint64_t compressed_size;
    uint32_t flags;
    uint32_t hash[8];
    uint32_t name_offset;
    uint32_t name_size;
};

// PKG content ID
struct PKGContentID {
    uint8_t type;
    uint8_t category;
    uint8_t version[2];
    uint8_t package_id[12];
    uint8_t digest[16];
    uint8_t reserved[16];
};

// PKG decryptor
class PKGDecryptor {
public:
    PKGDecryptor();
    ~PKGDecryptor();
    
    // Initialization
    bool initialize(const std::string& key_file = "");
    
    // PKG operations
    bool loadPKG(const std::string& path);
    bool decrypt();
    bool extract(const std::string& output_dir);
    
    // Information
    const PKGHeader& getHeader() const { return header_; }
    const std::vector<PKGFileEntry>& getFiles() const { return files_; }
    const PKGContentID& getContentID() const { return content_id_; }
    const std::string& getContentTitle() const { return content_title_; }
    
    // Extraction
    bool extractFile(size_t index, const std::string& output_path);
    bool extractAll(const std::string& output_dir);
    
private:
    bool initialized_;
    PKGHeader header_;
    std::vector<PKGFileEntry> files_;
    PKGContentID content_id_;
    std::string content_title_;
    
    std::vector<uint8_t> pkg_data_;
    std::vector<uint8_t> decrypted_data_;
    
    std::string key_file_;
    
    // Keys (these would be loaded from a key file in production)
    uint8_t pkg_key_[16];
    uint8_t pkg_iv_[16];
    uint8_t pkg_mac_[16];
    
    // Helper functions
    bool readHeader();
    bool readFiles();
    bool readContentID();
    bool readContentTitle();
    bool decryptData();
    bool verifySignature();
};

} // namespace ps5