//
//  allocation.c
//  IntelWifi
//
//  Created by Roman Peshkov on 08/07/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "allocation.h"

void* iwh_malloc(vm_size_t len) {
    void *addr = IOMalloc(len + sizeof(vm_size_t));
    if (addr == NULL)
        return NULL;
    
    *((vm_size_t*)addr) = len;
    return (void*)((uint8_t*)addr + sizeof(vm_size_t));
}

void* iwh_zalloc(vm_size_t len) {
    void* addr = iwh_malloc(len);
    if (addr == NULL)
        return NULL;
    
    bzero(addr, len);
    return addr;
}

void iwh_free(void* ptr) {
    if (!ptr)
        return;
    
    void* start_addr = (void*)((uint8_t*)ptr - sizeof(vm_size_t));
    vm_size_t block_size = *((vm_size_t*)start_addr) + sizeof(vm_size_t);
    IOFree(start_addr, block_size);
}
