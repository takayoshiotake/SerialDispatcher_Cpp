//
//  main.cpp
//
//  Copyright © 2016 OTAKE Takayoshi. All rights reserved.
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
    
    // Wait for action "async in async in sync in async" is registered
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    printf("end of main()\n");
    return 0;
}
