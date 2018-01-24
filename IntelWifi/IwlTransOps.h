//
//  IwlTransOps.h
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef IwlTransOps_h
#define IwlTransOps_h

#include <linux/types.h>

#include "iwl-trans.h"

class IwlTransOps {
public:
    virtual int start_hw(struct iwl_trans *trans, bool low_power) = 0;
    virtual int start_fw(struct iwl_trans *trans, const struct fw_img *fw, bool run_in_rfkill) = 0;
    virtual void op_mode_leave(struct iwl_trans *trans) = 0;
    virtual void set_pmi(struct iwl_trans *trans, bool state) = 0;
    virtual void configure(struct iwl_trans *trans, const struct iwl_trans_config *trans_cfg) = 0;
    virtual void fw_alive(struct iwl_trans *trans, u32 scd_addr) = 0;
    virtual void stop_device(struct iwl_trans *trans, bool low_power) = 0;
    virtual bool txq_enable(struct iwl_trans *trans, int queue, u16 ssn,
                               const struct iwl_trans_txq_scd_cfg *cfg,
                            unsigned int queue_wdg_timeout) = 0;
    virtual void txq_disable(struct iwl_trans *trans, int queue, bool configure_scd) = 0;
    
    inline void iwl_trans_txq_enable(struct iwl_trans *trans, int queue,
                                     int fifo, int sta_id, int tid,
                                     int frame_limit, u16 ssn,
            
                                    unsigned int queue_wdg_timeout) {
        struct iwl_trans_txq_scd_cfg cfg = {
            .fifo = (u8)fifo,
            .sta_id = (u8)sta_id,
            .tid = (u8)tid,
            .frame_limit = frame_limit,
            .aggregate = sta_id >= 0,
        };
    
        txq_enable(trans, queue, ssn, &cfg, queue_wdg_timeout);
    }

    inline
    void iwl_trans_ac_txq_enable(struct iwl_trans *trans, int queue, int fifo,
                     unsigned int queue_wdg_timeout)
    {
        struct iwl_trans_txq_scd_cfg cfg = {
            .fifo = (u8)fifo,
            .sta_id = (u8)-1,
            .tid = IWL_MAX_TID_COUNT,
            .frame_limit = IWL_FRAME_LIMIT,
            .aggregate = false,
        };
    
        txq_enable(trans, queue, 0, &cfg, queue_wdg_timeout);
    }
    
    virtual int send_cmd(struct iwl_trans *trans, struct iwl_host_cmd *cmd) = 0;
    
    int iwl_trans_send_cmd(struct iwl_trans *trans, struct iwl_host_cmd *cmd)
    {
        int ret;
        
        if (unlikely(!(cmd->flags & CMD_SEND_IN_RFKILL) &&
                     test_bit(STATUS_RFKILL_OPMODE, &trans->status)))
            return -ERFKILL;

        if (unlikely(test_bit(STATUS_FW_ERROR, &trans->status)))
            return -EIO;

        if (unlikely(trans->state != IWL_TRANS_FW_ALIVE)) {
            IWL_ERR(trans, "%s bad state = %d\n", __func__, trans->state);
            return -EIO;
        }

        if (WARN_ON((cmd->flags & CMD_WANT_ASYNC_CALLBACK) &&
                    !(cmd->flags & CMD_ASYNC)))
            return -EINVAL;

#ifdef CONFIG_LOCKDEP
        if (!(cmd->flags & CMD_ASYNC))
            lock_map_acquire_read(&trans->sync_cmd_lockdep_map);
#endif
        
        if (trans->wide_cmd_header && !iwl_cmd_groupid(cmd->id))
            cmd->id = DEF_ID(cmd->id);
        
        ret = send_cmd(trans, cmd);
        
//#ifdef CONFIG_LOCKDEP
//        if (!(cmd->flags & CMD_ASYNC))
//            lock_map_release(&trans->sync_cmd_lockdep_map);
//#endif
//
//        if (WARN_ON((cmd->flags & CMD_WANT_SKB) && !ret && !cmd->resp_pkt))
//            return -EIO;
        
        return ret;
    }

    
    
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
