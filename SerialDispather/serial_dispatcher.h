//
//  serial_dispatcher.h
//
//  Copyright © 2016 OTAKE Takayoshi. All rights reserved.
//

#ifndef serial_dispatcher_h
#define serial_dispatcher_h

#include <mutex>
#include <future>
#include <thread>
#include <functional>
#include <deque>

struct serial_dispatcher {
private:
    std::recursive_mutex apiMutex_;
    bool isRunning_ = false;
    
    std::thread thread_;
    bool requiresStop_ = false;
    bool isExecutable_ = false;
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
        std::lock_guard<std::recursive_mutex> apiLock(apiMutex_);
        if (isRunning_) {
            return;
        }
        isRunning_ = true;
        requiresStop_ = false;
        // Clear all works
        {
            std::lock_guard<std::mutex> lock(worksMutex_);
            works_.clear();
            isExecutable_ = false;
        }
        thread_ = std::thread(&serial_dispatcher::execute, this);
    }
    
    void stop() {
        std::lock_guard<std::recursive_mutex> apiLock(apiMutex_);
        if (!isRunning_) {
            return;
        }
        if (thread_.joinable()) {
            async([this]() {
                requiresStop_ = true;
            });
            thread_.join();
        }
        isRunning_ = false;
    }
    
    void async(std::function<void(void)>&& work) {
        std::lock_guard<std::recursive_mutex> apiLock(apiMutex_);
        if (!isRunning_) {
            return;
        }
        std::lock_guard<std::mutex> lock(worksMutex_);
        works_.push_back(work);
        isExecutable_ = true;
    }
    
    void sync(std::function<void(void)>&& work) {
        std::lock_guard<std::recursive_mutex> apiLock(apiMutex_);
        if (!isRunning_) {
            return;
        }
        std::promise<void> promise;
        {
            std::lock_guard<std::mutex> lock(worksMutex_);
            works_.push_back(work);
            works_.push_back([&promise]() {
                promise.set_value();
            });
            isExecutable_ = true;
        }
        // Make sync
        promise.get_future().get();
    }
private:
    void execute() {
        while (!requiresStop_) {
            if (!isExecutable_) {
                // FIXME: Avoid sleep
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            std::function<void(void)> function;
            {
                std::lock_guard<std::mutex> lock(worksMutex_);
                function = works_.front();
                works_.pop_front();
                if (works_.empty()) {
                    isExecutable_ = false;
                }
            }
            function();
        }
    }

    serial_dispatcher(const serial_dispatcher&) = delete;
    serial_dispatcher& operator =(const serial_dispatcher&) = delete;
};

#endif /* serial_dispatcher_h */
