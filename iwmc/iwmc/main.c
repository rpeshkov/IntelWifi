//
//  main.c
//  iwmc
//
//  Created by Roman Peshkov on 05/02/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include <IOKit/IOKitLib.h>

#include <stdio.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>


int main(int argc, const char * argv[]) {
    mach_port_t master_port;
    io_iterator_t iterator;
    io_service_t service;
    kern_return_t kern_result;
    
    kern_result = IOMasterPort(MACH_PORT_NULL, &master_port);
    
    if (kern_result != KERN_SUCCESS) {
        printf("Failed to init master port\n");
        return -1;
    }
    
    CFMutableDictionaryRef matchingDict = IOServiceNameMatching("IntelWifi");
    
    if (matchingDict == NULL) {
        printf("Matching Dict is NULL\n");
        return -1;
    }
    
    service = IOServiceGetMatchingService(master_port, matchingDict);
    
    
    if (!service) {
        printf("Service is null\n");
        return -1;
    }
    
    io_connect_t data_port;
    
    kern_result = IOServiceOpen(service, mach_task_self(), 0, &data_port);
    if (kern_result != KERN_SUCCESS) {
        printf("IOServiceOpen failed\n");
        return -1;
    }
    
    kern_result = IOConnectCallScalarMethod(data_port, 0, 0, 0, 0, 0);
    if (kern_result != KERN_SUCCESS)
    {
        IOServiceClose(data_port);
        return kern_result;
    }
    
    printf("Finished\n");
    
    return 0;
}
