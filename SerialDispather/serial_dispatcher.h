//
//  serial_dispatcher.h
//
//  Copyright © 2016 OTAKE Takayoshi. All rights reserved.
//

#ifndef serial_dispatcher_h
#define serial_dispatcher_h

#include <mutex>
#include <thread>
#include <functional>
#include <deque>

struct serial_dispatcher {
private:
    std::mutex apiMutex_;
    bool isRunning_ = false;
    
    std::thread thread_;
    bool requiresStop_ = false;
    std::mutex executeMutex_;
    std::mutex worksMutex_;
    std::deque<std::function<void(void)>> works_;
    
public:
    serial_dispatcher() {
    }
    
    ~serial_dispatcher() {
        stop();
    }
    
    bool isRunning() const {
        return isRunning_;
    }
    
    void start() {
        std::lock_guard<std::mutex> apiLock(apiMutex_);
        if (isRunning_) {
            return;
        }
        isRunning_ = true;
        requiresStop_ = false;
        // Clear all works
        {
            std::lock_guard<std::mutex> lock(worksMutex_);
            works_.clear();
            executeMutex_.lock();   // not executable
        }
        thread_ = std::thread(&serial_dispatcher::execute, this);
    }
    
    void stop() {
        std::lock_guard<std::mutex> apiLock(apiMutex_);
        if (!isRunning_) {
            return;
        }
        if (thread_.joinable()) {
            // Do not call `async(std::function<void(void)>)`, it causes dead lock.
            async_without_api_lock([this]() {
                this->requiresStop_ = true;
            });
            thread_.join();
        }
        isRunning_ = false;
    }
    
    void execute() {
        while (!requiresStop_) {
            executeMutex_.lock();
            std::function<void(void)> function;
            {
                std::lock_guard<std::mutex> lock(worksMutex_);
                function = works_.front();
                works_.pop_front();
                if (works_.size() != 0) {
                    executeMutex_.unlock();   // executable
                }
            }
            function();
        }
    }
    
    void async(std::function<void(void)>&& work) {
        std::lock_guard<std::mutex> apiLock(apiMutex_);
        if (!isRunning_) {
            return;
        }
        async_without_api_lock(std::move(work));
    }
    
    void sync(std::function<void(void)>&& work) {
        std::lock_guard<std::mutex> apiLock(apiMutex_);
        if (!isRunning_) {
            return;
        }
        std::mutex wait;
        wait.lock();
        {
            std::lock_guard<std::mutex> lock(worksMutex_);
            works_.push_back(work);
            works_.push_back([&wait]() {
                // Make sync
                wait.unlock();
            });
            executeMutex_.unlock();   // executable
        }
        wait.lock();
    }
    
private:
    void async_without_api_lock(std::function<void(void)>&& work) {
        std::lock_guard<std::mutex> lock(worksMutex_);
        works_.push_back(work);
        executeMutex_.unlock();   // executable
    }
};

#endif /* serial_dispatcher_h */
