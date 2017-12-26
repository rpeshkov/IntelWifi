/* add your code here */

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>

#include "Logging.h"


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
    // Allow our driver's Mac address to be set
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
    
protected:
    IOPCIDevice *pciDevice;
    IOEthernetInterface *netif;
    IOWorkLoop *workloop;
    OSDictionary *mediumDict;
    
    IONetworkStats *fNetworkStats;
    IOEthernetStats *fEthernetStats;
    
    UInt8 *eeprom;
    
private:
    int iwl_poll_bit(volatile void * base, UInt32 addr,
                     UInt32 bits, UInt32 mask, int timeout);
};
