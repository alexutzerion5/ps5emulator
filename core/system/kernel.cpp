#include "core/system/kernel.h"
#include "core/system/vsh.h"
#include "core/cpu/cpu.h"
#include "core/memory/memory.h"
#include "core/threading/thread_pool.h"
#include <cstring>
#include <iostream>

namespace ps5 {

Kernel::Kernel() : next_id_(0) {
    vsh_ = std::make_unique<VSH>();
}

Kernel::~Kernel() {
}

bool Kernel::initialize() {
    if (!vsh_->initialize()) {
        return false;
    }
    
    return true;
}

void Kernel::reset() {
    objects_.clear();
    next_id_ = 0;
    vsh_->shutdown();
}

uint64_t Kernel::createProcess(const std::string& name, uint64_t entry_point) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto obj = std::make_unique<KernelObject>();
    obj->id = allocId();
    obj->type = KernelObjectType::PROCESS;
    obj->name = name;
    obj->handle = 0;
    
    objects_[obj->id] = std::move(obj);
    
    return obj->id;
}

KernelResult Kernel::destroyProcess(uint64_t process_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(process_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::PROCESS) {
        return KernelResult::ENOENT;
    }
    
    if (it->second->destructor) {
        it->second->destructor();
    }
    
    objects_.erase(it);
    
    return KernelResult::OK;
}

KernelResult Kernel::startProcess(uint64_t process_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(process_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::PROCESS) {
        return KernelResult::ENOENT;
    }
    
    // Start process execution
    vsh_->launchApplication(it->second->name);
    
    return KernelResult::OK;
}

KernelResult Kernel::stopProcess(uint64_t process_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(process_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::PROCESS) {
        return KernelResult::ENOENT;
    }
    
    vsh_->terminateApplication(process_id);
    
    return KernelResult::OK;
}

uint64_t Kernel::createThread(uint64_t process_id, const std::string& name,
                             uint64_t priority, uint64_t cpu_affinity,
                             uint64_t stack_size, uint64_t entry_point, uint64_t arg) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto obj = std::make_unique<KernelObject>();
    obj->id = allocId();
    obj->type = KernelObjectType::THREAD;
    obj->name = name;
    obj->handle = 0;
    
    objects_[obj->id] = std::move(obj);
    
    return obj->id;
}

KernelResult Kernel::destroyThread(uint64_t thread_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(thread_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::THREAD) {
        return KernelResult::ENOENT;
    }
    
    if (it->second->destructor) {
        it->second->destructor();
    }
    
    objects_.erase(it);
    
    return KernelResult::OK;
}

KernelResult Kernel::startThread(uint64_t thread_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(thread_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::THREAD) {
        return KernelResult::ENOENT;
    }
    
    return KernelResult::OK;
}

KernelResult Kernel::stopThread(uint64_t thread_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(thread_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::THREAD) {
        return KernelResult::ENOENT;
    }
    
    return KernelResult::OK;
}

KernelResult Kernel::sleepThread(uint64_t thread_id, uint64_t timeout_ns) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(thread_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::THREAD) {
        return KernelResult::ENOENT;
    }
    
    return KernelResult::OK;
}

uint64_t Kernel::createSemaphore(uint64_t initial_count, uint64_t max_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto obj = std::make_unique<KernelObject>();
    obj->id = allocId();
    obj->type = KernelObjectType::SEMAPHORE;
    obj->name = "semaphore";
    obj->handle = 0;
    
    objects_[obj->id] = std::move(obj);
    
    return obj->id;
}

KernelResult Kernel::destroySemaphore(uint64_t sem_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(sem_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::SEMAPHORE) {
        return KernelResult::ENOENT;
    }
    
    if (it->second->destructor) {
        it->second->destructor();
    }
    
    objects_.erase(it);
    
    return KernelResult::OK;
}

KernelResult Kernel::waitSemaphore(uint64_t sem_id, uint32_t count, uint64_t timeout_ns) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(sem_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::SEMAPHORE) {
        return KernelResult::ENOENT;
    }
    
    return KernelResult::OK;
}

KernelResult Kernel::signalSemaphore(uint64_t sem_id, uint32_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(sem_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::SEMAPHORE) {
        return KernelResult::ENOENT;
    }
    
    return KernelResult::OK;
}

uint64_t Kernel::createMutex(uint64_t initial_locked) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto obj = std::make_unique<KernelObject>();
    obj->id = allocId();
    obj->type = KernelObjectType::MUTEX;
    obj->name = "mutex";
    obj->handle = 0;
    
    objects_[obj->id] = std::move(obj);
    
    return obj->id;
}

KernelResult Kernel::destroyMutex(uint64_t mutex_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(mutex_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::MUTEX) {
        return KernelResult::ENOENT;
    }
    
    if (it->second->destructor) {
        it->second->destructor();
    }
    
    objects_.erase(it);
    
    return KernelResult::OK;
}

KernelResult Kernel::lockMutex(uint64_t mutex_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(mutex_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::MUTEX) {
        return KernelResult::ENOENT;
    }
    
    return KernelResult::OK;
}

KernelResult Kernel::unlockMutex(uint64_t mutex_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(mutex_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::MUTEX) {
        return KernelResult::ENOENT;
    }
    
    return KernelResult::OK;
}

uint64_t Kernel::createEventFlag(uint64_t flags) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto obj = std::make_unique<KernelObject>();
    obj->id = allocId();
    obj->type = KernelObjectType::EVENT_FLAG;
    obj->name = "event_flag";
    obj->handle = 0;
    
    objects_[obj->id] = std::move(obj);
    
    return obj->id;
}

KernelResult Kernel::destroyEventFlag(uint64_t ef_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(ef_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::EVENT_FLAG) {
        return KernelResult::ENOENT;
    }
    
    if (it->second->destructor) {
        it->second->destructor();
    }
    
    objects_.erase(it);
    
    return KernelResult::OK;
}

KernelResult Kernel::waitEventFlag(uint64_t ef_id, uint64_t wait_type, uint64_t clear_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(ef_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::EVENT_FLAG) {
        return KernelResult::ENOENT;
    }
    
    return KernelResult::OK;
}

KernelResult Kernel::signalEventFlag(uint64_t ef_id, uint64_t flags) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(ef_id);
    if (it == objects_.end() || it->second->type != KernelObjectType::EVENT_FLAG) {
        return KernelResult::ENOENT;
    }
    
    return KernelResult::OK;
}

uint64_t Kernel::allocateMemory(uint64_t size, uint64_t prot) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Simplified: return a memory address
    static uint64_t next_addr = 0x100000000;
    uint64_t addr = next_addr;
    next_addr += size;
    
    return addr;
}

KernelResult Kernel::freeMemory(uint64_t addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return KernelResult::OK;
}

KernelResult Kernel::mapMemory(uint64_t addr, uint64_t size, uint64_t prot) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return KernelResult::OK;
}

KernelResult Kernel::unmapMemory(uint64_t addr, uint64_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return KernelResult::OK;
}

uint64_t Kernel::openFile(const std::string& path, uint64_t flags) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Simplified: return a file descriptor
    static uint64_t next_fd = 3;
    uint64_t fd = next_fd++;
    
    return fd;
}

KernelResult Kernel::closeFile(uint64_t fd) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return KernelResult::OK;
}

KernelResult Kernel::readFile(uint64_t fd, uint64_t buf, uint64_t size, uint64_t* read_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    *read_size = size;
    return KernelResult::OK;
}

KernelResult Kernel::writeFile(uint64_t fd, uint64_t buf, uint64_t size, uint64_t* written_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    *written_size = size;
    return KernelResult::OK;
}

KernelResult Kernel::syscall(uint64_t num, uint64_t arg0, uint64_t arg1,
                            uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    switch (num) {
        case 0: // SYS_exit
            return KernelResult::OK;
        case 1: // SYS_fork
            return KernelResult::EPERM;
        case 2: // SYS_read
            return KernelResult::OK;
        case 3: // SYS_write
            return KernelResult::OK;
        case 4: // SYS_open
            return KernelResult::OK;
        case 5: // SYS_close
            return KernelResult::OK;
        case 10: // SYS_wait4
            return KernelResult::OK;
        case 11: // SYS_ioctl
            return KernelResult::OK;
        case 12: // SYS_execve
            return KernelResult::OK;
        case 14: // SYS_waitpid
            return KernelResult::OK;
        case 20: // SYS_getpid
            return KernelResult::OK;
        case 24: // SYS_getuid
            return KernelResult::OK;
        case 25: // SYS_geteuid
            return KernelResult::OK;
        case 26: // SYS_getgid
            return KernelResult::OK;
        case 27: // SYS_getegid
            return KernelResult::OK;
        case 37: // SYS_sigaction
            return KernelResult::OK;
        case 38: // SYS_sigprocmask
            return KernelResult::OK;
        case 42: // SYS_access
            return KernelResult::OK;
        case 46: // SYS_brk
            return KernelResult::OK;
        case 47: // SYS_mprotect
            return KernelResult::OK;
        case 48: // SYS_munmap
            return KernelResult::OK;
        case 49: // SYS_mlock
            return KernelResult::OK;
        case 50: // SYS_munlock
            return KernelResult::OK;
        case 51: // SYS_mlockall
            return KernelResult::OK;
        case 52: // SYS_munlockall
            return KernelResult::OK;
        case 57: // SYS_pipe
            return KernelResult::OK;
        case 66: // SYS_dup
            return KernelResult::OK;
        case 67: // SYS_dup2
            return KernelResult::OK;
        case 73: // SYS_fcntl
            return KernelResult::OK;
        case 74: // SYS_flock
            return KernelResult::OK;
        case 75: // SYS_fsync
            return KernelResult::OK;
        case 76: // SYS_fdatasync
            return KernelResult::OK;
        case 83: // SYS_getcwd
            return KernelResult::OK;
        case 90: // SYS_mkdir
            return KernelResult::OK;
        case 91: // SYS_rmdir
            return KernelResult::OK;
        case 98: // SYS_utimes
            return KernelResult::OK;
        case 103: // SYS_ftruncate
            return KernelResult::OK;
        case 104: // SYS_fstat
            return KernelResult::OK;
        case 105: // SYS_lstat
            return KernelResult::OK;
        case 106: // SYS_stat
            return KernelResult::OK;
        case 110: // SYS_getdents
            return KernelResult::OK;
        case 114: // SYS_readlink
            return KernelResult::OK;
        case 115: // SYS_rename
            return KernelResult::OK;
        case 116: // SYS_symlink
            return KernelResult::OK;
        case 120: // SYS_gettimeofday
            return KernelResult::OK;
        case 122: // SYS_uname
            return KernelResult::OK;
        case 131: // SYS_getrlimit
            return KernelResult::OK;
        case 139: // SYS_mmap
            return KernelResult::OK;
        case 142: // SYS_mremap
            return KernelResult::OK;
        case 143: // SYS_msync
            return KernelResult::OK;
        case 147: // SYS_getpriority
            return KernelResult::OK;
        case 148: // SYS_setpriority
            return KernelResult::OK;
        case 150: // SYS_settimeofday
            return KernelResult::OK;
        case 151: // SYS_gettimeofday
            return KernelResult::OK;
        case 159: // SYS_gettid
            return KernelResult::OK;
        case 160: // SYS_rt_sigaction
            return KernelResult::OK;
        case 161: // SYS_rt_sigprocmask
            return KernelResult::OK;
        case 162: // SYS_rt_sigpending
            return KernelResult::OK;
        case 163: // SYS_rt_sigtimedwait
            return KernelResult::OK;
        case 164: // SYS_rt_sigqueueinfo
            return KernelResult::OK;
        case 165: // SYS_rt_sigsuspend
            return KernelResult::OK;
        case 166: // SYS_pread64
            return KernelResult::OK;
        case 167: // SYS_pwrite64
            return KernelResult::OK;
        case 168: // SYS_readv
            return KernelResult::OK;
        case 169: // SYS_writev
            return KernelResult::OK;
        case 170: // SYS_select
            return KernelResult::OK;
        case 171: // SYS_pselect6
            return KernelResult::OK;
        case 172: // SYS_poll
            return KernelResult::OK;
        case 173: // SYS_ppoll
            return KernelResult::OK;
        case 174: // SYS_getrlimit
            return KernelResult::OK;
        case 175: // SYS_setrlimit
            return KernelResult::OK;
        case 176: // SYS_getrusage
            return KernelResult::OK;
        case 177: // SYS_gettimeofday
            return KernelResult::OK;
        case 178: // SYS_getuid
            return KernelResult::OK;
        case 179: // SYS_getgid
            return KernelResult::OK;
        case 180: // SYS_geteuid
            return KernelResult::OK;
        case 181: // SYS_getegid
            return KernelResult::OK;
        case 182: // SYS_setreuid
            return KernelResult::OK;
        case 183: // SYS_setregid
            return KernelResult::OK;
        case 184: // SYS_getgroups
            return KernelResult::OK;
        case 185: // SYS_setgroups
            return KernelResult::OK;
        case 186: // SYS_setresuid
            return KernelResult::OK;
        case 187: // SYS_getresuid
            return KernelResult::OK;
        case 188: // SYS_setresgid
            return KernelResult::OK;
        case 189: // SYS_getresgid
            return KernelResult::OK;
        case 190: // SYS_setfsuid
            return KernelResult::OK;
        case 191: // SYS_setfsgid
            return KernelResult::OK;
        case 192: // SYS_times
            return KernelResult::OK;
        case 195: // SYS_gettid
            return KernelResult::OK;
        case 196: // SYS_rt_sigaction
            return KernelResult::OK;
        case 197: // SYS_rt_sigprocmask
            return KernelResult::OK;
        case 198: // SYS_rt_sigpending
            return KernelResult::OK;
        case 199: // SYS_rt_sigtimedwait
            return KernelResult::OK;
        case 200: // SYS_rt_sigqueueinfo
            return KernelResult::OK;
        case 201: // SYS_rt_sigsuspend
            return KernelResult::OK;
        case 202: // SYS_pread64
            return KernelResult::OK;
        case 203: // SYS_pwrite64
            return KernelResult::OK;
        case 204: // SYS_readv
            return KernelResult::OK;
        case 205: // SYS_writev
            return KernelResult::OK;
        case 206: // SYS_select
            return KernelResult::OK;
        case 207: // SYS_pselect6
            return KernelResult::OK;
        case 208: // SYS_poll
            return KernelResult::OK;
        case 209: // SYS_ppoll
            return KernelResult::OK;
        case 210: // SYS_getrlimit
            return KernelResult::OK;
        case 211: // SYS_setrlimit
            return KernelResult::OK;
        case 212: // SYS_getrusage
            return KernelResult::OK;
        case 213: // SYS_gettimeofday
            return KernelResult::OK;
        case 214: // SYS_getuid
            return KernelResult::OK;
        case 215: // SYS_getgid
            return KernelResult::OK;
        case 216: // SYS_geteuid
            return KernelResult::OK;
        case 217: // SYS_getegid
            return KernelResult::OK;
        case 218: // SYS_setreuid
            return KernelResult::OK;
        case 219: // SYS_setregid
            return KernelResult::OK;
        case 220: // SYS_getgroups
            return KernelResult::OK;
        case 221: // SYS_setgroups
            return KernelResult::OK;
        case 222: // SYS_setresuid
            return KernelResult::OK;
        case 223: // SYS_getresuid
            return KernelResult::OK;
        case 224: // SYS_setresgid
            return KernelResult::OK;
        case 225: // SYS_getresgid
            return KernelResult::OK;
        case 226: // SYS_setfsuid
            return KernelResult::OK;
        case 227: // SYS_setfsgid
            return KernelResult::OK;
        case 228: // SYS_times
            return KernelResult::OK;
        case 292: // SYS_kqueue
            return KernelResult::OK;
        case 293: // SYS_kevent
            return KernelResult::OK;
        case 303: // SYS_preadv
            return KernelResult::OK;
        case 304: // SYS_pwritev
            return KernelResult::OK;
        default:
            return KernelResult::EINVAL;
    }
}

KernelObject* Kernel::getObject(uint64_t id) {
    auto it = objects_.find(id);
    if (it != objects_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const KernelObject* Kernel::getObject(uint64_t id) const {
    auto it = objects_.find(id);
    if (it != objects_.end()) {
        return it->second.get();
    }
    return nullptr;
}

uint64_t Kernel::allocId() {
    return next_id_++;
}

} // namespace ps5