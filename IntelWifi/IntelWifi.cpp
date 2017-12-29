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
#include "iwl-fh.h"
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


/* PCI registers */
#define PCI_CFG_RETRY_TIMEOUT    0x041


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
    TraceLog("Driver init()");
    return super::init(properties);
}

void IntelWifi::free() {
    TraceLog("Driver free()");
    RELEASE(eeprom);
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

    UInt16 vendorId = pciDevice->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceId = pciDevice->configRead16(kIOPCIConfigDeviceID);
    UInt16 subsystemId = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    DebugLog("Device identity: Vendor: %#06x, Device: %#06x, SubSystem: %#06x", vendorId, deviceId, subsystemId);
    
    fConfiguration = getConfiguration(deviceId, subsystemId);
    
    if (!fConfiguration) {
        TraceLog("ERROR: Failed to match configuration!");
        RELEASE(pciDevice);
        return false;
    }
    
    
    
    
    struct iwl_trans* trans = iwl_trans_alloc(0, 0, fConfiguration, 0);
    bool opmodeDown = true;
    
    if (!fConfiguration->base_params->pcie_l1_allowed) {
        /*
         * W/A - seems to solve weird behavior. We need to remove this
         * if we don't want to stay in L1 all the time. This wastes a
         * lot of power.
         */
        // TODO: Find out what to call on OSX
//        pci_disable_link_state(pdev, PCIE_LINK_STATE_L0S |
//                               PCIE_LINK_STATE_L1 |
//                               PCIE_LINK_STATE_CLKPM);
    }
    
    int addr_size;
    UInt8 max_tbs;
    UInt16 tfd_size;
    
    if (fConfiguration->use_tfh) {
        addr_size = 64;
        max_tbs = IWL_TFH_NUM_TBS;
        tfd_size = sizeof(struct iwl_tfh_tfd);
    } else {
        addr_size = 36;
        max_tbs = IWL_NUM_OF_TBS;
        tfd_size = sizeof(struct iwl_tfd);
    }
    
    // original code:     trans->max_skb_frags = IWL_PCIE_MAX_FRAGS(trans_pcie);
    /* We need 2 entries for the TX command and header, and another one might
     * be needed for potential data in the SKB's head. The remaining ones can
     * be used for frags.
     */
    // macro definition: #define IWL_PCIE_MAX_FRAGS(x) (x->max_tbs - 3)
    UInt8 max_skb_frags = max_tbs - 3;
    
    
    // original linux code: pci_set_master(pdev);
    pciDevice->setBusMasterEnable(true);
    

    // TODO: Find out what should be done in IOKit for the code below
//    ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(addr_size));
//    if (!ret)
//        ret = pci_set_consistent_dma_mask(pdev,
//                                          DMA_BIT_MASK(addr_size));
//    if (ret) {
//        ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
//        if (!ret)
//            ret = pci_set_consistent_dma_mask(pdev,
//                                              DMA_BIT_MASK(32));
//        /* both attempts failed: */
//        if (ret) {
//            dev_err(&pdev->dev, "No suitable DMA available\n");
//            goto out_no_pci;
//        }
//    }
    
    pciDevice->setMemoryEnable(true);
    fMemoryMap = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!fMemoryMap) {
        TraceLog("MemoryMap failed!");
        RELEASE(pciDevice);
        return false;
    }
    
    volatile void* hw_addr = reinterpret_cast<volatile void *>(fMemoryMap->getVirtualAddress());
    io = IntelIO::withAddress(hw_addr);
    
    /* We disable the RETRY_TIMEOUT register (0x41) to keep
     * PCI Tx retries from interfering with C3 CPU state */
    // original code: pci_write_config_byte(pdev, PCI_CFG_RETRY_TIMEOUT, 0x00);
    pciDevice->configWrite8(PCI_CFG_RETRY_TIMEOUT, 0);
    
    // original: trans->hw_rev = iwl_read32(trans, CSR_HW_REV);
    UInt32 hw_rev = io->iwl_read32(CSR_HW_REV);
    DebugLog("Hardware revision ID: %#010x", hw_rev);
    
    /*
     * In the 8000 HW family the format of the 4 bytes of CSR_HW_REV have
     * changed, and now the revision step also includes bit 0-1 (no more
     * "dash" value). To keep hw_rev backwards compatible - we'll store it
     * in the old format.
     */
    if (fConfiguration->device_family >= IWL_DEVICE_FAMILY_8000) {
        unsigned long flags;
        
        hw_rev = (hw_rev & 0xFFF0) | CSR_HW_REV_STEP(hw_rev << 2) << 2;
        
        // TODO: Implement
//        ret = iwl_pcie_prepare_card_hw(trans);
//        if (ret) {
//            IWL_WARN(trans, "Exit HW not ready\n");
//            goto out_no_pci;
//        }

        /*
         * in-order to recognize C step driver should read chip version
         * id located at the AUX bus MISC address space.
         */
        io->iwl_set_bit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
        
        IODelay(2);
        
        int ret = io->iwl_poll_bit(CSR_GP_CNTRL,
                                   CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                                   CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                                   25000);
        
        if (ret < 0) {
            IWL_DEBUG_INFO(trans, "Failed to wake up the nic");
            //goto out_no_pci; // TODO: Implement
        }

        // TODO: Implement
//        if (iwl_trans_grab_nic_access(trans, &flags)) {
//            u32 hw_step;
//
//            hw_step = iwl_read_prph_no_grab(trans, WFPM_CTRL_REG);
//            hw_step |= ENABLE_WFPM;
//            iwl_write_prph_no_grab(trans, WFPM_CTRL_REG, hw_step);
//            hw_step = iwl_read_prph_no_grab(trans, AUX_MISC_REG);
//            hw_step = (hw_step >> HW_STEP_LOCATION_BITS) & 0xF;
//            if (hw_step == 0x3)
//                trans->hw_rev = (trans->hw_rev & 0xFFFFFFF3) |
//                (SILICON_C_STEP << 2);
//            iwl_trans_release_nic_access(trans, &flags);
//        }


    }
    /*
     * 9000-series integrated A-step has a problem with suspend/resume
     * and sometimes even causes the whole platform to get stuck. This
     * workaround makes the hardware not go into the problematic state.
     */
    if (fConfiguration->integrated && fConfiguration->device_family == IWL_DEVICE_FAMILY_9000 && CSR_HW_REV_STEP(hw_rev) == SILICON_A_STEP)
        io->iwl_set_bit(CSR_HOST_CHICKEN, CSR_HOST_CHICKEN_PM_IDLE_SRC_DIS_SB_PME);
    
#ifdef CONFIG_IWLMVM
    UInt32 hw_rf_id = io->iwl_read32(CSR_HW_RF_ID);
    if (hw_rf_id == CSR_HW_RF_ID_TYPE_HR) {
        UInt32 hw_status;
        
        hw_status = io->iwl_read_prph(trans, UMAG_GEN_HW_STATUS);
        if (hw_status & UMAG_GEN_HW_IS_FPGA)
            fConfiguration = &iwla000_2ax_cfg_qnj_hr_f0;
        else
            fConfiguration = &iwla000_2ac_cfg_hr;
    }
#endif
    
    // TODO: Implement
    // iwl_pcie_set_interrupt_capa(pdev, trans);
    
    UInt32 hw_id = (deviceId << 16) + subsystemId;
    
    char hw_id_str[52];
    snprintf(hw_id_str, sizeof(hw_id_str), "PCI ID: 0x%04X:0x%04X", deviceId, subsystemId);
    DebugLog("%s", hw_id_str);
    
    /* Initialize the wait queue for commands */
    // TODO: Implement
//    init_waitqueue_head(&trans_pcie->wait_command_queue);
//    init_waitqueue_head(&trans_pcie->d0i3_waitq);
    
    // TODO: Implement
    // msix_enabled is set in iwl_pcie_set_interrupt_capa
//    if (trans_pcie->msix_enabled) {
//        ret = iwl_pcie_init_msix_handler(pdev, trans_pcie);
//        if (ret)
//            goto out_no_pci;
//    } else {
//        ret = iwl_pcie_alloc_ict(trans);
//        if (ret)
//            goto out_no_pci;
//
//        ret = devm_request_threaded_irq(&pdev->dev, pdev->irq,
//                                        iwl_pcie_isr,
//                                        iwl_pcie_irq_handler,
//                                        IRQF_SHARED, DRV_NAME, trans);
//        if (ret) {
//            IWL_ERR(trans, "Error allocating IRQ %d\n", pdev->irq);
//            goto out_free_ict;
//        }
//        trans_pcie->inta_mask = CSR_INI_SET_MASK;
//    }
//
//    trans_pcie->rba.alloc_wq = alloc_workqueue("rb_allocator",
//                                               WQ_HIGHPRI | WQ_UNBOUND, 1);
//    INIT_WORK(&trans_pcie->rba.rx_alloc, iwl_pcie_rx_allocator_work);
//
//#ifdef CONFIG_IWLWIFI_PCIE_RTPM
//    trans->runtime_pm_mode = IWL_PLAT_PM_MODE_D0I3;
//#else
//    trans->runtime_pm_mode = IWL_PLAT_PM_MODE_DISABLED;
//#endif /* CONFIG_IWLWIFI_PCIE_RTPM */
    

    
    

    


    
    struct iwl_drv* drv = iwl_drv_start(trans);
    
    
    
    
    

    
    
    
    //    PMinit();
    //    provider->joinPMtree(this);

    
    
    
    
        
    
    
    
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
    return OSString::withCString(fConfiguration->name);
}
void IntelWifi::interruptOccured(OSObject* owner, IOTimerEventSource*
                                                      sender) {
//    mbuf_t packet;
    DebugLog("Interrupt!");
    IntelWifi* me = (IntelWifi*)owner;
    
    if (!me)
    return;
}



struct iwl_cfg* IntelWifi::getConfiguration(UInt16 deviceId, UInt16 subSystemId) {
    for (int i = 0; i < ARRAY_SIZE(iwl_hw_card_ids); i++) {
        if (iwl_hw_card_ids[i].device == deviceId && iwl_hw_card_ids[i].subdevice == subSystemId) {
            return (struct iwl_cfg *)iwl_hw_card_ids[i].driver_data;
        }
    }
    
    return NULL;
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
    if (fConfiguration->device_family < IWL_DEVICE_FAMILY_8000)
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
    if (fConfiguration->base_params->pll_cfg)
        io->iwl_set_bit(CSR_ANA_PLL_CFG, CSR50_ANA_PLL_CFG_VAL);
    
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
    // TODO: Implement
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





