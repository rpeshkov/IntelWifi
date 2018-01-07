/******************************************************************************
 *
 * Copyright(c) 2003 - 2014 Intel Corporation. All rights reserved.
 * Copyright(c) 2013 - 2015 Intel Mobile Communications GmbH
 * Copyright(c) 2016 - 2017 Intel Deutschland GmbH
 *
 * Portions of this file are derived from the ipw3945 project, as well
 * as portions of the ieee80211 subsystem header files.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 *  Intel Linux Wireless <linuxwifi@intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 *****************************************************************************/

#include "IntelWifi.hpp"

#include <libkern/OSDebug.h>

#include "iwlwifi/fw/api/tx.h"

#define IWL_TX_CRC_SIZE 4
#define IWL_TX_DELIMITER_SIZE 4

/*************** DMA-QUEUE-GENERAL-FUNCTIONS  *****
 * DMA services
 *
 * Theory of operation
 *
 * A Tx or Rx queue resides in host DRAM, and is comprised of a circular buffer
 * of buffer descriptors, each of which points to one or more data buffers for
 * the device to read from or fill.  Driver and device exchange status of each
 * queue via "read" and "write" pointers.  Driver keeps minimum of 2 empty
 * entries in each circular buffer, to protect against confusing empty and full
 * queue states.
 *
 * The device reads or writes the data in the queues via the device's several
 * DMA/FIFO channels.  Each queue is mapped to a single DMA channel.
 *
 * For Tx queue, there are low mark and high mark limits. If, after queuing
 * the packet for Tx, free space become < low mark, Tx queue stopped. When
 * reclaiming packets (on 'tx done IRQ), if free space become > high mark,
 * Tx queue resumed.
 *
 ***************************************************/

int IntelWifi::iwl_queue_space(const struct iwl_txq *q)
{
    unsigned int max;
    unsigned int used;
    
    /*
     * To avoid ambiguity between empty and completely full queues, there
     * should always be less than TFD_QUEUE_SIZE_MAX elements in the queue.
     * If q->n_window is smaller than TFD_QUEUE_SIZE_MAX, there is no need
     * to reserve any queue entries for this purpose.
     */
    if (q->n_window < TFD_QUEUE_SIZE_MAX)
        max = q->n_window;
    else
        max = TFD_QUEUE_SIZE_MAX - 1;
    
    /*
     * TFD_QUEUE_SIZE_MAX is a power of 2, so the following is equivalent to
     * modulo by TFD_QUEUE_SIZE_MAX and is well defined.
     */
    used = (q->write_ptr - q->read_ptr) & (TFD_QUEUE_SIZE_MAX - 1);
    
    if (WARN_ON(used > max))
        return 0;
    
    return max - used;
}

/*
 * iwl_queue_init - Initialize queue's high/low-water and read/write indexes
 */
int IntelWifi::iwl_queue_init(struct iwl_txq *q, int slots_num)
{
    q->n_window = slots_num;
    
    /* slots_num must be power-of-two size, otherwise
     * iwl_pcie_get_cmd_index is broken. */
    if (WARN_ON(!is_power_of_2(slots_num)))
        return -EINVAL;
    
    q->low_mark = q->n_window / 4;
    if (q->low_mark < 4)
        q->low_mark = 4;
    
    q->high_mark = q->n_window / 8;
    if (q->high_mark < 2)
        q->high_mark = 2;
    
    q->write_ptr = 0;
    q->read_ptr = 0;
    
    return 0;
}

// line 127
int IntelWifi::iwl_pcie_alloc_dma_ptr(struct iwl_trans *trans,
                           struct iwl_dma_ptr *ptr, size_t size)
{
    if (ptr->addr)
        return -EINVAL;
    
    IOBufferMemoryDescriptor *bmd =
    IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task, (kIODirectionInOut | kIOMemoryPhysicallyContiguous | kIOMapInhibitCache), size, 0x00000000FFFFFFFFULL);
    
    IODMACommand *cmd = IODMACommand::withSpecification(kIODMACommandOutputHost64, 64, 0, IODMACommand::kMapped, 0, 1);
    cmd->setMemoryDescriptor(bmd);
    cmd->prepare();
    
    IODMACommand::Segment64 seg;
    UInt64 ofs = 0;
    UInt32 numSegs = 1;
    
    if (cmd->gen64IOVMSegments(&ofs, &seg, &numSegs) != kIOReturnSuccess) {
        TraceLog("EVERYTHING IS VEEEERY BAAAD :(");
        return -1;
    }
    
    ptr->addr = bmd->getBytesNoCopy();
    
    if (!ptr->addr)
        return -ENOMEM;
    ptr->dma = seg.fIOVMAddr;
    ptr->size = size;
    return 0;
}


// line 217
void IntelWifi::iwl_pcie_txq_inval_byte_cnt_tbl(struct iwl_trans *trans, struct iwl_txq *txq)
{
    struct iwl_trans_pcie *trans_pcie =
    IWL_TRANS_GET_PCIE_TRANS(trans);
    struct iwlagn_scd_bc_tbl *scd_bc_tbl = (struct iwlagn_scd_bc_tbl *)trans_pcie->scd_bc_tbls.addr;
    int txq_id = txq->id;
    int read_ptr = txq->read_ptr;
    u8 sta_id = 0;
    __le16 bc_ent;
    struct iwl_tx_cmd *tx_cmd = (struct iwl_tx_cmd *)txq->entries[read_ptr].cmd->payload;
    
    //WARN_ON(read_ptr >= TFD_QUEUE_SIZE_MAX);
    
    if (txq_id != trans_pcie->cmd_queue)
        sta_id = tx_cmd->sta_id;
    
    bc_ent = cpu_to_le16(1 | (sta_id << 12));
    
    scd_bc_tbl[txq_id].tfd_offset[read_ptr] = bc_ent;
    
    if (read_ptr < TFD_QUEUE_SIZE_BC_DUP)
        scd_bc_tbl[txq_id].
        tfd_offset[TFD_QUEUE_SIZE_MAX + read_ptr] = bc_ent;
}


/* line 244
 * iwl_pcie_txq_inc_wr_ptr - Send new write index to hardware
 */
void IntelWifi::iwl_pcie_txq_inc_wr_ptr(struct iwl_trans *trans,
                                    struct iwl_txq *txq)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    u32 reg = 0;
    int txq_id = txq->id;
    
    //lockdep_assert_held(&txq->lock);
    
    /*
     * explicitly wake up the NIC if:
     * 1. shadow registers aren't enabled
     * 2. NIC is woken up for CMD regardless of shadow outside this function
     * 3. there is a chance that the NIC is asleep
     */
    if (!trans->cfg->base_params->shadow_reg_enable &&
        txq_id != trans_pcie->cmd_queue &&
        test_bit(STATUS_TPOWER_PMI, &trans->status)) {
        /*
         * wake up nic if it's powered down ...
         * uCode will wake up, and interrupt us again, so next
         * time we'll skip this part.
         */
        reg = io->iwl_read32(CSR_UCODE_DRV_GP1);
        
        if (reg & CSR_UCODE_DRV_GP1_BIT_MAC_SLEEP) {
            IWL_DEBUG_INFO(trans, "Tx queue %d requesting wakeup, GP1 = 0x%x\n",
                           txq_id, reg);
            io->iwl_set_bit(CSR_GP_CNTRL,
                        CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
            txq->need_update = true;
            return;
        }
    }
    
    /*
     * if not in power-save mode, uCode will never sleep when we're
     * trying to tx (during RFKILL, we're not trying to tx).
     */
    IWL_DEBUG_TX(trans, "Q:%d WR: 0x%x\n", txq_id, txq->write_ptr);
    if (!txq->block)
        io->iwl_write32(HBUS_TARG_WRPTR,
                    txq->write_ptr | (txq_id << 8));
}

// line 292
void IntelWifi::iwl_pcie_txq_check_wrptrs(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    int i;
    
    for (i = 0; i < trans->cfg->base_params->num_of_queues; i++) {
        struct iwl_txq *txq = trans_pcie->txq[i];
        
        if (!test_bit(i, trans_pcie->queue_used))
            continue;

        //spin_lock_bh(&txq->lock);
        IOSimpleLockLock(txq->lock);
        if (txq->need_update) {
            iwl_pcie_txq_inc_wr_ptr(trans, txq);
            txq->need_update = false;
        }
        //spin_unlock_bh(&txq->lock);
        IOSimpleLockUnlock(txq->lock);
    }
}

// line 312
dma_addr_t IntelWifi::iwl_pcie_tfd_tb_get_addr(struct iwl_trans *trans, void *_tfd, u8 idx)
{
    
    if (trans->cfg->use_tfh) {
        struct iwl_tfh_tfd *tfd = (struct iwl_tfh_tfd *)_tfd;
        struct iwl_tfh_tb *tb = &tfd->tbs[idx];
        
        return (dma_addr_t)(le64_to_cpu(tb->addr));
    } else {
        struct iwl_tfd *tfd = (struct iwl_tfd *)_tfd;
        struct iwl_tfd_tb *tb = &tfd->tbs[idx];
        dma_addr_t addr = get_unaligned_le32(&tb->lo);
        dma_addr_t hi_len;
        
        if (sizeof(dma_addr_t) <= sizeof(u32))
            return addr;
        
        hi_len = le16_to_cpu(tb->hi_n_len) & 0xF;
        
        /*
         * shift by 16 twice to avoid warnings on 32-bit
         * (where this code never runs anyway due to the
         * if statement above)
         */
        return addr | ((hi_len << 16) << 16);
    }
}


// line 341
void IntelWifi::iwl_pcie_tfd_set_tb(struct iwl_trans *trans, void *tfd,
                                       u8 idx, dma_addr_t addr, u16 len)
{
    struct iwl_tfd *tfd_fh = (struct iwl_tfd *)tfd;
    struct iwl_tfd_tb *tb = &tfd_fh->tbs[idx];
    
    u16 hi_n_len = len << 4;
    
    put_unaligned_le32((u32)addr, &tb->lo);
    hi_n_len |= iwl_get_dma_hi_addr(addr);
    
    tb->hi_n_len = cpu_to_le16(hi_n_len);
    
    tfd_fh->num_tbs = idx + 1;
}

// line 357
u8 IntelWifi::iwl_pcie_tfd_get_num_tbs(struct iwl_trans *trans, void *_tfd)
{
    if (trans->cfg->use_tfh) {
        struct iwl_tfh_tfd *tfd = (struct iwl_tfh_tfd *)_tfd;
        
        return le16_to_cpu(tfd->num_tbs) & 0x1f;
    } else {
        struct iwl_tfd *tfd = (struct iwl_tfd *)_tfd;
        
        return tfd->num_tbs & 0x1f;
    }
}

// line 487
int IntelWifi::iwl_pcie_txq_alloc(struct iwl_trans *trans, struct iwl_txq *txq,
                       int slots_num, bool cmd_queue)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    size_t tfd_sz = trans_pcie->tfd_size * TFD_QUEUE_SIZE_MAX;
    size_t tb0_buf_sz;
    int i;
    int ret;
    struct iwl_dma_ptr tfds_dma;
    struct iwl_dma_ptr first_tb_bufs_dma;
    
    if (WARN_ON(txq->entries || txq->tfds))
        return -EINVAL;

    // TODO: Implement
//    setup_timer(&txq->stuck_timer, iwl_pcie_txq_stuck_timer,
//                (unsigned long)txq);
    txq->trans_pcie = trans_pcie;
    
    txq->n_window = slots_num;
    
    txq->entries = (struct iwl_pcie_txq_entry *) IOMalloc(sizeof(struct iwl_pcie_txq_entry) * slots_num);
    
    if (!txq->entries)
        goto error;
    
    bzero(txq->entries, sizeof(struct iwl_pcie_txq_entry) * slots_num);
    if (cmd_queue)
        for (i = 0; i < slots_num; i++) {
            txq->entries[i].cmd = (struct iwl_device_cmd *)IOMalloc(sizeof(struct iwl_device_cmd));
            if (!txq->entries[i].cmd)
                goto error;
        }
    
    /* Circular buffer of transmit frame descriptors (TFDs),
     * shared with device */
    tfds_dma = {.addr = NULL};
    ret = iwl_pcie_alloc_dma_ptr(trans, &tfds_dma, tfd_sz);
    if (ret) {
        goto error;
    }
    txq->tfds = tfds_dma.addr;
    txq->dma_addr = tfds_dma.dma;
    
    tb0_buf_sz = sizeof(*txq->first_tb_bufs) * slots_num;
    
    first_tb_bufs_dma = { .addr = NULL };
    ret = iwl_pcie_alloc_dma_ptr(trans, &first_tb_bufs_dma, tb0_buf_sz);
    if (ret) {
        goto err_free_tfds;
    }
    txq->first_tb_bufs = (struct iwl_pcie_first_tb_buf *)first_tb_bufs_dma.addr;
    txq->first_tb_dma = first_tb_bufs_dma.dma;
    
    return 0;
err_free_tfds:
    //dma_free_coherent(trans->dev, tfd_sz, txq->tfds, txq->dma_addr);
error:
//    if (txq->entries && cmd_queue)
//        for (i = 0; i < slots_num; i++)
//            kfree(txq->entries[i].cmd);
//    kfree(txq->entries);
//    txq->entries = NULL;
    
    return -ENOMEM;
    
}




// line 551
int IntelWifi::iwl_pcie_txq_init(struct iwl_trans *trans, struct iwl_txq *txq,
                      int slots_num, bool cmd_queue)
{
    int ret;
    
    txq->need_update = false;
    
    /* TFD_QUEUE_SIZE_MAX must be power-of-two size, otherwise
     * iwl_queue_inc_wrap and iwl_queue_dec_wrap are broken. */
    //BUILD_BUG_ON(TFD_QUEUE_SIZE_MAX & (TFD_QUEUE_SIZE_MAX - 1));
    
    /* Initialize queue's high/low-water marks, and head/tail indexes */
    ret = iwl_queue_init(txq, slots_num);
    if (ret)
        return ret;
    
    //spin_lock_init(&txq->lock);
    txq->lock = IOSimpleLockAlloc();
    
    if (cmd_queue) {
        // TODO: Implement
//        static struct lock_class_key iwl_pcie_cmd_queue_lock_class;
//
//        lockdep_set_class(&txq->lock, &iwl_pcie_cmd_queue_lock_class);
    }
    
    // TODO: Implement
    //__skb_queue_head_init(&txq->overflow_q);
    
    return 0;
}

// line 593
void IntelWifi::iwl_pcie_clear_cmd_in_flight(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    //lockdep_assert_held(&trans_pcie->reg_lock);
    
    if (trans_pcie->ref_cmd_in_flight) {
        trans_pcie->ref_cmd_in_flight = false;
        IWL_DEBUG_RPM(trans, "clear ref_cmd_in_flight - unref\n");
        // TODO: Implement ops
        //iwl_trans_unref(trans);
    }
    
    if (!trans->cfg->base_params->apmg_wake_up_wa)
        return;
    if (WARN_ON(!trans_pcie->cmd_hold_nic_awake))
        return;
    
    trans_pcie->cmd_hold_nic_awake = false;
    io->__iwl_trans_pcie_clear_bit(CSR_GP_CNTRL,
                               CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
}


// line 715
void IntelWifi::iwl_pcie_tx_start(struct iwl_trans *trans, u32 scd_base_addr)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    int nq = trans->cfg->base_params->num_of_queues;
    int chan;
    u32 reg_val;
    int clear_dwords = (SCD_TRANS_TBL_OFFSET_QUEUE(nq) -
                        SCD_CONTEXT_MEM_LOWER_BOUND) / sizeof(u32);
    
    /* make sure all queue are not stopped/used */
    memset(trans_pcie->queue_stopped, 0, sizeof(trans_pcie->queue_stopped));
    memset(trans_pcie->queue_used, 0, sizeof(trans_pcie->queue_used));
    
    trans_pcie->scd_base_addr =
    io->iwl_read_prph(SCD_SRAM_BASE_ADDR);
    
//    WARN_ON(scd_base_addr != 0 &&
//            scd_base_addr != trans_pcie->scd_base_addr);
    
    /* reset context data, TX status and translation data */
    io->iwl_trans_pcie_write_mem(trans_pcie->scd_base_addr +
                        SCD_CONTEXT_MEM_LOWER_BOUND,
                        NULL, clear_dwords);
    
    io->iwl_write_prph(SCD_DRAM_BASE_ADDR,
                   trans_pcie->scd_bc_tbls.dma >> 10);
    
    /* The chain extension of the SCD doesn't work well. This feature is
     * enabled by default by the HW, so we need to disable it manually.
     */
    if (trans->cfg->base_params->scd_chain_ext_wa)
        io->iwl_write_prph(SCD_CHAINEXT_EN, 0);
    
    iwl_trans_ac_txq_enable(trans, trans_pcie->cmd_queue,
                            trans_pcie->cmd_fifo,
                            trans_pcie->cmd_q_wdg_timeout);
    
    /* Activate all Tx DMA/FIFO channels */
    iwl_scd_activate_fifos(trans);
    
    /* Enable DMA channel */
    for (chan = 0; chan < FH_TCSR_CHNL_NUM; chan++)
        io->iwl_write_direct32(FH_TCSR_CHNL_TX_CONFIG_REG(chan),
                           FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_ENABLE |
                           FH_TCSR_TX_CONFIG_REG_VAL_DMA_CREDIT_ENABLE);
    
    /* Update FH chicken bits */
    reg_val = io->iwl_read_direct32(FH_TX_CHICKEN_BITS_REG);
    io->iwl_write_direct32(FH_TX_CHICKEN_BITS_REG,
                       reg_val | FH_TX_CHICKEN_BITS_SCD_AUTO_RETRY_EN);
    
    /* Enable L1-Active */
    if (trans->cfg->device_family < IWL_DEVICE_FAMILY_8000)
        io->iwl_clear_bits_prph(APMG_PCIDEV_STT_REG,
                            APMG_PCIDEV_STT_VAL_L1_ACT_DIS);
}

// line 812
void IntelWifi::iwl_pcie_tx_stop_fh(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    IOInterruptState state;
    int ch, ret;
    u32 mask = 0;

    IOSimpleLockLock(trans_pcie->irq_lock);
    
    if (!io->iwl_grab_nic_access(&state))
        goto out;
    
    /* Stop each Tx DMA channel */
    for (ch = 0; ch < FH_TCSR_CHNL_NUM; ch++) {
        io->iwl_write32(FH_TCSR_CHNL_TX_CONFIG_REG(ch), 0x0);
        mask |= FH_TSSR_TX_STATUS_REG_MSK_CHNL_IDLE(ch);
    }
    
    /* Wait for DMA channels to be idle */
    ret = io->iwl_poll_bit(FH_TSSR_TX_STATUS_REG, mask, mask, 5000);
    if (ret < 0)
        IWL_ERR(trans,
                "Failing on timeout while stopping DMA channel %d [0x%08x]\n",
                ch, io->iwl_read32(FH_TSSR_TX_STATUS_REG));
    
    io->iwl_release_nic_access(&state);
    
out:
    IOSimpleLockUnlock(trans_pcie->irq_lock);
}

/* line 907
 * iwl_pcie_tx_alloc - allocate TX context
 * Allocate all Tx DMA structures and initialize them
 */
int IntelWifi::iwl_pcie_tx_alloc(struct iwl_trans *trans)
{
    int ret;
    int txq_id, slots_num;
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    u16 scd_bc_tbls_size = trans->cfg->base_params->num_of_queues *
    sizeof(struct iwlagn_scd_bc_tbl);
    
    /*It is not allowed to alloc twice, so warn when this happens.
     * We cannot rely on the previous allocation, so free and fail */
    if (WARN_ON(trans_pcie->txq_memory)) {
        ret = -EINVAL;
        goto error;
    }
    
    ret = iwl_pcie_alloc_dma_ptr(trans, &trans_pcie->scd_bc_tbls,
                                 scd_bc_tbls_size);
    if (ret) {
        IWL_ERR(trans, "Scheduler BC Table allocation failed\n");
        goto error;
    }
    
    /* Alloc keep-warm buffer */
    ret = iwl_pcie_alloc_dma_ptr(trans, &trans_pcie->kw, IWL_KW_SIZE);
    if (ret) {
        IWL_ERR(trans, "Keep Warm allocation failed\n");
        goto error;
    }

    trans_pcie->txq_memory = (struct iwl_txq *)IOMalloc(sizeof(struct iwl_txq) * trans->cfg->base_params->num_of_queues);
    if (!trans_pcie->txq_memory) {
        IWL_ERR(trans, "Not enough memory for txq\n");
        ret = -ENOMEM;
        goto error;
    }
    bzero(trans_pcie->txq_memory, sizeof(struct iwl_txq) * trans->cfg->base_params->num_of_queues);
    
    /* Alloc and init all Tx queues, including the command queue (#4/#9) */
    for (txq_id = 0; txq_id < trans->cfg->base_params->num_of_queues;
         txq_id++) {
        bool cmd_queue = (txq_id == trans_pcie->cmd_queue);
        
        slots_num = cmd_queue ? TFD_CMD_SLOTS : TFD_TX_CMD_SLOTS;
        trans_pcie->txq[txq_id] = &trans_pcie->txq_memory[txq_id];
        ret = iwl_pcie_txq_alloc(trans, trans_pcie->txq[txq_id],
                                 slots_num, cmd_queue);
        if (ret) {
            IWL_ERR(trans, "Tx %d queue alloc failed\n", txq_id);
            goto error;
        }
        trans_pcie->txq[txq_id]->id = txq_id;
    }
    
    return 0;
    
error:
    // TODO: Implement
    //iwl_pcie_tx_free(trans);
    
    return ret;
}


// line 973
int IntelWifi::iwl_pcie_tx_init(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    int ret;
    int txq_id, slots_num;
    bool alloc = false;
    
    if (!trans_pcie->txq_memory) {
        ret = iwl_pcie_tx_alloc(trans);
        if (ret)
            goto error;
        alloc = true;
    }
    
    //spin_lock(&trans_pcie->irq_lock);
    IOSimpleLockLock(trans_pcie->irq_lock);
    
    /* Turn off all Tx DMA fifos */
    iwl_scd_deactivate_fifos(trans);
    
    /* Tell NIC where to find the "keep warm" buffer */
    io->iwl_write_direct32(FH_KW_MEM_ADDR_REG,
                       trans_pcie->kw.dma >> 4);
    
    // spin_unlock(&trans_pcie->irq_lock);
    IOSimpleLockUnlock(trans_pcie->irq_lock);
    
    /* Alloc and init all Tx queues, including the command queue (#4/#9) */
    for (txq_id = 0; txq_id < trans->cfg->base_params->num_of_queues;
         txq_id++) {
        bool cmd_queue = (txq_id == trans_pcie->cmd_queue);
        
        slots_num = cmd_queue ? TFD_CMD_SLOTS : TFD_TX_CMD_SLOTS;
        ret = iwl_pcie_txq_init(trans, trans_pcie->txq[txq_id],
                                slots_num, cmd_queue);
        if (ret) {
            IWL_ERR(trans, "Tx %d queue init failed\n", txq_id);
            goto error;
        }
        
        /*
         * Tell nic where to find circular buffer of TFDs for a
         * given Tx queue, and enable the DMA channel used for that
         * queue.
         * Circular buffer (TFD queue in DRAM) physical base address
         */
        io->iwl_write_direct32(FH_MEM_CBBC_QUEUE(trans, txq_id),
                           trans_pcie->txq[txq_id]->dma_addr >> 8);
    }
    
    io->iwl_set_bits_prph(SCD_GP_CTRL, SCD_GP_CTRL_AUTO_ACTIVE_MODE);
    if (trans->cfg->base_params->num_of_queues > 20)
        io->iwl_set_bits_prph(SCD_GP_CTRL,
                          SCD_GP_CTRL_ENABLE_31_QUEUES);
    
    return 0;
error:
    /*Upon error, free only if we allocated something */
    // TODO: Implement
//    if (alloc)
//        iwl_pcie_tx_free(trans);
    return ret;
}

// line 1034
void IntelWifi::iwl_pcie_txq_progress(struct iwl_txq *txq)
{
    //lockdep_assert_held(&txq->lock);
    
    if (!txq->wd_timeout)
        return;
    
    /*
     * station is asleep and we send data - that must
     * be uAPSD or PS-Poll. Don't rearm the timer.
     */
    if (txq->frozen)
        return;
    
    /*
     * if empty delete timer, otherwise move timer forward
     * since we're making progress on this queue
     */
    // TODO: Implement
//    if (txq->read_ptr == txq->write_ptr)
//        del_timer(&txq->stuck_timer);
//    else
//        mod_timer(&txq->stuck_timer, jiffies + txq->wd_timeout);
}



// line 1168
int IntelWifi::iwl_pcie_set_cmd_in_flight(struct iwl_trans *trans, const struct iwl_host_cmd *cmd)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    int ret;
    
    //lockdep_assert_held(&trans_pcie->reg_lock);
    
    if (!(cmd->flags & CMD_SEND_IN_IDLE) &&
        !trans_pcie->ref_cmd_in_flight) {
        trans_pcie->ref_cmd_in_flight = true;
        IWL_DEBUG_RPM(trans, "set ref_cmd_in_flight - ref\n");
        // TODO: Implement ops
        //iwl_trans_ref(trans);
    }
    
    /*
     * wake up the NIC to make sure that the firmware will see the host
     * command - we will let the NIC sleep once all the host commands
     * returned. This needs to be done only on NICs that have
     * apmg_wake_up_wa set.
     */
    if (trans->cfg->base_params->apmg_wake_up_wa &&
        !trans_pcie->cmd_hold_nic_awake) {
        io->__iwl_trans_pcie_set_bit(CSR_GP_CNTRL,
                                 CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
        
        ret = io->iwl_poll_bit(CSR_GP_CNTRL,
                           CSR_GP_CNTRL_REG_VAL_MAC_ACCESS_EN,
                           (CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY |
                            CSR_GP_CNTRL_REG_FLAG_GOING_TO_SLEEP),
                           15000);
        if (ret < 0) {
            io->__iwl_trans_pcie_clear_bit(CSR_GP_CNTRL,
                                       CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
            IWL_ERR(trans, "Failed to wake NIC for hcmd\n");
            return -EIO;
        }
        trans_pcie->cmd_hold_nic_awake = true;
    }
    
    return 0;
}


/* line 1211
 * iwl_pcie_cmdq_reclaim - Reclaim TX command queue entries already Tx'd
 *
 * When FW advances 'R' index, all entries between old and new 'R' index
 * need to be reclaimed. As result, some free space forms.  If there is
 * enough free space (> low mark), wake the stack that feeds us.
 */
void IntelWifi::iwl_pcie_cmdq_reclaim(struct iwl_trans *trans, int txq_id, int idx)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    struct iwl_txq *txq = trans_pcie->txq[txq_id];
    IOInterruptState state;
    int nfreed = 0;
    
    //lockdep_assert_held(&txq->lock);
    
    if ((idx >= TFD_QUEUE_SIZE_MAX) || (!iwl_queue_used(txq, idx))) {
        IWL_ERR(trans,
                "%s: Read index for DMA queue txq id (%d), index %d is out of range [0-%d] %d %d.\n",
                __func__, txq_id, idx, TFD_QUEUE_SIZE_MAX,
                txq->write_ptr, txq->read_ptr);
        return;
    }
    
    for (idx = iwl_queue_inc_wrap(idx); txq->read_ptr != idx;
         txq->read_ptr = iwl_queue_inc_wrap(txq->read_ptr)) {
        
        if (nfreed++ > 0) {
            IWL_ERR(trans, "HCMD skipped: index (%d) %d %d\n",
                    idx, txq->write_ptr, txq->read_ptr);
            io->iwl_force_nmi(trans);
        }
    }
    
    if (txq->read_ptr == txq->write_ptr) {
        //spin_lock_irqsave(&trans_pcie->reg_lock, flags);
        state = IOSimpleLockLockDisableInterrupt(trans_pcie->reg_lock);
        // TODO: Implement
        iwl_pcie_clear_cmd_in_flight(trans);
        //spin_unlock_irqrestore(&trans_pcie->reg_lock, flags);
        IOSimpleLockUnlockEnableInterrupt(trans_pcie->reg_lock, state);
    }
    
    iwl_pcie_txq_progress(txq);
}



