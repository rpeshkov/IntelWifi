//
//  allocation.h
//  IntelWifi
//
//  Helper functions that were not present in original sources, but very useful on OSX
//
//  Created by Roman Peshkov on 08/07/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef allocation_h
#define allocation_h

#include <IOKit/IOLib.h>
#include <sys/errno.h>

/**
 * Allocate IOKit memory saving allocated block len
 */
void *iwh_malloc(vm_size_t len);

/**
 * Allocate mem and fill it with zeroes
 */
void *iwh_zalloc(vm_size_t len);

/**
 * Typecast helper malloc
 */
#define iwh_talloc(len, type) ((type *)iwh_malloc(len))

/**
 * Free allocated block of memory. Size of memory to free is read from prepended bytes of the block.
 */
void iwh_free(void* ptr);

#endif /* allocation_h */
