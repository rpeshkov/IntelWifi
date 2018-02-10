/******************************************************************************
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2007 - 2015 Intel Corporation. All rights reserved.
 * Copyright(c) 2013 - 2015 Intel Mobile Communications GmbH
 * Copyright(c) 2016 - 2017 Intel Deutschland GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110,
 * USA
 *
 * The full GNU General Public License is included in this distribution
 * in the file called COPYING.
 *
 * Contact Information:
 *  Intel Linux Wireless <linuxwifi@intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 * BSD LICENSE
 *
 * Copyright(c) 2005 - 2015 Intel Corporation. All rights reserved.
 * Copyright(c) 2013 - 2015 Intel Mobile Communications GmbH
 * Copyright(c) 2016 - 2017 Intel Deutschland GmbH
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

//
//  trans.c
//  IntelWifi
//
//  Created by Roman Peshkov on 25/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "iwl-trans.h"
#include "iwl-csr.h"

#include "internal.h"

OS_INLINE void _OSWriteInt8(volatile void* base, uintptr_t byteOffset, uint8_t data)
{
    *(volatile uint8_t *)((uintptr_t)base + byteOffset) = data;
}

static void iwl_trans_pcie_write8(struct iwl_trans *trans, u32 ofs, u8 val)
{
    _OSWriteInt8(IWL_TRANS_GET_PCIE_TRANS(trans)->hw_base, ofs, val);
}

static void iwl_trans_pcie_write32(struct iwl_trans *trans, u32 ofs, u32 val)
{
    OSWriteLittleInt32(IWL_TRANS_GET_PCIE_TRANS(trans)->hw_base, ofs, val);
}

static u32 iwl_trans_pcie_read32(struct iwl_trans *trans, u32 ofs)
{
    return OSReadLittleInt32(IWL_TRANS_GET_PCIE_TRANS(trans)->hw_base, ofs);
}

static u32 iwl_trans_pcie_read_prph(struct iwl_trans *trans, u32 reg)
{
    iwl_trans_pcie_write32(trans, HBUS_TARG_PRPH_RADDR, ((reg & 0x000FFFFF) | (3 << 24)));
    return iwl_trans_pcie_read32(trans, HBUS_TARG_PRPH_RDAT);
}

static void iwl_trans_pcie_write_prph(struct iwl_trans *trans, u32 addr, u32 val)
{
    iwl_trans_pcie_write32(trans, HBUS_TARG_PRPH_WADDR, ((addr & 0x000FFFFF) | (3 << 24)));
    iwl_trans_pcie_write32(trans, HBUS_TARG_PRPH_WDAT, val);
}

static bool iwl_trans_pcie_grab_nic_access(struct iwl_trans *trans, IOInterruptState *state) {
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    *state = IOSimpleLockLockDisableInterrupt(trans_pcie->reg_lock);
    
    if (trans_pcie->cmd_hold_nic_awake) {
        return true;
    }
    
    /* this bit wakes up the NIC */
    __iwl_trans_pcie_set_bit(trans, CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
    
    
    if (trans->cfg->device_family >= IWL_DEVICE_FAMILY_8000)
        IODelay(2);
    
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
    int ret = iwl_poll_bit(trans, CSR_GP_CNTRL,
                           CSR_GP_CNTRL_REG_VAL_MAC_ACCESS_EN,
                           (CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY |
                            CSR_GP_CNTRL_REG_FLAG_GOING_TO_SLEEP), 15000);
    if (ret < 0) {
        iwl_write32(trans, CSR_RESET, CSR_RESET_REG_FLAG_FORCE_NMI);
        
        IWL_WARN(0, "Timeout waiting for hardware access (CSR_GP_CNTRL 0x%08x)\n",
                 iwl_read32(trans, CSR_GP_CNTRL));
        
        IOSimpleLockUnlockEnableInterrupt(trans_pcie->reg_lock, *state);
        return false;
    }
    
    return true;
}

static void iwl_trans_pcie_release_nic_access(struct iwl_trans *trans, IOInterruptState *state) {
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    if (trans_pcie->cmd_hold_nic_awake) {
        goto out;
    }
    
    __iwl_trans_pcie_clear_bit(trans, CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ);
    
    /*
     * Above we read the CSR_GP_CNTRL register, which will flush
     * any previous writes, but we need the write that clears the
     * MAC_ACCESS_REQ bit to be performed before any other writes
     * scheduled on different CPUs (after we drop reg_lock).
     */
    // original linux code: mmiowb();
    os_compiler_barrier();
    
out:
    
    IOSimpleLockUnlockEnableInterrupt(trans_pcie->reg_lock, *state);
}

static int iwl_trans_pcie_read_mem(struct iwl_trans *trans, u32 addr,
                                   void *buf, int dwords)
{
    IOInterruptState flags;
    int offs, ret = 0;
    u32 *vals = buf;
    
    if (iwl_trans_grab_nic_access(trans, &flags)) {
        iwl_write32(trans, HBUS_TARG_MEM_RADDR, addr);
        for (offs = 0; offs < dwords; offs++)
            vals[offs] = iwl_read32(trans, HBUS_TARG_MEM_RDAT);
        iwl_trans_release_nic_access(trans, &flags);
    } else {
        ret = -EBUSY;
    }
    return ret;
}

static int iwl_trans_pcie_write_mem(struct iwl_trans *trans, u32 addr,
                                    const void *buf, int dwords)
{
    IOInterruptState flags;
    int offs, ret = 0;
    const u32 *vals = buf;
    
    if (iwl_trans_grab_nic_access(trans, &flags)) {
        iwl_write32(trans, HBUS_TARG_MEM_WADDR, addr);
        for (offs = 0; offs < dwords; offs++)
            iwl_write32(trans, HBUS_TARG_MEM_WDAT,
                        vals ? vals[offs] : 0);
        iwl_trans_release_nic_access(trans, &flags);
    } else {
        ret = -EBUSY;
    }
    return ret;
}

static void iwl_trans_pcie_set_bits_mask(struct iwl_trans *trans, u32 reg,
                                         u32 mask, u32 value)
{
    struct iwl_trans_pcie *trans_pcie = IWL_TRANS_GET_PCIE_TRANS(trans);
    
    IOInterruptState state = IOSimpleLockLockDisableInterrupt(trans_pcie->reg_lock);
    __iwl_trans_pcie_set_bits_mask(trans, reg, mask, value);
    IOSimpleLockUnlockEnableInterrupt(trans_pcie->reg_lock, state);
}

// line 1821
static void iwl_trans_pcie_set_pmi(struct iwl_trans *trans, bool state)
{
    if (state)
        set_bit(STATUS_TPOWER_PMI, &trans->status);
    else
        clear_bit(STATUS_TPOWER_PMI, &trans->status);
}




#define IWL_TRANS_COMMON_OPS                                        \
        .write8 = iwl_trans_pcie_write8,                            \
        .write32 = iwl_trans_pcie_write32,                          \
        .read32 = iwl_trans_pcie_read32,                            \
        .read_prph = iwl_trans_pcie_read_prph,                      \
        .write_prph = iwl_trans_pcie_write_prph,                    \
        .read_mem = iwl_trans_pcie_read_mem,                        \
        .write_mem = iwl_trans_pcie_write_mem,                      \
        .grab_nic_access = iwl_trans_pcie_grab_nic_access,          \
        .release_nic_access = iwl_trans_pcie_release_nic_access,    \
        .set_bits_mask = iwl_trans_pcie_set_bits_mask,              \
        .set_pmi = iwl_trans_pcie_set_pmi,                          \
        .configure = iwl_trans_pcie_configure
//        .op_mode_leave = iwl_trans_pcie_op_mode_leave,              \
//        .ref = iwl_trans_pcie_ref,                                  \
//        .unref = iwl_trans_pcie_unref,                              \
//        .dump_data = iwl_trans_pcie_dump_data,                      \
//        .d3_suspend = iwl_trans_pcie_d3_suspend,                    \
//        .d3_resume = iwl_trans_pcie_d3_resume

#ifdef CONFIG_PM_SLEEP
#define IWL_TRANS_PM_OPS                                            \
        .suspend = iwl_trans_pcie_suspend,                          \
        .resume = iwl_trans_pcie_resume,
#else
#define IWL_TRANS_PM_OPS
#endif /* CONFIG_PM_SLEEP */

const struct iwl_trans_ops trans_ops_pcie = {
    IWL_TRANS_COMMON_OPS,
    IWL_TRANS_PM_OPS
    .send_cmd = iwl_trans_pcie_send_hcmd,
    .fw_alive = iwl_trans_pcie_fw_alive,
//    .start_hw = iwl_trans_pcie_start_hw,
//    .start_fw = iwl_trans_pcie_start_fw,
//    .stop_device = iwl_trans_pcie_stop_device,
//    .tx = iwl_trans_pcie_tx,
//    .reclaim = iwl_trans_pcie_reclaim,
//
    .txq_disable = iwl_trans_pcie_txq_disable,
    .txq_enable = iwl_trans_pcie_txq_enable,
//
    .txq_set_shared_mode = iwl_trans_pcie_txq_set_shared_mode,
    
//    .wait_tx_queues_empty = iwl_trans_pcie_wait_txqs_empty,
//
//    .freeze_txq_timer = iwl_trans_pcie_freeze_txq_timer,
//    .block_txq_ptrs = iwl_trans_pcie_block_txq_ptrs,
};

const struct iwl_trans_ops trans_ops_pcie_gen2 = {
    IWL_TRANS_COMMON_OPS,
    IWL_TRANS_PM_OPS
//    .start_hw = iwl_trans_pcie_start_hw,
//    .fw_alive = iwl_trans_pcie_gen2_fw_alive,
//    .start_fw = iwl_trans_pcie_gen2_start_fw,
//    .stop_device = iwl_trans_pcie_gen2_stop_device,
//
//    .send_cmd = iwl_trans_pcie_gen2_send_hcmd,
//
//    .tx = iwl_trans_pcie_gen2_tx,
//    .reclaim = iwl_trans_pcie_reclaim,
//
//    .txq_alloc = iwl_trans_pcie_dyn_txq_alloc,
//    .txq_free = iwl_trans_pcie_dyn_txq_free,
//    .wait_txq_empty = iwl_trans_pcie_wait_txq_empty,
};



