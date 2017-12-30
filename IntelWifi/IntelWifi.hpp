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
    struct iwl_cfg* fConfiguration;
    struct iwl_trans* trans;
    struct iwl_trans_pcie* trans_pcie;
    
private:
    inline void releaseAll() {
        RELEASE(fInterruptSource);
        RELEASE(fWorkLoop);
        RELEASE(mediumDict);
        if (fNvmData) {
            IOFree(fNvmData, sizeof(struct iwl_nvm_data));
            fNvmData = NULL;
        }
        RELEASE(eeprom);
        RELEASE(io);
        RELEASE(fMemoryMap);
        if (trans_pcie) {
            iwl_trans_pcie_free(trans_pcie);
            trans_pcie = NULL;
        }
        if (trans) {
            IOFree(trans, sizeof(struct iwl_trans));
            trans = NULL;
        }
        RELEASE(pciDevice);
    }
    
    struct iwl_cfg* getConfiguration(UInt16 deviceId, UInt16 subSystemId);
    
    static void  interruptOccured(OSObject* owner, IOTimerEventSource* sender);
    
    // IWL stuff
    int iwl_pcie_prepare_card_hw();
    int iwl_pcie_set_hw_ready();
    void iwl_pcie_sw_reset();
    int iwl_pcie_apm_init();
    void iwl_pcie_apm_config();
    
    ifnet_t fIfNet;
    
    
};
