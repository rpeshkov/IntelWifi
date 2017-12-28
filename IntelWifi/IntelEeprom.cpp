//
//  IntelEeprom.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 27/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#include "IntelEeprom.hpp"

#include "iwlwifi/iwl-csr.h"

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



IntelEeprom* IntelEeprom::withAddress(volatile void * p) {
    IntelEeprom *eeprom = new IntelEeprom;
    
    if (eeprom && !eeprom->initWithAddress(p)) {
        eeprom->release();
        return 0;
    }
    
    return eeprom;
}


bool IntelEeprom::initWithAddress(volatile void * p) {
    if (!super::init()) {
        return false;
    }
    
    lock = IOLockAlloc();
    
    baseHwAddr = p;
    
    UInt16 *e = (UInt16*)IOMalloc(OTP_LOW_IMAGE_SIZE);
    
    int ret = iwl_eeprom_acquire_semaphore();
    if (ret < 0) {
        TraceLog("Failed to acquire EEPROM semaphore.\n");
        IOFree(e, OTP_LOW_IMAGE_SIZE);
        return false;
    }


    
    for (UInt16 a = 0; a < OTP_LOW_IMAGE_SIZE; a += sizeof(UInt16)) {
        iwl_write32(CSR_EEPROM_REG, CSR_EEPROM_REG_MSK_ADDR & (a << 1));
        
        int ret = iwl_poll_bit(CSR_EEPROM_REG,
                               CSR_EEPROM_REG_READ_VALID_MSK,
                               CSR_EEPROM_REG_READ_VALID_MSK,
                               5000);
        
        if (ret < 0) {
            return false;
        }
        
        
        UInt32 r = iwl_read32(CSR_EEPROM_REG);
        
        e[a/2] = (r >> 16);
    }
    
    iwl_eeprom_release_semaphore();
    
    fEeprom = (UInt8 *)e;
    
    
    
    return true;
}

void IntelEeprom::release() {
    if (lock) {
        IOLockFree(lock);
        lock = NULL;
    }
    
    if (fEeprom) {
        IOFree(fEeprom, OTP_LOW_IMAGE_SIZE);
        fEeprom = NULL;
    }
    
    
    
    super::release();
}

struct iwl_nvm_data* IntelEeprom::parse() {
    struct iwl_nvm_data* nvmData = (struct iwl_nvm_data*)IOMalloc(sizeof(struct iwl_nvm_data));
    if (!nvmData) {
        return NULL;
    }
    
    const void *tmp = iwl_eeprom_query_addr(EEPROM_MAC_ADDRESS);;
    
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

UInt32 IntelEeprom::getHardwareRevisionId()  { return iwl_read32(CSR_HW_REV); }

// MARK: Private
int IntelEeprom::iwl_poll_bit(UInt32 addr, UInt32 bits, UInt32 mask, int timeout) {
    int t = 0;
    do {
        if ((OSReadLittleInt32(baseHwAddr, addr) & mask) == (bits & mask)) {
            return t;
        }
        
        IODelay(IWL_POLL_INTERVAL);
        t += IWL_POLL_INTERVAL;
    } while (t < timeout);
    
    return -ETIMEDOUT;
}

void IntelEeprom::iwl_write32(UInt32 ofs, UInt32 val) {
    OSWriteLittleInt32(baseHwAddr, ofs, val);
}

UInt32 IntelEeprom::iwl_read32(UInt32 ofs) {
    return OSReadLittleInt32(baseHwAddr, ofs);
}

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
    int ret;
    
    for (count = 0; count < EEPROM_SEM_RETRY_LIMIT; count++) {
        /* Request semaphore */
        iwl_set_bit(CSR_HW_IF_CONFIG_REG, CSR_HW_IF_CONFIG_REG_BIT_EEPROM_OWN_SEM);
        
        /* See if we got it */
        ret = iwl_poll_bit(CSR_HW_IF_CONFIG_REG,
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
    iwl_clear_bit(CSR_HW_IF_CONFIG_REG, CSR_HW_IF_CONFIG_REG_BIT_EEPROM_OWN_SEM);
}

void IntelEeprom::iwl_set_bit(UInt32 reg, UInt32 mask)
{
    iwl_trans_pcie_set_bits_mask(reg, mask, mask);
}

void IntelEeprom::iwl_clear_bit(UInt32 reg, UInt32 mask)
{
    iwl_trans_pcie_set_bits_mask(reg, mask, 0);
}

void IntelEeprom::iwl_trans_pcie_set_bits_mask(UInt32 reg, UInt32 mask, UInt32 value)
{
    //spin_lock_irqsave(&trans_pcie->reg_lock, flags);
    IOLockLock(lock);
    __iwl_trans_pcie_set_bits_mask(reg, mask, value);
    //spin_unlock_irqrestore(&trans_pcie->reg_lock, flags);
    IOLockUnlock(lock);
    
}

void IntelEeprom::__iwl_trans_pcie_set_bits_mask(UInt32 reg, UInt32 mask, UInt32 value)
{
    UInt32 v;
    
    v = iwl_read32(reg);
    v &= ~mask;
    v |= value;
    iwl_write32(reg, v);
}


