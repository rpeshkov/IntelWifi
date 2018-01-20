//
//  IwlDvmOpMode_calib.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 18/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

extern "C" {
#include "dev.h"
}

/*****************************************************************************
 * INIT calibrations framework
 *****************************************************************************/

/* Opaque calibration results */
struct iwl_calib_result {
    struct list_head list;
    size_t cmd_len;
    struct iwl_calib_hdr hdr;
    /* data follows */
};

struct statistics_general_data {
    u32 beacon_silence_rssi_a;
    u32 beacon_silence_rssi_b;
    u32 beacon_silence_rssi_c;
    u32 beacon_energy_a;
    u32 beacon_energy_b;
    u32 beacon_energy_c;
};

// line 93
int IwlDvmOpMode::iwl_send_calib_results(struct iwl_priv *priv)
{
    struct iwl_host_cmd hcmd = {
        .id = REPLY_PHY_CALIBRATION_CMD,
    };
    struct iwl_calib_result *res;
    
    list_for_each_entry(res, &priv->calib_results, list) {
        int ret;
        
        hcmd.len[0] = res->cmd_len;
        hcmd.data[0] = &res->hdr;
        hcmd.dataflags[0] = IWL_HCMD_DFL_NOCOPY;
        ret = iwl_dvm_send_cmd(priv, &hcmd);
        if (ret) {
            IWL_ERR(priv, "Error %d on calib cmd %d\n",
                    ret, res->hdr.op_code);
            return ret;
        }
    }
    
    return 0;
}



// line 117
int IwlDvmOpMode::iwl_calib_set(struct iwl_priv *priv,
                  const struct iwl_calib_hdr *cmd, int len)
{
    struct iwl_calib_result *res, *tmp;
    vm_size_t alloc_size = sizeof(*res) + len - sizeof(struct iwl_calib_hdr);
    res = (struct iwl_calib_result *)IOMalloc(alloc_size);

    if (!res)
        return -ENOMEM;
    memcpy(&res->hdr, cmd, len);
    res->cmd_len = len;
    
    list_for_each_entry(tmp, &priv->calib_results, list) {
        if (tmp->hdr.op_code == res->hdr.op_code) {
            list_replace(&tmp->list, &res->list);
            IOFree(tmp, alloc_size);
            return 0;
        }
    }
    
    /* wasn't in list already */
    list_add_tail(&res->list, &priv->calib_results);
    
    return 0;
}


// line 1098
void IwlDvmOpMode::iwl_reset_run_time_calib(struct iwl_priv *priv)
{
    int i;
    memset(&(priv->sensitivity_data), 0,
           sizeof(struct iwl_sensitivity_data));
    memset(&(priv->chain_noise_data), 0,
           sizeof(struct iwl_chain_noise_data));
    for (i = 0; i < NUM_RX_CHAINS; i++)
        priv->chain_noise_data.delta_gain_code[i] =
        CHAIN_NOISE_DELTA_GAIN_INIT_VAL;
    
    /* Ask for statistics now, the uCode will send notification
     * periodically after association */
    iwl_send_statistics_request(priv, CMD_ASYNC, true);
}

