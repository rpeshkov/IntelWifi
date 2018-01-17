//
//  IntelWifi_trans.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 01/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include <IntelWifi.hpp>

#include <kern/task.h>


/* PCI registers */
#define PCI_CFG_RETRY_TIMEOUT    0x041

#define HW_READY_TIMEOUT (50)

/* extended range in FW SRAM */
#define IWL_FW_MEM_EXTENDED_START    0x40000
#define IWL_FW_MEM_EXTENDED_END        0x57FFF


struct iwl_trans* IntelWifi::iwl_trans_pcie_alloc(const struct iwl_cfg *cfg) {
    
    //    if (cfg->gen2)
    //        trans = iwl_trans_alloc(sizeof(struct iwl_trans_pcie),
    //                                &pdev->dev, cfg, &trans_ops_pcie_gen2);
    //    else
    //        trans = iwl_trans_alloc(sizeof(struct iwl_trans_pcie),
    //                                &pdev->dev, cfg, &trans_ops_pcie);
    struct iwl_trans *trans = iwl_trans_alloc(sizeof(struct iwl_trans_pcie), NULL, cfg, NULL);
    
    if (!trans)
        return NULL;
    
    struct iwl_trans_pcie* trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    trans_pcie->trans = trans;
    trans_pcie->opmode_down = true;
    trans_pcie->irq_lock = IOSimpleLockAlloc();
    trans_pcie->reg_lock = IOSimpleLockAlloc();
    trans_pcie->mutex = IOLockAlloc();
    
    trans_pcie->ucode_write_waitq = IOLockAlloc();
    // Implement
    //trans_pcie->tso_hdr_page = alloc_percpu(struct iwl_tso_hdr_page);
    //if (!trans_pcie->tso_hdr_page) {
    //    ret = -ENOMEM;
    //    goto out_no_pci;
    //}
    
    if (!cfg->base_params->pcie_l1_allowed) {
        /*
         * W/A - seems to solve weird behavior. We need to remove this
         * if we don't want to stay in L1 all the time. This wastes a
         * lot of power.
         */
        // TODO: Find out what to call on OSX
        //pci_disable_link_state(pdev, PCIE_LINK_STATE_L0S |
        //                       PCIE_LINK_STATE_L1 |
        //                       PCIE_LINK_STATE_CLKPM);
    }
    
    int addr_size;
    if (cfg->use_tfh) {
        addr_size = 64;
        trans_pcie->max_tbs = IWL_TFH_NUM_TBS;
        trans_pcie->tfd_size = sizeof(struct iwl_tfh_tfd);
    } else {
        addr_size = 36;
        trans_pcie->max_tbs = IWL_NUM_OF_TBS;
        trans_pcie->tfd_size = sizeof(struct iwl_tfd);
    }
    trans->max_skb_frags = IWL_PCIE_MAX_FRAGS(trans_pcie);
    
    // original linux code: pci_set_master(pdev);
    pciDevice->setBusMasterEnable(true);
    
    // TODO: Find out what should be done in IOKit for the code below
    //ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(addr_size));
    //if (!ret)
    //    ret = pci_set_consistent_dma_mask(pdev,
    //                                      DMA_BIT_MASK(addr_size));
    //if (ret) {
    //    ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
    //    if (!ret)
    //        ret = pci_set_consistent_dma_mask(pdev,
    //                                          DMA_BIT_MASK(32));
    //    /* both attempts failed: */
    //    if (ret) {
    //        dev_err(&pdev->dev, "No suitable DMA available\n");
    //        goto out_no_pci;
    //    }
    //}
    
    pciDevice->setMemoryEnable(true);
    fMemoryMap = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!fMemoryMap) {
        TraceLog("MemoryMap failed!");
        return NULL;
    }
    
    trans_pcie->hw_base = reinterpret_cast<volatile void *>(fMemoryMap->getVirtualAddress());
    io = IntelIO::withTrans(trans_pcie);
    
    if (!io) {
        TraceLog("MemoryMap failed!");
        return NULL;
    }
    
    /* We disable the RETRY_TIMEOUT register (0x41) to keep
     * PCI Tx retries from interfering with C3 CPU state */
    pciDevice->configWrite8(PCI_CFG_RETRY_TIMEOUT, 0);
    
    iwl_disable_interrupts(trans);
    
    trans->hw_rev = io->iwl_read32(CSR_HW_REV);
    
    /*
     * In the 8000 HW family the format of the 4 bytes of CSR_HW_REV have
     * changed, and now the revision step also includes bit 0-1 (no more
     * "dash" value). To keep hw_rev backwards compatible - we'll store it
     * in the old format.
     */
    if (trans->cfg->device_family >= IWL_DEVICE_FAMILY_8000) {
        trans->hw_rev = (trans->hw_rev & 0xFFF0) | CSR_HW_REV_STEP(trans->hw_rev << 2) << 2;
        
        int ret = iwl_pcie_prepare_card_hw(trans);
        if (ret) {
            IWL_WARN(trans, "Exit HW not ready\n");
            return NULL;
        }
        
        /*
         * in-order to recognize C step driver should read chip version
         * id located at the AUX bus MISC address space.
         */
        io->iwl_set_bit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
        
        IODelay(2);
        
        ret = io->iwl_poll_bit(CSR_GP_CNTRL,
                               CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                               CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                               25000);
        
        if (ret < 0) {
            IWL_DEBUG_INFO(trans, "Failed to wake up the nic");
            return NULL;
        }
        
        IOInterruptState state;
        if (io->iwl_grab_nic_access(&state)) {
            u32 hw_step;
            
            hw_step = io->iwl_read_prph_no_grab(WFPM_CTRL_REG);
            hw_step |= ENABLE_WFPM;
            io->iwl_write_prph_no_grab(WFPM_CTRL_REG, hw_step);
            hw_step = io->iwl_read_prph_no_grab(AUX_MISC_REG);
            hw_step = (hw_step >> HW_STEP_LOCATION_BITS) & 0xF;
            if (hw_step == 0x3)
                trans->hw_rev = (trans->hw_rev & 0xFFFFFFF3) | (SILICON_C_STEP << 2);
            io->iwl_release_nic_access(&state);
        }
    }
    
    /*
     * 9000-series integrated A-step has a problem with suspend/resume
     * and sometimes even causes the whole platform to get stuck. This
     * workaround makes the hardware not go into the problematic state.
     */
    if (trans->cfg->integrated && trans->cfg->device_family == IWL_DEVICE_FAMILY_9000 && CSR_HW_REV_STEP(trans->hw_rev) == SILICON_A_STEP)
        io->iwl_set_bit(CSR_HOST_CHICKEN, CSR_HOST_CHICKEN_PM_IDLE_SRC_DIS_SB_PME);
    
#ifdef CONFIG_IWLMVM
    trans->hw_rf_id = io->iwl_read32(CSR_HW_RF_ID);
    if (trans->hw_rf_id == CSR_HW_RF_ID_TYPE_HR) {
        UInt32 hw_status;
        
        hw_status = io->iwl_read_prph(UMAG_GEN_HW_STATUS);
        if (hw_status & UMAG_GEN_HW_IS_FPGA)
            trans->cfg = const_cast<struct iwl_cfg *>(&iwla000_2ax_cfg_qnj_hr_f0);
        else
            trans->cfg = const_cast<struct iwl_cfg *>(&iwla000_2ac_cfg_hr);
    }
#endif
    
    iwl_pcie_set_interrupt_capa(trans);
    
    trans->hw_id = (fDeviceId << 16) + fSubsystemId;
    snprintf(trans->hw_id_str, sizeof(trans->hw_id_str), "PCI ID: 0x%04X:0x%04X", fDeviceId, fSubsystemId);
    DebugLog("%s", trans->hw_id_str);
    
    
    /* Initialize the wait queue for commands */
    trans_pcie->wait_command_queue = IOLockAlloc();
    trans_pcie->d0i3_waitq = IOLockAlloc();
    
    // TODO: Implement
    int ret;
    if (1 == 0 /*trans_pcie->msix_enabled*/) {
//        ret = iwl_pcie_init_msix_handler(pdev, trans_pcie);
//        if (ret)
//            goto out_no_pci;
    } else {
        ret = iwl_pcie_alloc_ict(trans);
//        if (ret)
//            goto out_no_pci;

//        ret = devm_request_threaded_irq(&pdev->dev, pdev->irq,
//                                        iwl_pcie_isr,
//                                        iwl_pcie_irq_handler,
//                                        IRQF_SHARED, DRV_NAME, trans);
//        if (ret) {
//            IWL_ERR(trans, "Error allocating IRQ %d\n", pdev->irq);
//            goto out_free_ict;
//        }
        trans_pcie->inta_mask = CSR_INI_SET_MASK;
    }

//    trans_pcie->rba.alloc_wq = alloc_workqueue("rb_allocator",
//                                               WQ_HIGHPRI | WQ_UNBOUND, 1);
//    INIT_WORK(&trans_pcie->rba.rx_alloc, iwl_pcie_rx_allocator_work);
    
    
#ifdef CONFIG_IWLWIFI_PCIE_RTPM
    trans->runtime_pm_mode = IWL_PLAT_PM_MODE_D0I3;
#else
    trans->runtime_pm_mode = IWL_PLAT_PM_MODE_DISABLED;
#endif /* CONFIG_IWLWIFI_PCIE_RTPM */
    
    return trans;
}

void IntelWifi::iwl_trans_pcie_free(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    //    int i;
    //
    //    iwl_pcie_synchronize_irqs(trans);
    //
    //    if (trans->cfg->gen2)
    //        iwl_pcie_gen2_tx_free(trans);
    //    else
    //        iwl_pcie_tx_free(trans);
    //    iwl_pcie_rx_free(trans);
    //
    //    if (trans_pcie->rba.alloc_wq) {
    //        destroy_workqueue(trans_pcie->rba.alloc_wq);
    //        trans_pcie->rba.alloc_wq = NULL;
    //    }
    //
    //    if (trans_pcie->msix_enabled) {
    //        for (i = 0; i < trans_pcie->alloc_vecs; i++) {
    //            irq_set_affinity_hint(
    //                                  trans_pcie->msix_entries[i].vector,
    //                                  NULL);
    //        }
    //
    //        trans_pcie->msix_enabled = false;
    //    } else {
    //        iwl_pcie_free_ict(trans);
    //    }
    //
    //    iwl_pcie_free_fw_monitor(trans);
    //
    //    for_each_possible_cpu(i) {
    //        struct iwl_tso_hdr_page *p =
    //        per_cpu_ptr(trans_pcie->tso_hdr_page, i);
    //
    //        if (p->page)
    //            __free_page(p->page);
    //    }
    
    //    free_percpu(trans_pcie->tso_hdr_page);
    IOSimpleLockFree(trans_pcie->irq_lock);
    IOSimpleLockFree(trans_pcie->reg_lock);
    IOLockFree(trans_pcie->mutex);
    iwl_trans_free(trans);
}

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
    
    iwl_pcie_init_msix(trans_pcie);
    
    /* From now on, the op_mode will be kept updated about RF kill state */
    iwl_enable_rfkill_int(trans);
    
    trans_pcie->opmode_down = false;
    
    /* Set is_down to false here so that...*/
    trans_pcie->is_down = false;
    
    /* ...rfkill can call stop_device and set it false if needed */
    iwl_pcie_check_hw_rf_kill(trans);
    
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
    
    if (trans->cfg->lp_xtal_workaround) {
        iwl_pcie_apm_lp_xtal_enable(trans);
        return;
    }
    
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
    iwl_trans_pcie_handle_stop_rfkill(trans, was_in_rfkill);
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
    iwl_disable_interrupts(trans);
    
    /* device going down, Stop using ICT table */
    iwl_pcie_disable_ict(trans);
    
    /*
     * If a HW restart happens during firmware loading,
     * then the firmware loading might call this function
     * and later it might be called again due to the
     * restart. So don't process again if the device is
     * already dead.
     */
    if (test_and_clear_bit(STATUS_DEVICE_ENABLED, &trans->status)) {
        IWL_DEBUG_INFO(trans, "DEVICE_ENABLED bit was set and is now cleared\n");
        // iwl_pcie_tx_stop(trans);
        iwl_pcie_rx_stop(trans);
        
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
    iwl_pcie_conf_msix_hw(trans_pcie);
    
    /*
     * Upon stop, the APM issues an interrupt if HW RF kill is set.
     * This is a bug in certain verions of the hardware.
     * Certain devices also keep sending HW RF kill interrupt all
     * the time, unless the interrupt is ACKed even if the interrupt
     * should be masked. Re-ACK all the interrupts here.
     */
    iwl_disable_interrupts(trans);
    
    /* clear all status bits */
    clear_bit(STATUS_SYNC_HCMD_ACTIVE, &trans->status);
    clear_bit(STATUS_INT_ENABLED, &trans->status);
    clear_bit(STATUS_TPOWER_PMI, &trans->status);
    
    /*
     * Even if we stop the HW, we still want the RF kill
     * interrupt
     */
    iwl_enable_rfkill_int(trans);
    
    /* re-take ownership to prevent other users from stealing the device */
    iwl_pcie_prepare_card_hw(trans);
}

/* Note: returns poll_bit return value, which is >= 0 if success */
int IntelWifi::iwl_pcie_set_hw_ready(struct iwl_trans *trans)
{
    int ret;
    
    io->iwl_set_bit(CSR_HW_IF_CONFIG_REG,
                    CSR_HW_IF_CONFIG_REG_BIT_NIC_READY);
    
    /* See if we got it */
    ret = io->iwl_poll_bit(CSR_HW_IF_CONFIG_REG,
                           CSR_HW_IF_CONFIG_REG_BIT_NIC_READY,
                           CSR_HW_IF_CONFIG_REG_BIT_NIC_READY,
                           HW_READY_TIMEOUT);
    
    if (ret >= 0)
        io->iwl_set_bit(CSR_MBOX_SET_REG, CSR_MBOX_SET_REG_OS_ALIVE);
    
    IWL_DEBUG_INFO(trans, "hardware%s ready\n", ret < 0 ? " not" : "");
    
    return ret;
}

void IntelWifi::_iwl_disable_interrupts(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    clear_bit(STATUS_INT_ENABLED, &trans->status);
    if (!trans_pcie->msix_enabled) {
        /* disable interrupts from uCode/NIC to host */
        io->iwl_write32(CSR_INT_MASK, 0x00000000);
        
        /* acknowledge/clear/reset any interrupts still pending
         * from uCode or flow handler (Rx/Tx DMA) */
        io->iwl_write32(CSR_INT, 0xffffffff);
        io->iwl_write32(CSR_FH_INT_STATUS, 0xffffffff);
    } else {
        /* disable all the interrupt we might use */
        io->iwl_write32(CSR_MSIX_FH_INT_MASK_AD,
                        trans_pcie->fh_init_mask);
        io->iwl_write32(CSR_MSIX_HW_INT_MASK_AD,
                        trans_pcie->hw_init_mask);
    }
    IWL_DEBUG_ISR(trans, "Disabled interrupts\n");
}

void IntelWifi::iwl_disable_interrupts(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);

    IOSimpleLockLock(trans_pcie->irq_lock);
    _iwl_disable_interrupts(trans);
    IOSimpleLockUnlock(trans_pcie->irq_lock);
}

void IntelWifi::iwl_enable_rfkill_int(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    IWL_DEBUG_ISR(trans, "Enabling rfkill interrupt\n");
    if (!trans_pcie->msix_enabled) {
        trans_pcie->inta_mask = CSR_INT_BIT_RF_KILL;
        io->iwl_write32(CSR_INT_MASK, trans_pcie->inta_mask);
    } else {
        io->iwl_write32(CSR_MSIX_FH_INT_MASK_AD,
                    trans_pcie->fh_init_mask);
        iwl_enable_hw_int_msk_msix(trans,
                                   MSIX_HW_INT_CAUSES_REG_RF_KILL);
    }
    
    if (trans->cfg->device_family == IWL_DEVICE_FAMILY_9000) {
        /*
         * On 9000-series devices this bit isn't enabled by default, so
         * when we power down the device we need set the bit to allow it
         * to wake up the PCI-E bus for RF-kill interrupts.
         */
        io->iwl_set_bit(CSR_GP_CNTRL,
                    CSR_GP_CNTRL_REG_FLAG_RFKILL_WAKE_L1A_EN);
    }
}

void IntelWifi::iwl_enable_hw_int_msk_msix(struct iwl_trans *trans, u32 msk)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    io->iwl_write32(CSR_MSIX_HW_INT_MASK_AD, ~msk);
    trans_pcie->hw_mask = msk;
}

bool IntelWifi::iwl_pcie_check_hw_rf_kill(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie =  IWL_TRANS_GET_PCIE_TRANS(trans);
    bool hw_rfkill = iwl_is_rfkill_set(trans);
    bool prev = test_bit(STATUS_RFKILL_OPMODE, &trans->status);
    bool report;
    
    if (hw_rfkill) {
        set_bit(STATUS_RFKILL_HW, &trans->status);
        set_bit(STATUS_RFKILL_OPMODE, &trans->status);
    } else {
        clear_bit(STATUS_RFKILL_HW, &trans->status);
        if (trans_pcie->opmode_down)
            clear_bit(STATUS_RFKILL_OPMODE, &trans->status);
    }
    
    report = test_bit(STATUS_RFKILL_OPMODE, &trans->status);
    
    if (prev != report)
        iwl_trans_pcie_rf_kill(trans, report);
    
    return hw_rfkill;
}

bool IntelWifi::iwl_is_rfkill_set(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    //lockdep_assert_held(&trans_pcie->mutex);
    
    if (trans_pcie->debug_rfkill)
        return true;
    
    return !(io->iwl_read32(CSR_GP_CNTRL) &
             CSR_GP_CNTRL_REG_FLAG_HW_RF_KILL_SW);
}

void IntelWifi::iwl_trans_pcie_rf_kill(struct iwl_trans *trans, bool state)
{
    //struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    //lockdep_assert_held(&trans_pcie->mutex);
    
    IWL_WARN(trans, "reporting RF_KILL (radio %s)\n",
             state ? "disabled" : "enabled");
    
    // TODO: implement
//    if (iwl_op_mode_hw_rf_kill(trans->op_mode, state)) {
//        if (trans->cfg->gen2)
//            _iwl_trans_pcie_gen2_stop_device(trans, true);
//        else
//            _iwl_trans_pcie_stop_device(trans, true);
//    }
}

/*
 * Enable LP XTAL to avoid HW bug where device may consume much power if
 * FW is not loaded after device reset. LP XTAL is disabled by default
 * after device HW reset. Do it only if XTAL is fed by internal source.
 * Configure device's "persistence" mode to avoid resetting XTAL again when
 * SHRD_HW_RST occurs in S3.
 */
void IntelWifi::iwl_pcie_apm_lp_xtal_enable(struct iwl_trans *trans)
{
    int ret;
    u32 apmg_gp1_reg;
    u32 apmg_xtal_cfg_reg;
    u32 dl_cfg_reg;
    
    /* Force XTAL ON */
    // TODO: Double check here. In original code you see that no lock is acquired. Weird... Below there are some lines with lock acquiring
    //__iwl_trans_pcie_set_bit(trans, CSR_GP_CNTRL,
    //                         CSR_GP_CNTRL_REG_FLAG_XTAL_ON);
    io->iwl_set_bit(CSR_GP_CNTRL,
                             CSR_GP_CNTRL_REG_FLAG_XTAL_ON);
    
    iwl_pcie_sw_reset(trans);
    
    /*
     * Set "initialization complete" bit to move adapter from
     * D0U* --> D0A* (powered-up active) state.
     */
    io->iwl_set_bit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
    
    /*
     * Wait for clock stabilization; once stabilized, access to
     * device-internal resources is possible.
     */
    ret = io->iwl_poll_bit(CSR_GP_CNTRL,
                       CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                       CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                       25000);
    if (WARN_ON(ret < 0)) {
        IWL_ERR(trans, "Access time out - failed to enable LP XTAL\n");
        /* Release XTAL ON request */
        io->iwl_clear_bit(CSR_GP_CNTRL,
                                   CSR_GP_CNTRL_REG_FLAG_XTAL_ON);
        return;
    }
    
    /*
     * Clear "disable persistence" to avoid LP XTAL resetting when
     * SHRD_HW_RST is applied in S3.
     */
    io->iwl_clear_bits_prph(APMG_PCIDEV_STT_REG,
                        APMG_PCIDEV_STT_VAL_PERSIST_DIS);
    
    /*
     * Force APMG XTAL to be active to prevent its disabling by HW
     * caused by APMG idle state.
     */
    apmg_xtal_cfg_reg = io->iwl_trans_pcie_read_shr(SHR_APMG_XTAL_CFG_REG);
    io->iwl_trans_pcie_write_shr(SHR_APMG_XTAL_CFG_REG,
                             apmg_xtal_cfg_reg |
                             SHR_APMG_XTAL_CFG_XTAL_ON_REQ);
    
    iwl_pcie_sw_reset(trans);
    
    /* Enable LP XTAL by indirect access through CSR */
    apmg_gp1_reg = io->iwl_trans_pcie_read_shr(SHR_APMG_GP1_REG);
    io->iwl_trans_pcie_write_shr(SHR_APMG_GP1_REG, apmg_gp1_reg |
                             SHR_APMG_GP1_WF_XTAL_LP_EN |
                             SHR_APMG_GP1_CHICKEN_BIT_SELECT);
    
    /* Clear delay line clock power up */
    dl_cfg_reg = io->iwl_trans_pcie_read_shr(SHR_APMG_DL_CFG_REG);
    io->iwl_trans_pcie_write_shr(SHR_APMG_DL_CFG_REG, dl_cfg_reg &
                             ~SHR_APMG_DL_CFG_DL_CLOCK_POWER_UP);
    
    /*
     * Enable persistence mode to avoid LP XTAL resetting when
     * SHRD_HW_RST is applied in S3.
     */
    io->iwl_set_bit(CSR_HW_IF_CONFIG_REG,
                CSR_HW_IF_CONFIG_REG_PERSIST_MODE);
    
    /*
     * Clear "initialization complete" bit to move adapter from
     * D0A* (powered-up Active) --> D0U* (Uninitialized) state.
     */
    io->iwl_clear_bit(CSR_GP_CNTRL,
                  CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
    
    /* Activates XTAL resources monitor */
    io->iwl_set_bit(CSR_MONITOR_CFG_REG,
                             CSR_MONITOR_XTAL_RESOURCES);
    
    /* Release XTAL ON request */
    io->iwl_clear_bit(CSR_GP_CNTRL,
                               CSR_GP_CNTRL_REG_FLAG_XTAL_ON);
    IODelay(10);
    
    /* Release APMG XTAL */
    io->iwl_trans_pcie_write_shr(SHR_APMG_XTAL_CFG_REG,
                             apmg_xtal_cfg_reg &
                             ~SHR_APMG_XTAL_CFG_XTAL_ON_REQ);
}

void IntelWifi::_iwl_enable_interrupts(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    IWL_DEBUG_ISR(trans, "Enabling interrupts\n");
    set_bit(STATUS_INT_ENABLED, &trans->status);

    if (!trans_pcie->msix_enabled) {
        trans_pcie->inta_mask = CSR_INI_SET_MASK;
        io->iwl_write32(CSR_INT_MASK, trans_pcie->inta_mask);
    } else {
        /*
         * fh/hw_mask keeps all the unmasked causes.
         * Unlike msi, in msix cause is enabled when it is unset.
         */
        trans_pcie->hw_mask = trans_pcie->hw_init_mask;
        trans_pcie->fh_mask = trans_pcie->fh_init_mask;
        io->iwl_write32(CSR_MSIX_FH_INT_MASK_AD,
                    ~trans_pcie->fh_mask);
        io->iwl_write32(CSR_MSIX_HW_INT_MASK_AD,
                    ~trans_pcie->hw_mask);
    }
}

void IntelWifi::iwl_enable_interrupts(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);

    IOSimpleLockLock(trans_pcie->irq_lock);
    _iwl_enable_interrupts(trans);
    IOSimpleLockUnlock(trans_pcie->irq_lock);
}

int IntelWifi::iwl_trans_pcie_start_fw(struct iwl_trans *trans, const struct fw_img *fw, bool run_in_rfkill)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    bool hw_rfkill;
    int ret;
    
    /* This may fail if AMT took ownership of the device */
    if (iwl_pcie_prepare_card_hw(trans)) {
        IWL_WARN(trans, "Exit HW not ready\n");
        ret = -EIO;
        goto out;
    }
    
    iwl_enable_rfkill_int(trans);
    
    io->iwl_write32(CSR_INT, 0xFFFFFFFF);
    
    /*
     * We enabled the RF-Kill interrupt and the handler may very
     * well be running. Disable the interrupts to make sure no other
     * interrupt can be fired.
     */
    iwl_disable_interrupts(trans);
    
    /* Make sure it finished running */
    // TODO: Implement
    //iwl_pcie_synchronize_irqs(trans);
    
    //mutex_lock(&trans_pcie->mutex);
    IOLockLock(trans_pcie->mutex);
    
    /* If platform's RF_KILL switch is NOT set to KILL */
    hw_rfkill = iwl_pcie_check_hw_rf_kill(trans);
    if (hw_rfkill && !run_in_rfkill) {
        ret = -ERFKILL;
        goto out;
    }
    
    /* Someone called stop_device, don't try to start_fw */
    if (trans_pcie->is_down) {
        IWL_WARN(trans,
                 "Can't start_fw since the HW hasn't been started\n");
        ret = -EIO;
        goto out;
    }
    
    /* make sure rfkill handshake bits are cleared */
    io->iwl_write32(CSR_UCODE_DRV_GP1_CLR, CSR_UCODE_SW_BIT_RFKILL);
    io->iwl_write32(CSR_UCODE_DRV_GP1_CLR,
                CSR_UCODE_DRV_GP1_BIT_CMD_BLOCKED);
    
    /* clear (again), then enable host interrupts */
    io->iwl_write32(CSR_INT, 0xFFFFFFFF);
    
    ret = iwl_pcie_nic_init(trans);
    if (ret) {
        IWL_ERR(trans, "Unable to init nic\n");
        goto out;
    }
    
    /*
     * Now, we load the firmware and don't want to be interrupted, even
     * by the RF-Kill interrupt (hence mask all the interrupt besides the
     * FH_TX interrupt which is needed to load the firmware). If the
     * RF-Kill switch is toggled, we will find out after having loaded
     * the firmware and return the proper value to the caller.
     */
    iwl_enable_fw_load_int(trans);
    
    /* really make sure rfkill handshake bits are cleared */
    io->iwl_write32(CSR_UCODE_DRV_GP1_CLR, CSR_UCODE_SW_BIT_RFKILL);
    io->iwl_write32(CSR_UCODE_DRV_GP1_CLR, CSR_UCODE_SW_BIT_RFKILL);
    
    /* Load the given image to the HW */
    if (trans->cfg->device_family >= IWL_DEVICE_FAMILY_8000)
        ret = iwl_pcie_load_given_ucode_8000(trans, fw);
    else
        ret = iwl_pcie_load_given_ucode(trans, fw);
    
    /* re-check RF-Kill state since we may have missed the interrupt */
    hw_rfkill = iwl_pcie_check_hw_rf_kill(trans);
    if (hw_rfkill && !run_in_rfkill)
        ret = -ERFKILL;
    
out:
    //mutex_unlock(&trans_pcie->mutex);
    IOLockUnlock(trans_pcie->mutex);
    return ret;
}

int IntelWifi::iwl_pcie_nic_init(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    int ret;
    
    /* nic_init */
    IOSimpleLockLock(trans_pcie->irq_lock);
    ret = iwl_pcie_apm_init(trans);
    IOSimpleLockUnlock(trans_pcie->irq_lock);
    
    if (ret)
        return ret;
    
    iwl_pcie_set_pwr(trans, false);
    
    // TODO: Deal with this. I'm not setting up op_mode functions mapping
    //iwl_op_mode_nic_config(trans->op_mode);
    opmode->nic_config(0);
    
    
    /* Allocate the RX queue, or reset if it is already allocated */
    iwl_pcie_rx_init(trans);
    
    /* Allocate or reset and init all Tx and Command queues */
    if (iwl_pcie_tx_init(trans))
        return -ENOMEM;
    
    if (trans->cfg->base_params->shadow_reg_enable) {
        /* enable shadow regs in HW */
        io->iwl_set_bit(CSR_MAC_SHADOW_REG_CTRL, 0x800FFFFF);
        IWL_DEBUG_INFO(trans, "Enabling shadow registers in device\n");
    }
    
    return 0;
}

void IntelWifi::iwl_enable_fw_load_int(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    IWL_DEBUG_ISR(trans, "Enabling FW load interrupt\n");
    // TODO: Implement msix
    if (!trans_pcie->msix_enabled) {
        trans_pcie->inta_mask = CSR_INT_BIT_FH_TX;
        io->iwl_write32(CSR_INT_MASK, trans_pcie->inta_mask);
    } else {
        io->iwl_write32(CSR_MSIX_HW_INT_MASK_AD,
                    trans_pcie->hw_init_mask);
//        iwl_enable_fh_int_msk_msix(trans,
//                                   MSIX_FH_INT_CAUSES_D2S_CH0_NUM);
    }
}

int IntelWifi::iwl_pcie_load_given_ucode(struct iwl_trans *trans,
                                     const struct fw_img *image)
{
    //struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    int ret = 0;
    int first_ucode_section;
    
    IWL_DEBUG_FW(trans, "working with %s CPU\n",
                 image->is_dual_cpus ? "Dual" : "Single");
    
    /* load to FW the binary non secured sections of CPU1 */
    ret = iwl_pcie_load_cpu_sections(trans, image, 1, &first_ucode_section);
    if (ret)
        return ret;
    
    if (image->is_dual_cpus) {
        /* set CPU2 header address */
        io->iwl_write_prph(
                       LMPM_SECURE_UCODE_LOAD_CPU2_HDR_ADDR,
                       LMPM_SECURE_CPU2_HDR_MEM_SPACE);
        
        /* load to FW the binary sections of CPU2 */
        ret = iwl_pcie_load_cpu_sections(trans, image, 2,
                                         &first_ucode_section);
        if (ret)
            return ret;
    }
    
    /* supported for 7000 only for the moment */
    // TODO: Implement
//    if (iwlwifi_mod_params.fw_monitor &&
//        trans->cfg->device_family == IWL_DEVICE_FAMILY_7000) {
//        iwl_pcie_alloc_fw_monitor(trans, 0);
//
//        if (trans_pcie->fw_mon_size) {
//            iwl_write_prph(trans, MON_BUFF_BASE_ADDR,
//                           trans_pcie->fw_mon_phys >> 4);
//            iwl_write_prph(trans, MON_BUFF_END_ADDR,
//                           (trans_pcie->fw_mon_phys +
//                            trans_pcie->fw_mon_size) >> 4);
//        }
//    } else
    if (trans->dbg_dest_tlv) {
        iwl_pcie_apply_destination(trans);
    }
    
    iwl_enable_interrupts(trans);
    
    /* release CPU reset */
    io->iwl_write32(CSR_RESET, 0);
    
    return 0;
}

int IntelWifi::iwl_pcie_load_given_ucode_8000(struct iwl_trans *trans,
                                          const struct fw_img *image)
{
    int ret = 0;
    int first_ucode_section;
    
    IWL_DEBUG_FW(trans, "working with %s CPU\n",
                 image->is_dual_cpus ? "Dual" : "Single");
    
    if (trans->dbg_dest_tlv)
        iwl_pcie_apply_destination(trans);
    
    IWL_DEBUG_POWER(trans, "Original WFPM value = 0x%08X\n",
                    io->iwl_read_prph(WFPM_GP2));
    
    /*
     * Set default value. On resume reading the values that were
     * zeored can provide debug data on the resume flow.
     * This is for debugging only and has no functional impact.
     */
    io->iwl_write_prph(WFPM_GP2, 0x01010101);
    
    /* configure the ucode to be ready to get the secured image */
    /* release CPU reset */
    io->iwl_write_prph(RELEASE_CPU_RESET, RELEASE_CPU_RESET_BIT);
    
    /* load to FW the binary Secured sections of CPU1 */
    ret = iwl_pcie_load_cpu_sections_8000(trans, image, 1,
                                          &first_ucode_section);
    if (ret)
        return ret;

    /* load to FW the binary sections of CPU2 */
    return iwl_pcie_load_cpu_sections_8000(trans, image, 2,
                                           &first_ucode_section);
}

int IntelWifi::iwl_pcie_load_cpu_sections(struct iwl_trans *trans,
                                      const struct fw_img *image,
                                      int cpu,
                                      int *first_ucode_section)
{
    int i, ret = 0;
    u32 last_read_idx = 0;
    
    if (cpu == 1)
        *first_ucode_section = 0;
    else
        (*first_ucode_section)++;
    
    for (i = *first_ucode_section; i < image->num_sec; i++) {
        last_read_idx = i;
        
        /*
         * CPU1_CPU2_SEPARATOR_SECTION delimiter - separate between
         * CPU1 to CPU2.
         * PAGING_SEPARATOR_SECTION delimiter - separate between
         * CPU2 non paged to CPU2 paging sec.
         */
        if (!image->sec[i].data ||
            image->sec[i].offset == CPU1_CPU2_SEPARATOR_SECTION ||
            image->sec[i].offset == PAGING_SEPARATOR_SECTION) {
            IWL_DEBUG_FW(trans,
                         "Break since Data not valid or Empty section, sec = %d\n",
                         i);
            break;
        }
        
        ret = iwl_pcie_load_section(trans, i, &image->sec[i]);
        if (ret)
            return ret;
    }
    
    *first_ucode_section = last_read_idx;
    
    return 0;
}

void IntelWifi::iwl_pcie_apply_destination(struct iwl_trans *trans)
{
    //struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    const struct iwl_fw_dbg_dest_tlv *dest = trans->dbg_dest_tlv;
    int i;
    
    if (dest->version)
        IWL_ERR(trans,
                "DBG DEST version is %d - expect issues\n",
                dest->version);
    
    IWL_INFO(trans, "Applying debug destination %s\n",
             get_fw_dbg_mode_string(dest->monitor_mode));

    // TODO: Implement
//    if (dest->monitor_mode == EXTERNAL_MODE)
//        iwl_pcie_alloc_fw_monitor(trans, dest->size_power);
//    else
//        IWL_WARN(trans, "PCI should have external buffer debug\n");
    
    for (i = 0; i < trans->dbg_dest_reg_num; i++) {
        u32 addr = le32_to_cpu(dest->reg_ops[i].addr);
        u32 val = le32_to_cpu(dest->reg_ops[i].val);
        
        switch (dest->reg_ops[i].op) {
            case CSR_ASSIGN:
                io->iwl_write32(addr, val);
                break;
            case CSR_SETBIT:
                io->iwl_set_bit(addr, (u32)BIT(val));
                break;
            case CSR_CLEARBIT:
                io->iwl_clear_bit(addr, (u32)BIT(val));
                break;
            case PRPH_ASSIGN:
                io->iwl_write_prph(addr, val);
                break;
            case PRPH_SETBIT:
                io->iwl_set_bits_prph(addr, (u32)BIT(val));
                break;
            case PRPH_CLEARBIT:
                io->iwl_clear_bits_prph(addr, (u32)BIT(val));
                break;
            case PRPH_BLOCKBIT:
                if (io->iwl_read_prph(addr) & BIT(val)) {
                    IWL_ERR(trans,
                            "BIT(%u) in address 0x%x is 1, stopping FW configuration\n",
                            val, addr);
                    //goto monitor;
                }
                break;
            default:
                IWL_ERR(trans, "FW debug - unknown OP %d\n",
                        dest->reg_ops[i].op);
                break;
        }
    }

    // TODO: Implement
//monitor:
//    if (dest->monitor_mode == EXTERNAL_MODE && trans_pcie->fw_mon_size) {
//        iwl_write_prph(trans, le32_to_cpu(dest->base_reg),
//                       trans_pcie->fw_mon_phys >> dest->base_shift);
//        if (trans->cfg->device_family >= IWL_DEVICE_FAMILY_8000)
//            iwl_write_prph(trans, le32_to_cpu(dest->end_reg),
//                           (trans_pcie->fw_mon_phys +
//                            trans_pcie->fw_mon_size - 256) >>
//                           dest->end_shift);
//        else
//            iwl_write_prph(trans, le32_to_cpu(dest->end_reg),
//                           (trans_pcie->fw_mon_phys +
//                            trans_pcie->fw_mon_size) >>
//                           dest->end_shift);
//    }
}

int IntelWifi::iwl_pcie_load_section(struct iwl_trans *trans, u8 section_num,
                                 const struct fw_desc *section)
{
//    u8 *v_addr;
//    dma_addr_t p_addr;
    u32 offset, chunk_sz = min(FH_MEM_TB_MAX_LENGTH, (u32)section->len);
    int ret = 0;

    IWL_DEBUG_FW(trans, "[%d] uCode section being loaded...\n",
                 section_num);

//    v_addr = dma_alloc_coherent(trans->dev, chunk_sz, &p_addr,
//                                GFP_KERNEL | __GFP_NOWARN);

    IOBufferMemoryDescriptor *bmd =
    IOBufferMemoryDescriptor::inTaskWithPhysicalMask(
                                                     // task to hold the memory
                                                     kernel_task,
                                                     // options
                                                     kIOMemoryPhysicallyContiguous,
                                                     // size
                                                     chunk_sz,
                                                     // physicalMask - 32 bit addressable and page aligned
                                                     0x00000000FFFFFFFFULL);

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
//    if (!v_addr) {
//        IWL_DEBUG_INFO(trans, "Falling back to small chunks of DMA\n");
//        chunk_sz = PAGE_SIZE;
//        v_addr = dma_alloc_coherent(trans->dev, chunk_sz,
//                                    &p_addr, GFP_KERNEL);
//        if (!v_addr)
//            return -ENOMEM;
//    }

    for (offset = 0; offset < section->len; offset += chunk_sz) {
        u32 copy_size, dst_addr;
        bool extended_addr = false;

        copy_size = min(chunk_sz, (u32)(section->len - offset));
        dst_addr = section->offset + offset;

        if (dst_addr >= IWL_FW_MEM_EXTENDED_START &&
            dst_addr <= IWL_FW_MEM_EXTENDED_END)
            extended_addr = true;

        if (extended_addr)
            io->iwl_set_bits_prph(LMPM_CHICK,
                              LMPM_CHICK_EXTENDED_ADDR_SPACE);
        
        cmd->writeBytes(0, (u8 *)section->data + offset, copy_size);
        ret = iwl_pcie_load_firmware_chunk(trans, dst_addr, seg.fIOVMAddr, copy_size);

        
        if (extended_addr)
            io->iwl_clear_bits_prph(LMPM_CHICK,
                                LMPM_CHICK_EXTENDED_ADDR_SPACE);

        if (ret) {
            IWL_ERR(trans,
                    "Could not load the [%d] uCode section\n",
                    section_num);
            break;
        }
    }
    cmd->complete();
    cmd->clearMemoryDescriptor();
    cmd->release();
//    dma_free_coherent(trans->dev, chunk_sz, v_addr, p_addr);
    return ret;
}

void IntelWifi::iwl_pcie_load_firmware_chunk_fh(struct iwl_trans *trans,
                                            u32 dst_addr, dma_addr_t phy_addr,
                                            u32 byte_cnt)
{
    io->iwl_write32(FH_TCSR_CHNL_TX_CONFIG_REG(FH_SRVC_CHNL),
                FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_PAUSE);
    
    io->iwl_write32(FH_SRVC_CHNL_SRAM_ADDR_REG(FH_SRVC_CHNL),
                dst_addr);
    
    io->iwl_write32(FH_TFDIB_CTRL0_REG(FH_SRVC_CHNL),
                phy_addr & FH_MEM_TFDIB_DRAM_ADDR_LSB_MSK);
    
    io->iwl_write32(FH_TFDIB_CTRL1_REG(FH_SRVC_CHNL),
                (iwl_get_dma_hi_addr(phy_addr)
                 << FH_MEM_TFDIB_REG1_ADDR_BITSHIFT) | byte_cnt);
    
    io->iwl_write32(FH_TCSR_CHNL_TX_BUF_STS_REG(FH_SRVC_CHNL),
                BIT(FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_NUM) |
                BIT(FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_IDX) |
                FH_TCSR_CHNL_TX_BUF_STS_REG_VAL_TFDB_VALID);
    
    io->iwl_write32(FH_TCSR_CHNL_TX_CONFIG_REG(FH_SRVC_CHNL),
                FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_ENABLE |
                FH_TCSR_TX_CONFIG_REG_VAL_DMA_CREDIT_DISABLE |
                FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_HOST_ENDTFD);
}

int IntelWifi::iwl_pcie_load_firmware_chunk(struct iwl_trans *trans,
                                        u32 dst_addr, dma_addr_t phy_addr,
                                        u32 byte_cnt)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    IOInterruptState state;
    int ret;
    
    trans_pcie->ucode_write_complete = false;
    
    if (!io->iwl_grab_nic_access(&state))
        return -EIO;
    
    iwl_pcie_load_firmware_chunk_fh(trans, dst_addr, phy_addr,
                                    byte_cnt);
    io->iwl_release_nic_access(&state);

    // TODO: Implement
    IOLockLock(trans_pcie->ucode_write_waitq);
    if (trans_pcie->ucode_write_complete) {
        IOLockUnlock(trans_pcie->ucode_write_waitq);
        return 0;
    }
    AbsoluteTime deadline;
    clock_interval_to_deadline(5, kSecondScale, (UInt64 *) &deadline);
    ret = IOLockSleepDeadline(trans_pcie->ucode_write_waitq, &trans_pcie->ucode_write_complete, *((AbsoluteTime *) &deadline), THREAD_INTERRUPTIBLE);
    IOLockUnlock(trans_pcie->ucode_write_waitq);
    if (ret != THREAD_AWAKENED) {
        IWL_ERR(trans, "Failed to load firmware chunk!\n");
        return -ETIMEDOUT;
    }
    
    return 0;
}

void IntelWifi::iwl_trans_pcie_handle_stop_rfkill(struct iwl_trans *trans,
                                       bool was_in_rfkill)
{
    bool hw_rfkill;
    
    /*
     * Check again since the RF kill state may have changed while
     * all the interrupts were disabled, in this case we couldn't
     * receive the RF kill interrupt and update the state in the
     * op_mode.
     * Don't call the op_mode if the rkfill state hasn't changed.
     * This allows the op_mode to call stop_device from the rfkill
     * notification without endless recursion. Under very rare
     * circumstances, we might have a small recursion if the rfkill
     * state changed exactly now while we were called from stop_device.
     * This is very unlikely but can happen and is supported.
     */
    hw_rfkill = iwl_is_rfkill_set(trans);
    if (hw_rfkill) {
        set_bit(STATUS_RFKILL_HW, &trans->status);
        set_bit(STATUS_RFKILL_OPMODE, &trans->status);
    } else {
        clear_bit(STATUS_RFKILL_HW, &trans->status);
        clear_bit(STATUS_RFKILL_OPMODE, &trans->status);
    }
    if (hw_rfkill != was_in_rfkill)
        iwl_trans_pcie_rf_kill(trans, hw_rfkill);
}

void IntelWifi::iwl_pcie_set_interrupt_capa(/*struct pci_dev *pdev,*/
                                        struct iwl_trans *trans)
{
    // I haven't found any way to enable MSI-X in OS X, so this method is basically not needed
    // MSI is enabled in start method of this class.
    
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    trans_pcie->msix_enabled = false;
//    int max_irqs, num_irqs, i, ret, nr_online_cpus;
//    u16 pci_cmd;
//
//    if (!trans->cfg->mq_rx_supported)
//        goto enable_msi;
//
//    nr_online_cpus = num_online_cpus();
//    max_irqs = min_t(u32, nr_online_cpus + 2, IWL_MAX_RX_HW_QUEUES);
//    for (i = 0; i < max_irqs; i++)
//        trans_pcie->msix_entries[i].entry = i;
//
//    num_irqs = pci_enable_msix_range(pdev, trans_pcie->msix_entries,
//                                     MSIX_MIN_INTERRUPT_VECTORS,
//                                     max_irqs);
//    if (num_irqs < 0) {
//        IWL_DEBUG_INFO(trans,
//                       "Failed to enable msi-x mode (ret %d). Moving to msi mode.\n",
//                       num_irqs);
//        goto enable_msi;
//    }
//    trans_pcie->def_irq = (num_irqs == max_irqs) ? num_irqs - 1 : 0;
//
//    IWL_DEBUG_INFO(trans,
//                   "MSI-X enabled. %d interrupt vectors were allocated\n",
//                   num_irqs);
//
//    /*
//     * In case the OS provides fewer interrupts than requested, different
//     * causes will share the same interrupt vector as follows:
//     * One interrupt less: non rx causes shared with FBQ.
//     * Two interrupts less: non rx causes shared with FBQ and RSS.
//     * More than two interrupts: we will use fewer RSS queues.
//     */
//    if (num_irqs <= nr_online_cpus) {
//        trans_pcie->trans->num_rx_queues = num_irqs + 1;
//        trans_pcie->shared_vec_mask = IWL_SHARED_IRQ_NON_RX |
//        IWL_SHARED_IRQ_FIRST_RSS;
//    } else if (num_irqs == nr_online_cpus + 1) {
//        trans_pcie->trans->num_rx_queues = num_irqs;
//        trans_pcie->shared_vec_mask = IWL_SHARED_IRQ_NON_RX;
//    } else {
//        trans_pcie->trans->num_rx_queues = num_irqs - 1;
//    }
//
//    trans_pcie->alloc_vecs = num_irqs;
//    trans_pcie->msix_enabled = true;
//    return;
//
//enable_msi:
//    ret = pci_enable_msi(pdev);
//    if (ret) {
//        dev_err(&pdev->dev, "pci_enable_msi failed - %d\n", ret);
//        /* enable rfkill interrupt: hw bug w/a */
//        pci_read_config_word(pdev, PCI_COMMAND, &pci_cmd);
//        if (pci_cmd & PCI_COMMAND_INTX_DISABLE) {
//            pci_cmd &= ~PCI_COMMAND_INTX_DISABLE;
//            pci_write_config_word(pdev, PCI_COMMAND, pci_cmd);
//        }
//    }
}

void IntelWifi::iwl_pcie_conf_msix_hw(struct iwl_trans_pcie *trans_pcie)
{
    struct iwl_trans *trans = trans_pcie->trans;
    
    if (!trans_pcie->msix_enabled) {
        if (trans->cfg->mq_rx_supported &&
            test_bit(STATUS_DEVICE_ENABLED, &trans->status))
            io->iwl_write_prph(UREG_CHICK,
                           UREG_CHICK_MSI_ENABLE);
        return;
    }
    /*
     * The IVAR table needs to be configured again after reset,
     * but if the device is disabled, we can't write to
     * prph.
     */
    if (test_bit(STATUS_DEVICE_ENABLED, &trans->status))
        io->iwl_write_prph(UREG_CHICK, UREG_CHICK_MSIX_ENABLE);
    
    /*
     * Each cause from the causes list above and the RX causes is
     * represented as a byte in the IVAR table. The first nibble
     * represents the bound interrupt vector of the cause, the second
     * represents no auto clear for this cause. This will be set if its
     * interrupt vector is bound to serve other causes.
     */
    iwl_pcie_map_rx_causes(trans);

    iwl_pcie_map_non_rx_causes(trans);
}

struct iwl_causes_list {
    u32 cause_num;
    u32 mask_reg;
    u8 addr;
};

static struct iwl_causes_list causes_list[] = {
    {MSIX_FH_INT_CAUSES_D2S_CH0_NUM,    CSR_MSIX_FH_INT_MASK_AD, 0},
    {MSIX_FH_INT_CAUSES_D2S_CH1_NUM,    CSR_MSIX_FH_INT_MASK_AD, 0x1},
    {MSIX_FH_INT_CAUSES_S2D,        CSR_MSIX_FH_INT_MASK_AD, 0x3},
    {MSIX_FH_INT_CAUSES_FH_ERR,        CSR_MSIX_FH_INT_MASK_AD, 0x5},
    {MSIX_HW_INT_CAUSES_REG_ALIVE,        CSR_MSIX_HW_INT_MASK_AD, 0x10},
    {MSIX_HW_INT_CAUSES_REG_WAKEUP,        CSR_MSIX_HW_INT_MASK_AD, 0x11},
    {MSIX_HW_INT_CAUSES_REG_CT_KILL,    CSR_MSIX_HW_INT_MASK_AD, 0x16},
    {MSIX_HW_INT_CAUSES_REG_RF_KILL,    CSR_MSIX_HW_INT_MASK_AD, 0x17},
    {MSIX_HW_INT_CAUSES_REG_PERIODIC,    CSR_MSIX_HW_INT_MASK_AD, 0x18},
    {MSIX_HW_INT_CAUSES_REG_SW_ERR,        CSR_MSIX_HW_INT_MASK_AD, 0x29},
    {MSIX_HW_INT_CAUSES_REG_SCD,        CSR_MSIX_HW_INT_MASK_AD, 0x2A},
    {MSIX_HW_INT_CAUSES_REG_FH_TX,        CSR_MSIX_HW_INT_MASK_AD, 0x2B},
    {MSIX_HW_INT_CAUSES_REG_HW_ERR,        CSR_MSIX_HW_INT_MASK_AD, 0x2D},
    {MSIX_HW_INT_CAUSES_REG_HAP,        CSR_MSIX_HW_INT_MASK_AD, 0x2E},
};

void IntelWifi::iwl_pcie_map_non_rx_causes(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie =  IWL_TRANS_GET_PCIE_TRANS(trans);
    int val = trans_pcie->def_irq | MSIX_NON_AUTO_CLEAR_CAUSE;
    int i;
    
    /*
     * Access all non RX causes and map them to the default irq.
     * In case we are missing at least one interrupt vector,
     * the first interrupt vector will serve non-RX and FBQ causes.
     */
    for (i = 0; i < ARRAY_SIZE(causes_list); i++) {
        io->iwl_write8(CSR_MSIX_IVAR(causes_list[i].addr), val);
        io->iwl_clear_bit(causes_list[i].mask_reg,
                      causes_list[i].cause_num);
    }
}

void IntelWifi::iwl_pcie_map_rx_causes(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    u32 offset =
    trans_pcie->shared_vec_mask & IWL_SHARED_IRQ_FIRST_RSS ? 1 : 0;
    u32 val, idx;
    
    /*
     * The first RX queue - fallback queue, which is designated for
     * management frame, command responses etc, is always mapped to the
     * first interrupt vector. The other RX queues are mapped to
     * the other (N - 2) interrupt vectors.
     */
    val = BIT(MSIX_FH_INT_CAUSES_Q(0));
    for (idx = 1; idx < trans->num_rx_queues; idx++) {
        io->iwl_write8(CSR_MSIX_RX_IVAR(idx),
                   MSIX_FH_INT_CAUSES_Q(idx - offset));
        val |= BIT(MSIX_FH_INT_CAUSES_Q(idx));
    }
    io->iwl_write32(CSR_MSIX_FH_INT_MASK_AD, ~val);
    
    val = MSIX_FH_INT_CAUSES_Q(0);
    if (trans_pcie->shared_vec_mask & IWL_SHARED_IRQ_NON_RX)
        val |= MSIX_NON_AUTO_CLEAR_CAUSE;
    io->iwl_write8(CSR_MSIX_RX_IVAR(0), val);
    
    if (trans_pcie->shared_vec_mask & IWL_SHARED_IRQ_FIRST_RSS)
        io->iwl_write8(CSR_MSIX_RX_IVAR(1), val);
}

void IntelWifi::iwl_pcie_init_msix(struct iwl_trans_pcie *trans_pcie)
{
    //struct iwl_trans *trans = trans_pcie->trans;
    
    iwl_pcie_conf_msix_hw(trans_pcie);
    
    if (!trans_pcie->msix_enabled)
        return;
    
    trans_pcie->fh_init_mask = ~io->iwl_read32(CSR_MSIX_FH_INT_MASK_AD);
    trans_pcie->fh_mask = trans_pcie->fh_init_mask;
    trans_pcie->hw_init_mask = ~io->iwl_read32(CSR_MSIX_HW_INT_MASK_AD);
    trans_pcie->hw_mask = trans_pcie->hw_init_mask;
}

int IntelWifi::iwl_pcie_load_cpu_sections_8000(struct iwl_trans *trans,
                                           const struct fw_img *image,
                                           int cpu,
                                           int *first_ucode_section)
{
    int shift_param;
    int i, ret = 0, sec_num = 0x1;
    u32 val, last_read_idx = 0;
    
    if (cpu == 1) {
        shift_param = 0;
        *first_ucode_section = 0;
    } else {
        shift_param = 16;
        (*first_ucode_section)++;
    }
    
    for (i = *first_ucode_section; i < image->num_sec; i++) {
        last_read_idx = i;
        
        /*
         * CPU1_CPU2_SEPARATOR_SECTION delimiter - separate between
         * CPU1 to CPU2.
         * PAGING_SEPARATOR_SECTION delimiter - separate between
         * CPU2 non paged to CPU2 paging sec.
         */
        if (!image->sec[i].data ||
            image->sec[i].offset == CPU1_CPU2_SEPARATOR_SECTION ||
            image->sec[i].offset == PAGING_SEPARATOR_SECTION) {
            IWL_DEBUG_FW(trans,
                         "Break since Data not valid or Empty section, sec = %d\n",
                         i);
            break;
        }
        
        ret = iwl_pcie_load_section(trans, i, &image->sec[i]);
        if (ret)
            return ret;
        
        /* Notify ucode of loaded section number and status */
        val = io->iwl_read_direct32(FH_UCODE_LOAD_STATUS);
        val = val | (sec_num << shift_param);
        io->iwl_write_direct32(FH_UCODE_LOAD_STATUS, val);
        
        sec_num = (sec_num << 1) | 0x1;
    }
    
    *first_ucode_section = last_read_idx;
    
    iwl_enable_interrupts(trans);
    
    if (trans->cfg->use_tfh) {
        if (cpu == 1)
            io->iwl_write_prph(UREG_UCODE_LOAD_STATUS,
                           0xFFFF);
        else
            io->iwl_write_prph(UREG_UCODE_LOAD_STATUS,
                           0xFFFFFFFF);
    } else {
        if (cpu == 1)
            io->iwl_write_direct32(FH_UCODE_LOAD_STATUS,
                               0xFFFF);
        else
            io->iwl_write_direct32(FH_UCODE_LOAD_STATUS,
                               0xFFFFFFFF);
    }
    
    return 0;
}

// line 1312
void IntelWifi::iwl_trans_pcie_fw_alive(struct iwl_trans *trans, u32 scd_addr)
{
    iwl_pcie_reset_ict(trans);
    iwl_pcie_tx_start(trans, scd_addr);
}


// line 1687
void IntelWifi::iwl_trans_pcie_op_mode_leave(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    IOLockLock(trans_pcie->mutex);
    
    /* disable interrupts - don't enable HW RF kill interrupt */
    iwl_disable_interrupts(trans);
    
    iwl_pcie_apm_stop(trans, true);
    
    iwl_disable_interrupts(trans);
    
    iwl_pcie_disable_ict(trans);
    
    IOLockUnlock(trans_pcie->mutex);
    
    // TODO: Implement
    //iwl_pcie_synchronize_irqs(trans);
}

// line 1737
void IntelWifi::iwl_trans_pcie_configure(struct iwl_trans *trans,
                                     const struct iwl_trans_config *trans_cfg)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    trans_pcie->cmd_queue = trans_cfg->cmd_queue;
    trans_pcie->cmd_fifo = trans_cfg->cmd_fifo;
    trans_pcie->cmd_q_wdg_timeout = trans_cfg->cmd_q_wdg_timeout;
    if (WARN_ON(trans_cfg->n_no_reclaim_cmds > MAX_NO_RECLAIM_CMDS))
        trans_pcie->n_no_reclaim_cmds = 0;
    else
        trans_pcie->n_no_reclaim_cmds = trans_cfg->n_no_reclaim_cmds;
    if (trans_pcie->n_no_reclaim_cmds)
        memcpy(trans_pcie->no_reclaim_cmds, trans_cfg->no_reclaim_cmds,
               trans_pcie->n_no_reclaim_cmds * sizeof(u8));
    
    trans_pcie->rx_buf_size = trans_cfg->rx_buf_size;
    trans_pcie->rx_page_order =
    iwl_trans_get_rb_size_order(trans_pcie->rx_buf_size);
    
    trans_pcie->bc_table_dword = trans_cfg->bc_table_dword;
    trans_pcie->scd_set_active = trans_cfg->scd_set_active;
    trans_pcie->sw_csum_tx = trans_cfg->sw_csum_tx;
    
    trans_pcie->page_offs = trans_cfg->cb_data_offs;
    trans_pcie->dev_cmd_offs = trans_cfg->cb_data_offs + sizeof(void *);
    
    trans->command_groups = trans_cfg->command_groups;
    trans->command_groups_size = trans_cfg->command_groups_size;
    
    /* Initialize NAPI here - it should be before registering to mac80211
     * in the opmode but after the HW struct is allocated.
     * As this function may be called again in some corner cases don't
     * do anything if NAPI was already initialized.
     */
//    if (trans_pcie->napi_dev.reg_state != NETREG_DUMMY)
//        init_dummy_netdev(&trans_pcie->napi_dev);
}








// SCD. TODO: Move to iwl-scd.h

void IntelWifi::iwl_scd_txq_set_chain(struct iwl_trans *trans,
                                         u16 txq_id)
{
    io->iwl_set_bits_prph(SCD_QUEUECHAIN_SEL, BIT(txq_id));
}

void IntelWifi::iwl_scd_txq_enable_agg(struct iwl_trans *trans,
                                          u16 txq_id)
{
    io->iwl_set_bits_prph(SCD_AGGR_SEL, BIT(txq_id));
}

void IntelWifi::iwl_scd_txq_disable_agg(struct iwl_trans *trans,
                                           u16 txq_id)
{
    io->iwl_clear_bits_prph(SCD_AGGR_SEL, BIT(txq_id));
}

void IntelWifi::iwl_scd_disable_agg(struct iwl_trans *trans)
{
    io->iwl_set_bits_prph(SCD_AGGR_SEL, 0);
}

void IntelWifi::iwl_scd_activate_fifos(struct iwl_trans *trans)
{
    io->iwl_write_prph(SCD_TXFACT, IWL_MASK(0, 7));
}

void IntelWifi::iwl_scd_deactivate_fifos(struct iwl_trans *trans)
{
    io->iwl_write_prph(SCD_TXFACT, 0);
}

void IntelWifi::iwl_scd_enable_set_active(struct iwl_trans *trans,
                                             u32 value)
{
    io->iwl_write_prph(SCD_EN_CTRL, value);
}

unsigned int IntelWifi::SCD_QUEUE_WRPTR(unsigned int chnl)
{
    if (chnl < 20)
        return SCD_BASE + 0x18 + chnl * 4;
    //WARN_ON_ONCE(chnl >= 32);
    return SCD_BASE + 0x284 + (chnl - 20) * 4;
}

unsigned int IntelWifi::SCD_QUEUE_RDPTR(unsigned int chnl)
{
    if (chnl < 20)
        return SCD_BASE + 0x68 + chnl * 4;
    //WARN_ON_ONCE(chnl >= 32);
    return SCD_BASE + 0x2B4 + chnl * 4;
}

unsigned int IntelWifi::SCD_QUEUE_STATUS_BITS(unsigned int chnl)
{
    if (chnl < 20)
        return SCD_BASE + 0x10c + chnl * 4;
    //WARN_ON_ONCE(chnl >= 32);
    return SCD_BASE + 0x334 + chnl * 4;
}

void IntelWifi::iwl_scd_txq_set_inactive(struct iwl_trans *trans,
                                            u16 txq_id)
{
    io->iwl_write_prph(SCD_QUEUE_STATUS_BITS(txq_id),
                   (0 << SCD_QUEUE_STTS_REG_POS_ACTIVE)|
                   (1 << SCD_QUEUE_STTS_REG_POS_SCD_ACT_EN));
}

void IntelWifi::iwl_pcie_set_pwr(struct iwl_trans *trans, bool vaux)
{
    if (trans->cfg->apmg_not_supported)
        return;
    
    if (vaux /*&& pci_pme_capable(to_pci_dev(trans->dev), PCI_D3cold)*/)
        io->iwl_set_bits_mask_prph(APMG_PS_CTRL_REG,
                               APMG_PS_CTRL_VAL_PWR_SRC_VAUX,
                               ~APMG_PS_CTRL_MSK_PWR_SRC);
    else
        io->iwl_set_bits_mask_prph(APMG_PS_CTRL_REG,
                               APMG_PS_CTRL_VAL_PWR_SRC_VMAIN,
                               ~APMG_PS_CTRL_MSK_PWR_SRC);
}






