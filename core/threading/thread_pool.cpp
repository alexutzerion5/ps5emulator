#include "core/threading/thread_pool.h"
#include <algorithm>
#include <iostream>

namespace ps5 {

ThreadPool::ThreadPool() {
}

ThreadPool::~ThreadPool() {
    shutdown();
}

bool ThreadPool::initialize(uint32_t num_threads) {
    workers_.reserve(num_threads);
    threads_.reserve(num_threads);
    
    for (uint32_t i = 0; i < num_threads; i++) {
        workers_.emplace_back(&ThreadPool::workerFunction, this, i);
    }
    
    return true;
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        shutdown_ = true;
    }
    
    condition_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
}

uint64_t ThreadPool::createThread(const std::string& name, uint64_t priority,
                                 uint64_t cpu_affinity, uint64_t stack_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ThreadContext thread;
    thread.id = next_thread_id_++;
    thread.name = name;
    thread.state = ThreadState::READY;
    thread.priority = priority;
    thread.cpu_affinity = cpu_affinity;
    thread.stack_size = stack_size;
    thread.stack_base = 0; // Will be allocated
    
    threads_.push_back(thread);
    return thread.id;
}

bool ThreadPool::startThread(uint64_t id, std::function<void()> func, uint64_t arg) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(threads_.begin(), threads_.end(),
        [id](const ThreadContext& t) { return t.id == id; });
    
    if (it == threads_.end()) {
        return false;
    }
    
    it->func = func;
    it->arg = arg;
    it->state = ThreadState::RUNNING;
    
    return true;
}

bool ThreadPool::stopThread(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(threads_.begin(), threads_.end(),
        [id](const ThreadContext& t) { return t.id == id; });
    
    if (it == threads_.end()) {
        return false;
    }
    
    it->cancelled = true;
    it->state = ThreadState::TERMINATED;
    
    return true;
}

bool ThreadPool::suspendThread(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(threads_.begin(), threads_.end(),
        [id](const ThreadContext& t) { return t.id == id; });
    
    if (it == threads_.end()) {
        return false;
    }
    
    it->state = ThreadState::SUSPENDED;
    
    return true;
}

bool ThreadPool::resumeThread(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(threads_.begin(), threads_.end(),
        [id](const ThreadContext& t) { return t.id == id; });
    
    if (it == threads_.end()) {
        return false;
    }
    
    it->state = ThreadState::READY;
    
    return true;
}

ThreadContext* ThreadPool::getThread(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(threads_.begin(), threads_.end(),
        [id](const ThreadContext& t) { return t.id == id; });
    
    if (it == threads_.end()) {
        return nullptr;
    }
    
    return &*it;
}

std::vector<ThreadContext*> ThreadPool::getThreads() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ThreadContext*> result;
    for (auto& thread : threads_) {
        result.push_back(&thread);
    }
    return result;
}

void ThreadPool::waitForThread(uint64_t id, uint64_t timeout_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(threads_.begin(), threads_.end(),
        [id](const ThreadContext& t) { return t.id == id; });
    
    if (it == threads_.end()) {
        return;
    }
    
    if (timeout_ms == 0) {
        while (it->state != ThreadState::TERMINATED) {
            it->cond.wait(mutex_);
        }
    } else {
        auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
        while (it->state != ThreadState::TERMINATED) {
            if (it->cond.wait_until(mutex_, end) == std::cv_status::timeout) {
                break;
            }
        }
    }
}

void ThreadPool::joinThread(uint64_t id) {
    waitForThread(id, 0);
}

void ThreadPool::workerFunction(uint32_t id) {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            condition_.wait(lock, [this] {
                return shutdown_ || !tasks_.empty();
            });
            
            if (shutdown_ && tasks_.empty()) {
                return;
            }
            
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        
        active_threads_++;
        task();
        active_threads_--;
    }
}

} // namespace ps5