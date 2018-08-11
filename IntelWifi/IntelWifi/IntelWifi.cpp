#include "IntelWifi.hpp"

extern "C" {
#include "Configuration.h"
}

#include <IOKit/IOInterruptController.h>
#include <IOKit/IOCommandGate.h>
#include "IwlDvmOpMode.hpp"

#include "IO80211WorkLoop.h"

#include "IwlTransOps.h"

#define super IO80211Controller
OSDefineMetaClassAndStructors(IntelWifi, IO80211Controller)

static struct MediumTable
{
    IOMediumType type;
    UInt32 speed;
} mediumTable[] = {
    {kIOMediumIEEE80211None, 0},
    {kIOMediumIEEE80211Auto, 0}
};


bool IntelWifi::init(OSDictionary *properties) {
    TraceLog("Driver init()");
    return super::init(properties);
}

void IntelWifi::free() {
    TraceLog("Driver free()");
    releaseAll();
    TraceLog("Fully finished");
    super::free();
}

IOService* IntelWifi::probe(IOService* provider, SInt32 *score) {
    super::probe(provider, score);
    
    TraceLog("Probing dev");
    
    pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!pciDevice) {
        TraceLog("Provider is not PCI device");
        return NULL;
    }

    fDeviceId = pciDevice->configRead16(kIOPCIConfigDeviceID);
    fSubsystemId = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    
    TraceLog("Probing: 0x%04x", fDeviceId);
    
    fConfiguration = getConfiguration(fDeviceId, fSubsystemId);
    if (!fConfiguration) {
        TraceLog("ERROR: Failed to match configuration!");
        pciDevice = NULL;
        return NULL;
    }
    pciDevice->retain();
    
    TraceLog("Probe success for 0x%04x", fDeviceId);
    
    return this;
}

SInt32 IntelWifi::apple80211Request(UInt32 request_type, int request_number,
                                    IO80211Interface* interface, void* data) {
    if (request_type != SIOCGA80211 && request_type != SIOCSA80211) {
        TraceLog("Invalid IOCTL request type: %u", request_type);
        return kIOReturnError;
    }
    IOReturn ret = kIOReturnError;
    
    bool isGet = (request_type == SIOCGA80211);
    TraceLog("IOCTL %s(%d)", isGet ? "get" : "set", request_number);
    
#define IOCTL_GET(REQ_TYPE, REQ, DATA_TYPE) \
if (REQ_TYPE == SIOCGA80211) { \
ret = opmode->get##REQ(interface, (struct DATA_TYPE* )data); \
}
#define IOCTL_SET(REQ_TYPE, REQ, DATA_TYPE) \
if (REQ_TYPE == SIOCSA80211) { \
ret = opmode->set##REQ(interface, (struct DATA_TYPE* )data); \
}

    switch (request_number) {
        case APPLE80211_IOC_CARD_CAPABILITIES: // 12
            IOCTL_GET(request_type, CARD_CAPABILITIES, apple80211_capability_data);
            break;
        case APPLE80211_IOC_PHY_MODE: // 14
            IOCTL_GET(request_type, PHY_MODE, apple80211_phymode_data);
        case APPLE80211_IOC_POWER: // 19
            IOCTL_GET(request_type, POWER, apple80211_power_data);
            IOCTL_SET(request_type, POWER, apple80211_power_data);
            break;
    }
    
    
    
#undef IOCTL_SET
#undef IOCTL_GET
    
    return ret;
}

bool IntelWifi::createWorkLoop() {
    if (!fWorkLoop) {
        fWorkLoop = IO80211WorkLoop::workLoop();
    }
    
    return (fWorkLoop != NULL);
}

IOWorkLoop* IntelWifi::getWorkLoop() const {
    return fWorkLoop;
}

bool IntelWifi::start(IOService *provider) {
    TraceLog("Driver start");
    
    if (!super::start(provider)) {
        TraceLog("Super start call failed!");
        releaseAll();
        return false;
    }
    
    int source = findMSIInterruptTypeIndex();
    fIrqLoop = IO80211WorkLoop::workLoop();
    fInterruptSource = IOFilterInterruptEventSource::filterInterruptEventSource(this,
                                                                                (IOInterruptEventAction) &IntelWifi::interruptOccured,
                                                                                (IOFilterInterruptAction) &IntelWifi::interruptFilter,
                                                                                pciDevice, source);
    if (!fInterruptSource) {
        TraceLog("InterruptSource init failed!");
        releaseAll();
        return 0;
    }
    
    if (fIrqLoop->addEventSource(fInterruptSource) != kIOReturnSuccess) {
        TraceLog("EventSource registration failed");
        releaseAll();
        return 0;
    }
    
    fInterruptSource->enable();
    
    gate = IOCommandGate::commandGate(this, (IOCommandGate::Action)&IntelWifi::gateAction);
    
    if (fWorkLoop->addEventSource(gate) != kIOReturnSuccess) {
        TraceLog("EventSource registration failed");
        releaseAll();
        return 0;
    }
    gate->enable();
    
    fTrans = iwl_trans_pcie_alloc(fConfiguration);
    if (!fTrans) {
        TraceLog("iwl_trans_pcie_alloc failed");
        releaseAll();
        return false;
    }
    fTrans->dev = this;
    fTrans->gate = gate;
    
    
#ifdef CONFIG_IWLMVM
    const struct iwl_cfg *cfg_7265d = NULL;

    /*
     * special-case 7265D, it has the same PCI IDs.
     *
     * Note that because we already pass the cfg to the transport above,
     * all the parameters that the transport uses must, until that is
     * changed, be identical to the ones in the 7265D configuration.
     */
    if (fConfiguration == &iwl7265_2ac_cfg)
        cfg_7265d = &iwl7265d_2ac_cfg;
    else if (fConfiguration == &iwl7265_2n_cfg)
        cfg_7265d = &iwl7265d_2n_cfg;
    else if (fConfiguration == &iwl7265_n_cfg)
        cfg_7265d = &iwl7265d_n_cfg;
    if (cfg_7265d &&
        (fTrans->hw_rev & CSR_HW_REV_TYPE_MSK) == CSR_HW_REV_TYPE_7265D) {
        fConfiguration = cfg_7265d;
        fTrans->cfg = cfg_7265d;
    }
    
    if (fTrans->cfg->rf_id && fConfiguration == &iwla000_2ac_cfg_hr_cdb &&
        fTrans->hw_rev != CSR_HW_REV_TYPE_HR_CDB) {
        u32 rf_id_chp = CSR_HW_RF_ID_TYPE_CHIP_ID(fTrans->hw_rf_id);
        u32 jf_chp_id = CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_JF);
        u32 hr_chp_id = CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HR);
        
        if (rf_id_chp == jf_chp_id) {
            if (fTrans->hw_rev == CSR_HW_REV_TYPE_QNJ)
                fConfiguration = &iwla000_2ax_cfg_qnj_jf_b0;
            else
                fConfiguration = &iwla000_2ac_cfg_jf;
        } else if (rf_id_chp == hr_chp_id) {
            if (fTrans->hw_rev == CSR_HW_REV_TYPE_QNJ)
                fConfiguration = &iwla000_2ax_cfg_qnj_hr_a0;
            else
                fConfiguration = &iwla000_2ac_cfg_hr;
        }
        fTrans->cfg = fConfiguration;
    }
#endif

    fTrans->drv = iwl_drv_start(fTrans);
    
    if (!fTrans->drv) {
        TraceLog("DRV init failed!");
        releaseAll();
        return false;
    }

    
    /* if RTPM is in use, enable it in our device */
    if (fTrans->runtime_pm_mode != IWL_PLAT_PM_MODE_DISABLED) {
        /* We explicitly set the device to active here to
         * clear contingent errors.
         */
        PMinit();
        provider->joinPMtree(this);
        
        changePowerStateTo(kOffPowerState);// Set the public power state to the lowest level
        registerPowerDriver(this, gPowerStates, kNumPowerStates);
        setIdleTimerPeriod(iwlwifi_mod_params.d0i3_timeout);
    }
    
    transOps = new IwlTransOps(this);
    opmode = new IwlDvmOpMode(transOps);
    hw = opmode->start(fTrans, fTrans->cfg, &fTrans->drv->fw);
    
    if (!hw) {
        TraceLog("ERROR: Error while preparing HW");
        releaseAll();
        return false;
    }
    
    if (!createMediumDict()) {
        TraceLog("MediumDict creation failed!");
        releaseAll();
        return false;
    }
    
    if (!attachInterface((IONetworkInterface**)&netif)) {
        TraceLog("Interface attach failed!");
        releaseAll();
        return false;
    }
    
    netif->registerService();

    registerService();
    
    return true;
}

void IntelWifi::stop(IOService *provider) {
    
    if (fWorkLoop) {
        if (fInterruptSource) {
            fInterruptSource->disable();
            fWorkLoop->removeEventSource(fInterruptSource);
        }
    }
    
    struct iwl_priv *priv = (struct iwl_priv *)hw->priv;

    opmode->stop(priv);
    iwl_drv_stop(fTrans->drv);
    iwl_trans_pcie_free(fTrans);
    fTrans = NULL;
    
    if (netif) {
        detachInterface(netif);
        netif = NULL;
    }
    
    PMstop();
    
    super::stop(provider);
    TraceLog("Stopped");
}


IOReturn IntelWifi::enable(IONetworkInterface *netif) {
    TraceLog("enable");
    
    IOMediumType mediumType = kIOMediumIEEE80211Auto;
    IONetworkMedium *medium = IONetworkMedium::getMediumWithType(mediumDict, mediumType);
    setLinkStatus(kIONetworkLinkActive | kIONetworkLinkValid, medium);
    fTrans->intf = netif;
    
    return kIOReturnSuccess;
}

IOReturn IntelWifi::disable(IONetworkInterface *netif) {
    TraceLog("disable");
    fTrans->intf = NULL;
    return kIOReturnSuccess;
}

IOReturn IntelWifi::getHardwareAddress(IOEthernetAddress *addrP) {
    memcpy(addrP->bytes, &hw->wiphy->addresses[0], ETHER_ADDR_LEN);
    return kIOReturnSuccess;
}

IOReturn IntelWifi::getHardwareAddressForInterface(IO80211Interface* netif, IOEthernetAddress* addr) {
    return getHardwareAddress(addr);
}

IOReturn IntelWifi::setPromiscuousMode(bool active) {
    return kIOReturnSuccess;
}

IOReturn IntelWifi::setMulticastMode(bool active) {
    return kIOReturnSuccess;
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

IOReturn IntelWifi::gateAction(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3) {
    if (!owner) {
        return kIOReturnSuccess;
    }
    
    return kIOReturnSuccess;
}

bool IntelWifi::interruptFilter(OSObject* owner, IOFilterInterruptEventSource * src) {
    IntelWifi* me = (IntelWifi*)owner;

    if (me == 0) {
        TraceLog("Interrupt filter");
        return false;
    }

    /* Disable (but don't clear!) interrupts here to avoid
     * back-to-back ISRs and sporadic interrupts from our NIC.
     * If we have something to service, the tasklet will re-enable ints.
     * If we *don't* have something, we'll re-enable before leaving here.
     */
    iwl_write32(me->fTrans, CSR_INT_MASK, 0x00000000);
    
    return true;
}

void IntelWifi::interruptOccured(OSObject* owner, IOInterruptEventSource* sender, int count) {
    IntelWifi* me = (IntelWifi*)owner;
    
    if (me == 0) {
        return;
    }
    
    me->iwl_pcie_irq_handler(0, me->fTrans);
}

IO80211Interface *IntelWifi::getNetworkInterface() {
    return netif;
}

const OSString* IntelWifi::newVendorString() const {
    return OSString::withCString("Intel");
}

const OSString* IntelWifi::newModelString() const {
    return OSString::withCString(fConfiguration->name);
}


// MARK: Private routines

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

/**
 * Search for Message Signaled Interrupts source
 */
int IntelWifi::findMSIInterruptTypeIndex() {
    IOReturn ret;
    int index, source = 0;
    for (index = 0; ; index++)
    {
        int interruptType;
        ret = pciDevice->getInterruptType(index, &interruptType);
        if (ret != kIOReturnSuccess)
            break;
        if (interruptType & kIOInterruptTypePCIMessaged)
        {
            source = index;
            break;
        }
    }
    return source;
}

/**
 * Release all internal fields
 */
void IntelWifi::releaseAll() {
    RELEASE(fInterruptSource);
    RELEASE(fWorkLoop);
    RELEASE(mediumDict);
    
    RELEASE(fMemoryMap);
    if (fTrans) {
        iwl_trans_pcie_free(fTrans);
        fTrans = NULL;
    }
    
    RELEASE(pciDevice);
}

