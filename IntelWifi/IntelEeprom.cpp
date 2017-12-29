//
//  IntelEeprom.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 27/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#include "IntelEeprom.hpp"

#include "iwlwifi/iwl-csr.h"
#include "iwl-prph.h"
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
        IOFree(fEeprom, OTP_LOW_IMAGE_SIZE);
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
//    if (iwlwifi_mod_params.disable_11n & IWL_DISABLE_HT_ALL)
//        data->sku_cap_11n_enable = false;
    
    return nvmData;

}

// MARK: Private

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
