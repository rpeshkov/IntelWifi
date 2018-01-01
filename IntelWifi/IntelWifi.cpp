/* add your code here */

#include "IntelWifi.hpp"


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
    
    releaseAll();
    
    TraceLog("Fully finished");
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

    fDeviceId = pciDevice->configRead16(kIOPCIConfigDeviceID);
    fSubsystemId = pciDevice->configRead16(kIOPCIConfigSubSystemID);
    
    fConfiguration = getConfiguration(fDeviceId, fSubsystemId);
    
    if (!fConfiguration) {
        TraceLog("ERROR: Failed to match configuration!");
        releaseAll();
        return false;
    }
    
    
    // Starting from here goes code that was taken from original iwl_trans_pcie_alloc
    //
    fTrans = iwl_trans_pcie_alloc(fConfiguration);
    
    if (!fTrans) {
        TraceLog("iwl_trans_pcie_alloc failed");
        releaseAll();
        return false;
    }
    
#ifdef CONFIG_IWLMVM
    const struct iwl_cfg *cfg_7265d = NULL;

    /*
     * special-case 7265D, it has the same PCI IDs.
     *
     * Note that because we already pass the cfg to the transport above,
     * all the parameters that the transport uses must, until that is
     * changed, be identical to the ones in the 7265D configuration.
     */
    if (fConfiguration == &iwl7265_2ac_cfg)
        cfg_7265d = &iwl7265d_2ac_cfg;
    else if (fConfiguration == &iwl7265_2n_cfg)
        cfg_7265d = &iwl7265d_2n_cfg;
    else if (fConfiguration == &iwl7265_n_cfg)
        cfg_7265d = &iwl7265d_n_cfg;
    if (cfg_7265d &&
        (fTrans->hw_rev & CSR_HW_REV_TYPE_MSK) == CSR_HW_REV_TYPE_7265D) {
        fConfiguration = cfg_7265d;
        fTrans->cfg = cfg_7265d;
    }
    
    if (fTrans->cfg->rf_id && fConfiguration == &iwla000_2ac_cfg_hr_cdb &&
        fTrans->hw_rev != CSR_HW_REV_TYPE_HR_CDB) {
        u32 rf_id_chp = CSR_HW_RF_ID_TYPE_CHIP_ID(fTrans->hw_rf_id);
        u32 jf_chp_id = CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_JF);
        u32 hr_chp_id = CSR_HW_RF_ID_TYPE_CHIP_ID(CSR_HW_RF_ID_TYPE_HR);
        
        if (rf_id_chp == jf_chp_id) {
            if (fTrans->hw_rev == CSR_HW_REV_TYPE_QNJ)
                fConfiguration = &iwla000_2ax_cfg_qnj_jf_b0;
            else
                fConfiguration = &iwla000_2ac_cfg_jf;
        } else if (rf_id_chp == hr_chp_id) {
            if (fTrans->hw_rev == CSR_HW_REV_TYPE_QNJ)
                fConfiguration = &iwla000_2ax_cfg_qnj_hr_a0;
            else
                fConfiguration = &iwla000_2ac_cfg_hr;
        }
        fTrans->cfg = fConfiguration;
    }
#endif

    
    fTrans->drv = iwl_drv_start(fTrans);
    if (!fTrans->drv) {
        TraceLog("DRV init failed!");
        releaseAll();
        return false;
    }
    
    /* if RTPM is in use, enable it in our device */
    // TODO: implement
//    if (fTrans->runtime_pm_mode != IWL_PLAT_PM_MODE_DISABLED) {
//        /* We explicitly set the device to active here to
//         * clear contingent errors.
//         */
//        pm_runtime_set_active(&pdev->dev);
//
//        pm_runtime_set_autosuspend_delay(&pdev->dev,
//                                         iwlwifi_mod_params.d0i3_timeout);
//        pm_runtime_use_autosuspend(&pdev->dev);
//
//        /* We are not supposed to call pm_runtime_allow() by
//         * ourselves, but let userspace enable runtime PM via
//         * sysfs.  However, since we don't enable this from
//         * userspace yet, we need to allow/forbid() ourselves.
//         */
//        pm_runtime_allow(&pdev->dev);
//    }

    
    PMinit();
    provider->joinPMtree(this);
    
    int err = iwl_trans_pcie_start_hw(fTrans, true);
    if (err) {
        TraceLog("ERROR: Error while preparing HW: %d", err);
        releaseAll();
        return false;
    }
    
    struct iwl_trans_pcie* trans_pcie = IWL_TRANS_GET_PCIE_TRANS(fTrans);
    
    /* Set is_down to false here so that...*/
    trans_pcie->is_down = false;
    
    makeUsable();
//    changePowerStateTo(kOffPowerState);// Set the public power state to the lowest level
    registerPowerDriver(this, gPowerStates, kNumPowerStates);

    eeprom = IntelEeprom::withIO(io, const_cast<struct iwl_cfg*>(fConfiguration), fTrans->hw_rev);
    if (!eeprom) {
        TraceLog("EEPROM init failed!");
        releaseAll();
        return false;
    }
    
    fNvmData = eeprom->parse();
    if (!fNvmData) {
        TraceLog("EEPROM parse failed!");
        releaseAll();
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
        releaseAll();
        return false;
    }
    
    fWorkLoop = getWorkLoop();
    if (!fWorkLoop) {
        TraceLog("getWorkLoop failed!");
        releaseAll();
        return false;
    }
    
    fWorkLoop->retain();
    
    if (!attachInterface((IONetworkInterface**)&netif)) {
        TraceLog("Interface attach failed!");
        releaseAll();
        return false;
    }
    
    netif->registerService();
    
    fInterruptSource = IOInterruptEventSource::interruptEventSource(this,
                                                                    (IOInterruptEventAction) &IntelWifi::interruptOccured,
                                                                    provider, 0);
    if (!fInterruptSource) {
        TraceLog("InterruptSource init failed!");
        releaseAll();
        return false;
    }
    
    if (fWorkLoop->addEventSource(fInterruptSource) != kIOReturnSuccess) {
        TraceLog("EventSource registration failed");
        releaseAll();
        return false;
    }
    
    fInterruptSource->enable();
    registerService();
    return true;
}



void IntelWifi::stop(IOService *provider) {
    
    iwl_trans_pcie_stop_device(fTrans, true);
    
    iwl_drv_stop(fTrans->drv);
    iwl_trans_pcie_free(fTrans);
    fTrans = NULL;
    
    if (netif) {
        detachInterface(netif);
        netif = NULL;
    }
    
    if (fWorkLoop) {
        if (fInterruptSource) {
            fInterruptSource->disable();
            fWorkLoop->removeEventSource(fInterruptSource);
        }
    }
    
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

void IntelWifi::interruptOccured(OSObject* owner, IOTimerEventSource* sender) {
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

struct iwl_trans* IntelWifi::iwl_trans_pcie_alloc(const struct iwl_cfg *cfg) {
    
//    if (cfg->gen2)
//        trans = iwl_trans_alloc(sizeof(struct iwl_trans_pcie),
//                                &pdev->dev, cfg, &trans_ops_pcie_gen2);
//    else
//        trans = iwl_trans_alloc(sizeof(struct iwl_trans_pcie),
//                                &pdev->dev, cfg, &trans_ops_pcie);
    struct iwl_trans *trans = iwl_trans_alloc(sizeof(struct iwl_trans_pcie), NULL, cfg, NULL);
    
    if (!trans)
        return NULL;
    
    struct iwl_trans_pcie* trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    trans_pcie->trans = trans;
    trans_pcie->opmode_down = true;
    trans_pcie->irq_lock = IOSimpleLockAlloc();
    trans_pcie->reg_lock = IOSimpleLockAlloc();
    trans_pcie->mutex = IOLockAlloc();
    
    // TODO: Implement
    //init_waitqueue_head(&trans_pcie->ucode_write_waitq);
    //trans_pcie->tso_hdr_page = alloc_percpu(struct iwl_tso_hdr_page);
    //if (!trans_pcie->tso_hdr_page) {
    //    ret = -ENOMEM;
    //    goto out_no_pci;
    //}
    
    if (!cfg->base_params->pcie_l1_allowed) {
        /*
         * W/A - seems to solve weird behavior. We need to remove this
         * if we don't want to stay in L1 all the time. This wastes a
         * lot of power.
         */
        // TODO: Find out what to call on OSX
        //pci_disable_link_state(pdev, PCIE_LINK_STATE_L0S |
        //                       PCIE_LINK_STATE_L1 |
        //                       PCIE_LINK_STATE_CLKPM);
    }
    
    int addr_size;
    if (cfg->use_tfh) {
        addr_size = 64;
        trans_pcie->max_tbs = IWL_TFH_NUM_TBS;
        trans_pcie->tfd_size = sizeof(struct iwl_tfh_tfd);
    } else {
        addr_size = 36;
        trans_pcie->max_tbs = IWL_NUM_OF_TBS;
        trans_pcie->tfd_size = sizeof(struct iwl_tfd);
    }
    trans->max_skb_frags = IWL_PCIE_MAX_FRAGS(trans_pcie);

    // original linux code: pci_set_master(pdev);
    pciDevice->setBusMasterEnable(true);
    
    // TODO: Find out what should be done in IOKit for the code below
    //ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(addr_size));
    //if (!ret)
    //    ret = pci_set_consistent_dma_mask(pdev,
    //                                      DMA_BIT_MASK(addr_size));
    //if (ret) {
    //    ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
    //    if (!ret)
    //        ret = pci_set_consistent_dma_mask(pdev,
    //                                          DMA_BIT_MASK(32));
    //    /* both attempts failed: */
    //    if (ret) {
    //        dev_err(&pdev->dev, "No suitable DMA available\n");
    //        goto out_no_pci;
    //    }
    //}

    pciDevice->setMemoryEnable(true);
    fMemoryMap = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!fMemoryMap) {
        TraceLog("MemoryMap failed!");
        return NULL;
    }
    
    trans_pcie->hw_base = reinterpret_cast<volatile void *>(fMemoryMap->getVirtualAddress());
    io = IntelIO::withTrans(trans_pcie);
    
    if (!io) {
        TraceLog("MemoryMap failed!");
        return NULL;
    }
    
    /* We disable the RETRY_TIMEOUT register (0x41) to keep
     * PCI Tx retries from interfering with C3 CPU state */
    // original code: pci_write_config_byte(pdev, PCI_CFG_RETRY_TIMEOUT, 0x00);
    pciDevice->configWrite8(PCI_CFG_RETRY_TIMEOUT, 0);
    
    iwl_disable_interrupts(trans);
    
    // original: trans->hw_rev = iwl_read32(trans, CSR_HW_REV);
    trans->hw_rev = io->iwl_read32(CSR_HW_REV);
    
    /*
     * In the 8000 HW family the format of the 4 bytes of CSR_HW_REV have
     * changed, and now the revision step also includes bit 0-1 (no more
     * "dash" value). To keep hw_rev backwards compatible - we'll store it
     * in the old format.
     */
    if (trans->cfg->device_family >= IWL_DEVICE_FAMILY_8000) {
        trans->hw_rev = (trans->hw_rev & 0xFFF0) | CSR_HW_REV_STEP(trans->hw_rev << 2) << 2;
        
        int ret = iwl_pcie_prepare_card_hw(trans);
        if (ret) {
            IWL_WARN(trans, "Exit HW not ready\n");
            return NULL;
        }
        
        /*
         * in-order to recognize C step driver should read chip version
         * id located at the AUX bus MISC address space.
         */
        io->iwl_set_bit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
        
        IODelay(2);
        
        ret = io->iwl_poll_bit(CSR_GP_CNTRL,
                               CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                               CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                               25000);
        
        if (ret < 0) {
            IWL_DEBUG_INFO(trans, "Failed to wake up the nic");
            return NULL;
        }
        
        IOInterruptState state;
        if (io->iwl_grab_nic_access(&state)) {
            u32 hw_step;
            
            hw_step = io->iwl_read_prph_no_grab(WFPM_CTRL_REG);
            hw_step |= ENABLE_WFPM;
            io->iwl_write_prph_no_grab(WFPM_CTRL_REG, hw_step);
            hw_step = io->iwl_read_prph_no_grab(AUX_MISC_REG);
            hw_step = (hw_step >> HW_STEP_LOCATION_BITS) & 0xF;
            if (hw_step == 0x3)
                trans->hw_rev = (trans->hw_rev & 0xFFFFFFF3) | (SILICON_C_STEP << 2);
            io->iwl_release_nic_access(&state);
        }
    }
    
    /*
     * 9000-series integrated A-step has a problem with suspend/resume
     * and sometimes even causes the whole platform to get stuck. This
     * workaround makes the hardware not go into the problematic state.
     */
    if (trans->cfg->integrated && trans->cfg->device_family == IWL_DEVICE_FAMILY_9000 && CSR_HW_REV_STEP(trans->hw_rev) == SILICON_A_STEP)
        io->iwl_set_bit(CSR_HOST_CHICKEN, CSR_HOST_CHICKEN_PM_IDLE_SRC_DIS_SB_PME);
    
#ifdef CONFIG_IWLMVM
    trans->hw_rf_id = io->iwl_read32(CSR_HW_RF_ID);
    if (trans->hw_rf_id == CSR_HW_RF_ID_TYPE_HR) {
        UInt32 hw_status;
        
        hw_status = io->iwl_read_prph(UMAG_GEN_HW_STATUS);
        if (hw_status & UMAG_GEN_HW_IS_FPGA)
            trans->cfg = const_cast<struct iwl_cfg *>(&iwla000_2ax_cfg_qnj_hr_f0);
        else
            trans->cfg = const_cast<struct iwl_cfg *>(&iwla000_2ac_cfg_hr);
    }
#endif
    
    // TODO: Implement
    // iwl_pcie_set_interrupt_capa(pdev, trans);
    trans->hw_id = (fDeviceId << 16) + fSubsystemId;
    snprintf(trans->hw_id_str, sizeof(trans->hw_id_str), "PCI ID: 0x%04X:0x%04X", fDeviceId, fSubsystemId);
    DebugLog("%s", trans->hw_id_str);
    
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

#ifdef CONFIG_IWLWIFI_PCIE_RTPM
    trans->runtime_pm_mode = IWL_PLAT_PM_MODE_D0I3;
#else
    trans->runtime_pm_mode = IWL_PLAT_PM_MODE_DISABLED;
#endif /* CONFIG_IWLWIFI_PCIE_RTPM */
   
    return trans;
}

void IntelWifi::iwl_trans_pcie_free(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
//    int i;
//
//    iwl_pcie_synchronize_irqs(trans);
//
//    if (trans->cfg->gen2)
//        iwl_pcie_gen2_tx_free(trans);
//    else
//        iwl_pcie_tx_free(trans);
//    iwl_pcie_rx_free(trans);
//
//    if (trans_pcie->rba.alloc_wq) {
//        destroy_workqueue(trans_pcie->rba.alloc_wq);
//        trans_pcie->rba.alloc_wq = NULL;
//    }
//
//    if (trans_pcie->msix_enabled) {
//        for (i = 0; i < trans_pcie->alloc_vecs; i++) {
//            irq_set_affinity_hint(
//                                  trans_pcie->msix_entries[i].vector,
//                                  NULL);
//        }
//
//        trans_pcie->msix_enabled = false;
//    } else {
//        iwl_pcie_free_ict(trans);
//    }
//
//    iwl_pcie_free_fw_monitor(trans);
//
//    for_each_possible_cpu(i) {
//        struct iwl_tso_hdr_page *p =
//        per_cpu_ptr(trans_pcie->tso_hdr_page, i);
//
//        if (p->page)
//            __free_page(p->page);
//    }
    
//    free_percpu(trans_pcie->tso_hdr_page);
    IOSimpleLockFree(trans_pcie->irq_lock);
    IOSimpleLockFree(trans_pcie->reg_lock);
    IOLockFree(trans_pcie->mutex);
    iwl_trans_free(trans);
}


void IntelWifi::_iwl_disable_interrupts(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    clear_bit(STATUS_INT_ENABLED, &trans->status);
    if (!trans_pcie->msix_enabled) {
        /* disable interrupts from uCode/NIC to host */
        io->iwl_write32(CSR_INT_MASK, 0x00000000);
        
        /* acknowledge/clear/reset any interrupts still pending
         * from uCode or flow handler (Rx/Tx DMA) */
        io->iwl_write32(CSR_INT, 0xffffffff);
        io->iwl_write32(CSR_FH_INT_STATUS, 0xffffffff);
    } else {
        /* disable all the interrupt we might use */
        io->iwl_write32(CSR_MSIX_FH_INT_MASK_AD,
                    trans_pcie->fh_init_mask);
        io->iwl_write32(CSR_MSIX_HW_INT_MASK_AD,
                    trans_pcie->hw_init_mask);
    }
    IWL_DEBUG_ISR(trans, "Disabled interrupts\n");
}

void IntelWifi::iwl_disable_interrupts(struct iwl_trans *trans)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    //spin_lock(&trans_pcie->irq_lock);
    IOSimpleLockLock(trans_pcie->irq_lock);
    _iwl_disable_interrupts(trans);
    //spin_unlock(&trans_pcie->irq_lock);
    IOSimpleLockUnlock(trans_pcie->irq_lock);
}




/* Note: returns poll_bit return value, which is >= 0 if success */
int IntelWifi::iwl_pcie_set_hw_ready(struct iwl_trans *trans)
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
    
    IWL_DEBUG_INFO(trans, "hardware%s ready\n", ret < 0 ? " not" : "");

    return ret;
}









