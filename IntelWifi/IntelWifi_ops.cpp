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
    return iwl_trans_pcie_start_fw(trans, fw, run_in_rfkill);
}

void IntelWifi::op_mode_leave(struct iwl_trans *trans) {
    iwl_trans_op_mode_leave(trans);
}

void IntelWifi::set_pmi(struct iwl_trans *trans, bool state) {
    iwl_trans_set_pmi(trans, state);
}

void IntelWifi::configure(struct iwl_trans *trans, const struct iwl_trans_config *trans_cfg) {
    iwl_trans_pcie_configure(trans, trans_cfg);
}

void IntelWifi::fw_alive(struct iwl_trans *trans, u32 scd_addr) {
    iwl_trans_pcie_fw_alive(trans, scd_addr);
}

void IntelWifi::stop_device(struct iwl_trans *trans, bool low_power) {
    iwl_trans_pcie_stop_device(trans, low_power);
}
