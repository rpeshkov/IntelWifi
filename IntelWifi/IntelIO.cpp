//
//  IntelIO.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 28/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#include "IntelIO.hpp"

#include <sys/errno.h>

#include "iwl-csr.h"
#include "Logging.h"

// MARK: Defines

#define IWL_POLL_INTERVAL 10    /* microseconds */


// MARK: Implementation
#define super OSObject
OSDefineMetaClassAndStructors( IntelIO, OSObject )

IntelIO* IntelIO::withTrans(struct iwl_trans_pcie* trans) {
    IntelIO *io = new IntelIO;
    
    if (io && !io->initWithTrans(trans)) {
        io->release();
        return NULL;
    }
    
    return io;
}

bool IntelIO::initWithTrans(struct iwl_trans_pcie* trans) {
    if (!super::init()) {
        return false;
    }
    
    fTrans = trans;
    
    return true;
}

void IntelIO::free() {
    fTrans = NULL;
    super::free();
}

int IntelIO::iwl_poll_bit(UInt32 addr, UInt32 bits, UInt32 mask, int timeout) {
    int t = 0;
    do {
        if ((iwl_read32(addr) & mask) == (bits & mask)) {
            return t;
        }
        
        IODelay(IWL_POLL_INTERVAL);
        t += IWL_POLL_INTERVAL;
    } while (t < timeout);
    
    return -ETIMEDOUT;
}


void IntelIO::iwl_write32(UInt32 ofs, UInt32 val) {
    OSWriteLittleInt32(fTrans->hw_base, ofs, val);
}

UInt32 IntelIO::iwl_read32(UInt32 ofs) {
    return OSReadLittleInt32(fTrans->hw_base, ofs);
}

void IntelIO::iwl_set_bit(UInt32 reg, UInt32 mask)
{
    iwl_trans_pcie_set_bits_mask(reg, mask, mask);
}

void IntelIO::iwl_clear_bit(UInt32 reg, UInt32 mask)
{
    iwl_trans_pcie_set_bits_mask(reg, mask, 0);
}

void IntelIO::iwl_trans_pcie_set_bits_mask(UInt32 reg, UInt32 mask, UInt32 value)
{
    IOInterruptState state = IOSimpleLockLockDisableInterrupt(fTrans->reg_lock);
    __iwl_trans_pcie_set_bits_mask(reg, mask, value);
    IOSimpleLockUnlockEnableInterrupt(fTrans->reg_lock, state);
}

void IntelIO::__iwl_trans_pcie_set_bits_mask(UInt32 reg, UInt32 mask, UInt32 value)
{
    UInt32 v = iwl_read32(reg);
    v &= ~mask;
    v |= value;
    iwl_write32(reg, v);
}

bool IntelIO::iwl_grab_nic_access(IOInterruptState *state) {
    *state = IOSimpleLockLockDisableInterrupt(fTrans->reg_lock);
    
    if (fTrans->cmd_hold_nic_awake) {
        return true;
    }

    /* this bit wakes up the NIC */
    __iwl_trans_pcie_set_bit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);

    // TODO: Implement
//    if (trans->cfg->device_family >= IWL_DEVICE_FAMILY_8000)
//        udelay(2);
    
    /*
     * These bits say the device is running, and should keep running for
     * at least a short while (at least as long as MAC_ACCESS_REQ stays 1),
     * but they do not indicate that embedded SRAM is restored yet;
     * HW with volatile SRAM must save/restore contents to/from
     * host DRAM when sleeping/waking for power-saving.
     * Each direction takes approximately 1/4 millisecond; with this
     * overhead, it's a good idea to grab and hold MAC_ACCESS_REQUEST if a
     * series of register accesses are expected (e.g. reading Event Log),
     * to keep device from sleeping.
     *
     * CSR_UCODE_DRV_GP1 register bit MAC_SLEEP == 0 indicates that
     * SRAM is okay/restored.  We don't check that here because this call
     * is just for hardware register access; but GP1 MAC_SLEEP
     * check is a good idea before accessing the SRAM of HW with
     * volatile SRAM (e.g. reading Event Log).
     *
     * 5000 series and later (including 1000 series) have non-volatile SRAM,
     * and do not save/restore SRAM when power cycling.
     */
    int ret = iwl_poll_bit(CSR_GP_CNTRL,
                       CSR_GP_CNTRL_REG_VAL_MAC_ACCESS_EN,
                       (CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY |
                        CSR_GP_CNTRL_REG_FLAG_GOING_TO_SLEEP), 15000);
    if (ret < 0) {
        iwl_write32(CSR_RESET, CSR_RESET_REG_FLAG_FORCE_NMI);
        
        IWL_WARN(0, "Timeout waiting for hardware access (CSR_GP_CNTRL 0x%08x)\n",
                  iwl_read32(CSR_GP_CNTRL));
        
        IOSimpleLockUnlockEnableInterrupt(fTrans->reg_lock, *state);
        return false;
    }
    
    return true;
}

void IntelIO::iwl_release_nic_access(IOInterruptState *state) {
    if (fTrans->cmd_hold_nic_awake) {
        IOSimpleLockUnlockEnableInterrupt(fTrans->reg_lock, *state);
        return;
    }
    
    __iwl_trans_pcie_clear_bit(CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
    
    /*
     * Above we read the CSR_GP_CNTRL register, which will flush
     * any previous writes, but we need the write that clears the
     * MAC_ACCESS_REQ bit to be performed before any other writes
     * scheduled on different CPUs (after we drop reg_lock).
     */
    // original linux code: mmiowb();
    os_compiler_barrier();

    IOSimpleLockUnlockEnableInterrupt(fTrans->reg_lock, *state);
}

void IntelIO::__iwl_trans_pcie_clear_bit(UInt32 reg, UInt32 mask)
{
    __iwl_trans_pcie_set_bits_mask(reg, mask, 0);
}

void IntelIO::__iwl_trans_pcie_set_bit(UInt32 reg, UInt32 mask)
{
    __iwl_trans_pcie_set_bits_mask(reg, mask, mask);
}

UInt32 IntelIO::iwl_read_prph_no_grab(UInt32 reg) { 
    iwl_write32(HBUS_TARG_PRPH_RADDR, ((reg & 0x000FFFFF) | (3 << 24)));
    return iwl_read32(HBUS_TARG_PRPH_RDAT);
}

void IntelIO::iwl_write_prph_no_grab(UInt32 addr, UInt32 val) { 
    iwl_write32(HBUS_TARG_PRPH_WADDR, ((addr & 0x000FFFFF) | (3 << 24)));
    iwl_write32(HBUS_TARG_PRPH_WDAT, val);
}

UInt32 IntelIO::iwl_read_prph(UInt32 ofs)
{
    IOInterruptState state;
    UInt32 val = 0x5a5a5a5a;
    
    if (iwl_grab_nic_access(&state)) {
        val = iwl_read_prph_no_grab(ofs);
        iwl_release_nic_access(&state);
    }
    
    return val;
}


void IntelIO::iwl_write_prph(UInt32 ofs, UInt32 val)
{
    IOInterruptState state;
    
    if (iwl_grab_nic_access(&state)) {
        iwl_write_prph_no_grab(ofs, val);
        iwl_release_nic_access(&state);
    }
}


int IntelIO::iwl_poll_prph_bit(UInt32 addr, UInt32 bits, UInt32 mask, int timeout)
{
    int t = 0;
    
    do {
        if ((iwl_read_prph(addr) & mask) == (bits & mask))
            return t;
        IODelay(IWL_POLL_INTERVAL);
        t += IWL_POLL_INTERVAL;
    } while (t < timeout);
    
    return -ETIMEDOUT;
}

void IntelIO::iwl_set_bits_prph(UInt32 ofs, UInt32 mask)
{
    IOInterruptState state;
    
    if (iwl_grab_nic_access(&state)) {
        iwl_write_prph_no_grab(ofs, iwl_read_prph_no_grab(ofs) | mask);
        iwl_release_nic_access(&state);
    }
}

void IntelIO::iwl_set_bits_mask_prph(UInt32 ofs, UInt32 bits, UInt32 mask)
{
    IOInterruptState state;
    
    if (iwl_grab_nic_access(&state)) {
        iwl_write_prph_no_grab(ofs, (iwl_read_prph_no_grab(ofs) & mask) | bits);
        iwl_release_nic_access(&state);
    }
}

void IntelIO::iwl_clear_bits_prph(UInt32 ofs, UInt32 mask)
{
    IOInterruptState state;
    
    if (iwl_grab_nic_access(&state)) {
        UInt32 val = iwl_read_prph_no_grab(ofs);
        iwl_write_prph_no_grab(ofs, (val & ~mask));
        iwl_release_nic_access(&state);
    }
}

u32 IntelIO::iwl_trans_pcie_read_shr(u32 reg)
{
    iwl_write32(HEEP_CTRL_WRD_PCIEX_CTRL_REG,
                ((reg & 0x0000ffff) | (2 << 28)));
    return iwl_read32(HEEP_CTRL_WRD_PCIEX_DATA_REG);
}

void IntelIO::iwl_trans_pcie_write_shr(u32 reg, u32 val)
{
    iwl_write32(HEEP_CTRL_WRD_PCIEX_DATA_REG, val);
    iwl_write32(HEEP_CTRL_WRD_PCIEX_CTRL_REG,
                ((reg & 0x0000ffff) | (3 << 28)));
}

int IntelIO::iwl_trans_pcie_read_mem(u32 addr, void *buf, int dwords)
{
    IOInterruptState state;
    int offs, ret = 0;
    u32 *vals = (u32*)buf;
    
    if (iwl_grab_nic_access(&state)) {
        iwl_write32(HBUS_TARG_MEM_RADDR, addr);
        for (offs = 0; offs < dwords; offs++)
            vals[offs] = iwl_read32(HBUS_TARG_MEM_RDAT);
        iwl_release_nic_access(&state);
    } else {
        ret = -EBUSY;
    }
    return ret;
}

int IntelIO::iwl_trans_pcie_write_mem(u32 addr, const void *buf, int dwords)
{
    IOInterruptState state;
    int offs, ret = 0;
    const u32 *vals = (u32*)buf;
    
    if (iwl_grab_nic_access(&state)) {
        iwl_write32(HBUS_TARG_MEM_WADDR, addr);
        for (offs = 0; offs < dwords; offs++)
            iwl_write32(HBUS_TARG_MEM_WDAT,
                        vals ? vals[offs] : 0);
        iwl_release_nic_access(&state);
    } else {
        ret = -EBUSY;
    }
    return ret;
}





