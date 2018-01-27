//
//  IntelWifi_ops.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IntelWifi.hpp"

int IntelWifi::start_hw(struct iwl_trans *trans, bool low_power) {
    return iwl_trans_pcie_start_hw(trans, low_power);
}

int IntelWifi::start_fw(struct iwl_trans *trans, const struct fw_img *fw, bool run_in_rfkill) {
    clear_bit(STATUS_FW_ERROR, &trans->status);

    return iwl_trans_pcie_start_fw(trans, fw, run_in_rfkill);
}

void IntelWifi::op_mode_leave(struct iwl_trans *trans) {
    //iwl_trans_op_mode_leave(trans);
    iwl_trans_pcie_op_mode_leave(trans);
}

void IntelWifi::set_pmi(struct iwl_trans *trans, bool state) {
    iwl_trans_set_pmi(trans, state);
}

void IntelWifi::configure(struct iwl_trans *trans, const struct iwl_trans_config *trans_cfg) {
    iwl_trans_pcie_configure(trans, trans_cfg);
}

void IntelWifi::fw_alive(struct iwl_trans *trans, u32 scd_addr) {
    trans->state = IWL_TRANS_FW_ALIVE;
    iwl_trans_pcie_fw_alive(trans, scd_addr);
}

void IntelWifi::stop_device(struct iwl_trans *trans, bool low_power) {
    iwl_trans_pcie_stop_device(trans, low_power);
    trans->state = IWL_TRANS_NO_FW;

}

bool IntelWifi::txq_enable(struct iwl_trans *trans, int queue, u16 ssn,
                   const struct iwl_trans_txq_scd_cfg *cfg,
                           unsigned int queue_wdg_timeout) {
    return iwl_trans_pcie_txq_enable(trans, queue, ssn, cfg, queue_wdg_timeout);
}
void IntelWifi::txq_disable(struct iwl_trans *trans, int queue, bool configure_scd) {
    iwl_trans_pcie_txq_disable(trans, queue, configure_scd);
}

int IntelWifi::send_cmd(struct iwl_trans *trans, struct iwl_host_cmd *cmd) {
    return iwl_trans_pcie_send_hcmd(trans, cmd);
}

