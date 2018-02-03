/* add your code here */

#include "IntelWifi.hpp"

extern "C" {
#include "Configuration.h"
}

#include <IOKit/IOInterruptController.h>
#include <IOKit/IOCommandGate.h>
#include "IwlDvmOpMode.hpp"




#include <sys/errno.h>

#define super IOEthernetController
OSDefineMetaClassAndStructors(IntelWifi, IOEthernetController)




static struct MediumTable
{
    IOMediumType type;
    UInt32 speed;
} mediumTable[] = {
    {kIOMediumIEEE80211None, 0},
    {kIOMediumIEEE80211Auto, 0}
};


int IntelWifi::findMSIInterruptTypeIndex()
{
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

bool IntelWifi::start(IOService *provider) {
    TraceLog("Driver start");
    
    if (!super::start(provider)) {
        TraceLog("Super start call failed!");
        return false;
    }
    
    pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!pciDevice) {
        TraceLog("Provider is not PCI device");
        return false;
    }
    pciDevice->retain();
    
    fDeviceId = pciDevice->configRead16(kIOPCIConfigDeviceID);
    fSubsystemId = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    
    fConfiguration = getConfiguration(fDeviceId, fSubsystemId);
    
    if (!fConfiguration) {
        TraceLog("ERROR: Failed to match configuration!");
        releaseAll();
        return false;
    }
    
    fWorkLoop = getWorkLoop();
    if (!fWorkLoop) {
        TraceLog("getWorkLoop failed!");
        releaseAll();
        return false;
    }
    
    fWorkLoop->retain();
    
    int source = findMSIInterruptTypeIndex();
    fInterruptSource = IOFilterInterruptEventSource::filterInterruptEventSource(this,
                                                                                (IOInterruptEventAction) &IntelWifi::interruptOccured,
                                                                                (IOFilterInterruptAction) &IntelWifi::interruptFilter,
                                                                                pciDevice, source);
    if (!fInterruptSource) {
        TraceLog("InterruptSource init failed!");
        releaseAll();
        return 0;
    }
    
    if (fWorkLoop->addEventSource(fInterruptSource) != kIOReturnSuccess) {
        TraceLog("EventSource registration failed");
        releaseAll();
        return 0;
    }
    
    gate = IOCommandGate::commandGate(this, (IOCommandGate::Action)&IntelWifi::gateAction);
    
    if (fWorkLoop->addEventSource(gate) != kIOReturnSuccess) {
        TraceLog("EventSource registration failed");
        releaseAll();
        return 0;
    }
    gate->enable();
    
//    fInterruptSource->enable();
//    fWorkLoop->enableAllInterrupts();
//    fWorkLoop->enableAllEventSources();
    
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
    
    opmode = new IwlDvmOpMode(this);
    hw = opmode->start(fTrans, fTrans->cfg, &fTrans->drv->fw, NULL);
    
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
    fTrans->intf = netif;
    return kIOReturnSuccess;
}



IOReturn IntelWifi::disable(IONetworkInterface *netif) {
    TraceLog("disable");
    fTrans->intf = NULL;
//    netif->flushInputQueue();
    return kIOReturnSuccess;
}



IOReturn IntelWifi::getHardwareAddress(IOEthernetAddress *addrP) {
    memcpy(addrP->bytes, &hw->wiphy->addresses[0], ETH_ALEN);
    return kIOReturnSuccess;
}



IOReturn IntelWifi::setHardwareAddress(const IOEthernetAddress *addrP) { 
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

const OSString* IntelWifi::newVendorString() const {
    return OSString::withCString("Intel");
}


const OSString* IntelWifi::newModelString() const {
    return OSString::withCString(fConfiguration->name);
}

IOReturn IntelWifi::gateAction(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3) {
    if (!owner) {
        return kIOReturnSuccess;
    }
    
    //IntelWifi *me = static_cast<IntelWifi *>(owner);
//    u32 len = (u32)arg1;
//    return me->netif->inputPacket((mbuf_t)arg0, len, IONetworkInterface::kInputOptionQueuePacket);
    return kIOReturnSuccess;
}

bool IntelWifi::interruptFilter(OSObject* owner, IOFilterInterruptEventSource * src) {
    IntelWifi* me = (IntelWifi*)owner;
    
    if (me == 0) {
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

//IOReturn IntelWifi::outputStart(IONetworkInterface *interface, IOOptionBits options) {
//    DebugLog("OUTPUT START");
//    return kIOReturnSuccess;
//}

