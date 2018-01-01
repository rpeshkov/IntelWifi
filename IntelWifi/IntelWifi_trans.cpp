//
//  IntelWifi_trans.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 01/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include <IntelWifi.hpp>

int IntelWifi::_iwl_trans_pcie_start_hw(struct iwl_trans *trans, bool low_power)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    int err;
    
    //lockdep_assert_held(&trans_pcie->mutex);
    
    err = iwl_pcie_prepare_card_hw(trans);
    if (err) {
        IWL_ERR(trans, "Error while preparing HW: %d\n", err);
        return err;
    }
    
    iwl_pcie_sw_reset(trans);
    
    err = iwl_pcie_apm_init(trans);
    if (err)
        return err;
    
    //iwl_pcie_init_msix(trans_pcie);
    
    /* From now on, the op_mode will be kept updated about RF kill state */
    //iwl_enable_rfkill_int(trans);
    
    trans_pcie->opmode_down = false;
    
    /* Set is_down to false here so that...*/
    trans_pcie->is_down = false;
    
    /* ...rfkill can call stop_device and set it false if needed */
    //iwl_pcie_check_hw_rf_kill(trans);
    
    /* Make sure we sync here, because we'll need full access later */
//    if (low_power)
//        pm_runtime_resume(trans->dev);
    
    return 0;
}

int IntelWifi::iwl_trans_pcie_start_hw(struct iwl_trans *trans, bool low_power)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    int ret;
    
    //mutex_lock(&trans_pcie->mutex);
    IOLockLock(trans_pcie->mutex);
    ret = _iwl_trans_pcie_start_hw(trans, low_power);
    //mutex_unlock(&trans_pcie->mutex);
    IOLockUnlock(trans_pcie->mutex);
    
    return ret;
}

void IntelWifi::iwl_pcie_sw_reset(struct iwl_trans* trans)
{
    /* Reset entire device - do controller reset (results in SHRD_HW_RST) */
    io->iwl_set_bit(CSR_RESET, CSR_RESET_REG_FLAG_SW_RESET);
    IODelay(6000);
}

/*
 * Start up NIC's basic functionality after it has been reset
 * (e.g. after platform boot, or shutdown via iwl_pcie_apm_stop())
 * NOTE:  This does not load uCode nor start the embedded processor
 */
int IntelWifi::iwl_pcie_apm_init(struct iwl_trans* trans)
{
    IWL_DEBUG_INFO(trans, "Init card's basic functions\n");
    
    /*
     * Use "set_bit" below rather than "write", to preserve any hardware
     * bits already set by default after reset.
     */
    
    /* Disable L0S exit timer (platform NMI Work/Around) */
    if (fConfiguration->device_family < IWL_DEVICE_FAMILY_8000)
        io->iwl_set_bit(CSR_GIO_CHICKEN_BITS,
                        CSR_GIO_CHICKEN_BITS_REG_BIT_DIS_L0S_EXIT_TIMER);
    
    /*
     * Disable L0s without affecting L1;
     *  don't wait for ICH L0s (ICH bug W/A)
     */
    io->iwl_set_bit(CSR_GIO_CHICKEN_BITS,
                    CSR_GIO_CHICKEN_BITS_REG_BIT_L1A_NO_L0S_RX);
    
    /* Set FH wait threshold to maximum (HW error during stress W/A) */
    io->iwl_set_bit(CSR_DBG_HPET_MEM_REG, CSR_DBG_HPET_MEM_REG_VAL);
    
    /*
     * Enable HAP INTA (interrupt from management bus) to
     * wake device's PCI Express link L1a -> L0s
     */
    io->iwl_set_bit(CSR_HW_IF_CONFIG_REG,
                    CSR_HW_IF_CONFIG_REG_BIT_HAP_WAKE_L1A);
    
    iwl_pcie_apm_config(trans);
    
    /* Configure analog phase-lock-loop before activating to D0A */
    if (fConfiguration->base_params->pll_cfg)
        io->iwl_set_bit(CSR_ANA_PLL_CFG, CSR50_ANA_PLL_CFG_VAL);
    
    /*
     * Set "initialization complete" bit to move adapter from
     * D0U* --> D0A* (powered-up active) state.
     */
    io->iwl_set_bit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
    
    /*
     * Wait for clock stabilization; once stabilized, access to
     * device-internal resources is supported, e.g. iwl_write_prph()
     * and accesses to uCode SRAM.
     */
    int ret = io->iwl_poll_bit(CSR_GP_CNTRL,
                               CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                               CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY, 25000);
    if (ret < 0) {
        IWL_ERR(trans, "Failed to init the card\n");
        return ret;
    }
    
    if (trans->cfg->host_interrupt_operation_mode) {
        /*
         * This is a bit of an abuse - This is needed for 7260 / 3160
         * only check host_interrupt_operation_mode even if this is
         * not related to host_interrupt_operation_mode.
         *
         * Enable the oscillator to count wake up time for L1 exit. This
         * consumes slightly more power (100uA) - but allows to be sure
         * that we wake up from L1 on time.
         *
         * This looks weird: read twice the same register, discard the
         * value, set a bit, and yet again, read that same register
         * just to discard the value. But that's the way the hardware
         * seems to like it.
         */
        io->iwl_read_prph(OSC_CLK);
        io->iwl_read_prph(OSC_CLK);
        io->iwl_set_bits_prph(OSC_CLK, OSC_CLK_FORCE_CONTROL);
        io->iwl_read_prph(OSC_CLK);
        io->iwl_read_prph(OSC_CLK);
    }
    
    /*
     * Enable DMA clock and wait for it to stabilize.
     *
     * Write to "CLK_EN_REG"; "1" bits enable clocks, while "0"
     * bits do not disable clocks.  This preserves any hardware
     * bits already set by default in "CLK_CTRL_REG" after reset.
     */
    if (!trans->cfg->apmg_not_supported) {
        io->iwl_write_prph(APMG_CLK_EN_REG,
                           APMG_CLK_VAL_DMA_CLK_RQT);
        IODelay(20);
        
        /* Disable L1-Active */
        io->iwl_set_bits_prph(APMG_PCIDEV_STT_REG,
                              APMG_PCIDEV_STT_VAL_L1_ACT_DIS);
        
        /* Clear the interrupt in APMG if the NIC is in RFKILL */
        io->iwl_write_prph(APMG_RTC_INT_STT_REG,
                           APMG_RTC_INT_STT_RFKILL);
    }
    
    set_bit(STATUS_DEVICE_ENABLED, &trans->status);
    
    return 0;
}


/* Note: returns standard 0/-ERROR code */
int IntelWifi::iwl_pcie_prepare_card_hw(struct iwl_trans *trans)
{
    int ret;
    int t = 0;
    int iter;
    
    IWL_DEBUG_INFO(trans, "iwl_trans_prepare_card_hw enter\n");
    
    ret = iwl_pcie_set_hw_ready(trans);
    /* If the card is ready, exit 0 */
    if (ret >= 0)
        return 0;
    
    io->iwl_set_bit(CSR_DBG_LINK_PWR_MGMT_REG,
                    CSR_RESET_LINK_PWR_MGMT_DISABLED);
    
    IODelay(2000);
    
    for (iter = 0; iter < 10; iter++) {
        /* If HW is not ready, prepare the conditions to check again */
        io->iwl_set_bit(CSR_HW_IF_CONFIG_REG,
                        CSR_HW_IF_CONFIG_REG_PREPARE);
        
        do {
            ret = iwl_pcie_set_hw_ready(trans);
            if (ret >= 0)
                return 0;
            
            IODelay(1000);
            t += 200;
        } while (t < 150000);
        IOSleep(25);
    }
    
    IWL_ERR(trans, "Couldn't prepare the card\n");
    
    return ret;
}

void IntelWifi::iwl_pcie_apm_config(struct iwl_trans *trans)
{
    
    /*
     * HW bug W/A for instability in PCIe bus L0S->L1 transition.
     * Check if BIOS (or OS) enabled L1-ASPM on this device.
     * If so (likely), disable L0S, so device moves directly L0->L1;
     *    costs negligible amount of power savings.
     * If not (unlikely), enable L0S, so there is at least some
     *    power savings, even without L1.
     */
    // TODO: Implement. Check implementation. May be wrong
    OSNumber *pciLinkCapabilities = OSDynamicCast(OSNumber, pciDevice->getProperty(kIOPCIExpressLinkCapabilitiesKey));
    if (pciLinkCapabilities) {
        UInt32 linkCaps = pciLinkCapabilities->unsigned32BitValue();
        IWL_DEBUG_INFO(trans, "ASPM State: %#10x", linkCaps);
        if (linkCaps & 0x800) {
            io->iwl_set_bit(CSR_GIO_REG, CSR_GIO_REG_VAL_L0S_ENABLED);
        } else {
            io->iwl_clear_bit(CSR_GIO_REG, CSR_GIO_REG_VAL_L0S_ENABLED);
        }
        trans->pm_support = !(linkCaps & 0x400);
    }
    
    
    //pcie_capability_read_word(trans_pcie->pci_dev, PCI_EXP_LNKCTL, &lctl);
    //if (lctl & PCI_EXP_LNKCTL_ASPM_L1)
    //    iwl_set_bit(trans, CSR_GIO_REG, CSR_GIO_REG_VAL_L0S_ENABLED);
    //else
    //    iwl_clear_bit(trans, CSR_GIO_REG, CSR_GIO_REG_VAL_L0S_ENABLED);
    //trans->pm_support = !(lctl & PCI_EXP_LNKCTL_ASPM_L0S);
    //
    //pcie_capability_read_word(trans_pcie->pci_dev, PCI_EXP_DEVCTL2, &cap);
    //trans->ltr_enabled = cap & PCI_EXP_DEVCTL2_LTR_EN;
    //IWL_DEBUG_POWER(trans, "L1 %sabled - LTR %sabled\n",
    //                (lctl & PCI_EXP_LNKCTL_ASPM_L1) ? "En" : "Dis",
    //                trans->ltr_enabled ? "En" : "Dis");
    
    
}

void IntelWifi::iwl_pcie_apm_stop(struct iwl_trans *trans, bool op_mode_leave)
{
    IWL_DEBUG_INFO(trans, "Stop card, put in low power state\n");
    
    if (op_mode_leave) {
        if (!test_bit(STATUS_DEVICE_ENABLED, &trans->status))
            iwl_pcie_apm_init(trans);
        
        /* inform ME that we are leaving */
        if (trans->cfg->device_family == IWL_DEVICE_FAMILY_7000)
            io->iwl_set_bits_prph(APMG_PCIDEV_STT_REG,
                                  APMG_PCIDEV_STT_VAL_WAKE_ME);
        else if (trans->cfg->device_family >= IWL_DEVICE_FAMILY_8000) {
            io->iwl_set_bit(CSR_DBG_LINK_PWR_MGMT_REG,
                            CSR_RESET_LINK_PWR_MGMT_DISABLED);
            io->iwl_set_bit(CSR_HW_IF_CONFIG_REG,
                            CSR_HW_IF_CONFIG_REG_PREPARE |
                            CSR_HW_IF_CONFIG_REG_ENABLE_PME);
            IODelay(1);
            io->iwl_clear_bit(CSR_DBG_LINK_PWR_MGMT_REG,
                              CSR_RESET_LINK_PWR_MGMT_DISABLED);
        }
        IODelay(5);
    }
    
    clear_bit(STATUS_DEVICE_ENABLED, &trans->status);
    
    /* Stop device's DMA activity */
    iwl_pcie_apm_stop_master(trans);
    
    // TODO: Implement
    //    if (trans->cfg->lp_xtal_workaround) {
    //        iwl_pcie_apm_lp_xtal_enable(trans);
    //        return;
    //    }
    
    iwl_pcie_sw_reset(trans);
    
    /*
     * Clear "initialization complete" bit to move adapter from
     * D0A* (powered-up Active) --> D0U* (Uninitialized) state.
     */
    io->iwl_clear_bit(CSR_GP_CNTRL,
                      CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
}

void IntelWifi::iwl_pcie_apm_stop_master(struct iwl_trans* trans)
{
    int ret;
    
    /* stop device's busmaster DMA activity */
    io->iwl_set_bit(CSR_RESET, CSR_RESET_REG_FLAG_STOP_MASTER);
    
    ret = io->iwl_poll_bit(CSR_RESET,
                           CSR_RESET_REG_FLAG_MASTER_DISABLED,
                           CSR_RESET_REG_FLAG_MASTER_DISABLED, 100);
    if (ret < 0)
        IWL_WARN(trans, "Master Disable Timed Out, 100 usec\n");
    
    IWL_DEBUG_INFO(trans, "stop master\n");
}

void IntelWifi::iwl_trans_pcie_stop_device(struct iwl_trans *trans, bool low_power)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    bool was_in_rfkill;
    
    IOLockLock(trans_pcie->mutex);
    trans_pcie->opmode_down = true;
    was_in_rfkill = test_bit(STATUS_RFKILL_OPMODE, &trans->status);
    _iwl_trans_pcie_stop_device(trans, low_power);
    //    iwl_trans_pcie_handle_stop_rfkill(was_in_rfkill);
    IOLockUnlock(trans_pcie->mutex);
}

void IntelWifi::_iwl_trans_pcie_stop_device(struct iwl_trans *trans, bool low_power)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    //    lockdep_assert_held(&trans_pcie->mutex);
    
    if (trans_pcie->is_down)
        return;
    
    trans_pcie->is_down = true;
    
    /* tell the device to stop sending interrupts */
    //    iwl_disable_interrupts(trans);
    
    /* device going down, Stop using ICT table */
    //    iwl_pcie_disable_ict(trans);
    
    /*
     * If a HW restart happens during firmware loading,
     * then the firmware loading might call this function
     * and later it might be called again due to the
     * restart. So don't process again if the device is
     * already dead.
     */
    if (test_and_clear_bit(STATUS_DEVICE_ENABLED, &trans->status)) {
        IWL_DEBUG_INFO(trans,
                       "DEVICE_ENABLED bit was set and is now cleared\n");
        //        iwl_pcie_tx_stop(trans);
        //        iwl_pcie_rx_stop(trans);
        
        /* Power-down device's busmaster DMA clocks */
        if (!trans->cfg->apmg_not_supported) {
            io->iwl_write_prph(APMG_CLK_DIS_REG,
                               APMG_CLK_VAL_DMA_CLK_RQT);
            IODelay(5);
        }
    }
    
    /* Make sure (redundant) we've released our request to stay awake */
    io->iwl_clear_bit(CSR_GP_CNTRL,
                      CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
    
    /* Stop the device, and put it in low power state */
    iwl_pcie_apm_stop(trans, false);
    
    iwl_pcie_sw_reset(trans);
    
    /*
     * Upon stop, the IVAR table gets erased, so msi-x won't
     * work. This causes a bug in RF-KILL flows, since the interrupt
     * that enables radio won't fire on the correct irq, and the
     * driver won't be able to handle the interrupt.
     * Configure the IVAR table again after reset.
     */
    //    iwl_pcie_conf_msix_hw(trans_pcie);
    
    /*
     * Upon stop, the APM issues an interrupt if HW RF kill is set.
     * This is a bug in certain verions of the hardware.
     * Certain devices also keep sending HW RF kill interrupt all
     * the time, unless the interrupt is ACKed even if the interrupt
     * should be masked. Re-ACK all the interrupts here.
     */
    //    iwl_disable_interrupts(trans);
    
    /* clear all status bits */
    clear_bit(STATUS_SYNC_HCMD_ACTIVE, &trans->status);
    clear_bit(STATUS_INT_ENABLED, &trans->status);
    clear_bit(STATUS_TPOWER_PMI, &trans->status);
    
    /*
     * Even if we stop the HW, we still want the RF kill
     * interrupt
     */
    //    iwl_enable_rfkill_int(trans);
    
    /* re-take ownership to prevent other users from stealing the device */
    iwl_pcie_prepare_card_hw(trans);
}

