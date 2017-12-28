//
//  IntelIO.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 28/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#include "IntelIO.hpp"

#include <sys/errno.h>

// MARK: Defines

#define IWL_POLL_INTERVAL 10    /* microseconds */


// MARK: Implementation
#define super OSObject
OSDefineMetaClassAndStructors( IntelIO, OSObject )

IntelIO* IntelIO::withAddress(volatile void * p) {
    IntelIO *io = new IntelIO;
    
    if (io && !io->initWithAddress(p)) {
        io->release();
        return NULL;
    }
    
    return io;
}

bool IntelIO::initWithAddress(volatile void * p) {
    if (!super::init()) {
        return false;
    }
    
    fBaseAddr = p;
    
    return true;
}

void IntelIO::release() {
    fBaseAddr = NULL;
    super::release();
}

int IntelIO::iwl_poll_bit(UInt32 addr, UInt32 bits, UInt32 mask, int timeout) {
    int t = 0;
    do {
        if ((iwl_read32(addr) & mask) == (bits & mask)) {
            return t;
        }
        
        IODelay(IWL_POLL_INTERVAL);
        t += IWL_POLL_INTERVAL;
    } while (t < timeout);
    
    return -ETIMEDOUT;
}


void IntelIO::iwl_write32(UInt32 ofs, UInt32 val) {
    OSWriteLittleInt32(fBaseAddr, ofs, val);
}

UInt32 IntelIO::iwl_read32(UInt32 ofs) {
    return OSReadLittleInt32(fBaseAddr, ofs);
}

void IntelIO::iwl_set_bit(UInt32 reg, UInt32 mask)
{
    iwl_trans_pcie_set_bits_mask(reg, mask, mask);
}

void IntelIO::iwl_clear_bit(UInt32 reg, UInt32 mask)
{
    iwl_trans_pcie_set_bits_mask(reg, mask, 0);
}

void IntelIO::iwl_trans_pcie_set_bits_mask(UInt32 reg, UInt32 mask, UInt32 value)
{
    // TODO: Maybe here IOSimpleLockLockDisableInterrupt must be used?
    //spin_lock_irqsave(&trans_pcie->reg_lock, flags);
    __iwl_trans_pcie_set_bits_mask(reg, mask, value);
    //spin_unlock_irqrestore(&trans_pcie->reg_lock, flags);
}

void IntelIO::__iwl_trans_pcie_set_bits_mask(UInt32 reg, UInt32 mask, UInt32 value)
{
    UInt32 v;
    
    v = iwl_read32(reg);
    v &= ~mask;
    v |= value;
    iwl_write32(reg, v);
}


