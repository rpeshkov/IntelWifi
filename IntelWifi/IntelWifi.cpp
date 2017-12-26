/* add your code here */

#include "IntelWifi.hpp"

#define super IOService

OSDefineMetaClassAndStructors(IntelWifi, IOService)

bool IntelWifi::init(OSDictionary *properties) {
    TraceLog("init()\n");
    return super::init(properties);
}



void IntelWifi::free() {
    TraceLog("free()\n");
    super::free();
}



bool IntelWifi::start(IOService *provider) {
    TraceLog("Start");
    
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
    
    UInt16 vendorId = pciDevice->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceId = pciDevice->configRead16(kIOPCIConfigDeviceID);
    UInt16 subsystemId = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    
    DebugLog("Device loaded. Vendor: %#06x, Device: %#06x, SubSystem: %#06x", vendorId, deviceId, subsystemId);
    
    return super::start(provider);
}



void IntelWifi::stop(IOService *provider) {
    if (pciDevice) {
        pciDevice->release();
        pciDevice = NULL;
    }
    
    super::stop(provider);
    TraceLog("Stopped");
}

