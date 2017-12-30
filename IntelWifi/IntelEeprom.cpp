//
//  IntelEeprom.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 27/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#include "IntelEeprom.hpp"

extern "C" {
#include "iwlwifi/iwl-csr.h"
#include "iwl-prph.h"
#include "iwl-modparams.h"
#include "iwl-drv.h"
}

#include "porting/linux/types.h"

#include "Logging.h"

#define super OSObject
OSDefineMetaClassAndStructors( IntelEeprom, OSObject )

/* EEPROM offset definitions */

/* indirect access definitions */
#define ADDRESS_MSK                 0x0000FFFF
#define INDIRECT_TYPE_MSK           0x000F0000
#define INDIRECT_HOST               0x00010000
#define INDIRECT_GENERAL            0x00020000
#define INDIRECT_REGULATORY         0x00030000
#define INDIRECT_CALIBRATION        0x00040000
#define INDIRECT_PROCESS_ADJST      0x00050000
#define INDIRECT_OTHERS             0x00060000
#define INDIRECT_TXP_LIMIT          0x00070000
#define INDIRECT_TXP_LIMIT_SIZE     0x00080000
#define INDIRECT_ADDRESS            0x00100000

/* corresponding link offsets in EEPROM */
#define EEPROM_LINK_HOST             (2*0x64)
#define EEPROM_LINK_GENERAL          (2*0x65)
#define EEPROM_LINK_REGULATORY       (2*0x66)
#define EEPROM_LINK_CALIBRATION      (2*0x67)
#define EEPROM_LINK_PROCESS_ADJST    (2*0x68)
#define EEPROM_LINK_OTHERS           (2*0x69)
#define EEPROM_LINK_TXP_LIMIT        (2*0x6a)
#define EEPROM_LINK_TXP_LIMIT_SIZE   (2*0x6b)

/* General */
#define EEPROM_DEVICE_ID                    (2*0x08)    /* 2 bytes */
#define EEPROM_SUBSYSTEM_ID            (2*0x0A)    /* 2 bytes */
#define EEPROM_MAC_ADDRESS                  (2*0x15)    /* 6  bytes */
#define EEPROM_BOARD_REVISION               (2*0x35)    /* 2  bytes */
#define EEPROM_BOARD_PBA_NUMBER             (2*0x3B+1)    /* 9  bytes */
#define EEPROM_VERSION                      (2*0x44)    /* 2  bytes */
#define EEPROM_SKU_CAP                      (2*0x45)    /* 2  bytes */
#define EEPROM_OEM_MODE                     (2*0x46)    /* 2  bytes */
#define EEPROM_RADIO_CONFIG                 (2*0x48)    /* 2  bytes */
#define EEPROM_NUM_MAC_ADDRESS              (2*0x4C)    /* 2  bytes */


#define EEPROM_CALIB_ALL    (INDIRECT_ADDRESS | INDIRECT_CALIBRATION)
#define EEPROM_XTAL        ((2*0x128) | EEPROM_CALIB_ALL)

/* temperature */
#define EEPROM_KELVIN_TEMPERATURE    ((2*0x12A) | EEPROM_CALIB_ALL)
#define EEPROM_RAW_TEMPERATURE        ((2*0x12B) | EEPROM_CALIB_ALL)

/* SKU Capabilities (actual values from EEPROM definition) */
enum eeprom_sku_bits {
    EEPROM_SKU_CAP_BAND_24GHZ    = BIT(4),
    EEPROM_SKU_CAP_BAND_52GHZ    = BIT(5),
    EEPROM_SKU_CAP_11N_ENABLE    = BIT(6),
    EEPROM_SKU_CAP_AMT_ENABLE    = BIT(7),
    EEPROM_SKU_CAP_IPAN_ENABLE    = BIT(8)
};

/* radio config bits (actual values from EEPROM definition) */
#define EEPROM_RF_CFG_TYPE_MSK(x)   (x & 0x3)         /* bits 0-1   */
#define EEPROM_RF_CFG_STEP_MSK(x)   ((x >> 2)  & 0x3) /* bits 2-3   */
#define EEPROM_RF_CFG_DASH_MSK(x)   ((x >> 4)  & 0x3) /* bits 4-5   */
#define EEPROM_RF_CFG_PNUM_MSK(x)   ((x >> 6)  & 0x3) /* bits 6-7   */
#define EEPROM_RF_CFG_TX_ANT_MSK(x) ((x >> 8)  & 0xF) /* bits 8-11  */
#define EEPROM_RF_CFG_RX_ANT_MSK(x) ((x >> 12) & 0xF) /* bits 12-15 */

/*
 * EEPROM bands
 * These are the channel numbers from each band in the order
 * that they are stored in the EEPROM band information. Note
 * that EEPROM bands aren't the same as mac80211 bands, and
 * there are even special "ht40 bands" in the EEPROM.
 */
static const u8 iwl_eeprom_band_1[14] = { /* 2.4 GHz */
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
};

static const u8 iwl_eeprom_band_2[] = {    /* 4915-5080MHz */
    183, 184, 185, 187, 188, 189, 192, 196, 7, 8, 11, 12, 16
};

static const u8 iwl_eeprom_band_3[] = {    /* 5170-5320MHz */
    34, 36, 38, 40, 42, 44, 46, 48, 52, 56, 60, 64
};

static const u8 iwl_eeprom_band_4[] = {    /* 5500-5700MHz */
    100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140
};

static const u8 iwl_eeprom_band_5[] = {    /* 5725-5825MHz */
    145, 149, 153, 157, 161, 165
};

static const u8 iwl_eeprom_band_6[] = {    /* 2.4 ht40 channel */
    1, 2, 3, 4, 5, 6, 7
};

static const u8 iwl_eeprom_band_7[] = {    /* 5.2 ht40 channel */
    36, 44, 52, 60, 100, 108, 116, 124, 132, 149, 157
};

#define IWL_NUM_CHANNELS    (ARRAY_SIZE(iwl_eeprom_band_1) + \
ARRAY_SIZE(iwl_eeprom_band_2) + \
ARRAY_SIZE(iwl_eeprom_band_3) + \
ARRAY_SIZE(iwl_eeprom_band_4) + \
ARRAY_SIZE(iwl_eeprom_band_5))

/* rate data (static) */
static struct ieee80211_rate iwl_cfg80211_rates[] = {
    { .bitrate = 1 * 10, .hw_value = 0, .hw_value_short = 0, },
    { .bitrate = 2 * 10, .hw_value = 1, .hw_value_short = 1,
        .flags = IEEE80211_RATE_SHORT_PREAMBLE, },
    { .bitrate = (u16)(5.5 * 10), .hw_value = 2, .hw_value_short = 2,
        .flags = IEEE80211_RATE_SHORT_PREAMBLE, },
    { .bitrate = 11 * 10, .hw_value = 3, .hw_value_short = 3,
        .flags = IEEE80211_RATE_SHORT_PREAMBLE, },
    { .bitrate = 6 * 10, .hw_value = 4, .hw_value_short = 4, },
    { .bitrate = 9 * 10, .hw_value = 5, .hw_value_short = 5, },
    { .bitrate = 12 * 10, .hw_value = 6, .hw_value_short = 6, },
    { .bitrate = 18 * 10, .hw_value = 7, .hw_value_short = 7, },
    { .bitrate = 24 * 10, .hw_value = 8, .hw_value_short = 8, },
    { .bitrate = 36 * 10, .hw_value = 9, .hw_value_short = 9, },
    { .bitrate = 48 * 10, .hw_value = 10, .hw_value_short = 10, },
    { .bitrate = 54 * 10, .hw_value = 11, .hw_value_short = 11, },
};
#define RATES_24_OFFS    0
#define N_RATES_24    ARRAY_SIZE(iwl_cfg80211_rates)
#define RATES_52_OFFS    4
#define N_RATES_52    (N_RATES_24 - RATES_52_OFFS)




/*
 * EEPROM access time values:
 *
 * Driver initiates EEPROM read by writing byte address << 1 to CSR_EEPROM_REG.
 * Driver then polls CSR_EEPROM_REG for CSR_EEPROM_REG_READ_VALID_MSK (0x1).
 * When polling, wait 10 uSec between polling loops, up to a maximum 5000 uSec.
 * Driver reads 16-bit value from bits 31-16 of CSR_EEPROM_REG.
 */
#define IWL_EEPROM_ACCESS_TIMEOUT    5000 /* uSec */

#define IWL_EEPROM_SEM_TIMEOUT        10   /* microseconds */
#define IWL_EEPROM_SEM_RETRY_LIMIT    1000 /* number of attempts (not time) */


/*
 * The device's EEPROM semaphore prevents conflicts between driver and uCode
 * when accessing the EEPROM; each access is a series of pulses to/from the
 * EEPROM chip, not a single event, so even reads could conflict if they
 * weren't arbitrated by the semaphore.
 */

#define    EEPROM_SEM_TIMEOUT 10        /* milliseconds */
#define EEPROM_SEM_RETRY_LIMIT 1000    /* number of attempts (not time) */

IntelEeprom* IntelEeprom::withIO(IntelIO *io, struct iwl_cfg *cfg, UInt32 hwRev) {
    IntelEeprom *eeprom = new IntelEeprom;
    
    if (eeprom && !eeprom->initWithIO(io, cfg, hwRev)) {
        eeprom->release();
        return 0;
    }
    
    return eeprom;
}

bool IntelEeprom::initWithIO(IntelIO *io, struct iwl_cfg *cfg, UInt32 hwRev) {
    if (!super::init()) {
        return false;
    }
    
    fIO = io;
    fConfiguration = cfg;
    fHwRev = hwRev;
    
    return read();
}

void IntelEeprom::release() {
    if (fEeprom) {
        IOFree(fEeprom, fEepromSize);
        fEeprom = NULL;
    }
    
    super::release();
}

bool IntelEeprom::read() {
    __le16 *e;
    u32 gp = fIO->iwl_read32(CSR_EEPROM_GP);
    int sz;
    int ret;
    u16 addr;
    u16 validblockaddr = 0;
    u16 cache_addr = 0;
    int nvm_is_otp;
    
    nvm_is_otp = iwl_nvm_is_otp();
    if (nvm_is_otp < 0)
        return nvm_is_otp;
    
    sz = fConfiguration->base_params->eeprom_size;
    fEepromSize = sz;
    IWL_DEBUG_EEPROM("", "NVM size = %d\n", sz);
    
    e = (UInt16*)IOMalloc(sz);
    if (!e) {
        return false;
    }
    
    ret = iwl_eeprom_verify_signature(nvm_is_otp);
    if (ret < 0) {
        IWL_ERR(trans, "EEPROM not found, EEPROM_GP=0x%08x\n", gp);
        IOFree(e, sz);
        return false;
    }
    
    ret = iwl_eeprom_acquire_semaphore();
    if (ret < 0) {
        TraceLog("Failed to acquire EEPROM semaphore.\n");
        IOFree(e, sz);
        return false;
    }
    
    if (nvm_is_otp) {
        ret = iwl_init_otp_access();
        if (ret) {
            IWL_ERR(trans, "Failed to initialize OTP access.\n");
            iwl_eeprom_release_semaphore();
            IOFree(e, sz);
            return false;
        }
        
        fIO->iwl_write32(CSR_EEPROM_GP,
                    fIO->iwl_read32(CSR_EEPROM_GP) &
                    ~CSR_EEPROM_GP_IF_OWNER_MSK);
        
        fIO->iwl_set_bit(CSR_OTP_GP_REG,
                    CSR_OTP_GP_REG_ECC_CORR_STATUS_MSK |
                    CSR_OTP_GP_REG_ECC_UNCORR_STATUS_MSK);
        /* traversing the linked list if no shadow ram supported */
        if (!fConfiguration->base_params->shadow_ram_support) {
            ret = iwl_find_otp_image(&validblockaddr);
            if (ret) {
                iwl_eeprom_release_semaphore();
                IOFree(e, sz);
                return false;
            }
        }
        
        for (addr = validblockaddr; addr < validblockaddr + sz;
             addr += sizeof(u16)) {
            __le16 eeprom_data;
            
            ret = iwl_read_otp_word(addr, &eeprom_data);
            if (ret) {
                iwl_eeprom_release_semaphore();
                IOFree(e, sz);
                return false;
            }
                
            e[cache_addr / 2] = eeprom_data;
            cache_addr += sizeof(u16);
        }

    } else {
        for (UInt16 a = 0; a < sz; a += sizeof(UInt16)) {
            fIO->iwl_write32(CSR_EEPROM_REG, CSR_EEPROM_REG_MSK_ADDR & (a << 1));
            
            int ret = fIO->iwl_poll_bit(CSR_EEPROM_REG,
                                        CSR_EEPROM_REG_READ_VALID_MSK,
                                        CSR_EEPROM_REG_READ_VALID_MSK,
                                        IWL_EEPROM_ACCESS_TIMEOUT);
            
            if (ret < 0) {
                IWL_ERR(trans, "Time out reading EEPROM[%d]\n", a);
                iwl_eeprom_release_semaphore();

                return false;
            }
            
            
            UInt32 r = fIO->iwl_read32(CSR_EEPROM_REG);
            
            e[a/2] = (r >> 16);
        }
    }
    
    IWL_DEBUG_EEPROM(trans->dev, "NVM Type: %s\n",
                     nvm_is_otp ? "OTP" : "EEPROM");
   
    
    iwl_eeprom_release_semaphore();
    
    fEeprom = (UInt8 *)e;
    return true;
}

struct iwl_nvm_data* IntelEeprom::parse() {
    struct iwl_nvm_data* nvmData = (struct iwl_nvm_data*)IOMalloc(sizeof(struct iwl_nvm_data));
    if (!nvmData) {
        return NULL;
    }
    
    const void *tmp = iwl_eeprom_query_addr(EEPROM_MAC_ADDRESS);
    
    if (!tmp) {
        IOFree(nvmData, sizeof(struct iwl_nvm_data));
        return NULL;
    }
    
    memcpy(nvmData->hw_addr, tmp, ETH_ALEN);
    nvmData->n_hw_addrs = iwl_eeprom_query16(EEPROM_NUM_MAC_ADDRESS);
    
    if (iwl_eeprom_read_calib(nvmData)) {
        IOFree(nvmData, sizeof(struct iwl_nvm_data));
        return NULL;
    }
    
    tmp = iwl_eeprom_query_addr(EEPROM_XTAL);
    if (!tmp) {
        IOFree(nvmData, sizeof(struct iwl_nvm_data));
        return NULL;
    }
    memcpy(nvmData->xtal_calib, tmp, sizeof(nvmData->xtal_calib));
    
    tmp = iwl_eeprom_query_addr(EEPROM_RAW_TEMPERATURE);
    if (!tmp) {
        IOFree(nvmData, sizeof(struct iwl_nvm_data));
        return NULL;
    }
    nvmData->raw_temperature = *(UInt16 *)tmp;
    
    tmp = iwl_eeprom_query_addr(EEPROM_KELVIN_TEMPERATURE);
    if (!tmp) {
        IOFree(nvmData, sizeof(struct iwl_nvm_data));
        return NULL;
    }
    nvmData->kelvin_temperature = *(UInt16 *)tmp;
    nvmData->kelvin_voltage = *((UInt16 *)tmp + 1);
    
    UInt16 radio_cfg = iwl_eeprom_query16(EEPROM_RADIO_CONFIG);
    nvmData->radio_cfg_dash = EEPROM_RF_CFG_DASH_MSK(radio_cfg);
    nvmData->radio_cfg_pnum = EEPROM_RF_CFG_PNUM_MSK(radio_cfg);
    nvmData->radio_cfg_step = EEPROM_RF_CFG_STEP_MSK(radio_cfg);
    nvmData->radio_cfg_type = EEPROM_RF_CFG_TYPE_MSK(radio_cfg);
    nvmData->valid_rx_ant = EEPROM_RF_CFG_RX_ANT_MSK(radio_cfg);
    nvmData->valid_tx_ant = EEPROM_RF_CFG_TX_ANT_MSK(radio_cfg);
    
    UInt16 sku = iwl_eeprom_query16(EEPROM_SKU_CAP);
    nvmData->sku_cap_11n_enable = sku & EEPROM_SKU_CAP_11N_ENABLE;
    nvmData->sku_cap_amt_enable = sku & EEPROM_SKU_CAP_AMT_ENABLE;
    nvmData->sku_cap_band_24GHz_enable = sku & EEPROM_SKU_CAP_BAND_24GHZ;
    nvmData->sku_cap_band_52GHz_enable = sku & EEPROM_SKU_CAP_BAND_52GHZ;
    nvmData->sku_cap_ipan_enable = sku & EEPROM_SKU_CAP_IPAN_ENABLE;
    if (iwlwifi_mod_params.disable_11n & IWL_DISABLE_HT_ALL)
        nvmData->sku_cap_11n_enable = false;
    
    nvmData->nvm_version = iwl_eeprom_query16(EEPROM_VERSION);
    
    /* check overrides (some devices have wrong EEPROM) */
    if (fConfiguration->valid_tx_ant)
        nvmData->valid_tx_ant = fConfiguration->valid_tx_ant;
    if (fConfiguration->valid_rx_ant)
        nvmData->valid_rx_ant = fConfiguration->valid_rx_ant;
    
    if (!nvmData->valid_tx_ant || !nvmData->valid_rx_ant) {
        IWL_ERR_DEV(dev, "invalid antennas (0x%x, 0x%x)\n",
                    nvmData->valid_tx_ant, nvmData->valid_rx_ant);
        IOFree(nvmData, sizeof(struct iwl_nvm_data));
        return NULL;
    }

    iwl_init_sbands(nvmData);
    
    return nvmData;
}

//
// MARK: Private. Reading routines
//

/*
 * iwl_is_otp_empty: check for empty OTP
 */
bool IntelEeprom::iwl_is_otp_empty()
{
    u16 next_link_addr = 0;
    __le16 link_value;
    bool is_empty = false;
    
    /* locate the beginning of OTP link list */
    if (!iwl_read_otp_word(next_link_addr, &link_value)) {
        if (!link_value) {
            IWL_ERR(trans, "OTP is empty\n");
            is_empty = true;
        }
    } else {
        IWL_ERR(trans, "Unable to read first block of OTP list.\n");
        is_empty = true;
    }
    
    return is_empty;
}

/*
 * iwl_find_otp_image: find EEPROM image in OTP
 *   finding the OTP block that contains the EEPROM image.
 *   the last valid block on the link list (the block _before_ the last block)
 *   is the block we should read and used to configure the device.
 *   If all the available OTP blocks are full, the last block will be the block
 *   we should read and used to configure the device.
 *   only perform this operation if shadow RAM is disabled
 */
int IntelEeprom::iwl_find_otp_image(UInt16 *validblockaddr)
{
    UInt16 next_link_addr = 0, valid_addr;
    UInt16 link_value = 0;
    int usedblocks = 0;
    
    /* set addressing mode to absolute to traverse the link list */
    iwl_set_otp_access_absolute();
    
    /* checking for empty OTP or error */
    if (iwl_is_otp_empty())
        return -EINVAL;
    
    /*
     * start traverse link list
     * until reach the max number of OTP blocks
     * different devices have different number of OTP blocks
     */
    do {
        /* save current valid block address
         * check for more block on the link list
         */
        valid_addr = next_link_addr;
        next_link_addr = le16_to_cpu(link_value) * sizeof(u16);
        IWL_DEBUG_EEPROM(trans->dev, "OTP blocks %d addr 0x%x\n",
                         usedblocks, next_link_addr);
        if (iwl_read_otp_word(next_link_addr, &link_value))
            return -EINVAL;
        
        if (!link_value) {
            /*
             * reach the end of link list, return success and
             * set address point to the starting address
             * of the image
             */
            *validblockaddr = valid_addr;
            /* skip first 2 bytes (link list pointer) */
            *validblockaddr += 2;
            return 0;
        }
        /* more in the link list, continue */
        usedblocks++;
    } while (usedblocks <= fConfiguration->base_params->max_ll_items);
    
    /* OTP has no valid blocks */
    IWL_DEBUG_EEPROM(trans->dev, "OTP has no valid blocks\n");
    return -EINVAL;
}

void IntelEeprom::iwl_set_otp_access_absolute()
{
    fIO->iwl_read32(CSR_OTP_GP_REG);
    
    fIO->iwl_clear_bit(CSR_OTP_GP_REG,
                       CSR_OTP_GP_REG_OTP_ACCESS_MODE);
}

int IntelEeprom::iwl_read_otp_word(u16 addr, __le16 *eeprom_data)
{
    int ret = 0;
    u32 r;
    u32 otpgp;
    
    fIO->iwl_write32(CSR_EEPROM_REG,
                     CSR_EEPROM_REG_MSK_ADDR & (addr << 1));
    ret = fIO->iwl_poll_bit(CSR_EEPROM_REG,
                            CSR_EEPROM_REG_READ_VALID_MSK,
                            CSR_EEPROM_REG_READ_VALID_MSK,
                            IWL_EEPROM_ACCESS_TIMEOUT);
    if (ret < 0) {
        IWL_ERR(trans, "Time out reading OTP[%d]\n", addr);
        return ret;
    }
    r = fIO->iwl_read32(CSR_EEPROM_REG);
    /* check for ECC errors: */
    otpgp = fIO->iwl_read32(CSR_OTP_GP_REG);
    if (otpgp & CSR_OTP_GP_REG_ECC_UNCORR_STATUS_MSK) {
        /* stop in this case */
        /* set the uncorrectable OTP ECC bit for acknowledgment */
        fIO->iwl_set_bit(CSR_OTP_GP_REG,
                         CSR_OTP_GP_REG_ECC_UNCORR_STATUS_MSK);
        IWL_ERR(trans, "Uncorrectable OTP ECC error, abort OTP read\n");
        return -EINVAL;
    }
    if (otpgp & CSR_OTP_GP_REG_ECC_CORR_STATUS_MSK) {
        /* continue in this case */
        /* set the correctable OTP ECC bit for acknowledgment */
        fIO->iwl_set_bit(CSR_OTP_GP_REG,
                         CSR_OTP_GP_REG_ECC_CORR_STATUS_MSK);
        IWL_ERR(trans, "Correctable OTP ECC error, continue read\n");
    }
    *eeprom_data = cpu_to_le16(r >> 16);
    return 0;
}

int IntelEeprom::iwl_init_otp_access()
{
    int ret;
    
    /* Enable 40MHz radio clock */
    fIO->iwl_write32(CSR_GP_CNTRL, fIO->iwl_read32(CSR_GP_CNTRL) | CSR_GP_CNTRL_REG_FLAG_INIT_DONE);
    
    /* wait for clock to be ready */
    ret = fIO->iwl_poll_bit(CSR_GP_CNTRL,
                            CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                            CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                            25000);
    if (ret < 0) {
        IWL_ERR(trans, "Time out access OTP\n");
    } else {
        fIO->iwl_set_bits_prph(APMG_PS_CTRL_REG,
                               APMG_PS_CTRL_VAL_RESET_REQ);
        IODelay(5);
        fIO->iwl_clear_bits_prph(APMG_PS_CTRL_REG,
                                 APMG_PS_CTRL_VAL_RESET_REQ);
        
        /*
         * CSR auto clock gate disable bit -
         * this is only applicable for HW with OTP shadow RAM
         */
        if (fConfiguration->base_params->shadow_ram_support)
            fIO->iwl_set_bit(CSR_DBG_LINK_PWR_MGMT_REG,
                             CSR_RESET_LINK_PWR_MGMT_DISABLED);
    }
    return ret;
}

int IntelEeprom::iwl_nvm_is_otp() {
    u32 otpgp;
    
    /* OTP only valid for CP/PP and after */
    switch (fHwRev & CSR_HW_REV_TYPE_MSK) {
        case CSR_HW_REV_TYPE_NONE:
            IWL_ERR(trans, "Unknown hardware type\n");
            return -EIO;
        case CSR_HW_REV_TYPE_5300:
        case CSR_HW_REV_TYPE_5350:
        case CSR_HW_REV_TYPE_5100:
        case CSR_HW_REV_TYPE_5150:
            return 0;
        default:
            otpgp = fIO->iwl_read32(CSR_OTP_GP_REG);
            if (otpgp & CSR_OTP_GP_REG_DEVICE_SELECT)
                return 1;
            return 0;
    }
}

int IntelEeprom::iwl_eeprom_verify_signature(bool nvm_is_otp)
{
    u32 gp = fIO->iwl_read32(CSR_EEPROM_GP) & CSR_EEPROM_GP_VALID_MSK;
    
    IWL_DEBUG_EEPROM(trans->dev, "EEPROM signature=0x%08x\n", gp);
    
    switch (gp) {
        case CSR_EEPROM_GP_BAD_SIG_EEP_GOOD_SIG_OTP:
            if (!nvm_is_otp) {
                IWL_ERR(trans->dev, "EEPROM with bad signature: 0x%08x\n",
                        gp);
                return -ENOENT;
            }
            return 0;
        case CSR_EEPROM_GP_GOOD_SIG_EEP_LESS_THAN_4K:
        case CSR_EEPROM_GP_GOOD_SIG_EEP_MORE_THAN_4K:
            if (nvm_is_otp) {
                IWL_ERR(trans, "OTP with bad signature: 0x%08x\n", gp);
                return -ENOENT;
            }
            return 0;
        case CSR_EEPROM_GP_BAD_SIGNATURE_BOTH_EEP_AND_OTP:
        default:
            IWL_ERR(trans,
                    "bad EEPROM/OTP signature, type=%s, EEPROM_GP=0x%08x\n",
                    nvm_is_otp ? "OTP" : "EEPROM", gp);
            return -ENOENT;
    }
}

int IntelEeprom::iwl_eeprom_acquire_semaphore()
{
    UInt16 count;
    int ret = -1;
    
    for (count = 0; count < EEPROM_SEM_RETRY_LIMIT; count++) {
        /* Request semaphore */
        fIO->iwl_set_bit(CSR_HW_IF_CONFIG_REG, CSR_HW_IF_CONFIG_REG_BIT_EEPROM_OWN_SEM);
        
        /* See if we got it */
        ret = fIO->iwl_poll_bit(CSR_HW_IF_CONFIG_REG,
                                CSR_HW_IF_CONFIG_REG_BIT_EEPROM_OWN_SEM,
                                CSR_HW_IF_CONFIG_REG_BIT_EEPROM_OWN_SEM,
                                EEPROM_SEM_TIMEOUT);
        if (ret >= 0) {
            TraceLog("Acquired semaphore after %d tries.\n", count+1);
            return ret;
        }
    }
    
    return ret;
}

void IntelEeprom::iwl_eeprom_release_semaphore()
{
    fIO->iwl_clear_bit(CSR_HW_IF_CONFIG_REG, CSR_HW_IF_CONFIG_REG_BIT_EEPROM_OWN_SEM);
}

//
// MARK: Private. Parsing routines


const UInt8 *IntelEeprom::iwl_eeprom_query_addr(UInt32 offset)
{
    UInt32 address = eeprom_indirect_address(offset);
    
    return &fEeprom[address];
}

UInt32 IntelEeprom::eeprom_indirect_address(UInt32 address)
{
    UInt16 offset = 0;
    
    if ((address & INDIRECT_ADDRESS) == 0)
        return address;
    
    switch (address & INDIRECT_TYPE_MSK) {
        case INDIRECT_HOST:
            offset = iwl_eeprom_query16(EEPROM_LINK_HOST);
            break;
        case INDIRECT_GENERAL:
            offset = iwl_eeprom_query16(EEPROM_LINK_GENERAL);
            break;
        case INDIRECT_REGULATORY:
            offset = iwl_eeprom_query16(EEPROM_LINK_REGULATORY);
            break;
        case INDIRECT_TXP_LIMIT:
            offset = iwl_eeprom_query16(EEPROM_LINK_TXP_LIMIT);
            break;
        case INDIRECT_TXP_LIMIT_SIZE:
            offset = iwl_eeprom_query16(EEPROM_LINK_TXP_LIMIT_SIZE);
            break;
        case INDIRECT_CALIBRATION:
            offset = iwl_eeprom_query16(EEPROM_LINK_CALIBRATION);
            break;
        case INDIRECT_PROCESS_ADJST:
            offset = iwl_eeprom_query16(EEPROM_LINK_PROCESS_ADJST);
            break;
        case INDIRECT_OTHERS:
            offset = iwl_eeprom_query16(EEPROM_LINK_OTHERS);
            break;
        default:
            
            break;
    }
    
    /* translate the offset from words to byte */
    return (address & ADDRESS_MSK) + (offset << 1);
}

UInt16 IntelEeprom::iwl_eeprom_query16(int offset)
{
    return *(UInt16 *)(fEeprom + offset);
}

int IntelEeprom::iwl_eeprom_read_calib(struct iwl_nvm_data *data)
{
    struct iwl_eeprom_calib_hdr *hdr = (struct iwl_eeprom_calib_hdr *)iwl_eeprom_query_addr(EEPROM_CALIB_ALL);
    
    if (!hdr)
        return -ENODATA;
    data->calib_version = hdr->version;
    data->calib_voltage = hdr->voltage;
    
    return 0;
}

void IntelEeprom::iwl_init_sbands(struct iwl_nvm_data *data)
{
    int n_channels = iwl_init_channel_map(data);
    int n_used = 0;
    struct ieee80211_supported_band *sband;

    sband = &data->bands[NL80211_BAND_2GHZ];
    sband->band = NL80211_BAND_2GHZ;
    sband->bitrates = &iwl_cfg80211_rates[RATES_24_OFFS];
    sband->n_bitrates = N_RATES_24;
    n_used += iwl_init_sband_channels(data, sband, n_channels,
                                      NL80211_BAND_2GHZ);
    iwl_init_ht_hw_capab(data, &sband->ht_cap, NL80211_BAND_2GHZ,
                         data->valid_tx_ant, data->valid_rx_ant);

    sband = &data->bands[NL80211_BAND_5GHZ];
    sband->band = NL80211_BAND_5GHZ;
    sband->bitrates = &iwl_cfg80211_rates[RATES_52_OFFS];
    sband->n_bitrates = N_RATES_52;
    n_used += iwl_init_sband_channels(data, sband, n_channels,
                                      NL80211_BAND_5GHZ);
    iwl_init_ht_hw_capab(data, &sband->ht_cap, NL80211_BAND_5GHZ,
                         data->valid_tx_ant, data->valid_rx_ant);

    if (n_channels != n_used)
        IWL_ERR_DEV(dev, "EEPROM: used only %d of %d channels\n",
                    n_used, n_channels);
}

int IntelEeprom::iwl_init_channel_map(struct iwl_nvm_data *data)
{
    int band, ch_idx;
    const struct iwl_eeprom_channel *eeprom_ch_info;
    const u8 *eeprom_ch_array;
    int eeprom_ch_count;
    int n_channels = 0;
    
    /*
     * Loop through the 5 EEPROM bands and add them to the parse list
     */
    for (band = 1; band <= 5; band++) {
        struct ieee80211_channel *channel;
        
        iwl_init_band_reference(band,
                                &eeprom_ch_count, &eeprom_ch_info,
                                &eeprom_ch_array);
        
        /* Loop through each band adding each of the channels */
        for (ch_idx = 0; ch_idx < eeprom_ch_count; ch_idx++) {
            const struct iwl_eeprom_channel *eeprom_ch;
            
            eeprom_ch = &eeprom_ch_info[ch_idx];
            
            if (!(eeprom_ch->flags & EEPROM_CHANNEL_VALID)) {
                IWL_DEBUG_EEPROM(dev,
                                 "Ch. %d Flags %x [%sGHz] - No traffic\n",
                                 eeprom_ch_array[ch_idx],
                                 eeprom_ch_info[ch_idx].flags,
                                 (band != 1) ? "5.2" : "2.4");
                continue;
            }
            
            channel = &data->channels[n_channels];
            n_channels++;
            
            channel->hw_value = eeprom_ch_array[ch_idx];
            channel->band = (band == 1) ? NL80211_BAND_2GHZ
            : NL80211_BAND_5GHZ;
            channel->center_freq =
            ieee80211_channel_to_frequency(
                                           channel->hw_value, channel->band);
            

        }
    }
    
    
    if (fConfiguration->eeprom_params->enhanced_txpower) {
        /*
         * for newer device (6000 series and up)
         * EEPROM contain enhanced tx power information
         * driver need to process addition information
         * to determine the max channel tx power limits
         */
        iwl_eeprom_enhanced_txpower(data, n_channels);
    } else {
        /* All others use data from channel map */
        int i;
        
        data->max_tx_pwr_half_dbm = -128;
        
        for (i = 0; i < n_channels; i++)
            data->max_tx_pwr_half_dbm =
                max(data->max_tx_pwr_half_dbm,
                    data->channels[i].max_power * 2);

    }
    
    /* Check if we do have HT40 channels */
    if (fConfiguration->eeprom_params->regulatory_bands[5] ==
        EEPROM_REGULATORY_BAND_NO_HT40 &&
        fConfiguration->eeprom_params->regulatory_bands[6] ==
        EEPROM_REGULATORY_BAND_NO_HT40)
        return n_channels;
    
    /* Two additional EEPROM bands for 2.4 and 5 GHz HT40 channels */
    for (band = 6; band <= 7; band++) {
        enum nl80211_band ieeeband;
        
        iwl_init_band_reference(band,
                                &eeprom_ch_count, &eeprom_ch_info,
                                &eeprom_ch_array);
        
        /* EEPROM band 6 is 2.4, band 7 is 5 GHz */
        ieeeband = (band == 6) ? NL80211_BAND_2GHZ
        : NL80211_BAND_5GHZ;
        
        /* Loop through each band adding each of the channels */
        for (ch_idx = 0; ch_idx < eeprom_ch_count; ch_idx++) {
            /* Set up driver's info for lower half */
            iwl_mod_ht40_chan_info(data, n_channels, ieeeband,
                                   eeprom_ch_array[ch_idx],
                                   &eeprom_ch_info[ch_idx],
                                   IEEE80211_CHAN_NO_HT40PLUS);
            
            /* Set up driver's info for upper half */
            iwl_mod_ht40_chan_info(data, n_channels, ieeeband,
                                   eeprom_ch_array[ch_idx] + 4,
                                   &eeprom_ch_info[ch_idx],
                                   IEEE80211_CHAN_NO_HT40MINUS);
        }
    }

    
    return n_channels;
}

int IntelEeprom::iwl_init_sband_channels(struct iwl_nvm_data *data,
                            struct ieee80211_supported_band *sband,
                            int n_channels, enum nl80211_band band)
{
    struct ieee80211_channel *chan = &data->channels[0];
    int n = 0, idx = 0;
    
    while (idx < n_channels && chan->band != band)
        chan = &data->channels[++idx];
    
    sband->channels = &data->channels[idx];
    
    while (idx < n_channels && chan->band == band) {
        chan = &data->channels[++idx];
        n++;
    }
    
    sband->n_channels = n;
    
    return n;
}

#define MAX_BIT_RATE_40_MHZ    150 /* Mbps */
#define MAX_BIT_RATE_20_MHZ    72 /* Mbps */

void IntelEeprom::iwl_init_ht_hw_capab(struct iwl_nvm_data *data,
                          struct ieee80211_sta_ht_cap *ht_info,
                          enum nl80211_band band,
                          u8 tx_chains, u8 rx_chains)
{
    int max_bit_rate = 0;
    
    tx_chains = hweight8(tx_chains);
    if (fConfiguration->rx_with_siso_diversity)
        rx_chains = 1;
    else
        rx_chains = hweight8(rx_chains);
    
    if (!(data->sku_cap_11n_enable) || !fConfiguration->ht_params) {
        ht_info->ht_supported = false;
        return;
    }
    
    if (data->sku_cap_mimo_disabled)
        rx_chains = 1;
    
    ht_info->ht_supported = true;
    ht_info->cap = IEEE80211_HT_CAP_DSSSCCK40;
    
    if (fConfiguration->ht_params->stbc) {
        ht_info->cap |= (1 << IEEE80211_HT_CAP_RX_STBC_SHIFT);
        
        if (tx_chains > 1)
            ht_info->cap |= IEEE80211_HT_CAP_TX_STBC;
    }
    
    if (fConfiguration->ht_params->ldpc)
        ht_info->cap |= IEEE80211_HT_CAP_LDPC_CODING;
    
    if ((fConfiguration->mq_rx_supported &&
         iwlwifi_mod_params.amsdu_size != IWL_AMSDU_4K) ||
        iwlwifi_mod_params.amsdu_size >= IWL_AMSDU_8K)
        ht_info->cap |= IEEE80211_HT_CAP_MAX_AMSDU;
    
    ht_info->ampdu_factor = fConfiguration->max_ht_ampdu_exponent;
    ht_info->ampdu_density = IEEE80211_HT_MPDU_DENSITY_4;
    
    ht_info->mcs.rx_mask[0] = 0xFF;
    if (rx_chains >= 2)
        ht_info->mcs.rx_mask[1] = 0xFF;
    if (rx_chains >= 3)
        ht_info->mcs.rx_mask[2] = 0xFF;
    
    if (fConfiguration->ht_params->ht_greenfield_support)
        ht_info->cap |= IEEE80211_HT_CAP_GRN_FLD;
    ht_info->cap |= IEEE80211_HT_CAP_SGI_20;
    
    max_bit_rate = MAX_BIT_RATE_20_MHZ;
    
    if (fConfiguration->ht_params->ht40_bands & BIT(band)) {
        ht_info->cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
        ht_info->cap |= IEEE80211_HT_CAP_SGI_40;
        max_bit_rate = MAX_BIT_RATE_40_MHZ;
    }
    
    /* Highest supported Rx data rate */
    max_bit_rate *= rx_chains;
//    WARN_ON(max_bit_rate & ~IEEE80211_HT_MCS_RX_HIGHEST_MASK);
    ht_info->mcs.rx_highest = cpu_to_le16(max_bit_rate);
    
    /* Tx MCS capabilities */
    ht_info->mcs.tx_params = IEEE80211_HT_MCS_TX_DEFINED;
    if (tx_chains != rx_chains) {
        ht_info->mcs.tx_params |= IEEE80211_HT_MCS_TX_RX_DIFF;
        ht_info->mcs.tx_params |= ((tx_chains - 1) <<
                                   IEEE80211_HT_MCS_TX_MAX_STREAMS_SHIFT);
    }
}

void IntelEeprom::iwl_init_band_reference(int eeprom_band, int *eeprom_ch_count,
                                    const struct iwl_eeprom_channel **ch_info,
                                    const u8 **eeprom_ch_array)
{
    u32 offset = fConfiguration->eeprom_params->regulatory_bands[eeprom_band - 1];
    
    offset |= INDIRECT_ADDRESS | INDIRECT_REGULATORY;
    
    *ch_info = (struct iwl_eeprom_channel *)iwl_eeprom_query_addr(offset);
    
    switch (eeprom_band) {
        case 1:        /* 2.4GHz band */
            *eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_1);
            *eeprom_ch_array = iwl_eeprom_band_1;
            break;
        case 2:        /* 4.9GHz band */
            *eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_2);
            *eeprom_ch_array = iwl_eeprom_band_2;
            break;
        case 3:        /* 5.2GHz band */
            *eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_3);
            *eeprom_ch_array = iwl_eeprom_band_3;
            break;
        case 4:        /* 5.5GHz band */
            *eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_4);
            *eeprom_ch_array = iwl_eeprom_band_4;
            break;
        case 5:        /* 5.7GHz band */
            *eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_5);
            *eeprom_ch_array = iwl_eeprom_band_5;
            break;
        case 6:        /* 2.4GHz ht40 channels */
            *eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_6);
            *eeprom_ch_array = iwl_eeprom_band_6;
            break;
        case 7:        /* 5 GHz ht40 channels */
            *eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_7);
            *eeprom_ch_array = iwl_eeprom_band_7;
            break;
        default:
            *eeprom_ch_count = 0;
            *eeprom_ch_array = NULL;
//            WARN_ON(1);
    }
}

#define EEPROM_TXP_OFFS    (0x00 | INDIRECT_ADDRESS | INDIRECT_TXP_LIMIT)
#define EEPROM_TXP_ENTRY_LEN sizeof(struct iwl_eeprom_enhanced_txpwr)
#define EEPROM_TXP_SZ_OFFS (0x00 | INDIRECT_ADDRESS | INDIRECT_TXP_LIMIT_SIZE)

#define TXP_CHECK_AND_PRINT(x) \
((txp->flags & IWL_EEPROM_ENH_TXP_FL_##x) ? # x " " : "")


void IntelEeprom::iwl_eeprom_enhanced_txpower(struct iwl_nvm_data *data,
                                        int n_channels)
{
    struct iwl_eeprom_enhanced_txpwr *txp_array, *txp;
    int idx, entries;
    __le16 *txp_len;
    s8 max_txp_avg_halfdbm;
    
//    BUILD_BUG_ON(sizeof(struct iwl_eeprom_enhanced_txpwr) != 8);
    
    /* the length is in 16-bit words, but we want entries */
    txp_len = (__le16 *)iwl_eeprom_query_addr(EEPROM_TXP_SZ_OFFS);
    entries = le16_to_cpup(txp_len) * 2 / EEPROM_TXP_ENTRY_LEN;
    
    txp_array = (struct iwl_eeprom_enhanced_txpwr *)iwl_eeprom_query_addr(EEPROM_TXP_OFFS);
    
    for (idx = 0; idx < entries; idx++) {
        txp = &txp_array[idx];
        /* skip invalid entries */
        if (!(txp->flags & IWL_EEPROM_ENH_TXP_FL_VALID))
            continue;
        
        IWL_DEBUG_EEPROM(dev, "%s %d:\t %s%s%s%s%s%s%s%s (0x%02x)\n",
                         (txp->channel && (txp->flags &
                                           IWL_EEPROM_ENH_TXP_FL_COMMON_TYPE)) ?
                         "Common " : (txp->channel) ?
                         "Channel" : "Common",
                         (txp->channel),
                         TXP_CHECK_AND_PRINT(VALID),
                         TXP_CHECK_AND_PRINT(BAND_52G),
                         TXP_CHECK_AND_PRINT(OFDM),
                         TXP_CHECK_AND_PRINT(40MHZ),
                         TXP_CHECK_AND_PRINT(HT_AP),
                         TXP_CHECK_AND_PRINT(RES1),
                         TXP_CHECK_AND_PRINT(RES2),
                         TXP_CHECK_AND_PRINT(COMMON_TYPE),
                         txp->flags);
        IWL_DEBUG_EEPROM(dev,
                         "\t\t chain_A: %d chain_B: %d chain_C: %d\n",
                         txp->chain_a_max, txp->chain_b_max,
                         txp->chain_c_max);
        IWL_DEBUG_EEPROM(dev,
                         "\t\t MIMO2: %d MIMO3: %d High 20_on_40: 0x%02x Low 20_on_40: 0x%02x\n",
                         txp->mimo2_max, txp->mimo3_max,
                         ((txp->delta_20_in_40 & 0xf0) >> 4),
                         (txp->delta_20_in_40 & 0x0f));
        
        max_txp_avg_halfdbm = iwl_get_max_txpwr_half_dbm(data, txp);
        
        iwl_eeprom_enh_txp_read_element(data, txp, n_channels,
                                        DIV_ROUND_UP(max_txp_avg_halfdbm, 2));
        
        if (max_txp_avg_halfdbm > data->max_tx_pwr_half_dbm)
            data->max_tx_pwr_half_dbm = max_txp_avg_halfdbm;
    }
}

s8 IntelEeprom::iwl_get_max_txpwr_half_dbm(const struct iwl_nvm_data *data,
                                     struct iwl_eeprom_enhanced_txpwr *txp)
{
    s8 result = 0; /* (.5 dBm) */
    
    /* Take the highest tx power from any valid chains */
    if (data->valid_tx_ant & ANT_A && txp->chain_a_max > result)
        result = txp->chain_a_max;
    
    if (data->valid_tx_ant & ANT_B && txp->chain_b_max > result)
        result = txp->chain_b_max;
    
    if (data->valid_tx_ant & ANT_C && txp->chain_c_max > result)
        result = txp->chain_c_max;
    
    if ((data->valid_tx_ant == ANT_AB ||
         data->valid_tx_ant == ANT_BC ||
         data->valid_tx_ant == ANT_AC) && txp->mimo2_max > result)
        result = txp->mimo2_max;
    
    if (data->valid_tx_ant == ANT_ABC && txp->mimo3_max > result)
        result = txp->mimo3_max;
    
    return result;
}

void IntelEeprom::iwl_eeprom_enh_txp_read_element(struct iwl_nvm_data *data,
                                struct iwl_eeprom_enhanced_txpwr *txp,
                                int n_channels, s8 max_txpower_avg)
{
    int ch_idx;
    enum nl80211_band band;
    
    band = txp->flags & IWL_EEPROM_ENH_TXP_FL_BAND_52G ?
    NL80211_BAND_5GHZ : NL80211_BAND_2GHZ;
    
    for (ch_idx = 0; ch_idx < n_channels; ch_idx++) {
        struct ieee80211_channel *chan = &data->channels[ch_idx];
        
        /* update matching channel or from common data only */
        if (txp->channel != 0 && chan->hw_value != txp->channel)
            continue;
        
        /* update matching band only */
        if (band != chan->band)
            continue;
        
        if (chan->max_power < max_txpower_avg &&
            !(txp->flags & IWL_EEPROM_ENH_TXP_FL_40MHZ))
            chan->max_power = max_txpower_avg;
    }
}

#define CHECK_AND_PRINT(x) \
((eeprom_ch->flags & EEPROM_CHANNEL_##x) ? # x " " : "")

void IntelEeprom::iwl_mod_ht40_chan_info(struct iwl_nvm_data *data, int n_channels,
                                   enum nl80211_band band, u16 channel,
                                   const struct iwl_eeprom_channel *eeprom_ch,
                                   u8 clear_ht40_extension_channel)
{
    struct ieee80211_channel *chan = NULL;
    int i;
    
    for (i = 0; i < n_channels; i++) {
        if (data->channels[i].band != band)
            continue;
        if (data->channels[i].hw_value != channel)
            continue;
        chan = &data->channels[i];
        break;
    }
    
    if (!chan)
        return;
    
    IWL_DEBUG_EEPROM(dev,
                     "HT40 Ch. %d [%sGHz] %s%s%s%s%s(0x%02x %ddBm): Ad-Hoc %ssupported\n",
                     channel,
                     band == NL80211_BAND_5GHZ ? "5.2" : "2.4",
                     CHECK_AND_PRINT(IBSS),
                     CHECK_AND_PRINT(ACTIVE),
                     CHECK_AND_PRINT(RADAR),
                     CHECK_AND_PRINT(WIDE),
                     CHECK_AND_PRINT(DFS),
                     eeprom_ch->flags,
                     eeprom_ch->max_power_avg,
                     ((eeprom_ch->flags & EEPROM_CHANNEL_IBSS) &&
                      !(eeprom_ch->flags & EEPROM_CHANNEL_RADAR)) ? ""
                     : "not ");
    
    if (eeprom_ch->flags & EEPROM_CHANNEL_VALID)
        chan->flags &= ~clear_ht40_extension_channel;
}





