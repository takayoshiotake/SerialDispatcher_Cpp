//
//  main.cpp
//
//  Copyright © 2016 OTAKE Takayoshi. All rights reserved.
//

#include <stdio.h>
#include "serial_dispatcher.h"

void func() {
    printf("func()\n");
}

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
    
    dispatcher.sync([]() {
        printf("sync: 2\n");
    });
    
    dispatcher.async(func);
    
    printf("end of sync\n");
    
    dispatcher.async([]() {
        printf("async: first\n");
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
    
    printf("end of main()\n");
    return 0;
}
