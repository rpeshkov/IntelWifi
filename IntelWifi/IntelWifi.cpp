/* add your code here */

#include "IntelWifi.hpp"

#include "iwlwifi/iwl-csr.h"

#include <sys/errno.h>


#define super IOEthernetController

OSDefineMetaClassAndStructors(IntelWifi, IOEthernetController)

#define    RELEASE(x)    if(x){(x)->release();(x)=NULL;}

#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_BYTES(x) (x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5]

#define HW_READY_TIMEOUT (50)


enum {
    kOffPowerState,
    kOnPowerState,
    //
    kNumPowerStates
};
static IOPMPowerState gPowerStates[kNumPowerStates] = {
    // kOffPowerState
    {kIOPMPowerStateVersion1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // kOnPowerState
    {kIOPMPowerStateVersion1, (kIOPMPowerOn | kIOPMDeviceUsable), kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
    };


static struct MediumTable
{
    IOMediumType type;
    UInt32 speed;
} mediumTable[] = {
    {kIOMediumIEEE80211None, 0},
    {kIOMediumIEEE80211Auto, 0}
};

bool IntelWifi::init(OSDictionary *properties) {
    TraceLog("init()\n");
    
    return super::init(properties);
}



void IntelWifi::free() {
    TraceLog("free()\n");
    
    if (eeprom) {
        eeprom->release();
        eeprom = NULL;
    }
    
    super::free();
}



bool IntelWifi::start(IOService *provider) {
    TraceLog("Start");
    
    if (!super::start(provider)) {
        TraceLog("Super start call failed!");
        return false;
    }
    
    PMinit();
    provider->joinPMtree(this);
    
    
    pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!pciDevice) {
        TraceLog("Provider is not PCI device");
        return false;
    }
    pciDevice->retain();

    //    pci_set_master(pdev);
    pciDevice->setBusMasterEnable(true);
    pciDevice->setMemoryEnable(true);
    
    
    

    
    UInt16 vendorId = pciDevice->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceId = pciDevice->configRead16(kIOPCIConfigDeviceID);
    UInt16 subsystemId = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    DebugLog("Device loaded. Vendor: %#06x, Device: %#06x, SubSystem: %#06x", vendorId, deviceId, subsystemId);
    
    
    fMemoryMap = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!fMemoryMap) {
        TraceLog("MemoryMap failed!");
        RELEASE(pciDevice);
        return false;
    }
    
    pciDevice->configWrite8(0x41, 0);
        
    volatile void *baseAddr = reinterpret_cast<volatile void *>(fMemoryMap->getVirtualAddress());
    
    io = IntelIO::withAddress(baseAddr);
    
    int err = iwl_pcie_prepare_card_hw();
    if (err) {
        TraceLog("ERROR: Error while preparing HW: %d", err);
        RELEASE(io);
        RELEASE(pciDevice);
        
        return false;
    }
    
    iwl_pcie_sw_reset();
    err = iwl_pcie_apm_init();
    if (err) {
        TraceLog("ERROR: PCIE APM Init error: %d", err);
        RELEASE(io);
        RELEASE(pciDevice);
        
        return false;
    }
    
    makeUsable();
//    changePowerStateTo(kOffPowerState);// Set the public power state to the lowest level
    registerPowerDriver(this, gPowerStates, kNumPowerStates);
    
        
    eeprom = IntelEeprom::withIO(io);
    if (!eeprom) {
        TraceLog("EEPROM init failed!");
        RELEASE(fMemoryMap);
        RELEASE(pciDevice);
        
        return false;
    }
    
    fNvmData = eeprom->parse();
    if (!fNvmData) {
        TraceLog("EEPROM parse failed!");
        RELEASE(eeprom);
        RELEASE(io);
        RELEASE(fMemoryMap);
        RELEASE(pciDevice);
        
        return false;
    }
    
    DebugLog("MAC: " MAC_FMT "\n"
             "Num addr: %d\n"
             "Calib: version - %d, voltage - %d\n"
             "Raw temperature: %u",
             
             MAC_BYTES(fNvmData->hw_addr),
             fNvmData->n_hw_addrs,
             fNvmData->calib_version, fNvmData->calib_voltage,
             fNvmData->raw_temperature);
    
    // original: trans->hw_rev = iwl_read32(trans, CSR_HW_REV);
    UInt32 hardwareRevisionId = io->iwl_read32(CSR_HW_REV);
    DebugLog("Hardware revision ID: %#010x", hardwareRevisionId);
    
    if (!createMediumDict()) {
        TraceLog("MediumDict creation failed!");
        RELEASE(eeprom);
        RELEASE(io);
        RELEASE(fMemoryMap);
        RELEASE(pciDevice);
        return false;
    }
    
    fWorkLoop = getWorkLoop();
    if (!fWorkLoop) {
        TraceLog("getWorkLoop failed!");
        RELEASE(eeprom);
        RELEASE(io);
        RELEASE(fMemoryMap);
        RELEASE(pciDevice);
        return false;
    }
    
    fWorkLoop->retain();
    
    if (!attachInterface((IONetworkInterface**)&netif)) {
        TraceLog("Interface attach failed!");
        RELEASE(fWorkLoop);
        RELEASE(mediumDict);
        RELEASE(eeprom);
        RELEASE(io);
        RELEASE(fMemoryMap);
        RELEASE(pciDevice);
        return false;
    }
    
    netif->registerService();
    
    fInterruptSource = IOTimerEventSource::timerEventSource(this, interruptOccured);
    if (!fInterruptSource)
        return false;
    if (fWorkLoop->addEventSource(fInterruptSource) != kIOReturnSuccess)
        return false;
    
    fInterruptSource->enable();
    
    
    registerService();
    
    return true;
}



void IntelWifi::stop(IOService *provider) {
    if (netif) {
        detachInterface(netif);
        netif = NULL;
    }
    RELEASE(fInterruptSource);
    RELEASE(fWorkLoop);
    RELEASE(mediumDict);
    RELEASE(eeprom);
    RELEASE(io);
    RELEASE(fMemoryMap);
    RELEASE(pciDevice);
    
    PMstop();
    
    super::stop(provider);
    TraceLog("Stopped");
}



bool IntelWifi::createMediumDict() {
    UInt32 capacity = sizeof(mediumTable) / sizeof(struct MediumTable);
    
    mediumDict = OSDictionary::withCapacity(capacity);
    if (mediumDict == 0) {
        return false;
    }
    
    for (UInt32 i = 0; i < capacity; i++) {
        IONetworkMedium* medium = IONetworkMedium::medium(mediumTable[i].type, mediumTable[i].speed);
        if (medium) {
            IONetworkMedium::addMedium(mediumDict, medium);
            medium->release();
        }
    }
    
    if (!publishMediumDictionary(mediumDict)) {
        return false;
    }
    
    IONetworkMedium *m = IONetworkMedium::getMediumWithType(mediumDict, kIOMediumIEEE80211Auto);
    setSelectedMedium(m);
    return true;
}



IOReturn IntelWifi::enable(IONetworkInterface *netif) {
    
    TraceLog("enable");
    
    IOMediumType mediumType = kIOMediumIEEE80211Auto;
    IONetworkMedium *medium = IONetworkMedium::getMediumWithType(mediumDict, mediumType);
    
    setLinkStatus(kIONetworkLinkActive | kIONetworkLinkValid, medium);
    return kIOReturnSuccess;
}



IOReturn IntelWifi::disable(IONetworkInterface *netif) {
    TraceLog("disable");
    netif->flushInputQueue();
    return kIOReturnSuccess;
}



IOReturn IntelWifi::getHardwareAddress(IOEthernetAddress *addrP) {
    memcpy(addrP->bytes, fNvmData->hw_addr, 6);
    
    return kIOReturnSuccess;
}



IOReturn IntelWifi::setHardwareAddress(const IOEthernetAddress *addrP) { 
    return kIOReturnSuccess;
}



UInt32 IntelWifi::outputPacket(mbuf_t m, void *param) { 
    return 0;
}


bool IntelWifi::configureInterface(IONetworkInterface *netif) {
    TraceLog("Configure interface");
    if (!super::configureInterface(netif)) {
        return false;
    }
    
    IONetworkData *nd = netif->getNetworkData(kIONetworkStatsKey);
    if (!nd || !(fNetworkStats = (IONetworkStats*)nd->getBuffer())) {
        return false;
    }
    
    nd = netif->getNetworkData(kIOEthernetStatsKey);
    if (!nd || !(fEthernetStats = (IOEthernetStats*)nd->getBuffer())) {
        return false;
    }
    
    return true;
}

const OSString* IntelWifi::newVendorString() const {
    return OSString::withCString("Intel");
}


const OSString* IntelWifi::newModelString() const {
    return OSString::withCString("Centrino N-130");
}
void IntelWifi::interruptOccured(OSObject* owner, IOTimerEventSource*
                                                      sender) {
//    mbuf_t packet;
    DebugLog("Interrupt!");
    IntelWifi* me = (IntelWifi*)owner;
    
    if (!me)
    return;
}




// MARK: iwl trans.c copy paste

/* Note: returns standard 0/-ERROR code */
int IntelWifi::iwl_pcie_prepare_card_hw()
{
    int ret;
    int t = 0;
    int iter;
    
    DebugLog("iwl_trans_prepare_card_hw enter\n");
    
    ret = iwl_pcie_set_hw_ready();
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
            ret = iwl_pcie_set_hw_ready();
            if (ret >= 0)
                return 0;
            
            IODelay(1000);
            t += 200;
        } while (t < 150000);
        IOSleep(25);
    }
    
    DebugLog("ERROR: Couldn't prepare the card\n");
    
    return ret;
}

/* Note: returns poll_bit return value, which is >= 0 if success */
int IntelWifi::iwl_pcie_set_hw_ready()
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
    
    DebugLog("hardware%s ready\n", ret < 0 ? " not" : "");
    return ret;
}

void IntelWifi::iwl_pcie_sw_reset()
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
int IntelWifi::iwl_pcie_apm_init()
{
    int ret;
    
    IWL_DEBUG_INFO(trans, "Init card's basic functions\n");
    
    /*
     * Use "set_bit" below rather than "write", to preserve any hardware
     * bits already set by default after reset.
     */
    
    /* Disable L0S exit timer (platform NMI Work/Around) */
    // TODO: Currently working only on Centrino 130 and that's family 6000
    //if (trans->cfg->device_family < IWL_DEVICE_FAMILY_8000)
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
    
    iwl_pcie_apm_config();
    
    /* Configure analog phase-lock-loop before activating to D0A */
    // TODO: Return back here
//    if (trans->cfg->base_params->pll_cfg)
//        iwl_set_bit(trans, CSR_ANA_PLL_CFG, CSR50_ANA_PLL_CFG_VAL);
    
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
    ret = io->iwl_poll_bit(CSR_GP_CNTRL,
                       CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                       CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY, 25000);
    if (ret < 0) {
        IWL_ERR(trans, "Failed to init the card\n");
        return ret;
    }
    
#if DISABLED_CODE
    
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
        iwl_read_prph(trans, OSC_CLK);
        iwl_read_prph(trans, OSC_CLK);
        iwl_set_bits_prph(trans, OSC_CLK, OSC_CLK_FORCE_CONTROL);
        iwl_read_prph(trans, OSC_CLK);
        iwl_read_prph(trans, OSC_CLK);
    }
#endif
    
    /*
     * Enable DMA clock and wait for it to stabilize.
     *
     * Write to "CLK_EN_REG"; "1" bits enable clocks, while "0"
     * bits do not disable clocks.  This preserves any hardware
     * bits already set by default in "CLK_CTRL_REG" after reset.
     */
#if DISABLED_CODE
    if (!trans->cfg->apmg_not_supported) {
        iwl_write_prph(trans, APMG_CLK_EN_REG,
                       APMG_CLK_VAL_DMA_CLK_RQT);
        udelay(20);
        
        /* Disable L1-Active */
        iwl_set_bits_prph(trans, APMG_PCIDEV_STT_REG,
                          APMG_PCIDEV_STT_VAL_L1_ACT_DIS);
        
        /* Clear the interrupt in APMG if the NIC is in RFKILL */
        iwl_write_prph(trans, APMG_RTC_INT_STT_REG,
                       APMG_RTC_INT_STT_RFKILL);
    }
#endif
    
//    set_bit(STATUS_DEVICE_ENABLED, &trans->status);
    
    return 0;
}

void IntelWifi::iwl_pcie_apm_config()
{
    
    /*
     * HW bug W/A for instability in PCIe bus L0S->L1 transition.
     * Check if BIOS (or OS) enabled L1-ASPM on this device.
     * If so (likely), disable L0S, so device moves directly L0->L1;
     *    costs negligible amount of power savings.
     * If not (unlikely), enable L0S, so there is at least some
     *    power savings, even without L1.
     */
    
//    UInt8 offset = 0;
//    if (pciDevice->findPCICapability(kIOPCIPCIExpressCapability, &offset))
//    {
//        UInt16 lctl = pciDevice->configRead16(offset);
//        if (lctl & PCI_EXP_LNKCTL_ASPM_L1)
//            io->iwl_set_bit(CSR_GIO_REG, CSR_GIO_REG_VAL_L0S_ENABLED);
//        else
//            io->iwl_clear_bit(CSR_GIO_REG, CSR_GIO_REG_VAL_L0S_ENABLED);
////        trans->pm_support = !(lctl & PCI_EXP_LNKCTL_ASPM_L0S);
//
//        pcie_capability_read_word(trans_pcie->pci_dev, PCI_EXP_DEVCTL2, &cap);
//        trans->ltr_enabled = cap & PCI_EXP_DEVCTL2_LTR_EN;
//        IWL_DEBUG_POWER(trans, "L1 %sabled - LTR %sabled\n",
//                        (lctl & PCI_EXP_LNKCTL_ASPM_L1) ? "En" : "Dis",
//                        trans->ltr_enabled ? "En" : "Dis");
//    }
    
    
    

    
}





