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

#include "iwl-trans-pcie.h"

class IntelIO : public OSObject {
    OSDeclareDefaultStructors(IntelIO);
public:
    static IntelIO* withTrans(struct iwl_trans_pcie*);
    bool initWithTrans(struct iwl_trans_pcie*);
    virtual void free() override;
    
    int iwl_poll_bit(UInt32 addr, UInt32 bits, UInt32 mask, int timeout);
    void iwl_write8(u32 ofs, u8 val);
    
    void iwl_write32(UInt32 ofs, UInt32 val);
    UInt32 iwl_read32(UInt32 ofs);
    void iwl_set_bit(UInt32 reg, UInt32 mask);
    void iwl_clear_bit(UInt32 reg, UInt32 mask);
    
    bool iwl_grab_nic_access(IOInterruptState*);
    void iwl_release_nic_access(IOInterruptState*);
    UInt32 iwl_read_prph_no_grab(UInt32 reg);
    void iwl_write_prph_no_grab(UInt32 addr, UInt32 val);
    UInt32 iwl_read_prph(UInt32 ofs);
    void iwl_write_prph(UInt32 ofs, UInt32 val);
    int iwl_poll_prph_bit(UInt32 addr, UInt32 bits, UInt32 mask, int timeout);
    void iwl_set_bits_prph(UInt32 ofs, UInt32 mask);
    
    void iwl_set_bits_mask_prph(UInt32 ofs, UInt32 bits, UInt32 mask);
    void iwl_clear_bits_prph(UInt32 ofs, UInt32 mask);
    u32 iwl_trans_pcie_read_shr(u32 reg);
    void iwl_trans_pcie_write_shr(u32 reg, u32 val);
    int iwl_trans_pcie_read_mem(u32 addr, void *buf, int dwords);
    int iwl_trans_pcie_write_mem(u32 addr, const void *buf, int dwords);
    
    void iwl_write_direct32(u32 reg, u32 value);
    void iwl_write_direct64(u64 reg, u64 value);
    u32 iwl_read_direct32(u32 reg);
    void iwl_write64(u64 ofs, u64 val);
    int iwl_poll_direct_bit(u32 addr, u32 mask, int timeout);
    void iwl_write_prph64_no_grab(u64 ofs, u64 val);
    void iwl_force_nmi(struct iwl_trans *trans);
    
    void __iwl_trans_pcie_clear_bit(UInt32 reg, UInt32 mask);
    void __iwl_trans_pcie_set_bit(UInt32 reg, UInt32 mask);
    
private:
    struct iwl_trans_pcie* fTrans;
    
    void iwl_trans_pcie_set_bits_mask(UInt32 reg, UInt32 mask, UInt32 value);
    void __iwl_trans_pcie_set_bits_mask(UInt32 reg, UInt32 mask, UInt32 value);
    
   
};

#endif /* IntelIO_hpp */
