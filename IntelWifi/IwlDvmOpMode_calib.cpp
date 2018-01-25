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

// line 430
static void iwl_prepare_legacy_sensitivity_tbl(struct iwl_priv *priv,
                                               struct iwl_sensitivity_data *data,
                                               __le16 *tbl)
{
    tbl[HD_AUTO_CORR32_X4_TH_ADD_MIN_INDEX] =
    cpu_to_le16((u16)data->auto_corr_ofdm);
    tbl[HD_AUTO_CORR32_X4_TH_ADD_MIN_MRC_INDEX] =
    cpu_to_le16((u16)data->auto_corr_ofdm_mrc);
    tbl[HD_AUTO_CORR32_X1_TH_ADD_MIN_INDEX] =
    cpu_to_le16((u16)data->auto_corr_ofdm_x1);
    tbl[HD_AUTO_CORR32_X1_TH_ADD_MIN_MRC_INDEX] =
    cpu_to_le16((u16)data->auto_corr_ofdm_mrc_x1);
    
    tbl[HD_AUTO_CORR40_X4_TH_ADD_MIN_INDEX] =
    cpu_to_le16((u16)data->auto_corr_cck);
    tbl[HD_AUTO_CORR40_X4_TH_ADD_MIN_MRC_INDEX] =
    cpu_to_le16((u16)data->auto_corr_cck_mrc);
    
    tbl[HD_MIN_ENERGY_CCK_DET_INDEX] =
    cpu_to_le16((u16)data->nrg_th_cck);
    tbl[HD_MIN_ENERGY_OFDM_DET_INDEX] =
    cpu_to_le16((u16)data->nrg_th_ofdm);
    
    tbl[HD_BARKER_CORR_TH_ADD_MIN_INDEX] =
    cpu_to_le16(data->barker_corr_th_min);
    tbl[HD_BARKER_CORR_TH_ADD_MIN_MRC_INDEX] =
    cpu_to_le16(data->barker_corr_th_min_mrc);
    tbl[HD_OFDM_ENERGY_TH_IN_INDEX] =
    cpu_to_le16(data->nrg_th_cca);
    
    IWL_DEBUG_CALIB(priv, "ofdm: ac %u mrc %u x1 %u mrc_x1 %u thresh %u\n",
                    data->auto_corr_ofdm, data->auto_corr_ofdm_mrc,
                    data->auto_corr_ofdm_x1, data->auto_corr_ofdm_mrc_x1,
                    data->nrg_th_ofdm);
    
    IWL_DEBUG_CALIB(priv, "cck: ac %u mrc %u thresh %u\n",
                    data->auto_corr_cck, data->auto_corr_cck_mrc,
                    data->nrg_th_cck);
}

/* line 470
 * Prepare a SENSITIVITY_CMD, send to uCode if values have changed
 */
int IwlDvmOpMode::iwl_sensitivity_write(struct iwl_priv *priv)
{
    struct iwl_sensitivity_cmd cmd;
    struct iwl_sensitivity_data *data = NULL;
    struct iwl_host_cmd cmd_out = {
        .id = SENSITIVITY_CMD,
        .len = { sizeof(struct iwl_sensitivity_cmd), },
        .flags = CMD_ASYNC,
        .data = { &cmd, },
    };
    
    data = &(priv->sensitivity_data);
    
    memset(&cmd, 0, sizeof(cmd));
    
    iwl_prepare_legacy_sensitivity_tbl(priv, data, &cmd.table[0]);
    
    /* Update uCode's "work" table, and copy it to DSP */
    cmd.control = SENSITIVITY_CMD_CONTROL_WORK_TABLE;
    
    /* Don't send command to uCode if nothing has changed */
    if (!memcmp(&cmd.table[0], &(priv->sensitivity_tbl[0]),
                sizeof(u16)*HD_TABLE_SIZE)) {
        IWL_DEBUG_CALIB(priv, "No change in SENSITIVITY_CMD\n");
        return 0;
    }
    
    /* Copy table for comparison next time */
    memcpy(&(priv->sensitivity_tbl[0]), &(cmd.table[0]),
           sizeof(u16)*HD_TABLE_SIZE);
    
    return iwl_dvm_send_cmd(priv, &cmd_out);
}

/* line 505
 * Prepare a SENSITIVITY_CMD, send to uCode if values have changed
 */
int IwlDvmOpMode::iwl_enhance_sensitivity_write(struct iwl_priv *priv)
{
    struct iwl_enhance_sensitivity_cmd cmd;
    struct iwl_sensitivity_data *data = NULL;
    struct iwl_host_cmd cmd_out = {
        .id = SENSITIVITY_CMD,
        .len = { sizeof(struct iwl_enhance_sensitivity_cmd), },
        .flags = CMD_ASYNC,
        .data = { &cmd, },
    };
    
    data = &(priv->sensitivity_data);
    
    memset(&cmd, 0, sizeof(cmd));
    
    iwl_prepare_legacy_sensitivity_tbl(priv, data, &cmd.enhance_table[0]);
    
    if (priv->lib->hd_v2) {
        cmd.enhance_table[HD_INA_NON_SQUARE_DET_OFDM_INDEX] =
        HD_INA_NON_SQUARE_DET_OFDM_DATA_V2;
        cmd.enhance_table[HD_INA_NON_SQUARE_DET_CCK_INDEX] =
        HD_INA_NON_SQUARE_DET_CCK_DATA_V2;
        cmd.enhance_table[HD_CORR_11_INSTEAD_OF_CORR_9_EN_INDEX] =
        HD_CORR_11_INSTEAD_OF_CORR_9_EN_DATA_V2;
        cmd.enhance_table[HD_OFDM_NON_SQUARE_DET_SLOPE_MRC_INDEX] =
        HD_OFDM_NON_SQUARE_DET_SLOPE_MRC_DATA_V2;
        cmd.enhance_table[HD_OFDM_NON_SQUARE_DET_INTERCEPT_MRC_INDEX] =
        HD_OFDM_NON_SQUARE_DET_INTERCEPT_MRC_DATA_V2;
        cmd.enhance_table[HD_OFDM_NON_SQUARE_DET_SLOPE_INDEX] =
        HD_OFDM_NON_SQUARE_DET_SLOPE_DATA_V2;
        cmd.enhance_table[HD_OFDM_NON_SQUARE_DET_INTERCEPT_INDEX] =
        HD_OFDM_NON_SQUARE_DET_INTERCEPT_DATA_V2;
        cmd.enhance_table[HD_CCK_NON_SQUARE_DET_SLOPE_MRC_INDEX] =
        HD_CCK_NON_SQUARE_DET_SLOPE_MRC_DATA_V2;
        cmd.enhance_table[HD_CCK_NON_SQUARE_DET_INTERCEPT_MRC_INDEX] =
        HD_CCK_NON_SQUARE_DET_INTERCEPT_MRC_DATA_V2;
        cmd.enhance_table[HD_CCK_NON_SQUARE_DET_SLOPE_INDEX] =
        HD_CCK_NON_SQUARE_DET_SLOPE_DATA_V2;
        cmd.enhance_table[HD_CCK_NON_SQUARE_DET_INTERCEPT_INDEX] =
        HD_CCK_NON_SQUARE_DET_INTERCEPT_DATA_V2;
    } else {
        cmd.enhance_table[HD_INA_NON_SQUARE_DET_OFDM_INDEX] =
        HD_INA_NON_SQUARE_DET_OFDM_DATA_V1;
        cmd.enhance_table[HD_INA_NON_SQUARE_DET_CCK_INDEX] =
        HD_INA_NON_SQUARE_DET_CCK_DATA_V1;
        cmd.enhance_table[HD_CORR_11_INSTEAD_OF_CORR_9_EN_INDEX] =
        HD_CORR_11_INSTEAD_OF_CORR_9_EN_DATA_V1;
        cmd.enhance_table[HD_OFDM_NON_SQUARE_DET_SLOPE_MRC_INDEX] =
        HD_OFDM_NON_SQUARE_DET_SLOPE_MRC_DATA_V1;
        cmd.enhance_table[HD_OFDM_NON_SQUARE_DET_INTERCEPT_MRC_INDEX] =
        HD_OFDM_NON_SQUARE_DET_INTERCEPT_MRC_DATA_V1;
        cmd.enhance_table[HD_OFDM_NON_SQUARE_DET_SLOPE_INDEX] =
        HD_OFDM_NON_SQUARE_DET_SLOPE_DATA_V1;
        cmd.enhance_table[HD_OFDM_NON_SQUARE_DET_INTERCEPT_INDEX] =
        HD_OFDM_NON_SQUARE_DET_INTERCEPT_DATA_V1;
        cmd.enhance_table[HD_CCK_NON_SQUARE_DET_SLOPE_MRC_INDEX] =
        HD_CCK_NON_SQUARE_DET_SLOPE_MRC_DATA_V1;
        cmd.enhance_table[HD_CCK_NON_SQUARE_DET_INTERCEPT_MRC_INDEX] =
        HD_CCK_NON_SQUARE_DET_INTERCEPT_MRC_DATA_V1;
        cmd.enhance_table[HD_CCK_NON_SQUARE_DET_SLOPE_INDEX] =
        HD_CCK_NON_SQUARE_DET_SLOPE_DATA_V1;
        cmd.enhance_table[HD_CCK_NON_SQUARE_DET_INTERCEPT_INDEX] =
        HD_CCK_NON_SQUARE_DET_INTERCEPT_DATA_V1;
    }
    
    /* Update uCode's "work" table, and copy it to DSP */
    cmd.control = SENSITIVITY_CMD_CONTROL_WORK_TABLE;
    
    /* Don't send command to uCode if nothing has changed */
    if (!memcmp(&cmd.enhance_table[0], &(priv->sensitivity_tbl[0]),
                sizeof(u16)*HD_TABLE_SIZE) &&
        !memcmp(&cmd.enhance_table[HD_INA_NON_SQUARE_DET_OFDM_INDEX],
                &(priv->enhance_sensitivity_tbl[0]),
                sizeof(u16)*ENHANCE_HD_TABLE_ENTRIES)) {
            IWL_DEBUG_CALIB(priv, "No change in SENSITIVITY_CMD\n");
            return 0;
        }
    
    /* Copy table for comparison next time */
    memcpy(&(priv->sensitivity_tbl[0]), &(cmd.enhance_table[0]),
           sizeof(u16)*HD_TABLE_SIZE);
    memcpy(&(priv->enhance_sensitivity_tbl[0]),
           &(cmd.enhance_table[HD_INA_NON_SQUARE_DET_OFDM_INDEX]),
           sizeof(u16)*ENHANCE_HD_TABLE_ENTRIES);
    
    return iwl_dvm_send_cmd(priv, &cmd_out);
}


// line 594
void IwlDvmOpMode::iwl_init_sensitivity(struct iwl_priv *priv)
{
    int ret = 0;
    int i;
    struct iwl_sensitivity_data *data = NULL;
    const struct iwl_sensitivity_ranges *ranges = priv->hw_params.sens;
    
    if (priv->calib_disabled & IWL_SENSITIVITY_CALIB_DISABLED)
        return;
    
    IWL_DEBUG_CALIB(priv, "Start iwl_init_sensitivity\n");
    
    /* Clear driver's sensitivity algo data */
    data = &(priv->sensitivity_data);
    
    if (ranges == NULL)
        return;
    
    memset(data, 0, sizeof(struct iwl_sensitivity_data));
    
    data->num_in_cck_no_fa = 0;
    data->nrg_curr_state = IWL_FA_TOO_MANY;
    data->nrg_prev_state = IWL_FA_TOO_MANY;
    data->nrg_silence_ref = 0;
    data->nrg_silence_idx = 0;
    data->nrg_energy_idx = 0;
    
    for (i = 0; i < 10; i++)
        data->nrg_value[i] = 0;
    
    for (i = 0; i < NRG_NUM_PREV_STAT_L; i++)
        data->nrg_silence_rssi[i] = 0;
    
    data->auto_corr_ofdm =  ranges->auto_corr_min_ofdm;
    data->auto_corr_ofdm_mrc = ranges->auto_corr_min_ofdm_mrc;
    data->auto_corr_ofdm_x1  = ranges->auto_corr_min_ofdm_x1;
    data->auto_corr_ofdm_mrc_x1 = ranges->auto_corr_min_ofdm_mrc_x1;
    data->auto_corr_cck = AUTO_CORR_CCK_MIN_VAL_DEF;
    data->auto_corr_cck_mrc = ranges->auto_corr_min_cck_mrc;
    data->nrg_th_cck = ranges->nrg_th_cck;
    data->nrg_th_ofdm = ranges->nrg_th_ofdm;
    data->barker_corr_th_min = ranges->barker_corr_th_min;
    data->barker_corr_th_min_mrc = ranges->barker_corr_th_min_mrc;
    data->nrg_th_cca = ranges->nrg_th_cca;
    
    data->last_bad_plcp_cnt_ofdm = 0;
    data->last_fa_cnt_ofdm = 0;
    data->last_bad_plcp_cnt_cck = 0;
    data->last_fa_cnt_cck = 0;
    
    if (priv->fw->enhance_sensitivity_table)
        ret |= iwl_enhance_sensitivity_write(priv);
    else
        ret |= iwl_sensitivity_write(priv);
    IWL_DEBUG_CALIB(priv, "<<return 0x%X\n", ret);
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

