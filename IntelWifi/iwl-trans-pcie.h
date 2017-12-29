//
//  iwl-trans-pcie.h
//  This file contains ported structure iwl_trans_pcie from original linux driver.
//  All linux specific things were changed to macos specific.
//  Some fields are redundand here, because those things will be handled in the main driver class IntelWifi
//
//  Created by Roman Peshkov on 29/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef iwl_trans_pcie_h
#define iwl_trans_pcie_h

struct iwl_trans_pcie {
    volatile void* hw_base;
    bool opmode_down;
    UInt8 max_tbs;
    UInt16 tfd_size;
    UInt8 max_skb_frags;
    UInt32 hw_rev;
    bool cmd_hold_nic_awake;
    
    IOSimpleLock* reg_lock;
};

static inline struct iwl_trans_pcie* iwl_trans_pcie_alloc() {
    struct iwl_trans_pcie* trans = (struct iwl_trans_pcie *)IOMalloc(sizeof(struct iwl_trans_pcie));
    trans->reg_lock = IOSimpleLockAlloc();
    return trans;
}

#endif /* iwl_trans_pcie_h */
