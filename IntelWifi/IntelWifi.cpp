/* add your code here */

#include "IntelWifi.hpp"

#include "iwlwifi/iwl-csr.h"

#define super IOEthernetController

OSDefineMetaClassAndStructors(IntelWifi, IOEthernetController)


struct iwl_nvm_data {
    int n_hw_addrs;
    UInt8 hw_addr[6];
    
    UInt8 calib_version;
    UInt16 calib_voltage;
    
    UInt16 raw_temperature;
    UInt16 kelvin_temperature;
    UInt16 kelvin_voltage;
    UInt16 xtal_calib[2];
    
    bool sku_cap_band_24GHz_enable;
    bool sku_cap_band_52GHz_enable;
    bool sku_cap_11n_enable;
    bool sku_cap_11ac_enable;
    bool sku_cap_amt_enable;
    bool sku_cap_ipan_enable;
    bool sku_cap_mimo_disabled;
    
    UInt16 radio_cfg_type;
    UInt8 radio_cfg_step;
    UInt8 radio_cfg_dash;
    UInt8 radio_cfg_pnum;
    UInt8 valid_tx_ant, valid_rx_ant;
    
    UInt32 nvm_version;
    int8_t max_tx_pwr_half_dbm;
    
    bool lar_enabled;
    bool vht160_supported;
    
};

#define EEPROM_MAC_ADDRESS                  (2*0x15)

#define OTP_LOW_IMAGE_SIZE        (2 * 512 * sizeof(UInt16)) /* 2 KB */


static struct MediumTable
{
    IOMediumType type;
    UInt32 speed;
}
mediumTable[] =


{
    {kIOMediumEthernetNone, 0},
    {kIOMediumEthernetAuto, 0},
    {kIOMediumEthernet10BaseT | kIOMediumOptionFullDuplex,  10},
    {kIOMediumEthernet100BaseTX | kIOMediumOptionFullDuplex, 100},
    {kIOMediumEthernet1000BaseT | kIOMediumOptionFullDuplex, 1000},
};

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
    
    pciDevice->setMemoryEnable(true);
    
    UInt16 vendorId = pciDevice->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceId = pciDevice->configRead16(kIOPCIConfigDeviceID);
    UInt16 subsystemId = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    
    UInt16 *e = (UInt16*)IOMalloc(OTP_LOW_IMAGE_SIZE / 2);
    
    IOMemoryMap *map = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (map) {
        
        volatile void *baseAddr = reinterpret_cast<volatile void *>(map->getVirtualAddress());
        
       
        for (UInt16 a = 0; a < OTP_LOW_IMAGE_SIZE; a += sizeof(UInt16)) {
            OSWriteLittleInt32(baseAddr, CSR_EEPROM_REG, CSR_EEPROM_REG_MSK_ADDR & (a << 1));
            
            int ret = iwl_poll_bit(baseAddr,
                         CSR_EEPROM_REG,
                         CSR_EEPROM_REG_READ_VALID_MSK,
                         CSR_EEPROM_REG_READ_VALID_MSK,
                         5000);
            
            
            
            if (ret < 0) {
                
                map->release();
                pciDevice->release();
                pciDevice = NULL;
                return false;
            }

            
            UInt32 r = OSReadLittleInt32(baseAddr, CSR_EEPROM_REG);

            e[a/2] = (r >> 16);
        }
        
        eeprom = (UInt8 *)e;

        map->release();
    }
    
    
    DebugLog("Device loaded. Vendor: %#06x, Device: %#06x, SubSystem: %#06x", vendorId, deviceId, subsystemId);
   
    
    if (!createMediumDict()) {
        TraceLog("MediumDict creation failed!");
        return false;
    }
    
    if (!attachInterface((IONetworkInterface**)&netif)) {
        TraceLog("Interface attach failed!");
        return false;
    }
    
    
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
    
    IONetworkMedium *m = IONetworkMedium::getMediumWithType(mediumDict, kIOMediumEthernetAuto);
    setSelectedMedium(m);
    return true;
}


IOReturn IntelWifi::enable(IONetworkInterface *netif) { 
    IOMediumType mediumType = kIOMediumEthernet1000BaseT | kIOMediumOptionFullDuplex;
    IONetworkMedium *medium = IONetworkMedium::getMediumWithType(mediumDict, mediumType);
    
    setLinkStatus(kIONetworkLinkActive | kIONetworkLinkValid, medium, 1000 * 1000000);
    return kIOReturnSuccess;
}

IOReturn IntelWifi::disable(IONetworkInterface *netif) { 
    netif->flushInputQueue();
    return kIOReturnSuccess;
}

IOReturn IntelWifi::getHardwareAddress(IOEthernetAddress *addrP) {
    memcpy(&addrP->bytes, eeprom + EEPROM_MAC_ADDRESS, 6);
    
    
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

int IntelWifi::iwl_poll_bit(volatile void * base, UInt32 addr,
                            UInt32 bits, UInt32 mask, int timeout) {
    int t = 0;
    do {
        if ((OSReadLittleInt32(base, addr) & mask) == (bits & mask)) {
            return t;
        }
        
        IODelay(10);
        t += 10;
    } while (t < timeout);
    
    return -110;
}


