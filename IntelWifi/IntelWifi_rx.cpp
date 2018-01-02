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
    // TODO: Implement modern IRQ handling. For now, using legacy interrupt
    //if (likely(trans_pcie->use_ict))
    //    inta = iwl_pcie_int_cause_ict(trans);
    //else
    //    inta = iwl_pcie_int_cause_non_ict(trans);
    inta = iwl_pcie_int_cause_non_ict(trans);
    
    // TODO: Implement
    //if (iwl_have_debug_level(IWL_DL_ISR)) {
    //    IWL_DEBUG_ISR(trans,
    //                  "ISR inta 0x%08x, enabled 0x%08x(sw), enabled(hw) 0x%08x, fh 0x%08x\n",
    //                  inta, trans_pcie->inta_mask,
    //                  iwl_read32(trans, CSR_INT_MASK),
    //                  iwl_read32(trans, CSR_FH_INT_STATUS));
    //    if (inta & (~trans_pcie->inta_mask))
    //        IWL_DEBUG_ISR(trans,
    //                      "We got a masked interrupt (0x%08x)\n",
    //                      inta & (~trans_pcie->inta_mask));
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
        
        //spin_unlock(&trans_pcie->irq_lock);
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
        // spin_unlock(&trans_pcie->irq_lock);
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
    
    // TODO: Implement
    /*if (iwl_have_debug_level(IWL_DL_ISR))
        IWL_DEBUG_ISR(trans, "inta 0x%08x, enabled 0x%08x\n",
                      inta, iwl_read32(trans, CSR_INT_MASK));*/
    
    //spin_unlock(&trans_pcie->irq_lock);
    IOSimpleLockUnlock(trans_pcie->irq_lock);
    
    /* Now service all interrupt bits discovered above. */
    if (inta & CSR_INT_BIT_HW_ERR) {
        IWL_ERR(trans, "Hardware error detected.  Restarting.\n");
        
        /* Tell the device to stop sending interrupts */
        iwl_disable_interrupts(trans);
        
        isr_stats->hw++;
        // TODO: implement
        //iwl_pcie_irq_handle_error(trans);
        
        handled |= CSR_INT_BIT_HW_ERR;
        
        goto out;
    }
    
    // TODO: Implement
    //if (iwl_have_debug_level(IWL_DL_ISR)) {
    //    /* NIC fires this, but we don't use it, redundant with WAKEUP */
    //    if (inta & CSR_INT_BIT_SCD) {
    //        IWL_DEBUG_ISR(trans,
    //                      "Scheduler finished to transmit the frame/frames.\n");
    //        isr_stats->sch++;
    //    }
    //
    //    /* Alive notification via Rx interrupt will do the real work */
    //    if (inta & CSR_INT_BIT_ALIVE) {
    //        IWL_DEBUG_ISR(trans, "Alive interrupt\n");
    //        isr_stats->alive++;
    //        if (trans->cfg->gen2) {
    //            /*
    //             * We can restock, since firmware configured
    //             * the RFH
    //             */
    //            iwl_pcie_rxmq_restock(trans, trans_pcie->rxq);
    //        }
    //    }
    //}
    
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
    
    // TODO: Implement
//    /* Error detected by uCode */
//    if (inta & CSR_INT_BIT_SW_ERR) {
//        IWL_ERR(trans, "Microcode SW error detected. "
//                " Restarting 0x%X.\n", inta);
//        isr_stats->sw++;
//        iwl_pcie_irq_handle_error(trans);
//        handled |= CSR_INT_BIT_SW_ERR;
//    }
//
//    /* uCode wakes up after power-down sleep */
//    if (inta & CSR_INT_BIT_WAKEUP) {
//        IWL_DEBUG_ISR(trans, "Wakeup interrupt\n");
//        iwl_pcie_rxq_check_wrptr(trans);
//        iwl_pcie_txq_check_wrptrs(trans);
//
//        isr_stats->wakeup++;
//
//        handled |= CSR_INT_BIT_WAKEUP;
//    }
    
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
        trans_pcie->ucode_write_complete = true;
        //wake_up(&trans_pcie->ucode_write_waitq);
    }
    
    if (inta & ~handled) {
        IWL_ERR(trans, "Unhandled INTA bits 0x%08x\n", inta & ~handled);
        isr_stats->unhandled++;
    }
    
    if (inta & ~(trans_pcie->inta_mask)) {
        IWL_WARN(trans, "Disabled INTA bits 0x%08x were pending\n",
                 inta & ~trans_pcie->inta_mask);
    }
    
    //spin_lock(&trans_pcie->irq_lock);
    IOSimpleLockLock(trans_pcie->irq_lock);
    /* only Re-enable all interrupt if disabled by irq */
    if (test_bit(STATUS_INT_ENABLED, &trans->status))
        _iwl_enable_interrupts(trans);
    /* we are loading the firmware, enable FH_TX interrupt only */
    // TODO: Implement
//    else if (handled & CSR_INT_BIT_FH_TX)
//        iwl_enable_fw_load_int(trans);
    /* Re-enable RF_KILL if it occurred */
    else if (handled & CSR_INT_BIT_RF_KILL)
        iwl_enable_rfkill_int(trans);
    //spin_unlock(&trans_pcie->irq_lock);
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
    
    //mutex_lock(&trans_pcie->mutex);
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
    //mutex_unlock(&trans_pcie->mutex);
    IOLockUnlock(trans_pcie->mutex);
    
    if (hw_rfkill) {
        if (test_and_clear_bit(STATUS_SYNC_HCMD_ACTIVE,
                               &trans->status))
            IWL_DEBUG_RF_KILL(trans,
                              "Rfkill while SYNC HCMD in flight\n");
        //wake_up(&trans_pcie->wait_command_queue);
    } else {
        clear_bit(STATUS_RFKILL_HW, &trans->status);
        if (trans_pcie->opmode_down)
            clear_bit(STATUS_RFKILL_OPMODE, &trans->status);
    }
}


