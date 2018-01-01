/* add your code here */

#include <IOKit/IOLib.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOFilterInterruptEventSource.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IOPacketQueue.h>

#include <net/if.h>

extern "C" {
#include "iwlwifi/iwl-csr.h"
#include "iwl-drv.h"
#include "iwl-trans.h"
#include "iwl-fh.h"
#include "iwl-prph.h"
#include "iwl-config.h"
#include "iwl-eeprom-parse.h"
#include "iwl-trans-pcie.h"
}


#include "Logging.h"

#include "IntelEeprom.hpp"




// Configuration
#define CONFIG_IWLMVM // Need NVM mode at least to see that code is compiling
#define CONFIG_IWLWIFI_PCIE_RTPM // Powerman









#define    RELEASE(x)    if(x){(x)->release();(x)=NULL;}

class IntelWifi : public IOEthernetController
{
    OSDeclareDefaultStructors(IntelWifi)
    
public:
    virtual bool init(OSDictionary *properties) override;
    virtual void free() override;
    
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    
    virtual bool configureInterface(IONetworkInterface* netif) override;
    
    virtual IOReturn enable(IONetworkInterface* netif) override;
    virtual IOReturn disable(IONetworkInterface* netif) override;
    virtual IOReturn getHardwareAddress(IOEthernetAddress* addrP) override;
    virtual IOReturn setHardwareAddress(const IOEthernetAddress* addrP) override;
    virtual UInt32 outputPacket(mbuf_t m, void* param) override;
    
    virtual IOReturn setPromiscuousMode(bool active) override
    {
        return kIOReturnSuccess;
    }
    
    virtual IOReturn setMulticastMode(bool active) override
    {
        return kIOReturnSuccess;
    }
    
    bool createMediumDict();
    
    virtual const OSString* newVendorString() const override;
    virtual const OSString* newModelString() const override;
    
protected:
    IOPCIDevice *pciDevice;
    IOEthernetInterface *netif;
    IOWorkLoop *fWorkLoop;
    OSDictionary *mediumDict;
    
    IONetworkStats *fNetworkStats;
    IOEthernetStats *fEthernetStats;
    IOInterruptEventSource* fInterruptSource;
    
    IntelIO* io;
    
    IntelEeprom* eeprom;
    
    IOMemoryMap *fMemoryMap;
    
    struct iwl_nvm_data *fNvmData;
    const struct iwl_cfg* fConfiguration;
    struct iwl_trans* fTrans;
    
    
private:
    inline void releaseAll() {
        RELEASE(fInterruptSource);
        RELEASE(fWorkLoop);
        RELEASE(mediumDict);
        RELEASE(eeprom);
        RELEASE(io);
        RELEASE(fMemoryMap);
        if (fTrans) {
            iwl_trans_pcie_free(fTrans);
            fTrans = NULL;
        }
        
        RELEASE(pciDevice);
    }
    
    struct iwl_cfg* getConfiguration(UInt16 deviceId, UInt16 subSystemId);
    
    static void  interruptOccured(OSObject* owner, IOTimerEventSource* sender);
    
    // IWL stuff
    int iwl_pcie_prepare_card_hw(struct iwl_trans *trans);
    int iwl_pcie_set_hw_ready(struct iwl_trans *trans);
    
    int iwl_pcie_apm_init(struct iwl_trans *trans);
    void iwl_pcie_apm_config(struct iwl_trans *trans);
    void iwl_pcie_apm_stop(struct iwl_trans *trans, bool op_mode_leave);
    void iwl_pcie_apm_stop_master(struct iwl_trans *trans);
    void iwl_trans_pcie_stop_device(struct iwl_trans *trans, bool low_power);
    void _iwl_trans_pcie_stop_device(struct iwl_trans *trans, bool low_power);
    
    struct iwl_trans* iwl_trans_pcie_alloc(const struct iwl_cfg *cfg);
    void iwl_trans_pcie_free(struct iwl_trans* trans);
    
    
    void _iwl_disable_interrupts(struct iwl_trans *trans);
    void iwl_disable_interrupts(struct iwl_trans *trans);
    
    // trans.c
    int _iwl_trans_pcie_start_hw(struct iwl_trans *trans, bool low_power);
    int iwl_trans_pcie_start_hw(struct iwl_trans *trans, bool low_power);
    void iwl_pcie_sw_reset(struct iwl_trans *trans);

    
    ifnet_t fIfNet;
    
    UInt16 fDeviceId;
    UInt16 fSubsystemId;
    
    
};

static inline struct iwl_trans_pcie *
IWL_TRANS_GET_PCIE_TRANS(struct iwl_trans *trans)
{
    return (struct iwl_trans_pcie *)trans->trans_specific;
}

