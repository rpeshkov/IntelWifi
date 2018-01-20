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

//
//  IntelWifi_rx.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 02/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IntelWifi.hpp"

#include <IOKit/IODMACommand.h>
#include <sys/queue.h>

extern "C" {
#include "iwl-fh.h"
}

/******************************************************************************
 *
 * RX path functions
 *
 ******************************************************************************/

/*
 * Rx theory of operation
 *
 * Driver allocates a circular buffer of Receive Buffer Descriptors (RBDs),
 * each of which point to Receive Buffers to be filled by the NIC.  These get
 * used not only for Rx frames, but for any command response or notification
 * from the NIC.  The driver and NIC manage the Rx buffers by means
 * of indexes into the circular buffer.
 *
 * Rx Queue Indexes
 * The host/firmware share two index registers for managing the Rx buffers.
 *
 * The READ index maps to the first position that the firmware may be writing
 * to -- the driver can read up to (but not including) this position and get
 * good data.
 * The READ index is managed by the firmware once the card is enabled.
 *
 * The WRITE index maps to the last position the driver has read from -- the
 * position preceding WRITE is the last slot the firmware can place a packet.
 *
 * The queue is empty (no good data) if WRITE = READ - 1, and is full if
 * WRITE = READ.
 *
 * During initialization, the host sets up the READ queue position to the first
 * INDEX position, and WRITE to the last (READ - 1 wrapped)
 *
 * When the firmware places a packet in a buffer, it will advance the READ index
 * and fire the RX interrupt.  The driver can then query the READ index and
 * process as many packets as possible, moving the WRITE index forward as it
 * resets the Rx queue buffers with new memory.
 *
 * The management in the driver is as follows:
 * + A list of pre-allocated RBDs is stored in iwl->rxq->rx_free.
 *   When the interrupt handler is called, the request is processed.
 *   The page is either stolen - transferred to the upper layer
 *   or reused - added immediately to the iwl->rxq->rx_free list.
 * + When the page is stolen - the driver updates the matching queue's used
 *   count, detaches the RBD and transfers it to the queue used list.
 *   When there are two used RBDs - they are transferred to the allocator empty
 *   list. Work is then scheduled for the allocator to start allocating
 *   eight buffers.
 *   When there are another 6 used RBDs - they are transferred to the allocator
 *   empty list and the driver tries to claim the pre-allocated buffers and
 *   add them to iwl->rxq->rx_free. If it fails - it continues to claim them
 *   until ready.
 *   When there are 8+ buffers in the free list - either from allocation or from
 *   8 reused unstolen pages - restock is called to update the FW and indexes.
 * + In order to make sure the allocator always has RBDs to use for allocation
 *   the allocator has initial pool in the size of num_queues*(8-2) - the
 *   maximum missing RBDs per allocation request (request posted with 2
 *    empty RBDs, there is no guarantee when the other 6 RBDs are supplied).
 *   The queues supplies the recycle of the rest of the RBDs.
 * + A received packet is processed and handed to the kernel network stack,
 *   detached from the iwl->rxq.  The driver 'processed' index is updated.
 * + If there are no allocated buffers in iwl->rxq->rx_free,
 *   the READ INDEX is not incremented and iwl->status(RX_STALLED) is set.
 *   If there were enough free buffers and RX_STALLED is set it is cleared.
 *
 *
 * Driver sequence:
 *
 * iwl_rxq_alloc()            Allocates rx_free
 * iwl_pcie_rx_replenish()    Replenishes rx_free list from rx_used, and calls
 *                            iwl_pcie_rxq_restock.
 *                            Used only during initialization.
 * iwl_pcie_rxq_restock()     Moves available buffers from rx_free into Rx
 *                            queue, updates firmware pointers, and updates
 *                            the WRITE index.
 * iwl_pcie_rx_allocator()     Background work for allocating pages.
 *
 * -- enable interrupts --
 * ISR - iwl_rx()             Detach iwl_rx_mem_buffers from pool up to the
 *                            READ INDEX, detaching the SKB from the pool.
 *                            Moves the packet buffer from queue to rx_used.
 *                            Posts and claims requests to the allocator.
 *                            Calls iwl_pcie_rxq_restock to refill any empty
 *                            slots.
 *
 * RBD life-cycle:
 *
 * Init:
 * rxq.pool -> rxq.rx_used -> rxq.rx_free -> rxq.queue
 *
 * Regular Receive interrupt:
 * Page Stolen:
 * rxq.queue -> rxq.rx_used -> allocator.rbd_empty ->
 * allocator.rbd_allocated -> rxq.rx_free -> rxq.queue
 * Page not Stolen:
 * rxq.queue -> rxq.rx_free -> rxq.queue
 * ...
 *
 */

/*
 * iwl_rxq_space - Return number of free slots available in queue.
 */
int IntelWifi::iwl_rxq_space(const struct iwl_rxq *rxq)
{
    /* Make sure rx queue size is a power of 2 */
    //WARN_ON(rxq->queue_size & (rxq->queue_size - 1));
    
    /*
     * There can be up to (RX_QUEUE_ISZE - 1) free slots, to avoid ambiguity
     * between empty and completely full queues.
     * The following is equivalent to modulo by RX_QUEUE_SIZE and is well
     * defined for negative dividends.
     */
    return (rxq->read - rxq->write - 1) & (rxq->queue_size - 1);
}

/*
 * iwl_dma_addr2rbd_ptr - convert a DMA address to a uCode read buffer ptr
 */
__le32 IntelWifi::iwl_pcie_dma_addr2rbd_ptr(dma_addr_t dma_addr)
{
    return cpu_to_le32((u32)(dma_addr >> 8));
}

/*
 * iwl_pcie_rx_stop - stops the Rx DMA
 */
int IntelWifi::iwl_pcie_rx_stop(struct iwl_trans *trans)
{
    if (trans->cfg->mq_rx_supported) {
        io->iwl_write_prph(RFH_RXF_DMA_CFG, 0);
        return io->iwl_poll_prph_bit(RFH_GEN_STATUS,
                                 RXF_DMA_IDLE, RXF_DMA_IDLE, 1000);
    } else {
        io->iwl_write_direct32(FH_MEM_RCSR_CHNL0_CONFIG_REG, 0);
        return io->iwl_poll_direct_bit(FH_MEM_RSSR_RX_STATUS_REG,
                                   FH_RSSR_CHNL0_RX_STATUS_CHNL_IDLE,
                                   1000);
    }
}

/*
 * iwl_pcie_rxq_inc_wr_ptr - Update the write pointer for the RX queue
 */
void IntelWifi::iwl_pcie_rxq_inc_wr_ptr(struct iwl_trans *trans, struct iwl_rxq *rxq)
{
    u32 reg;
    
    //lockdep_assert_held(&rxq->lock);
    
    /*
     * explicitly wake up the NIC if:
     * 1. shadow registers aren't enabled
     * 2. there is a chance that the NIC is asleep
     */
    if (!trans->cfg->base_params->shadow_reg_enable &&
        test_bit(STATUS_TPOWER_PMI, &trans->status)) {
        reg = io->iwl_read32(CSR_UCODE_DRV_GP1);
        
        if (reg & CSR_UCODE_DRV_GP1_BIT_MAC_SLEEP) {
            IWL_DEBUG_INFO(trans, "Rx queue requesting wakeup, GP1 = 0x%x\n",
                           reg);
            io->iwl_set_bit(CSR_GP_CNTRL,
                        CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
            rxq->need_update = true;
            return;
        }
    }
    
    rxq->write_actual = round_down(rxq->write, 8);
    if (trans->cfg->mq_rx_supported)
        io->iwl_write32(RFH_Q_FRBDCB_WIDX_TRG(rxq->id),
                    rxq->write_actual);
    else
        io->iwl_write32(FH_RSCSR_CHNL0_WPTR, rxq->write_actual);
}

void IntelWifi::iwl_pcie_rxq_check_wrptr(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    int i;
    
    for (i = 0; i < trans->num_rx_queues; i++) {
        struct iwl_rxq *rxq = &trans_pcie->rxq[i];
        
        if (!rxq->need_update)
            continue;
        IOSimpleLockLock(rxq->lock);
        iwl_pcie_rxq_inc_wr_ptr(trans, rxq);
        rxq->need_update = false;
        IOSimpleLockUnlock(rxq->lock);
    }
}

/*
 * iwl_pcie_rxmq_restock - restock implementation for multi-queue rx
 */
void IntelWifi::iwl_pcie_rxmq_restock(struct iwl_trans *trans,
                                  struct iwl_rxq *rxq)
{
    struct iwl_rx_mem_buffer *rxb;
    
    /*
     * If the device isn't enabled - no need to try to add buffers...
     * This can happen when we stop the device and still have an interrupt
     * pending. We stop the APM before we sync the interrupts because we
     * have to (see comment there). On the other hand, since the APM is
     * stopped, we cannot access the HW (in particular not prph).
     * So don't try to restock if the APM has been already stopped.
     */
    if (!test_bit(STATUS_DEVICE_ENABLED, &trans->status))
        return;
    
    //spin_lock(&rxq->lock);
    IOSimpleLockLock(rxq->lock);
    while (rxq->free_count) {
        __le64 *bd = (__le64 *)rxq->bd;
        
        /* Get next free Rx buffer, remove from free list */
        rxb = list_first_entry(&rxq->rx_free, struct iwl_rx_mem_buffer,
                               list);
        list_del(&rxb->list);
        rxb->invalid = false;
        /* 12 first bits are expected to be empty */
        
        //WARN_ON(rxb->page_dma & DMA_BIT_MASK(12));
        /* Point to Rx buffer via next RBD in circular buffer */
        bd[rxq->write] = cpu_to_le64(rxb->page_dma | rxb->vid);
        rxq->write = (rxq->write + 1) & MQ_RX_TABLE_MASK;
        rxq->free_count--;
    }
    IOSimpleLockUnlock(rxq->lock);
    
    /*
     * If we've added more space for the firmware to place data, tell it.
     * Increment device's write pointer in multiples of 8.
     */
    if (rxq->write_actual != (rxq->write & ~0x7)) {
        IOSimpleLockLock(rxq->lock);
        iwl_pcie_rxq_inc_wr_ptr(trans, rxq);
        IOSimpleLockUnlock(rxq->lock);
    }
}

/*
 * iwl_pcie_rxsq_restock - restock implementation for single queue rx
 */
void IntelWifi::iwl_pcie_rxsq_restock(struct iwl_trans *trans,
                                  struct iwl_rxq *rxq)
{
    struct iwl_rx_mem_buffer *rxb;
    
    /*
     * If the device isn't enabled - not need to try to add buffers...
     * This can happen when we stop the device and still have an interrupt
     * pending. We stop the APM before we sync the interrupts because we
     * have to (see comment there). On the other hand, since the APM is
     * stopped, we cannot access the HW (in particular not prph).
     * So don't try to restock if the APM has been already stopped.
     */
    if (!test_bit(STATUS_DEVICE_ENABLED, &trans->status))
        return;
    
    IOSimpleLockLock(rxq->lock);
    while ((iwl_rxq_space(rxq) > 0) && (rxq->free_count)) {
        __le32 *bd = (__le32 *)rxq->bd;
        /* The overwritten rxb must be a used one */
        rxb = rxq->queue[rxq->write];
        //BUG_ON(rxb && rxb->page);
        
        /* Get next free Rx buffer, remove from free list */
        rxb = list_first_entry(&rxq->rx_free, struct iwl_rx_mem_buffer,
                               list);
        list_del(&rxb->list);
        rxb->invalid = false;
        
        /* Point to Rx buffer via next RBD in circular buffer */
        bd[rxq->write] = iwl_pcie_dma_addr2rbd_ptr(rxb->page_dma);
        rxq->queue[rxq->write] = rxb;
        rxq->write = (rxq->write + 1) & RX_QUEUE_MASK;
        rxq->free_count--;
    }
    IOSimpleLockUnlock(rxq->lock);
    
    /* If we've added more space for the firmware to place data, tell it.
     * Increment device's write pointer in multiples of 8. */
    if (rxq->write_actual != (rxq->write & ~0x7)) {
        IOSimpleLockLock(rxq->lock);
        iwl_pcie_rxq_inc_wr_ptr(trans, rxq);
        IOSimpleLockUnlock(rxq->lock);
    }
}


/*
 * iwl_pcie_rxq_restock - refill RX queue from pre-allocated pool
 *
 * If there are slots in the RX queue that need to be restocked,
 * and we have free pre-allocated buffers, fill the ranks as much
 * as we can, pulling from rx_free.
 *
 * This moves the 'write' index forward to catch up with 'processed', and
 * also updates the memory address in the firmware to reference the new
 * target buffer.
 */
void IntelWifi::iwl_pcie_rxq_restock(struct iwl_trans *trans, struct iwl_rxq *rxq)
{
    if (trans->cfg->mq_rx_supported)
        iwl_pcie_rxmq_restock(trans, rxq);
    else
        iwl_pcie_rxsq_restock(trans, rxq);
}


int IntelWifi::iwl_pcie_rx_alloc(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    struct iwl_rb_allocator *rba = &trans_pcie->rba;
    //struct device *dev = trans->dev;
    int i;
    int free_size = trans->cfg->mq_rx_supported ? sizeof(__le64) : sizeof(__le32);
    
    if (WARN_ON(trans_pcie->rxq))
        return -EINVAL;
    
//    trans_pcie->rxq = kcalloc(trans->num_rx_queues, sizeof(struct iwl_rxq),
//                              GFP_KERNEL);
    trans_pcie->rxq = (struct iwl_rxq *)IOMalloc(sizeof(struct iwl_rxq) * trans->num_rx_queues);
    
    if (!trans_pcie->rxq)
        return -EINVAL;
    
    memset(trans_pcie->rxq, 0, sizeof(struct iwl_rxq) * trans->num_rx_queues);
    
    rba->lock = IOSimpleLockAlloc();
    
    for (i = 0; i < trans->num_rx_queues; i++) {
        struct iwl_rxq *rxq = &trans_pcie->rxq[i];
        
        rxq->lock = IOSimpleLockAlloc();
        if (trans->cfg->mq_rx_supported)
            rxq->queue_size = MQ_RX_TABLE_SIZE;
        else
            rxq->queue_size = RX_QUEUE_SIZE;
        
        /*
         * Allocate the circular buffer of Read Buffer Descriptors
         * (RBDs)
         */
        // TODO: Implement
//        rxq->bd = dma_zalloc_coherent(dev,
//                                      free_size * rxq->queue_size,
//                                      &rxq->bd_dma, GFP_KERNEL);
//        if (!rxq->bd)
//            goto err;
//
//        if (trans->cfg->mq_rx_supported) {
//            rxq->used_bd = dma_zalloc_coherent(dev,
//                                               sizeof(__le32) *
//                                               rxq->queue_size,
//                                               &rxq->used_bd_dma,
//                                               GFP_KERNEL);
//            if (!rxq->used_bd)
//                goto err;
//        }
        IOBufferMemoryDescriptor *rxqBd =
        IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task, (kIODirectionInOut | kIOMemoryPhysicallyContiguous | kIOMapInhibitCache), free_size * rxq->queue_size, 0x00000000FFFFFFFFULL);
        
        IODMACommand *rxqBdCmd = IODMACommand::withSpecification(kIODMACommandOutputHost64, 64, 0, IODMACommand::kMapped, 0, 1);
        rxqBdCmd->setMemoryDescriptor(rxqBd);
        rxqBdCmd->prepare();
        
        IODMACommand::Segment64 rxqBdSeg;
        UInt64 rxqBdOfs = 0;
        UInt32 rxqBdNumSegs = 1;
        
        if (rxqBdCmd->gen64IOVMSegments(&rxqBdOfs, &rxqBdSeg, &rxqBdNumSegs) != kIOReturnSuccess) {
            TraceLog("EVERYTHING IS VEEEERY BAAAD :(");
            return -1;
        }
        rxq->bd = rxqBd->getBytesNoCopy();
        rxq->bd_dma = rxqBdSeg.fIOVMAddr;
        bzero(rxq->bd, free_size * rxq->queue_size);
        
        //        if (trans->cfg->mq_rx_supported) {
        //            rxq->used_bd = dma_zalloc_coherent(dev,
        //                                               sizeof(__le32) *
        //                                               rxq->queue_size,
        //                                               &rxq->used_bd_dma,
        //                                               GFP_KERNEL);
        if (trans->cfg->mq_rx_supported) {
            IOBufferMemoryDescriptor *usedBd =
            IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task, (kIODirectionInOut | kIOMemoryPhysicallyContiguous | kIOMapInhibitCache), sizeof(__le32) * rxq->queue_size, 0x00000000FFFFFFFFULL);
            
            IODMACommand *usedBdCmd = IODMACommand::withSpecification(kIODMACommandOutputHost64, 64, 0, IODMACommand::kMapped, 0, 1);
            usedBdCmd->setMemoryDescriptor(usedBd);
            usedBdCmd->prepare();
            
            IODMACommand::Segment64 usedBdSeg;
            UInt64 usedBdOfs = 0;
            UInt32 usedBdNumSegs = 1;
            
            if (usedBdCmd->gen64IOVMSegments(&usedBdOfs, &usedBdSeg, &usedBdNumSegs) != kIOReturnSuccess) {
                TraceLog("EVERYTHING IS VEEEERY BAAAD :(");
                return -1;
            }
            rxq->used_bd = (__le32 *)usedBd->getBytesNoCopy();
            rxq->used_bd_dma = usedBdSeg.fIOVMAddr;
            bzero(rxq->used_bd, free_size * rxq->queue_size);
        }
        
        
        
        
        
        /*Allocate the driver's pointer to receive buffer status */
        // TODO: Implement
        
        IOBufferMemoryDescriptor *bmd =
        IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task, (kIODirectionInOut | kIOMemoryPhysicallyContiguous | kIOMapInhibitCache), sizeof(*rxq->rb_stts), 0x00000000FFFFFFFFULL);
        
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
        
        rxq->rb_stts = (struct iwl_rb_status*)bmd->getBytesNoCopy();
        rxq->rb_stts_dma = seg.fIOVMAddr;
        bzero(rxq->rb_stts, sizeof(struct iwl_rb_status));
        
//        rxq->rb_stts = dma_zalloc_coherent(dev, sizeof(*rxq->rb_stts),
//                                           &rxq->rb_stts_dma,
//                                           GFP_KERNEL);
        if (!rxq->rb_stts) {
            //goto err;
            return -ENOMEM;
        }
        
    }
    return 0;
    
    // TODO: Implement
//err:
//    for (i = 0; i < trans->num_rx_queues; i++) {
//        struct iwl_rxq *rxq = &trans_pcie->rxq[i];
//
//        if (rxq->bd)
//            dma_free_coherent(dev, free_size * rxq->queue_size,
//                              rxq->bd, rxq->bd_dma);
//        rxq->bd_dma = 0;
//        rxq->bd = NULL;
//
//        if (rxq->rb_stts)
//            dma_free_coherent(trans->dev,
//                              sizeof(struct iwl_rb_status),
//                              rxq->rb_stts, rxq->rb_stts_dma);
//
//        if (rxq->used_bd)
//            dma_free_coherent(dev, sizeof(__le32) * rxq->queue_size,
//                              rxq->used_bd, rxq->used_bd_dma);
//        rxq->used_bd_dma = 0;
//        rxq->used_bd = NULL;
//    }
//    kfree(trans_pcie->rxq);
    
    return -ENOMEM;
}









void IntelWifi::iwl_pcie_enable_rx_wake(struct iwl_trans *trans, bool enable)
{
    if (trans->cfg->device_family != IWL_DEVICE_FAMILY_9000)
        return;
    
    if (CSR_HW_REV_STEP(trans->hw_rev) != SILICON_A_STEP)
        return;
    
    if (!trans->cfg->integrated)
        return;
    
    /*
     * Turn on the chicken-bits that cause MAC wakeup for RX-related
     * values.
     * This costs some power, but needed for W/A 9000 integrated A-step
     * bug where shadow registers are not in the retention list and their
     * value is lost when NIC powers down
     */
    io->iwl_set_bit(CSR_MAC_SHADOW_REG_CTRL,
                CSR_MAC_SHADOW_REG_CTRL_RX_WAKE);
    io->iwl_set_bit(CSR_MAC_SHADOW_REG_CTL2,
                CSR_MAC_SHADOW_REG_CTL2_RX_WAKE);
}

void IntelWifi::iwl_pcie_rx_mq_hw_init(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    u32 rb_size, enabled = 0;
    IOInterruptState state;
    int i;
    
    switch (trans_pcie->rx_buf_size) {
        case IWL_AMSDU_4K:
            rb_size = RFH_RXF_DMA_RB_SIZE_4K;
            break;
        case IWL_AMSDU_8K:
            rb_size = RFH_RXF_DMA_RB_SIZE_8K;
            break;
        case IWL_AMSDU_12K:
            rb_size = RFH_RXF_DMA_RB_SIZE_12K;
            break;
        default:
            rb_size = RFH_RXF_DMA_RB_SIZE_4K;
    }
    
    if (!io->iwl_grab_nic_access(&state))
        return;
    
    /* Stop Rx DMA */
    io->iwl_write_prph_no_grab(RFH_RXF_DMA_CFG, 0);
    /* disable free amd used rx queue operation */
    io->iwl_write_prph_no_grab(RFH_RXF_RXQ_ACTIVE, 0);
    
    for (i = 0; i < trans->num_rx_queues; i++) {
        /* Tell device where to find RBD free table in DRAM */
        io->iwl_write_prph64_no_grab(
                                 RFH_Q_FRBDCB_BA_LSB(i),
                                 trans_pcie->rxq[i].bd_dma);
        /* Tell device where to find RBD used table in DRAM */
        io->iwl_write_prph64_no_grab(
                                 RFH_Q_URBDCB_BA_LSB(i),
                                 trans_pcie->rxq[i].used_bd_dma);
        /* Tell device where in DRAM to update its Rx status */
        io->iwl_write_prph64_no_grab(
                                 RFH_Q_URBD_STTS_WPTR_LSB(i),
                                 trans_pcie->rxq[i].rb_stts_dma);
        /* Reset device indice tables */
        io->iwl_write_prph_no_grab(RFH_Q_FRBDCB_WIDX(i), 0);
        io->iwl_write_prph_no_grab(RFH_Q_FRBDCB_RIDX(i), 0);
        io->iwl_write_prph_no_grab(RFH_Q_URBDCB_WIDX(i), 0);
        
        enabled |= BIT(i) | BIT(i + 16);
    }
    
    /*
     * Enable Rx DMA
     * Rx buffer size 4 or 8k or 12k
     * Min RB size 4 or 8
     * Drop frames that exceed RB size
     * 512 RBDs
     */
    io->iwl_write_prph_no_grab(RFH_RXF_DMA_CFG,
                           RFH_DMA_EN_ENABLE_VAL | rb_size |
                           RFH_RXF_DMA_MIN_RB_4_8 |
                           RFH_RXF_DMA_DROP_TOO_LARGE_MASK |
                           RFH_RXF_DMA_RBDCB_SIZE_512);
    
    /*
     * Activate DMA snooping.
     * Set RX DMA chunk size to 64B for IOSF and 128B for PCIe
     * Default queue is 0
     */
    io->iwl_write_prph_no_grab(RFH_GEN_CFG,
                           (u32)(RFH_GEN_CFG_RFH_DMA_SNOOP |
                           RFH_GEN_CFG_VAL(DEFAULT_RXQ_NUM, 0) |
                           RFH_GEN_CFG_SERVICE_DMA_SNOOP |
                           RFH_GEN_CFG_VAL(RB_CHUNK_SIZE,
                                           trans->cfg->integrated ?
                                           RFH_GEN_CFG_RB_CHUNK_SIZE_64 :
                                           RFH_GEN_CFG_RB_CHUNK_SIZE_128)));
    /* Enable the relevant rx queues */
    io->iwl_write_prph_no_grab(RFH_RXF_RXQ_ACTIVE, enabled);
    
    io->iwl_release_nic_access(&state);
    
    /* Set interrupt coalescing timer to default (2048 usecs) */
    io->iwl_write8(CSR_INT_COALESCING, IWL_HOST_INT_TIMEOUT_DEF);
    
    iwl_pcie_enable_rx_wake(trans, true);
}

void IntelWifi::iwl_pcie_rx_init_rxb_lists(struct iwl_rxq *rxq)
{
    //lockdep_assert_held(&rxq->lock);
    
    INIT_LIST_HEAD(&rxq->rx_free);
    INIT_LIST_HEAD(&rxq->rx_used);
    rxq->free_count = 0;
    rxq->used_count = 0;
}

int IntelWifi::_iwl_pcie_rx_init(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    struct iwl_rxq *def_rxq;
    struct iwl_rb_allocator *rba = &trans_pcie->rba;
    int i, err, queue_size, allocator_pool_size, num_alloc;
    
    if (!trans_pcie->rxq) {
        err = iwl_pcie_rx_alloc(trans);
        if (err)
            return err;
    }
    def_rxq = trans_pcie->rxq;

    IOSimpleLockLock(rba->lock);
    rba->req_pending = 0;
    rba->req_ready = 0;
    
    INIT_LIST_HEAD(&rba->rbd_allocated);
    INIT_LIST_HEAD(&rba->rbd_empty);
    IOSimpleLockUnlock(rba->lock);
    
    /* free all first - we might be reconfigured for a different size */
    // TODO: Implement
    //iwl_pcie_free_rbs_pool(trans);
    
    for (i = 0; i < RX_QUEUE_SIZE; i++)
        def_rxq->queue[i] = NULL;
    
    for (i = 0; i < trans->num_rx_queues; i++) {
        struct iwl_rxq *rxq = &trans_pcie->rxq[i];
        
        rxq->id = i;
        
        IOSimpleLockLock(rxq->lock);
        /*
         * Set read write pointer to reflect that we have processed
         * and used all buffers, but have not restocked the Rx queue
         * with fresh buffers
         */
        rxq->read = 0;
        rxq->write = 0;
        rxq->write_actual = 0;
        memset(rxq->rb_stts, 0, sizeof(*rxq->rb_stts));
        
        iwl_pcie_rx_init_rxb_lists(rxq);

        // TODO: Implement
//        if (!rxq->napi.poll)
//            netif_napi_add(&trans_pcie->napi_dev, &rxq->napi,
//                           iwl_pcie_dummy_napi_poll, 64);
        
        IOSimpleLockUnlock(rxq->lock);
    }
    
    /* move the pool to the default queue and allocator ownerships */
    queue_size = trans->cfg->mq_rx_supported ?
    MQ_RX_NUM_RBDS : RX_QUEUE_SIZE;
    allocator_pool_size = trans->num_rx_queues *
    (RX_CLAIM_REQ_ALLOC - RX_POST_REQ_ALLOC);
    num_alloc = queue_size + allocator_pool_size;
    for (i = 0; i < num_alloc; i++) {
        struct iwl_rx_mem_buffer *rxb = &trans_pcie->rx_pool[i];
        
        if (i < allocator_pool_size)
            list_add(&rxb->list, &rba->rbd_empty);
        else
            list_add(&rxb->list, &def_rxq->rx_used);
        trans_pcie->global_table[i] = rxb;
        rxb->vid = (u16)(i + 1);
        rxb->invalid = true;
    }
    
    // TODO: Implement
    //iwl_pcie_rxq_alloc_rbs(trans, GFP_KERNEL, def_rxq);
    
    return 0;
}







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
    
    IOBufferMemoryDescriptor *bmd =
    IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task, (kIODirectionInOut | kIOMemoryPhysicallyContiguous | kIOMapInhibitCache), ICT_SIZE, 0x00000000FFFFFFFFULL);
    
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
    
    trans_pcie->ict_tbl = (__le32*)bmd->getBytesNoCopy();
    trans_pcie->ict_tbl_dma = seg.fIOVMAddr;
    
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

irqreturn_t IntelWifi::iwl_pcie_irq_handler(int irq, void *dev_id)
{
    struct iwl_trans *trans = (struct iwl_trans *)dev_id;
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    struct isr_statistics *isr_stats = &trans_pcie->isr_stats;
    u32 inta = 0;
    u32 handled = 0;
    
    // TODO: implement
//    lock_map_acquire(&trans->sync_cmd_lockdep_map);
    
    IOSimpleLockLock(trans_pcie->irq_lock);
    
    /* dram interrupt table not set yet,
     * use legacy interrupt.
     */
    if (likely(trans_pcie->use_ict))
        inta = iwl_pcie_int_cause_ict(trans);
    else
        inta = iwl_pcie_int_cause_non_ict(trans);
    
    //if (iwl_have_debug_level(IWL_DL_ISR)) {
        IWL_DEBUG_ISR(trans,
                      "ISR inta 0x%08x, enabled 0x%08x(sw), enabled(hw) 0x%08x, fh 0x%08x\n",
                      inta, trans_pcie->inta_mask,
                      io->iwl_read32(CSR_INT_MASK),
                      io->iwl_read32(CSR_FH_INT_STATUS));
        if (inta & (~trans_pcie->inta_mask))
            IWL_DEBUG_ISR(trans,
                          "We got a masked interrupt (0x%08x)\n",
                          inta & (~trans_pcie->inta_mask));
    //}
    
    inta &= trans_pcie->inta_mask;
    
    /*
     * Ignore interrupt if there's nothing in NIC to service.
     * This may be due to IRQ shared with another device,
     * or due to sporadic interrupts thrown from our NIC.
     */
    if (unlikely(!inta)) {
        IWL_DEBUG_ISR(trans, "Ignore interrupt, inta == 0\n");
        /*
         * Re-enable interrupts here since we don't
         * have anything to service
         */
        if (test_bit(STATUS_INT_ENABLED, &trans->status))
            _iwl_enable_interrupts(trans);
        
        IOSimpleLockUnlock(trans_pcie->irq_lock);
        //lock_map_release(&trans->sync_cmd_lockdep_map);
        return IRQ_NONE;
    }
    
    if (unlikely(inta == 0xFFFFFFFF || (inta & 0xFFFFFFF0) == 0xa5a5a5a0)) {
        /*
         * Hardware disappeared. It might have
         * already raised an interrupt.
         */
        IWL_WARN(trans, "HARDWARE GONE?? INTA == 0x%08x\n", inta);
        IOSimpleLockUnlock(trans_pcie->irq_lock);
        goto out;
    }
    
    /* Ack/clear/reset pending uCode interrupts.
     * Note:  Some bits in CSR_INT are "OR" of bits in CSR_FH_INT_STATUS,
     */
    /* There is a hardware bug in the interrupt mask function that some
     * interrupts (i.e. CSR_INT_BIT_SCD) can still be generated even if
     * they are disabled in the CSR_INT_MASK register. Furthermore the
     * ICT interrupt handling mechanism has another bug that might cause
     * these unmasked interrupts fail to be detected. We workaround the
     * hardware bugs here by ACKing all the possible interrupts so that
     * interrupt coalescing can still be achieved.
     */
    io->iwl_write32(CSR_INT, inta | ~trans_pcie->inta_mask);
    
    //if (iwl_have_debug_level(IWL_DL_ISR))
        IWL_DEBUG_ISR(trans, "inta 0x%08x, enabled 0x%08x\n",
                      inta, io->iwl_read32(CSR_INT_MASK));

    IOSimpleLockUnlock(trans_pcie->irq_lock);
    
    /* Now service all interrupt bits discovered above. */
    if (inta & CSR_INT_BIT_HW_ERR) {
        IWL_ERR(trans, "Hardware error detected.  Restarting.\n");
        
        /* Tell the device to stop sending interrupts */
        iwl_disable_interrupts(trans);
        
        isr_stats->hw++;
        iwl_pcie_irq_handle_error(trans);
        
        handled |= CSR_INT_BIT_HW_ERR;
        
        goto out;
    }
    
    if (iwl_have_debug_level(IWL_DL_ISR)) {
        /* NIC fires this, but we don't use it, redundant with WAKEUP */
        if (inta & CSR_INT_BIT_SCD) {
            IWL_DEBUG_ISR(trans,
                          "Scheduler finished to transmit the frame/frames.\n");
            isr_stats->sch++;
        }
    
        /* Alive notification via Rx interrupt will do the real work */
        if (inta & CSR_INT_BIT_ALIVE) {
            IWL_DEBUG_ISR(trans, "Alive interrupt\n");
            isr_stats->alive++;
            if (trans->cfg->gen2) {
                /*
                 * We can restock, since firmware configured
                 * the RFH
                 */
                iwl_pcie_rxmq_restock(trans, trans_pcie->rxq);
            }
        }
    }
    
    /* Safely ignore these bits for debug checks below */
    inta &= ~(CSR_INT_BIT_SCD | CSR_INT_BIT_ALIVE);
    
    /* HW RF KILL switch toggled */
    if (inta & CSR_INT_BIT_RF_KILL) {
        iwl_pcie_handle_rfkill_irq(trans);
        handled |= CSR_INT_BIT_RF_KILL;
    }
    
    /* Chip got too hot and stopped itself */
    if (inta & CSR_INT_BIT_CT_KILL) {
        IWL_ERR(trans, "Microcode CT kill error detected.\n");
        isr_stats->ctkill++;
        handled |= CSR_INT_BIT_CT_KILL;
    }
    
    
    /* Error detected by uCode */
    if (inta & CSR_INT_BIT_SW_ERR) {
        IWL_ERR(trans, "Microcode SW error detected. "
                " Restarting 0x%X.\n", inta);
        isr_stats->sw++;
        iwl_pcie_irq_handle_error(trans);
        handled |= CSR_INT_BIT_SW_ERR;
    }

    /* uCode wakes up after power-down sleep */
    if (inta & CSR_INT_BIT_WAKEUP) {
        IWL_DEBUG_ISR(trans, "Wakeup interrupt\n");
        
        iwl_pcie_rxq_check_wrptr(trans);
        iwl_pcie_txq_check_wrptrs(trans);

        isr_stats->wakeup++;

        handled |= CSR_INT_BIT_WAKEUP;
    }
    
    /* All uCode command responses, including Tx command responses,
     * Rx "responses" (frame-received notification), and other
     * notifications from uCode come through here*/
    if (inta & (CSR_INT_BIT_FH_RX | CSR_INT_BIT_SW_RX |
                CSR_INT_BIT_RX_PERIODIC)) {
        IWL_DEBUG_ISR(trans, "Rx interrupt\n");
        if (inta & (CSR_INT_BIT_FH_RX | CSR_INT_BIT_SW_RX)) {
            handled |= (CSR_INT_BIT_FH_RX | CSR_INT_BIT_SW_RX);
            io->iwl_write32(CSR_FH_INT_STATUS,
                        CSR_FH_INT_RX_MASK);
        }
        if (inta & CSR_INT_BIT_RX_PERIODIC) {
            handled |= CSR_INT_BIT_RX_PERIODIC;
            io->iwl_write32(
                        CSR_INT, CSR_INT_BIT_RX_PERIODIC);
        }
        /* Sending RX interrupt require many steps to be done in the
         * the device:
         * 1- write interrupt to current index in ICT table.
         * 2- dma RX frame.
         * 3- update RX shared data to indicate last write index.
         * 4- send interrupt.
         * This could lead to RX race, driver could receive RX interrupt
         * but the shared data changes does not reflect this;
         * periodic interrupt will detect any dangling Rx activity.
         */
        
        /* Disable periodic interrupt; we use it as just a one-shot. */
        io->iwl_write8(CSR_INT_PERIODIC_REG,
                   CSR_INT_PERIODIC_DIS);
        
        /*
         * Enable periodic interrupt in 8 msec only if we received
         * real RX interrupt (instead of just periodic int), to catch
         * any dangling Rx interrupt.  If it was just the periodic
         * interrupt, there was no dangling Rx activity, and no need
         * to extend the periodic interrupt; one-shot is enough.
         */
        if (inta & (CSR_INT_BIT_FH_RX | CSR_INT_BIT_SW_RX))
            io->iwl_write8(CSR_INT_PERIODIC_REG,
                       CSR_INT_PERIODIC_ENA);
        
        isr_stats->rx++;
        
        DebugLog("Frame interrupt!");
        
//        local_bh_disable();
//        iwl_pcie_rx_handle(trans, 0);
//        local_bh_enable();
    }
    
    /* This "Tx" DMA channel is used only for loading uCode */
    if (inta & CSR_INT_BIT_FH_TX) {
        io->iwl_write32(CSR_FH_INT_STATUS, CSR_FH_INT_TX_MASK);
        IWL_DEBUG_ISR(trans, "uCode load interrupt\n");
        isr_stats->tx++;
        handled |= CSR_INT_BIT_FH_TX;
        /* Wake up uCode load routine, now that load is complete */
        IOLockLock(trans_pcie->ucode_write_waitq);
        trans_pcie->ucode_write_complete = true;
        IOLockWakeup(trans_pcie->ucode_write_waitq, &trans_pcie->ucode_write_complete, true);
        IOLockUnlock(trans_pcie->ucode_write_waitq);
    }
    
    if (inta & ~handled) {
        IWL_ERR(trans, "Unhandled INTA bits 0x%08x\n", inta & ~handled);
        isr_stats->unhandled++;
    }
    
    if (inta & ~(trans_pcie->inta_mask)) {
        IWL_WARN(trans, "Disabled INTA bits 0x%08x were pending\n",
                 inta & ~trans_pcie->inta_mask);
    }
    
    IOSimpleLockLock(trans_pcie->irq_lock);
    /* only Re-enable all interrupt if disabled by irq */
    if (test_bit(STATUS_INT_ENABLED, &trans->status))
        _iwl_enable_interrupts(trans);
    /* we are loading the firmware, enable FH_TX interrupt only */
    else if (handled & CSR_INT_BIT_FH_TX)
        iwl_enable_fw_load_int(trans);
    /* Re-enable RF_KILL if it occurred */
    else if (handled & CSR_INT_BIT_RF_KILL)
        iwl_enable_rfkill_int(trans);

    IOSimpleLockUnlock(trans_pcie->irq_lock);
    
out:
    //lock_map_release(&trans->sync_cmd_lockdep_map);
    return IRQ_HANDLED;
}

u32 IntelWifi::iwl_pcie_int_cause_non_ict(struct iwl_trans *trans)
{
    u32 inta;
    
//    lockdep_assert_held(&IWL_TRANS_GET_PCIE_TRANS(trans)->irq_lock);
    
//    trace_iwlwifi_dev_irq(trans->dev);
    
    /* Discover which interrupts are active/pending */
    inta = io->iwl_read32(CSR_INT);
    
    /* the thread will service interrupts and re-enable them */
    return inta;
}

void IntelWifi::iwl_pcie_handle_rfkill_irq(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    struct isr_statistics *isr_stats = &trans_pcie->isr_stats;
    bool hw_rfkill, prev, report;
    
    IOLockLock(trans_pcie->mutex);
    prev = test_bit(STATUS_RFKILL_OPMODE, &trans->status);
    hw_rfkill = iwl_is_rfkill_set(trans);
    if (hw_rfkill) {
        set_bit(STATUS_RFKILL_OPMODE, &trans->status);
        set_bit(STATUS_RFKILL_HW, &trans->status);
    }
    if (trans_pcie->opmode_down)
        report = hw_rfkill;
    else
        report = test_bit(STATUS_RFKILL_OPMODE, &trans->status);
    
    IWL_WARN(trans, "RF_KILL bit toggled to %s.\n",
             hw_rfkill ? "disable radio" : "enable radio");
    
    isr_stats->rfkill++;
    
    if (prev != report)
        iwl_trans_pcie_rf_kill(trans, report);

    IOLockUnlock(trans_pcie->mutex);
    
    if (hw_rfkill) {
        if (test_and_clear_bit(STATUS_SYNC_HCMD_ACTIVE,
                               &trans->status))
            IWL_DEBUG_RF_KILL(trans,
                              "Rfkill while SYNC HCMD in flight\n");
        IOLockLock(trans_pcie->wait_command_queue);
        IOLockWakeup(trans_pcie->wait_command_queue, 0, true);
        IOLockUnlock(trans_pcie->wait_command_queue);
    } else {
        clear_bit(STATUS_RFKILL_HW, &trans->status);
        if (trans_pcie->opmode_down)
            clear_bit(STATUS_RFKILL_OPMODE, &trans->status);
    }
}

/*
 * iwl_pcie_irq_handle_error - called for HW or SW error interrupt from card
 */
void IntelWifi::iwl_pcie_irq_handle_error(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    //int i;
    
    /* W/A for WiFi/WiMAX coex and WiMAX own the RF */
    if (trans->cfg->internal_wimax_coex &&
        !trans->cfg->apmg_not_supported &&
        (!(io->iwl_read_prph(APMG_CLK_CTRL_REG) &
           APMS_CLK_VAL_MRB_FUNC_MODE) ||
         (io->iwl_read_prph(APMG_PS_CTRL_REG) &
          APMG_PS_CTRL_VAL_RESET_REQ))) {
             clear_bit(STATUS_SYNC_HCMD_ACTIVE, &trans->status);
             iwl_op_mode_wimax_active(trans->op_mode);
             IOLockLock(trans_pcie->wait_command_queue);
             IOLockWakeup(trans_pcie->wait_command_queue, 0, true);
             IOLockUnlock(trans_pcie->wait_command_queue);
             return;
         }
    
    // TODO: Implement
//    for (i = 0; i < trans->cfg->base_params->num_of_queues; i++) {
//        if (!trans_pcie->txq[i])
//            continue;
//        del_timer(&trans_pcie->txq[i]->stuck_timer);
//    }
    
    /* The STATUS_FW_ERROR bit is set in this function. This must happen
     * before we wake up the command caller, to ensure a proper cleanup. */
    // TODO: Implement. Inside trans->ops->op is used which is undefined and will cause kernel panic
    //iwl_trans_fw_error(trans);
    
    clear_bit(STATUS_SYNC_HCMD_ACTIVE, &trans->status);

    IOLockLock(trans_pcie->wait_command_queue);
    IOLockWakeup(trans_pcie->wait_command_queue, 0, true);
    IOLockUnlock(trans_pcie->wait_command_queue);
}

/* Device is going up inform it about using ICT interrupt table,
 * also we need to tell the driver to start using ICT interrupt.
 */
void IntelWifi::iwl_pcie_reset_ict(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    u32 val;
    
    if (!trans_pcie->ict_tbl)
        return;
    
    IOSimpleLockLock(trans_pcie->irq_lock);
    _iwl_disable_interrupts(trans);
    
    memset(trans_pcie->ict_tbl, 0, ICT_SIZE);
    
    val = (u32)(trans_pcie->ict_tbl_dma >> ICT_SHIFT);
    
    val |= CSR_DRAM_INT_TBL_ENABLE |
    CSR_DRAM_INIT_TBL_WRAP_CHECK |
    CSR_DRAM_INIT_TBL_WRITE_POINTER;
    
    IWL_DEBUG_ISR(trans, "CSR_DRAM_INT_TBL_REG =0x%x\n", val);
    
    io->iwl_write32(CSR_DRAM_INT_TBL_REG, val);
    trans_pcie->use_ict = true;
    trans_pcie->ict_index = 0;
    io->iwl_write32(CSR_INT, trans_pcie->inta_mask);
    _iwl_enable_interrupts(trans);
    IOSimpleLockUnlock(trans_pcie->irq_lock);
}

/* interrupt handler using ict table, with this interrupt driver will
 * stop using INTA register to get device's interrupt, reading this register
 * is expensive, device will write interrupts in ICT dram table, increment
 * index then will fire interrupt to driver, driver will OR all ICT table
 * entries from current index up to table entry with 0 value. the result is
 * the interrupt we need to service, driver will set the entries back to 0 and
 * set index.
 */
u32 IntelWifi::iwl_pcie_int_cause_ict(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    u32 inta;
    u32 val = 0;
    u32 read;
    
    //trace_iwlwifi_dev_irq(trans->dev);
    
    /* Ignore interrupt if there's nothing in NIC to service.
     * This may be due to IRQ shared with another device,
     * or due to sporadic interrupts thrown from our NIC. */
    read = le32_to_cpu(trans_pcie->ict_tbl[trans_pcie->ict_index]);
    //trace_iwlwifi_dev_ict_read(trans->dev, trans_pcie->ict_index, read);
    if (!read)
        return 0;
    
    /*
     * Collect all entries up to the first 0, starting from ict_index;
     * note we already read at ict_index.
     */
    do {
        val |= read;
        IWL_DEBUG_ISR(trans, "ICT index %d value 0x%08X\n",
                      trans_pcie->ict_index, read);
        trans_pcie->ict_tbl[trans_pcie->ict_index] = 0;
        trans_pcie->ict_index =
        ((trans_pcie->ict_index + 1) & (ICT_COUNT - 1));
        
        read = le32_to_cpu(trans_pcie->ict_tbl[trans_pcie->ict_index]);
//        trace_iwlwifi_dev_ict_read(trans->dev, trans_pcie->ict_index,
//                                   read);
    } while (read);
    
    /* We should not get this value, just ignore it. */
    if (val == 0xffffffff)
        val = 0;
    
    /*
     * this is a w/a for a h/w bug. the h/w bug may cause the Rx bit
     * (bit 15 before shifting it to 31) to clear when using interrupt
     * coalescing. fortunately, bits 18 and 19 stay set when this happens
     * so we use them to decide on the real state of the Rx bit.
     * In order words, bit 15 is set if bit 18 or bit 19 are set.
     */
    if (val & 0xC0000)
        val |= 0x8000;
    
    inta = (0xff & val) | ((0xff00 & val) << 16);
    return inta;
}

void IntelWifi::iwl_pcie_rx_hw_init(struct iwl_trans *trans, struct iwl_rxq *rxq)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    u32 rb_size;
    IOInterruptState state;
    const u32 rfdnlog = RX_QUEUE_SIZE_LOG; /* 256 RBDs */
    
    switch (trans_pcie->rx_buf_size) {
        case IWL_AMSDU_4K:
            rb_size = FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_4K;
            break;
        case IWL_AMSDU_8K:
            rb_size = FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_8K;
            break;
        case IWL_AMSDU_12K:
            rb_size = FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_12K;
            break;
        default:
            //WARN_ON(1);
            rb_size = FH_RCSR_RX_CONFIG_REG_VAL_RB_SIZE_4K;
    }
    
    if (!io->iwl_grab_nic_access(&state))
        return;
    
    /* Stop Rx DMA */
    io->iwl_write32(FH_MEM_RCSR_CHNL0_CONFIG_REG, 0);
    /* reset and flush pointers */
    io->iwl_write32(FH_MEM_RCSR_CHNL0_RBDCB_WPTR, 0);
    io->iwl_write32(FH_MEM_RCSR_CHNL0_FLUSH_RB_REQ, 0);
    io->iwl_write32(FH_RSCSR_CHNL0_RDPTR, 0);
    
    /* Reset driver's Rx queue write index */
    io->iwl_write32(FH_RSCSR_CHNL0_RBDCB_WPTR_REG, 0);
    
    /* Tell device where to find RBD circular buffer in DRAM */
    io->iwl_write32(FH_RSCSR_CHNL0_RBDCB_BASE_REG,
                (u32)(rxq->bd_dma >> 8));
    
    /* Tell device where in DRAM to update its Rx status */
    io->iwl_write32(FH_RSCSR_CHNL0_STTS_WPTR_REG,
                (u32)(rxq->rb_stts_dma >> 4));
    
    /* Enable Rx DMA
     * FH_RCSR_CHNL0_RX_IGNORE_RXF_EMPTY is set because of HW bug in
     *      the credit mechanism in 5000 HW RX FIFO
     * Direct rx interrupts to hosts
     * Rx buffer size 4 or 8k or 12k
     * RB timeout 0x10
     * 256 RBDs
     */
    io->iwl_write32(FH_MEM_RCSR_CHNL0_CONFIG_REG,
                FH_RCSR_RX_CONFIG_CHNL_EN_ENABLE_VAL |
                FH_RCSR_CHNL0_RX_IGNORE_RXF_EMPTY |
                FH_RCSR_CHNL0_RX_CONFIG_IRQ_DEST_INT_HOST_VAL |
                rb_size |
                (RX_RB_TIMEOUT << FH_RCSR_RX_CONFIG_REG_IRQ_RBTH_POS) |
                (rfdnlog << FH_RCSR_RX_CONFIG_RBDCB_SIZE_POS));
    
    io->iwl_release_nic_access(&state);
    
    /* Set interrupt coalescing timer to default (2048 usecs) */
    io->iwl_write8(CSR_INT_COALESCING, IWL_HOST_INT_TIMEOUT_DEF);
    
    /* W/A for interrupt coalescing bug in 7260 and 3160 */
    if (trans->cfg->host_interrupt_operation_mode)
        io->iwl_set_bit(CSR_INT_COALESCING, IWL_HOST_INT_OPER_MODE);
}

int IntelWifi::iwl_pcie_rx_init(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    int ret = _iwl_pcie_rx_init(trans);

    if (ret)
        return ret;

    if (trans->cfg->mq_rx_supported)
        iwl_pcie_rx_mq_hw_init(trans);
    else
        iwl_pcie_rx_hw_init(trans, trans_pcie->rxq);

    iwl_pcie_rxq_restock(trans, trans_pcie->rxq);

    IOSimpleLockLock(trans_pcie->rxq->lock);
    iwl_pcie_rxq_inc_wr_ptr(trans, trans_pcie->rxq);
    IOSimpleLockUnlock(trans_pcie->rxq->lock);
    
    return 0;
}



