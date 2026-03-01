#pragma once

#include <cstdint>
<include>
#include <vector>
#include <memory>
#include <string>
#include <fstream>

namespace ps5 {

// NVMe commands
enum class NVMeCommand : uint8_t {
    IDENTIFY = 0x06,
    READ = 0x02,
    WRITE = 0x01,
    FLUSH = 0x00,
    SET_FEATURES = 0x09,
    GET_FEATURES = 0x0A
};

// NVMe status codes
enum class NVMeStatus : uint16_t {
    SUCCESS = 0x0000,
    INVALID_OPCODE = 0x0001,
    INVALID_FIELD = 0x0002,
    COMMAND_ID_CONFLICT = 0x0003,
    DATA_TRANSFER_ERROR = 0x0004,
    ABORTED_POWER_LOSS = 0x0005,
    INTERNAL_DEVICE_ERROR = 0x0006,
    ABORTED_BY_REQUEST = 0x0007,
    ABORTED_SQ_DELETION = 0x0008,
    ABORTED_FAILED_FUSED = 0x0009,
    ABORTED_MISSING_FUSED = 0x000A,
    INVALID_NAMESPACE = 0x000B,
    LBA_OUT_OF_RANGE = 0x000C,
    CAPACITY_EXCEEDED = 0x000D,
    LBA_ALLOCATION_FAILED = 0x000E,
    RESOURCE_ALLOCATION_FAILED = 0x000F,
    HOST_PATH_ERROR = 0x0010,
    INCOMPLETE_HOST_COMMAND = 0x0011,
    QUEUE_SIZE_INVALID = 0x0012,
    CIRCULAR_QUEUE_FULL = 0x0013,
    ABORTED_QUEUE_FULL = 0x0014,
    ABORTED_QUEUE_DELETED = 0x0015,
    ABORTED_INVALID_QUEUE_ID = 0x0016,
    FEATURE_NOT_SAVEABLE = 0x0017,
    FEATURE_NOT_CHANGEABLE = 0x0018,
    FEATURE_NOT_PERMANENT = 0x0019,
    FIRMWARE_SLOT_INVALID = 0x001A,
    FIRMWARE_IMAGE_ERROR = 0x001B,
    CONFLICTING_ATTRIBUTES = 0x001C,
    INVALID_PROTECTION_INFO = 0x001D,
    STORE_DATA_ERROR = 0x001E
};

// NVMe controller registers
struct NVMeControllerRegisters {
    uint64_t cap;      // Controller Capabilities
    uint32_t vs;       // Version
    uint32_t intms;    // Interrupt Mask Set
    uint32_t intmc;    // Interrupt Mask Clear
    uint32_t cc;       // Controller Configuration
    uint32_t csts;     // Controller Status
    uint32_t aqa;      // Admin Queue Attributes
    uint64_t asq;      // Admin Submission Queue Base
    uint64_t acq;      // Admin Completion Queue Base
    uint64_t cmbloc;   // Controller Memory Buffer Location
    uint64_t cmbsz;    // Controller Memory Buffer Size
};

// NVMe namespace
struct NVMeNamespace {
    uint32_t nsid;
    uint64_t size;
    uint64_t capacity;
    uint64_t utilization;
    uint32_t sector_size;
    uint32_t metadata_size;
    bool formatted;
};

// NVMe controller
class NVMeController {
public:
    NVMeController();
    ~NVMeController();
    
    // Initialization
    bool initialize(const std::string& disk_image_path = "");
    void shutdown();
    
    // PCI configuration
    uint32_t readConfig(uint32_t offset);
    void writeConfig(uint32_t offset, uint32_t value);
    
    // MMIO access
    uint32_t readRegister(uint32_t offset);
    void writeRegister(uint32_t offset, uint32_t value);
    
    // Command processing
    void processAdminCommand(uint64_t sqe_addr);
    void processIOCommand(uint64_t sqe_addr, uint32_t cqid);
    
    // I/O processing
    void processRead(uint64_t nsid, uint64_t slba, uint32_t nlb, uint64_t data_addr);
    void processWrite(uint64_t nsid, uint64_t slba, uint32_t nlb, uint64_t data_addr);
    void processFlush(uint64_t nsid);
    
    // Status
    bool isReady() const { return ready_; }
    uint32_t getSectorSize() const { return sector_size_; }
    uint64_t getTotalSectors() const { return total_sectors_; }
    
private:
    bool ready_;
    std::string disk_image_path_;
    std::fstream disk_image_;
    
    NVMeControllerRegisters regs_;
    std::vector<NVMeNamespace> namespaces_;
    
    uint32_t sector_size_;
    uint64_t total_sectors_;
    
    // Admin queues
    std::vector<uint8_t> admin_sq_;
    std::vector<uint8_t> admin_cq_;
    
    // I/O queues
    std::unordered_map<uint32_t, std::vector<uint8_t>> io_sq_;
    std::unordered_map<uint32_t, std::vector<uint8_t>> io_cq_;
    
    // Helper functions
    void completeCommand(uint32_t cqid, uint16_t cid, NVMeStatus status, uint64_t result);
    void updateCQ(uint32_t cqid, uint16_t head, uint16_t cid, NVMeStatus status, uint64_t result);
};

} // namespace ps5