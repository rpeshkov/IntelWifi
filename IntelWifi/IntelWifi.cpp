/* add your code here */

#include "IntelWifi.hpp"

#include "iwlwifi/iwl-csr.h"


#include <sys/errno.h>







#define super IOEthernetController

OSDefineMetaClassAndStructors(IntelWifi, IOEthernetController)

#define EEPROM_MAC_ADDRESS                  (2*0x15)

#define OTP_LOW_IMAGE_SIZE        (2 * 512 * sizeof(UInt16)) /* 2 KB */

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
    
    pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!pciDevice) {
        TraceLog("Provider is not PCI device");
        return false;
    }
    pciDevice->retain();
    
    pciDevice->setMemoryEnable(true);
    
    UInt16 vendorId = pciDevice->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceId = pciDevice->configRead16(kIOPCIConfigDeviceID);
    UInt16 subsystemId = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    
    fMemoryMap = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!fMemoryMap) {
        if (pciDevice) {
            pciDevice->release();
            pciDevice = NULL;
        }
        
        return false;
    }
    
        
    volatile void *baseAddr = reinterpret_cast<volatile void *>(fMemoryMap->getVirtualAddress());
        
    eeprom = IntelEeprom::withAddress(baseAddr);
    if (!eeprom) {
        if (fMemoryMap) {
            fMemoryMap->release();
            fMemoryMap = NULL;
        }
        
        if (pciDevice) {
            pciDevice->release();
            pciDevice = NULL;
        }
        
        return false;
    }
    fNvmData = eeprom->parse();
    if (!fNvmData) {
        if (eeprom) {
            eeprom->release();
            eeprom = NULL;
        }
        
        if (fMemoryMap) {
            fMemoryMap->release();
            fMemoryMap = NULL;
        }
        
        if (pciDevice) {
            pciDevice->release();
            pciDevice = NULL;
        }
        
        return false;
    }
    
    DebugLog("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n"
             "Num addr: %d\n"
             "Calib: version - %d, voltage - %d\n"
             "Raw temperature: %u",
             
             fNvmData->hw_addr[0],fNvmData->hw_addr[1],fNvmData->hw_addr[2],fNvmData->hw_addr[3],fNvmData->hw_addr[4],fNvmData->hw_addr[5],
             fNvmData->n_hw_addrs,
             fNvmData->calib_version, fNvmData->calib_voltage,
             fNvmData->raw_temperature);
     
    
    DebugLog("Device loaded. Vendor: %#06x, Device: %#06x, SubSystem: %#06x", vendorId, deviceId, subsystemId);
   
    
    if (!createMediumDict()) {
        TraceLog("MediumDict creation failed!");
        return false;
    }
    
    if (!attachInterface((IONetworkInterface**)&netif)) {
        TraceLog("Interface attach failed!");
        return false;
    }
    
    netif->registerService();
    
    
    
    return true;
}



void IntelWifi::stop(IOService *provider) {
    if (netif) {
        detachInterface(netif);
        netif = NULL;
    }
    
    if (mediumDict) {
        mediumDict->release();
        mediumDict = NULL;
    }
    
    if (pciDevice) {
        pciDevice->release();
        pciDevice = NULL;
    }
    
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
    
    IOMediumType mediumType = kIOMediumIEEE80211Auto;
    IONetworkMedium *medium = IONetworkMedium::getMediumWithType(mediumDict, mediumType);
    
    setLinkStatus(kIONetworkLinkActive | kIONetworkLinkValid, medium);
    return kIOReturnSuccess;
}



IOReturn IntelWifi::disable(IONetworkInterface *netif) { 
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
    const char    *model = "Centrino N-130";
    return OSString::withCString(model);
}


// MARK: iwl-io.c
int IntelWifi::iwl_poll_bit(volatile void * base, UInt32 addr, UInt32 bits, UInt32 mask, int timeout) {
    int t = 0;
    do {
        if ((OSReadLittleInt32(base, addr) & mask) == (bits & mask)) {
            return t;
        }
        
        IODelay(1);
        t += 1;
    } while (t < timeout);
    
    return -ETIMEDOUT;
}



