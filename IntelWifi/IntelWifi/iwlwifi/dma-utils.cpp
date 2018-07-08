//
//  dma-utils.c
//  IntelWifi
//
//  Created by Roman Peshkov on 10/02/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "dma-utils.h"

#include "../iw_utils/allocation.h"

struct iwl_dma_ptr* allocate_dma_buf(size_t size, mach_vm_address_t physical_mask) {
    IOOptionBits options = kIODirectionInOut | kIOMemoryPhysicallyContiguous | kIOMapInhibitCache;
    
    IOBufferMemoryDescriptor *bmd;
    bmd = IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task, options, size, physical_mask);
    
    IODMACommand *cmd = IODMACommand::withSpecification(kIODMACommandOutputHost64, 64, 0, IODMACommand::kMapped, 0, 1);
    cmd->setMemoryDescriptor(bmd);
    cmd->prepare();
    
    IODMACommand::Segment64 seg;
    UInt64 ofs = 0;
    UInt32 numSegs = 1;
    
    if (cmd->gen64IOVMSegments(&ofs, &seg, &numSegs) != kIOReturnSuccess) {
        cmd->complete();
        cmd->release();
        cmd = NULL;
        
        bmd->complete();
        bmd->release();
        bmd = NULL;
        
        return NULL;
    }
    
    struct iwl_dma_ptr *result = (struct iwl_dma_ptr *) iwh_malloc(sizeof(struct iwl_dma_ptr));
    result->addr = bmd->getBytesNoCopy();
    result->dma = seg.fIOVMAddr;
    result->size = size;
    result->bmd = bmd;
    result->cmd = cmd;
    return result;
}

void free_dma_buf(struct iwl_dma_ptr *dma_ptr) {
    IODMACommand *cmd = static_cast<IODMACommand *>(dma_ptr->cmd);
    cmd->complete();
    cmd->release();
    dma_ptr->cmd = NULL;
    
    IOBufferMemoryDescriptor *bmd = static_cast<IOBufferMemoryDescriptor *>(dma_ptr->bmd);
//    bmd->complete();
    bmd->release();
    dma_ptr->bmd = NULL;
    
    dma_ptr->addr = NULL;
    dma_ptr->dma = 0;
    
    iwh_free(dma_ptr);
}
