//
//  dma-utils.h
//  IntelWifi
//
//  Created by Roman Peshkov on 10/02/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef dma_utils_h
#define dma_utils_h

extern "C" {
#include "pcie/internal.h"
}

#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>

struct iwl_dma_ptr* allocate_dma_buf(size_t size, mach_vm_address_t physical_mask);
void free_dma_buf(struct iwl_dma_ptr *dma_ptr);

#endif /* dma_utils_h */
