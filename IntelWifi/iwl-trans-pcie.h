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

#include <linux/types.h>

/* We need 2 entries for the TX command and header, and another one might
 * be needed for potential data in the SKB's head. The remaining ones can
 * be used for frags.
 */
#define IWL_PCIE_MAX_FRAGS(x) (x->max_tbs - 3)


struct iwl_trans_pcie {
    struct iwl_trans *trans;
    
    volatile void* hw_base;
    bool opmode_down;
    UInt8 max_tbs;
    UInt16 tfd_size;
    UInt8 max_skb_frags;
    UInt32 hw_rev;
    bool cmd_hold_nic_awake;
    
    IOSimpleLock* reg_lock;
    IOSimpleLock* irq_lock;
    IOLock *mutex;
    bool is_down;
    
    bool msix_enabled;
    u8 shared_vec_mask;
    u32 alloc_vecs;
    u32 def_irq;
    u32 fh_init_mask;
    u32 hw_init_mask;
    u32 fh_mask;
    u32 hw_mask;
};

/*static inline struct iwl_trans_pcie* iwl_trans_pcie_alloc() {
    struct iwl_trans_pcie* trans = (struct iwl_trans_pcie *)IOMalloc(sizeof(struct iwl_trans_pcie));
    trans->reg_lock = IOSimpleLockAlloc();
    trans->mutex = IOLockAlloc();
    return trans;
}

static inline void iwl_trans_pcie_free(struct iwl_trans_pcie *trans) {
    IOLockFree(trans->mutex);
    IOSimpleLockFree(trans->reg_lock);
    IOFree(trans, sizeof(struct iwl_trans_pcie));
}*/

#endif /* iwl_trans_pcie_h */
