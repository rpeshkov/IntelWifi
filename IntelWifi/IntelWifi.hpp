/* add your code here */

#include <IOKit/IOLib.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>

#include "Logging.h"

#include "IntelEeprom.hpp"

#include "iwl-eeprom-parse.h"
#include "iwl-trans-pcie.h"

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
    IOTimerEventSource *fInterruptSource;
    OSDictionary *mediumDict;
    
    IONetworkStats *fNetworkStats;
    IOEthernetStats *fEthernetStats;
    
    IntelIO* io;
    
    IntelEeprom* eeprom;
    
    IOMemoryMap *fMemoryMap;
    
    struct iwl_nvm_data *fNvmData;
    struct iwl_cfg* fConfiguration;
    struct iwl_trans* trans;
    struct iwl_trans_pcie* trans_pcie;
    
private:
    struct iwl_cfg* getConfiguration(UInt16 deviceId, UInt16 subSystemId);
    
    static void  interruptOccured(OSObject* owner, IOTimerEventSource* sender);
    
    // IWL stuff
    int iwl_pcie_prepare_card_hw();
    int iwl_pcie_set_hw_ready();
    void iwl_pcie_sw_reset();
    int iwl_pcie_apm_init();
    void iwl_pcie_apm_config();
    
    
};
