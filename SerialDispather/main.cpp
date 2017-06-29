//
//  main.cpp
//
//  Copyright © 2016-2017 OTAKE Takayoshi. All rights reserved.
//

#include <stdio.h>
#include "serial_dispatcher.hpp"

void func() {
    printf("func()\n");
}

template <typename _R>
struct func_t {
    _R operator()() {
        printf("func_t<%s>::()\n", typeid(_R).name());
        return _R{};
    }
};

template <>
struct func_t<void> {
    void operator()() {
        printf("func_t<%s>::()\n", typeid(void).name());
    }
};

template <typename _T>
struct func_sum_t {
    _T initial_;
    
    func_sum_t(const _T& initial)
    : initial_(initial) {
    }
    
    _T operator()(_T a, _T b) {
        printf("func_sum_t<%s>::()\n", typeid(_T).name());
        return initial_ + a + b;
    }
};

template <typename _T>
struct func_sum2_t {
    _T sum_;
    
    void operator()(_T a, _T b) {
        printf("func_sum2_t<%s>::()\n", typeid(_T).name());
        sum_ = a + b;
    }
};

int main(int argc, const char * argv[]) {
    serial_dispatcher dispatcher;
    dispatcher.start();
    
    printf("Hello, serial_dispatcher!\n");
    
    dispatcher.sync([]() {
        printf("sync: 0\n");
    });
    
    dispatcher.sync([]() {
        printf("sync: 1\n");
    });
    
    dispatcher.sync([&dispatcher]() {
        printf("sync: 2\n");
        dispatcher.sync([&dispatcher]() {
            printf("sync: 2-1\n");
            dispatcher.sync([]() {
                printf("sync: 2-1-1\n");
            });
        });
    });
    
    dispatcher.async(func);
    
    printf("end of sync\n");
    
    dispatcher.async([&dispatcher]() {
        printf("async: first\n");
        dispatcher.sync([&dispatcher]() {
            printf("sync in async\n");
            dispatcher.async([&dispatcher]() {
                printf("async in sync in async\n");
                dispatcher.async([]() {
                    printf("async in async in sync in async\n");
                });
            });
        });
    });
    
    int x;
    dispatcher.async([&x]() {
        printf("async: x:=100\n");
        x = 100;
    });
    
    dispatcher.async([&x]() {
        printf("async: x=%d\n", x);
    });
    
    dispatcher.async([]() {
        printf("async: last\n");
    });
    
    auto y = dispatcher.sync<int>([&x]()  {
        return x * 100;
    });
    printf("sync: %d\n", y);
    
    dispatcher.sync(func_t<void>{});
    
    dispatcher.sync(func_t<int>{});
    
    auto z = dispatcher.sync<int>(func_t<int>{});
    printf("sync: %d\n", z);
    
    auto sum = dispatcher.sync<int>(func_sum_t<int>(2), 10, 4);
    printf("sync: %d\n", sum);
    
    dispatcher.sync(func_sum2_t<int>{}, 10, 4);
    
    func_sum2_t<int> sum2;
    dispatcher.sync(sum2, 10, 4);
    printf("sync: %d\n", sum2.sum_);
    
    try {
        dispatcher.sync([]() {
            throw std::runtime_error("error_sync");
        });
    }
    catch (const std::runtime_error& e) {
        printf("exception in sync: %s\n", e.what());
    }
    
    // Wait for action "async in async in sync in async" is registered
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    printf("end of main()\n");
    return 0;
}
