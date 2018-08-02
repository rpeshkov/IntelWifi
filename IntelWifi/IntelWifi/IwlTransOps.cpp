//
//  IntelWifi_ops.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlTransOps.h"

IwlTransOps::IwlTransOps(IntelWifi *iw)
    : iw(iw) {
    
}

int IwlTransOps::start_hw(struct iwl_trans *trans, bool low_power) {
    return iw->iwl_trans_pcie_start_hw(trans, low_power);
}

int IwlTransOps::start_fw(struct iwl_trans *trans, const struct fw_img *fw, bool run_in_rfkill) {
    clear_bit(STATUS_FW_ERROR, &trans->status);
    return iw->iwl_trans_pcie_start_fw(trans, fw, run_in_rfkill);
}

void IwlTransOps::op_mode_leave(struct iwl_trans *trans) {
    //iwl_trans_op_mode_leave(trans);
    iw->iwl_trans_pcie_op_mode_leave(trans);
}

void IwlTransOps::stop_device(struct iwl_trans *trans, bool low_power) {
    iw->iwl_trans_pcie_stop_device(trans, low_power);
    trans->state = IWL_TRANS_NO_FW;
}
