//
//  IntelWifi_trans.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 01/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include <IntelWifi.hpp>

/* PCI registers */
#define PCI_CFG_RETRY_TIMEOUT    0x041

#define HW_READY_TIMEOUT (50)

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
    
    // TODO: Implement
    //init_waitqueue_head(&trans_pcie->ucode_write_waitq);
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
    // original code: pci_write_config_byte(pdev, PCI_CFG_RETRY_TIMEOUT, 0x00);
    pciDevice->configWrite8(PCI_CFG_RETRY_TIMEOUT, 0);
    
    iwl_disable_interrupts(trans);
    
    // original: trans->hw_rev = iwl_read32(trans, CSR_HW_REV);
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
    
    // TODO: Implement
    // iwl_pcie_set_interrupt_capa(pdev, trans);
    trans->hw_id = (fDeviceId << 16) + fSubsystemId;
    snprintf(trans->hw_id_str, sizeof(trans->hw_id_str), "PCI ID: 0x%04X:0x%04X", fDeviceId, fSubsystemId);
    DebugLog("%s", trans->hw_id_str);
    
    
    /* Initialize the wait queue for commands */
    // TODO: Implement
    //    init_waitqueue_head(&trans_pcie->wait_command_queue);
    //    init_waitqueue_head(&trans_pcie->d0i3_waitq);
    
    // TODO: Implement
    // msix_enabled is set in iwl_pcie_set_interrupt_capa
    //    if (trans_pcie->msix_enabled) {
    //        ret = iwl_pcie_init_msix_handler(pdev, trans_pcie);
    //        if (ret)
    //            goto out_no_pci;
    //    } else {
    //        ret = iwl_pcie_alloc_ict(trans);
    //        if (ret)
    //            goto out_no_pci;
    //
    //        ret = devm_request_threaded_irq(&pdev->dev, pdev->irq,
    //                                        iwl_pcie_isr,
    //                                        iwl_pcie_irq_handler,
    //                                        IRQF_SHARED, DRV_NAME, trans);
    //        if (ret) {
    //            IWL_ERR(trans, "Error allocating IRQ %d\n", pdev->irq);
    //            goto out_free_ict;
    //        }
    //        trans_pcie->inta_mask = CSR_INI_SET_MASK;
    //    }
    //
    //    trans_pcie->rba.alloc_wq = alloc_workqueue("rb_allocator",
    //                                               WQ_HIGHPRI | WQ_UNBOUND, 1);
    //    INIT_WORK(&trans_pcie->rba.rx_alloc, iwl_pcie_rx_allocator_work);
    //
    
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
    
    //iwl_pcie_init_msix(trans_pcie);
    
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
    
    //spin_lock(&trans_pcie->irq_lock);
    IOSimpleLockLock(trans_pcie->irq_lock);
    _iwl_disable_interrupts(trans);
    //spin_unlock(&trans_pcie->irq_lock);
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
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
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
    // TODO: Implement msix
    //if (!trans_pcie->msix_enabled) {
    if (1 == 1) {
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
    
    //spin_lock(&trans_pcie->irq_lock);
    IOSimpleLockLock(trans_pcie->irq_lock);
    _iwl_enable_interrupts(trans);
    //spin_unlock(&trans_pcie->irq_lock);
    IOSimpleLockUnlock(trans_pcie->irq_lock);
}




