//
//  Configuration.c
//  IntelWifi
//
//  Created by Roman Peshkov on 02/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "Configuration.h"

struct pci_device_id {
    __u32 vendor, device;        /* Vendor and device ID or PCI_ANY_ID*/
    __u32 subvendor, subdevice;    /* Subsystem ID's or PCI_ANY_ID */
    
    u8 * driver_data;    /* Data private to the driver */
};


#define IWL_PCI_DEVICE(dev, subdev, cfg)        \
    .vendor = 0x8086,  .device = (dev),         \
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

struct iwl_cfg* getConfiguration(u16 deviceId, u16 subSystemId) {
    for (int i = 0; i < ARRAY_SIZE(iwl_hw_card_ids); i++) {
        if (iwl_hw_card_ids[i].device == deviceId && iwl_hw_card_ids[i].subdevice == subSystemId) {
            return (struct iwl_cfg *)iwl_hw_card_ids[i].driver_data;
        }
    }
    
    return NULL;
}
