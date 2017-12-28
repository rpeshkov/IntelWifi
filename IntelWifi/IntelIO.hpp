//
//  IntelIO.hpp
//  IntelWifi
//
//  Created by Roman Peshkov on 28/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef IntelIO_hpp
#define IntelIO_hpp

#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSMetaClass.h>

#include <IOKit/IOLib.h>

class IntelIO : public OSObject {
    OSDeclareDefaultStructors(IntelIO);
public:
    static IntelIO* withAddress(volatile void * p);
    bool initWithAddress(volatile void * p);
    void release();
    
    int iwl_poll_bit(UInt32 addr, UInt32 bits, UInt32 mask, int timeout);
    void iwl_write32(UInt32 ofs, UInt32 val);
    UInt32 iwl_read32(UInt32 ofs);
    void iwl_set_bit(UInt32 reg, UInt32 mask);
    void iwl_clear_bit(UInt32 reg, UInt32 mask);
    
private:
    volatile void *fBaseAddr;
    void iwl_trans_pcie_set_bits_mask(UInt32 reg, UInt32 mask, UInt32 value);
    void __iwl_trans_pcie_set_bits_mask(UInt32 reg, UInt32 mask, UInt32 value);
    
};

#endif /* IntelIO_hpp */
