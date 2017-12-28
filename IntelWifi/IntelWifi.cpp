/* add your code here */

#include "IntelWifi.hpp"

#include "iwlwifi/iwl-csr.h"
//#include "iwl-op-mode.h"
//#include "iwl-prph.h"
//#include "iwl-fh.h"
//#include "iwl-modparams.h"
//#include "iwl-context-info.h"
//#include "iwl-trans.h"
//
//#include "iwlwifi/fw/file.h"
//#include "iwlwifi/fw/img.h"
//#include "iwlwifi/fw/error-dump.h"
//
//#include "iwlwifi/fw/api/cmdhdr.h"
//#include "iwlwifi/fw/api/txq.h"

extern "C" {
#include "iwl-drv.h"
#include "iwl-trans.h"

}


#include <sys/errno.h>

struct pci_device_id {
    __u32 vendor, device;        /* Vendor and device ID or PCI_ANY_ID*/
    __u32 subvendor, subdevice;    /* Subsystem ID's or PCI_ANY_ID */
    
    u8 * driver_data;    /* Data private to the driver */
};


#define IWL_PCI_DEVICE(dev, subdev, cfg) \
.vendor = 0x8086,  .device = (dev), \
.subvendor = 0xFFFF, .subdevice = (subdev), \
.driver_data = (u8 *)&(cfg)

/* Hardware specific file defines the PCI IDs table for that hardware module */
static const struct pci_device_id iwl_hw_card_ids[] = {

    {IWL_PCI_DEVICE(0x4232, 0x1201, iwl5100_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1301, iwl5100_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1204, iwl5100_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1304, iwl5100_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1205, iwl5100_bgn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1305, iwl5100_bgn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1206, iwl5100_abg_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1306, iwl5100_abg_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1221, iwl5100_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1321, iwl5100_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1224, iwl5100_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1324, iwl5100_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1225, iwl5100_bgn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1325, iwl5100_bgn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1226, iwl5100_abg_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4232, 0x1326, iwl5100_abg_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4237, 0x1211, iwl5100_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4237, 0x1311, iwl5100_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4237, 0x1214, iwl5100_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4237, 0x1314, iwl5100_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4237, 0x1215, iwl5100_bgn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4237, 0x1315, iwl5100_bgn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4237, 0x1216, iwl5100_abg_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4237, 0x1316, iwl5100_abg_cfg)}, /* Half Mini Card */
    
    /* 5300 Series WiFi */
    {IWL_PCI_DEVICE(0x4235, 0x1021, iwl5300_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4235, 0x1121, iwl5300_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4235, 0x1024, iwl5300_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4235, 0x1124, iwl5300_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4235, 0x1001, iwl5300_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4235, 0x1101, iwl5300_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4235, 0x1004, iwl5300_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4235, 0x1104, iwl5300_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4236, 0x1011, iwl5300_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4236, 0x1111, iwl5300_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x4236, 0x1014, iwl5300_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x4236, 0x1114, iwl5300_agn_cfg)}, /* Half Mini Card */
    
    /* 5350 Series WiFi/WiMax */
    {IWL_PCI_DEVICE(0x423A, 0x1001, iwl5350_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x423A, 0x1021, iwl5350_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x423B, 0x1011, iwl5350_agn_cfg)}, /* Mini Card */
    
    /* 5150 Series Wifi/WiMax */
    {IWL_PCI_DEVICE(0x423C, 0x1201, iwl5150_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x423C, 0x1301, iwl5150_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x423C, 0x1206, iwl5150_abg_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x423C, 0x1306, iwl5150_abg_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x423C, 0x1221, iwl5150_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x423C, 0x1321, iwl5150_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x423C, 0x1326, iwl5150_abg_cfg)}, /* Half Mini Card */
    
    {IWL_PCI_DEVICE(0x423D, 0x1211, iwl5150_agn_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x423D, 0x1311, iwl5150_agn_cfg)}, /* Half Mini Card */
    {IWL_PCI_DEVICE(0x423D, 0x1216, iwl5150_abg_cfg)}, /* Mini Card */
    {IWL_PCI_DEVICE(0x423D, 0x1316, iwl5150_abg_cfg)}, /* Half Mini Card */
    
    /* 6x00 Series */
    {IWL_PCI_DEVICE(0x422B, 0x1101, iwl6000_3agn_cfg)},
    {IWL_PCI_DEVICE(0x422B, 0x1108, iwl6000_3agn_cfg)},
    {IWL_PCI_DEVICE(0x422B, 0x1121, iwl6000_3agn_cfg)},
    {IWL_PCI_DEVICE(0x422B, 0x1128, iwl6000_3agn_cfg)},
    {IWL_PCI_DEVICE(0x422C, 0x1301, iwl6000i_2agn_cfg)},
    {IWL_PCI_DEVICE(0x422C, 0x1306, iwl6000i_2abg_cfg)},
    {IWL_PCI_DEVICE(0x422C, 0x1307, iwl6000i_2bg_cfg)},
    {IWL_PCI_DEVICE(0x422C, 0x1321, iwl6000i_2agn_cfg)},
    {IWL_PCI_DEVICE(0x422C, 0x1326, iwl6000i_2abg_cfg)},
    {IWL_PCI_DEVICE(0x4238, 0x1111, iwl6000_3agn_cfg)},
    {IWL_PCI_DEVICE(0x4238, 0x1118, iwl6000_3agn_cfg)},
    {IWL_PCI_DEVICE(0x4239, 0x1311, iwl6000i_2agn_cfg)},
    {IWL_PCI_DEVICE(0x4239, 0x1316, iwl6000i_2abg_cfg)},
    
    /* 6x05 Series */
    {IWL_PCI_DEVICE(0x0082, 0x1301, iwl6005_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0082, 0x1306, iwl6005_2abg_cfg)},
    {IWL_PCI_DEVICE(0x0082, 0x1307, iwl6005_2bg_cfg)},
    {IWL_PCI_DEVICE(0x0082, 0x1308, iwl6005_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0082, 0x1321, iwl6005_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0082, 0x1326, iwl6005_2abg_cfg)},
    {IWL_PCI_DEVICE(0x0082, 0x1328, iwl6005_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0085, 0x1311, iwl6005_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0085, 0x1318, iwl6005_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0085, 0x1316, iwl6005_2abg_cfg)},
    {IWL_PCI_DEVICE(0x0082, 0xC020, iwl6005_2agn_sff_cfg)},
    {IWL_PCI_DEVICE(0x0085, 0xC220, iwl6005_2agn_sff_cfg)},
    {IWL_PCI_DEVICE(0x0085, 0xC228, iwl6005_2agn_sff_cfg)},
    {IWL_PCI_DEVICE(0x0082, 0x4820, iwl6005_2agn_d_cfg)},
    {IWL_PCI_DEVICE(0x0082, 0x1304, iwl6005_2agn_mow1_cfg)},/* low 5GHz active */
    {IWL_PCI_DEVICE(0x0082, 0x1305, iwl6005_2agn_mow2_cfg)},/* high 5GHz active */
    
    /* 6x30 Series */
    {IWL_PCI_DEVICE(0x008A, 0x5305, iwl1030_bgn_cfg)},
    {IWL_PCI_DEVICE(0x008A, 0x5307, iwl1030_bg_cfg)},
    {IWL_PCI_DEVICE(0x008A, 0x5325, iwl1030_bgn_cfg)},
    {IWL_PCI_DEVICE(0x008A, 0x5327, iwl1030_bg_cfg)},
    {IWL_PCI_DEVICE(0x008B, 0x5315, iwl1030_bgn_cfg)},
    {IWL_PCI_DEVICE(0x008B, 0x5317, iwl1030_bg_cfg)},
    {IWL_PCI_DEVICE(0x0090, 0x5211, iwl6030_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0090, 0x5215, iwl6030_2bgn_cfg)},
    {IWL_PCI_DEVICE(0x0090, 0x5216, iwl6030_2abg_cfg)},
    {IWL_PCI_DEVICE(0x0091, 0x5201, iwl6030_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0091, 0x5205, iwl6030_2bgn_cfg)},
    {IWL_PCI_DEVICE(0x0091, 0x5206, iwl6030_2abg_cfg)},
    {IWL_PCI_DEVICE(0x0091, 0x5207, iwl6030_2bg_cfg)},
    {IWL_PCI_DEVICE(0x0091, 0x5221, iwl6030_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0091, 0x5225, iwl6030_2bgn_cfg)},
    {IWL_PCI_DEVICE(0x0091, 0x5226, iwl6030_2abg_cfg)},
    
    /* 6x50 WiFi/WiMax Series */
    {IWL_PCI_DEVICE(0x0087, 0x1301, iwl6050_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0087, 0x1306, iwl6050_2abg_cfg)},
    {IWL_PCI_DEVICE(0x0087, 0x1321, iwl6050_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0087, 0x1326, iwl6050_2abg_cfg)},
    {IWL_PCI_DEVICE(0x0089, 0x1311, iwl6050_2agn_cfg)},
    {IWL_PCI_DEVICE(0x0089, 0x1316, iwl6050_2abg_cfg)},
    
    /* 6150 WiFi/WiMax Series */
    {IWL_PCI_DEVICE(0x0885, 0x1305, iwl6150_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0885, 0x1307, iwl6150_bg_cfg)},
    {IWL_PCI_DEVICE(0x0885, 0x1325, iwl6150_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0885, 0x1327, iwl6150_bg_cfg)},
    {IWL_PCI_DEVICE(0x0886, 0x1315, iwl6150_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0886, 0x1317, iwl6150_bg_cfg)},
    
    /* 1000 Series WiFi */
    {IWL_PCI_DEVICE(0x0083, 0x1205, iwl1000_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0083, 0x1305, iwl1000_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0083, 0x1225, iwl1000_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0083, 0x1325, iwl1000_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0084, 0x1215, iwl1000_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0084, 0x1315, iwl1000_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0083, 0x1206, iwl1000_bg_cfg)},
    {IWL_PCI_DEVICE(0x0083, 0x1306, iwl1000_bg_cfg)},
    {IWL_PCI_DEVICE(0x0083, 0x1226, iwl1000_bg_cfg)},
    {IWL_PCI_DEVICE(0x0083, 0x1326, iwl1000_bg_cfg)},
    {IWL_PCI_DEVICE(0x0084, 0x1216, iwl1000_bg_cfg)},
    {IWL_PCI_DEVICE(0x0084, 0x1316, iwl1000_bg_cfg)},
    
    /* 100 Series WiFi */
    {IWL_PCI_DEVICE(0x08AE, 0x1005, iwl100_bgn_cfg)},
    {IWL_PCI_DEVICE(0x08AE, 0x1007, iwl100_bg_cfg)},
    {IWL_PCI_DEVICE(0x08AF, 0x1015, iwl100_bgn_cfg)},
    {IWL_PCI_DEVICE(0x08AF, 0x1017, iwl100_bg_cfg)},
    {IWL_PCI_DEVICE(0x08AE, 0x1025, iwl100_bgn_cfg)},
    {IWL_PCI_DEVICE(0x08AE, 0x1027, iwl100_bg_cfg)},
    
    /* 130 Series WiFi */
    {IWL_PCI_DEVICE(0x0896, 0x5005, iwl130_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0896, 0x5007, iwl130_bg_cfg)},
    {IWL_PCI_DEVICE(0x0897, 0x5015, iwl130_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0897, 0x5017, iwl130_bg_cfg)},
    {IWL_PCI_DEVICE(0x0896, 0x5025, iwl130_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0896, 0x5027, iwl130_bg_cfg)},
    
    /* 2x00 Series */
    {IWL_PCI_DEVICE(0x0890, 0x4022, iwl2000_2bgn_cfg)},
    {IWL_PCI_DEVICE(0x0891, 0x4222, iwl2000_2bgn_cfg)},
    {IWL_PCI_DEVICE(0x0890, 0x4422, iwl2000_2bgn_cfg)},
    {IWL_PCI_DEVICE(0x0890, 0x4822, iwl2000_2bgn_d_cfg)},
    
    /* 2x30 Series */
    {IWL_PCI_DEVICE(0x0887, 0x4062, iwl2030_2bgn_cfg)},
    {IWL_PCI_DEVICE(0x0888, 0x4262, iwl2030_2bgn_cfg)},
    {IWL_PCI_DEVICE(0x0887, 0x4462, iwl2030_2bgn_cfg)},
    
    /* 6x35 Series */
    {IWL_PCI_DEVICE(0x088E, 0x4060, iwl6035_2agn_cfg)},
    {IWL_PCI_DEVICE(0x088E, 0x406A, iwl6035_2agn_sff_cfg)},
    {IWL_PCI_DEVICE(0x088F, 0x4260, iwl6035_2agn_cfg)},
    {IWL_PCI_DEVICE(0x088F, 0x426A, iwl6035_2agn_sff_cfg)},
    {IWL_PCI_DEVICE(0x088E, 0x4460, iwl6035_2agn_cfg)},
    {IWL_PCI_DEVICE(0x088E, 0x446A, iwl6035_2agn_sff_cfg)},
    {IWL_PCI_DEVICE(0x088E, 0x4860, iwl6035_2agn_cfg)},
    {IWL_PCI_DEVICE(0x088F, 0x5260, iwl6035_2agn_cfg)},
    
    /* 105 Series */
    {IWL_PCI_DEVICE(0x0894, 0x0022, iwl105_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0895, 0x0222, iwl105_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0894, 0x0422, iwl105_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0894, 0x0822, iwl105_bgn_d_cfg)},
    
    /* 135 Series */
    {IWL_PCI_DEVICE(0x0892, 0x0062, iwl135_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0893, 0x0262, iwl135_bgn_cfg)},
    {IWL_PCI_DEVICE(0x0892, 0x0462, iwl135_bgn_cfg)}

    
    
};





#define super IOEthernetController

OSDefineMetaClassAndStructors(IntelWifi, IOEthernetController)

#define    RELEASE(x)    if(x){(x)->release();(x)=NULL;}

#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_BYTES(x) (x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5]

#define HW_READY_TIMEOUT (50)


enum {
    kOffPowerState,
    kOnPowerState,
    //
    kNumPowerStates
};
static IOPMPowerState gPowerStates[kNumPowerStates] = {
    // kOffPowerState
    {kIOPMPowerStateVersion1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // kOnPowerState
    {kIOPMPowerStateVersion1, (kIOPMPowerOn | kIOPMDeviceUsable), kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
    };


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
    
    
    
    
    PMinit();
    provider->joinPMtree(this);
    
    
    pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!pciDevice) {
        TraceLog("Provider is not PCI device");
        return false;
    }
    pciDevice->retain();

    //    pci_set_master(pdev);
    pciDevice->setBusMasterEnable(true);
    pciDevice->setMemoryEnable(true);
    
    UInt16 vendorId = pciDevice->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceId = pciDevice->configRead16(kIOPCIConfigDeviceID);
    UInt16 subsystemId = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    DebugLog("Device loaded. Vendor: %#06x, Device: %#06x, SubSystem: %#06x", vendorId, deviceId, subsystemId);
    
    struct iwl_cfg *cfg = 0;
    
    for (int i = 0; i < ARRAY_SIZE(iwl_hw_card_ids); i++) {
        if (iwl_hw_card_ids[i].device == deviceId && iwl_hw_card_ids[i].subdevice == subsystemId) {
            cfg = (struct iwl_cfg *)iwl_hw_card_ids[i].driver_data;
            break;
        }
    }
    
    if (!cfg) {
        TraceLog("Config not loaded :(");
        RELEASE(pciDevice);
        return false;
    }
    
    struct iwl_trans* trans = iwl_trans_alloc(0, 0, cfg, 0);
    struct iwl_drv* drv = iwl_drv_start(trans);
    
    
    
    
    fMemoryMap = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!fMemoryMap) {
        TraceLog("MemoryMap failed!");
        RELEASE(pciDevice);
        return false;
    }
    
    pciDevice->configWrite8(0x41, 0);
        
    volatile void *baseAddr = reinterpret_cast<volatile void *>(fMemoryMap->getVirtualAddress());
    
    io = IntelIO::withAddress(baseAddr);
    
    int err = iwl_pcie_prepare_card_hw();
    if (err) {
        TraceLog("ERROR: Error while preparing HW: %d", err);
        RELEASE(io);
        RELEASE(pciDevice);
        
        return false;
    }
    
    iwl_pcie_sw_reset();
    err = iwl_pcie_apm_init();
    if (err) {
        TraceLog("ERROR: PCIE APM Init error: %d", err);
        RELEASE(io);
        RELEASE(pciDevice);
        
        return false;
    }
    
    makeUsable();
//    changePowerStateTo(kOffPowerState);// Set the public power state to the lowest level
    registerPowerDriver(this, gPowerStates, kNumPowerStates);
    
        
    eeprom = IntelEeprom::withIO(io);
    if (!eeprom) {
        TraceLog("EEPROM init failed!");
        RELEASE(fMemoryMap);
        RELEASE(pciDevice);
        
        return false;
    }
    
    fNvmData = eeprom->parse();
    if (!fNvmData) {
        TraceLog("EEPROM parse failed!");
        RELEASE(eeprom);
        RELEASE(io);
        RELEASE(fMemoryMap);
        RELEASE(pciDevice);
        
        return false;
    }
    
    DebugLog("MAC: " MAC_FMT "\n"
             "Num addr: %d\n"
             "Calib: version - %d, voltage - %d\n"
             "Raw temperature: %u",
             
             MAC_BYTES(fNvmData->hw_addr),
             fNvmData->n_hw_addrs,
             fNvmData->calib_version, fNvmData->calib_voltage,
             fNvmData->raw_temperature);
    
    // original: trans->hw_rev = iwl_read32(trans, CSR_HW_REV);
    UInt32 hardwareRevisionId = io->iwl_read32(CSR_HW_REV);
    DebugLog("Hardware revision ID: %#010x", hardwareRevisionId);
    
    if (!createMediumDict()) {
        TraceLog("MediumDict creation failed!");
        RELEASE(eeprom);
        RELEASE(io);
        RELEASE(fMemoryMap);
        RELEASE(pciDevice);
        return false;
    }
    
    fWorkLoop = getWorkLoop();
    if (!fWorkLoop) {
        TraceLog("getWorkLoop failed!");
        RELEASE(eeprom);
        RELEASE(io);
        RELEASE(fMemoryMap);
        RELEASE(pciDevice);
        return false;
    }
    
    fWorkLoop->retain();
    
    if (!attachInterface((IONetworkInterface**)&netif)) {
        TraceLog("Interface attach failed!");
        RELEASE(fWorkLoop);
        RELEASE(mediumDict);
        RELEASE(eeprom);
        RELEASE(io);
        RELEASE(fMemoryMap);
        RELEASE(pciDevice);
        return false;
    }
    
    netif->registerService();
    
    fInterruptSource = IOTimerEventSource::timerEventSource(this, interruptOccured);
    if (!fInterruptSource)
        return false;
    if (fWorkLoop->addEventSource(fInterruptSource) != kIOReturnSuccess)
        return false;
    
    fInterruptSource->enable();
    
    
    registerService();
    
    return true;
}



void IntelWifi::stop(IOService *provider) {
    if (netif) {
        detachInterface(netif);
        netif = NULL;
    }
    RELEASE(fInterruptSource);
    RELEASE(fWorkLoop);
    RELEASE(mediumDict);
    RELEASE(eeprom);
    RELEASE(io);
    RELEASE(fMemoryMap);
    RELEASE(pciDevice);
    
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
    return kIOReturnSuccess;
}



IOReturn IntelWifi::disable(IONetworkInterface *netif) {
    TraceLog("disable");
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
    return OSString::withCString("Centrino N-130");
}
void IntelWifi::interruptOccured(OSObject* owner, IOTimerEventSource*
                                                      sender) {
//    mbuf_t packet;
    DebugLog("Interrupt!");
    IntelWifi* me = (IntelWifi*)owner;
    
    if (!me)
    return;
}




// MARK: iwl trans.c copy paste

/* Note: returns standard 0/-ERROR code */
int IntelWifi::iwl_pcie_prepare_card_hw()
{
    int ret;
    int t = 0;
    int iter;
    
    DebugLog("iwl_trans_prepare_card_hw enter\n");
    
    ret = iwl_pcie_set_hw_ready();
    /* If the card is ready, exit 0 */
    if (ret >= 0)
        return 0;
    
    io->iwl_set_bit(CSR_DBG_LINK_PWR_MGMT_REG,
                CSR_RESET_LINK_PWR_MGMT_DISABLED);
    
    IODelay(2000);
    
    for (iter = 0; iter < 10; iter++) {
        /* If HW is not ready, prepare the conditions to check again */
        io->iwl_set_bit(CSR_HW_IF_CONFIG_REG,
                    CSR_HW_IF_CONFIG_REG_PREPARE);
        
        do {
            ret = iwl_pcie_set_hw_ready();
            if (ret >= 0)
                return 0;
            
            IODelay(1000);
            t += 200;
        } while (t < 150000);
        IOSleep(25);
    }
    
    DebugLog("ERROR: Couldn't prepare the card\n");
    
    return ret;
}

/* Note: returns poll_bit return value, which is >= 0 if success */
int IntelWifi::iwl_pcie_set_hw_ready()
{
    int ret;
    
    io->iwl_set_bit(CSR_HW_IF_CONFIG_REG,
                CSR_HW_IF_CONFIG_REG_BIT_NIC_READY);
    
    /* See if we got it */
    ret = io->iwl_poll_bit(CSR_HW_IF_CONFIG_REG,
                       CSR_HW_IF_CONFIG_REG_BIT_NIC_READY,
                       CSR_HW_IF_CONFIG_REG_BIT_NIC_READY,
                       HW_READY_TIMEOUT);
    
    if (ret >= 0)
        io->iwl_set_bit(CSR_MBOX_SET_REG, CSR_MBOX_SET_REG_OS_ALIVE);
    
    DebugLog("hardware%s ready\n", ret < 0 ? " not" : "");
    return ret;
}

void IntelWifi::iwl_pcie_sw_reset()
{
    /* Reset entire device - do controller reset (results in SHRD_HW_RST) */
    io->iwl_set_bit(CSR_RESET, CSR_RESET_REG_FLAG_SW_RESET);
    IODelay(6000);
}

/*
 * Start up NIC's basic functionality after it has been reset
 * (e.g. after platform boot, or shutdown via iwl_pcie_apm_stop())
 * NOTE:  This does not load uCode nor start the embedded processor
 */
int IntelWifi::iwl_pcie_apm_init()
{
    int ret;
    
    IWL_DEBUG_INFO(trans, "Init card's basic functions\n");
    
    /*
     * Use "set_bit" below rather than "write", to preserve any hardware
     * bits already set by default after reset.
     */
    
    /* Disable L0S exit timer (platform NMI Work/Around) */
    // TODO: Currently working only on Centrino 130 and that's family 6000
    //if (trans->cfg->device_family < IWL_DEVICE_FAMILY_8000)
        io->iwl_set_bit(CSR_GIO_CHICKEN_BITS,
                    CSR_GIO_CHICKEN_BITS_REG_BIT_DIS_L0S_EXIT_TIMER);
    
    /*
     * Disable L0s without affecting L1;
     *  don't wait for ICH L0s (ICH bug W/A)
     */
    io->iwl_set_bit(CSR_GIO_CHICKEN_BITS,
                CSR_GIO_CHICKEN_BITS_REG_BIT_L1A_NO_L0S_RX);
    
    /* Set FH wait threshold to maximum (HW error during stress W/A) */
    io->iwl_set_bit(CSR_DBG_HPET_MEM_REG, CSR_DBG_HPET_MEM_REG_VAL);
    
    /*
     * Enable HAP INTA (interrupt from management bus) to
     * wake device's PCI Express link L1a -> L0s
     */
    io->iwl_set_bit(CSR_HW_IF_CONFIG_REG,
                CSR_HW_IF_CONFIG_REG_BIT_HAP_WAKE_L1A);
    
    iwl_pcie_apm_config();
    
    /* Configure analog phase-lock-loop before activating to D0A */
    // TODO: Return back here
//    if (trans->cfg->base_params->pll_cfg)
//        iwl_set_bit(trans, CSR_ANA_PLL_CFG, CSR50_ANA_PLL_CFG_VAL);
    
    /*
     * Set "initialization complete" bit to move adapter from
     * D0U* --> D0A* (powered-up active) state.
     */
    io->iwl_set_bit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
    
    /*
     * Wait for clock stabilization; once stabilized, access to
     * device-internal resources is supported, e.g. iwl_write_prph()
     * and accesses to uCode SRAM.
     */
    ret = io->iwl_poll_bit(CSR_GP_CNTRL,
                       CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                       CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY, 25000);
    if (ret < 0) {
        IWL_ERR(trans, "Failed to init the card\n");
        return ret;
    }
    
#if DISABLED_CODE
    
    if (trans->cfg->host_interrupt_operation_mode) {
        /*
         * This is a bit of an abuse - This is needed for 7260 / 3160
         * only check host_interrupt_operation_mode even if this is
         * not related to host_interrupt_operation_mode.
         *
         * Enable the oscillator to count wake up time for L1 exit. This
         * consumes slightly more power (100uA) - but allows to be sure
         * that we wake up from L1 on time.
         *
         * This looks weird: read twice the same register, discard the
         * value, set a bit, and yet again, read that same register
         * just to discard the value. But that's the way the hardware
         * seems to like it.
         */
        iwl_read_prph(trans, OSC_CLK);
        iwl_read_prph(trans, OSC_CLK);
        iwl_set_bits_prph(trans, OSC_CLK, OSC_CLK_FORCE_CONTROL);
        iwl_read_prph(trans, OSC_CLK);
        iwl_read_prph(trans, OSC_CLK);
    }
#endif
    
    /*
     * Enable DMA clock and wait for it to stabilize.
     *
     * Write to "CLK_EN_REG"; "1" bits enable clocks, while "0"
     * bits do not disable clocks.  This preserves any hardware
     * bits already set by default in "CLK_CTRL_REG" after reset.
     */
#if DISABLED_CODE
    if (!trans->cfg->apmg_not_supported) {
        iwl_write_prph(trans, APMG_CLK_EN_REG,
                       APMG_CLK_VAL_DMA_CLK_RQT);
        udelay(20);
        
        /* Disable L1-Active */
        iwl_set_bits_prph(trans, APMG_PCIDEV_STT_REG,
                          APMG_PCIDEV_STT_VAL_L1_ACT_DIS);
        
        /* Clear the interrupt in APMG if the NIC is in RFKILL */
        iwl_write_prph(trans, APMG_RTC_INT_STT_REG,
                       APMG_RTC_INT_STT_RFKILL);
    }
#endif
    
//    set_bit(STATUS_DEVICE_ENABLED, &trans->status);
    
    return 0;
}

void IntelWifi::iwl_pcie_apm_config()
{
    
    /*
     * HW bug W/A for instability in PCIe bus L0S->L1 transition.
     * Check if BIOS (or OS) enabled L1-ASPM on this device.
     * If so (likely), disable L0S, so device moves directly L0->L1;
     *    costs negligible amount of power savings.
     * If not (unlikely), enable L0S, so there is at least some
     *    power savings, even without L1.
     */
    
//    UInt8 offset = 0;
//    if (pciDevice->findPCICapability(kIOPCIPCIExpressCapability, &offset))
//    {
//        UInt16 lctl = pciDevice->configRead16(offset);
//        if (lctl & PCI_EXP_LNKCTL_ASPM_L1)
//            io->iwl_set_bit(CSR_GIO_REG, CSR_GIO_REG_VAL_L0S_ENABLED);
//        else
//            io->iwl_clear_bit(CSR_GIO_REG, CSR_GIO_REG_VAL_L0S_ENABLED);
////        trans->pm_support = !(lctl & PCI_EXP_LNKCTL_ASPM_L0S);
//
//        pcie_capability_read_word(trans_pcie->pci_dev, PCI_EXP_DEVCTL2, &cap);
//        trans->ltr_enabled = cap & PCI_EXP_DEVCTL2_LTR_EN;
//        IWL_DEBUG_POWER(trans, "L1 %sabled - LTR %sabled\n",
//                        (lctl & PCI_EXP_LNKCTL_ASPM_L1) ? "En" : "Dis",
//                        trans->ltr_enabled ? "En" : "Dis");
//    }
    
    
    

    
}





