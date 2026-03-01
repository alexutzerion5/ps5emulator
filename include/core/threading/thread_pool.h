#pragma once

#include <cstdint>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <future>
#include <chrono>

namespace ps5 {

// Thread states
enum class ThreadState {
    READY = 0,
    RUNNING = 1,
    WAITING = 2,
    SUSPENDED = 3,
    TERMINATED = 4
};

// Thread context
struct ThreadContext {
    uint64_t id;
    std::string name;
    ThreadState state;
    uint64_t priority;
    uint64_t cpu_affinity;
    uint64_t stack_base;
    uint64_t stack_size;
    uint64_t entry_point;
    uint64_t arg;
    std::function<void()> func;
    std::mutex mutex;
    std::condition_variable cond;
    std::atomic<bool> cancelled{false};
};

// Thread pool
class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();
    
    // Initialization
    bool initialize(uint32_t num_threads = 8);
    void shutdown();
    
    // Thread creation
    uint64_t createThread(const std::string& name, uint64_t priority = 0,
                         uint64_t cpu_affinity = 0xFFFFFFFF,
                         uint64_t stack_size = 1024 * 1024);
    bool startThread(uint64_t id, std::function<void()> func, uint64_t arg = 0);
    bool stopThread(uint64_t id);
    bool suspendThread(uint64_t id);
    bool resumeThread(uint64_t id);
    
    // Thread management
    ThreadContext* getThread(uint64_t id);
    std::vector<ThreadContext*> getThreads();
    void waitForThread(uint64_t id, uint64_t timeout_ms = 0);
    void joinThread(uint64_t id);
    
    // Task submission
    template<typename Func, typename... Args>
    auto submitTask(Func&& f, Args&&... args)
        -> std::future<typename std::invoke_result<Func, Args...>::type> {
        using return_type = typename std::invoke_result<Func, Args...>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (shutdown_) {
                throw std::runtime_error("Cannot submit task to stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
    
    // Statistics
    uint32_t getActiveThreads() const { return active_threads_.load(); }
    uint32_t getPendingTasks() const { return tasks_.size(); }
    
private:
    std::vector<std::thread> workers_;
    std::vector<ThreadContext> threads_;
    std::queue<std::function<void()>> tasks_;
    
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> shutdown_{false};
    std::atomic<uint32_t> active_threads_{0};
    
    uint64_t next_thread_id_{0};
    
    void workerFunction(uint32_t id);
};

} // namespace ps5