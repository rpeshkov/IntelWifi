//
//  IntelWifi_rx.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 02/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IntelWifi.hpp"

#include <IOKit/IODMACommand.h>

/* a device (PCI-E) page is 4096 bytes long */
#define ICT_SHIFT    12
#define ICT_SIZE    (1 << ICT_SHIFT)
#define ICT_COUNT    (ICT_SIZE / sizeof(u32))


/*
 * allocate dram shared table, it is an aligned memory
 * block of ICT_SIZE.
 * also reset all data related to ICT table interrupt.
 */
int IntelWifi::iwl_pcie_alloc_ict(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    
    
//    trans_pcie->ict_tbl =
//    dma_zalloc_coherent(trans->dev, ICT_SIZE,
//                        &trans_pcie->ict_tbl_dma,
//                        GFP_KERNEL);
    if (!trans_pcie->ict_tbl)
        return -ENOMEM;
    
    /* just an API sanity check ... it is guaranteed to be aligned */
    if (trans_pcie->ict_tbl_dma & (ICT_SIZE - 1)) {
        iwl_pcie_free_ict(trans);
        return -EINVAL;
    }
    
    return 0;
}

/* Free dram table */
void IntelWifi::iwl_pcie_free_ict(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    if (trans_pcie->ict_tbl) {
//        dma_free_coherent(trans->dev, ICT_SIZE,
//                          trans_pcie->ict_tbl,
//                          trans_pcie->ict_tbl_dma);
        trans_pcie->ict_tbl = NULL;
        trans_pcie->ict_tbl_dma = 0;
    }
}



/* Device is going down disable ict interrupt usage */
void IntelWifi::iwl_pcie_disable_ict(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    IOSimpleLockLock(trans_pcie->irq_lock);
    trans_pcie->use_ict = false;
    IOSimpleLockUnlock(trans_pcie->irq_lock);
}


