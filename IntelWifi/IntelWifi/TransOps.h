//
//  IwlTransOps.h
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef IwlTransOps_h
#define IwlTransOps_h

#include "iwl-trans.h"

class TransOps {
public:
    virtual int start_hw(struct iwl_trans *trans, bool low_power) = 0;
    virtual void op_mode_leave(struct iwl_trans *trans) = 0;
    virtual void stop_device(struct iwl_trans *trans, bool low_power) = 0;
    virtual int start_fw(struct iwl_trans *trans, const struct fw_img *fw, bool run_in_rfkill) = 0;
    
    
//    int (*start_fw)(struct iwl_trans *trans, const struct fw_img *fw,
//                    bool run_in_rfkill);
//    int (*update_sf)(struct iwl_trans *trans,
//                     struct iwl_sf_region *st_fwrd_space);


//
//    void (*d3_suspend)(struct iwl_trans *trans, bool test, bool reset);
//    int (*d3_resume)(struct iwl_trans *trans, enum iwl_d3_status *status,
//                     bool test, bool reset);
//
//    int (*send_cmd)(struct iwl_trans *trans, struct iwl_host_cmd *cmd);
//
//    int (*tx)(struct iwl_trans *trans, struct sk_buff *skb,
//              struct iwl_device_cmd *dev_cmd, int queue);
//    void (*reclaim)(struct iwl_trans *trans, int queue, int ssn,
//                    struct sk_buff_head *skbs);
//
//    bool (*txq_enable)(struct iwl_trans *trans, int queue, u16 ssn,
//                       const struct iwl_trans_txq_scd_cfg *cfg,
//                       unsigned int queue_wdg_timeout);
//    void (*txq_disable)(struct iwl_trans *trans, int queue,
//                        bool configure_scd);
//    /* a000 functions */
//    int (*txq_alloc)(struct iwl_trans *trans,
//                     struct iwl_tx_queue_cfg_cmd *cmd,
//                     int cmd_id,
//                     unsigned int queue_wdg_timeout);
//    void (*txq_free)(struct iwl_trans *trans, int queue);
//
//    void (*txq_set_shared_mode)(struct iwl_trans *trans, u32 txq_id,
//                                bool shared);
//
//    int (*wait_tx_queues_empty)(struct iwl_trans *trans, u32 txq_bm);
//    int (*wait_txq_empty)(struct iwl_trans *trans, int queue);
//    void (*freeze_txq_timer)(struct iwl_trans *trans, unsigned long txqs,
//                             bool freeze);
//    void (*block_txq_ptrs)(struct iwl_trans *trans, bool block);
//
//    void (*write8)(struct iwl_trans *trans, u32 ofs, u8 val);
//    void (*write32)(struct iwl_trans *trans, u32 ofs, u32 val);
//    u32 (*read32)(struct iwl_trans *trans, u32 ofs);
//    u32 (*read_prph)(struct iwl_trans *trans, u32 ofs);
//    void (*write_prph)(struct iwl_trans *trans, u32 ofs, u32 val);
//    int (*read_mem)(struct iwl_trans *trans, u32 addr,
//                    void *buf, int dwords);
//    int (*write_mem)(struct iwl_trans *trans, u32 addr,
//                     const void *buf, int dwords);


//    bool (*grab_nic_access)(struct iwl_trans *trans, unsigned long *flags);
//    void (*release_nic_access)(struct iwl_trans *trans,
//                               unsigned long *flags);
//    void (*set_bits_mask)(struct iwl_trans *trans, u32 reg, u32 mask,
//                          u32 value);
//    void (*ref)(struct iwl_trans *trans);
//    void (*unref)(struct iwl_trans *trans);
//    int  (*suspend)(struct iwl_trans *trans);
//    void (*resume)(struct iwl_trans *trans);

};


#endif /* IwlTransOps_h */
