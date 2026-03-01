#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>
#include <string>

namespace ps5 {

// Kernel return codes
enum class KernelResult : uint64_t {
    OK = 0,
    EPERM = 1,
    ENOENT = 2,
    ESRCH = 3,
    EINTR = 4,
    EIO = 5,
    EACCES = 13,
    EFAULT = 14,
    EBUSY = 16,
    EEXIST = 17,
    ENODEV = 19,
    EINVAL = 22,
    ENOMEM = 12,
    EAGAIN = 11,
    ETIMEDOUT = 110
};

// Kernel object types
enum class KernelObjectType {
    PROCESS = 0,
    THREAD = 1,
    SEMAPHORE = 2,
    EVENT_FLAG = 3,
    MUTEX = 4,
    CONDITION_VARIABLE = 5,
    TIMER = 6,
    ALIAS = 7,
    MEMORY = 8,
    FILE = 9,
    SOCKET = 10,
    EVENT = 11,
    PORT = 12,
    VFS = 13
};

// Kernel object
struct KernelObject {
    uint64_t id;
    KernelObjectType type;
    std::string name;
    uint64_t handle;
    std::function<void()> destructor;
};

// Kernel system calls
class Kernel {
public:
    Kernel();
    ~Kernel();
    
    // Initialization
    bool initialize();
    void reset();
    
    // Process management
    uint64_t createProcess(const std::string& name, uint64_t entry_point);
    KernelResult destroyProcess(uint64_t process_id);
    KernelResult startProcess(uint64_t process_id);
    KernelResult stopProcess(uint64_t process_id);
    
    // Thread management
    uint64_t createThread(uint64_t process_id, const std::string& name,
                         uint64_t priority, uint64_t cpu_affinity,
                         uint64_t stack_size, uint64_t entry_point, uint64_t arg);
    KernelResult destroyThread(uint64_t thread_id);
    KernelResult startThread(uint64_t thread_id);
    KernelResult stopThread(uint64_t thread_id);
    KernelResult sleepThread(uint64_t thread_id, uint64_t timeout_ns);
    
    // Synchronization
    uint64_t createSemaphore(uint64_t initial_count, uint64_t max_count);
    KernelResult destroySemaphore(uint64_t sem_id);
    KernelResult waitSemaphore(uint64_t sem_id, uint32_t count, uint64_t timeout_ns);
    KernelResult signalSemaphore(uint64_t sem_id, uint32_t count);
    
    uint64_t createMutex(uint64_t initial_locked);
    KernelResult destroyMutex(uint64_t mutex_id);
    KernelResult lockMutex(uint64_t mutex_id);
    KernelResult unlockMutex(uint64_t mutex_id);
    
    uint64_t createEventFlag(uint64_t flags);
    KernelResult destroyEventFlag(uint64_t ef_id);
    KernelResult waitEventFlag(uint64_t ef_id, uint64_t wait_type, uint64_t clear_type);
    KernelResult signalEventFlag(uint64_t ef_id, uint64_t flags);
    
    // Memory management
    uint64_t allocateMemory(uint64_t size, uint64_t prot);
    KernelResult freeMemory(uint64_t addr);
    KernelResult mapMemory(uint64_t addr, uint64_t size, uint64_t prot);
    KernelResult unmapMemory(uint64_t addr, uint64_t size);
    
    // File system
    uint64_t openFile(const std::string& path, uint64_t flags);
    KernelResult closeFile(uint64_t fd);
    KernelResult readFile(uint64_t fd, uint64_t buf, uint64_t size, uint64_t* read_size);
    KernelResult writeFile(uint64_t fd, uint64_t buf, uint64_t size, uint64_t* written_size);
    
    // System calls
    KernelResult syscall(uint64_t num, uint64_t arg0, uint64_t arg1,
                        uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);
    
    // VSH (Visual Shell) interface
    class VSH* getVSH() { return vsh_.get(); }
    
private:
    std::unordered_map<uint64_t, std::unique_ptr<KernelObject>> objects_;
    std::mutex mutex_;
    uint64_t next_id_{0};
    
    std::unique_ptr<class VSH> vsh_;
    
    // Helper functions
    uint64_t allocId();
    KernelObject* getObject(uint64_t id);
    const KernelObject* getObject(uint64_t id) const;
};

} // namespace ps5