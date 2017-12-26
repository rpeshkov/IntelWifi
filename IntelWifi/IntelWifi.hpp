/* add your code here */

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>

#include "Logging.h"


class IntelWifi : public IOService
{
    OSDeclareDefaultStructors(IntelWifi)
    
public:
    virtual bool init(OSDictionary *properties) override;
    virtual void free() override;
    
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    
protected:
    IOPCIDevice *pciDevice;
};
