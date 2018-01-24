/******************************************************************************
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2008 - 2014 Intel Corporation. All rights reserved.
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
 *****************************************************************************/

//
//  IwlDvmOpMode_scan.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 20/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

extern "C" {
#include "dev.h"
#include "agn.h"
}

/* For active scan, listen ACTIVE_DWELL_TIME (msec) on each channel after
 * sending probe req.  This should be set long enough to hear probe responses
 * from more than one AP.  */
#define IWL_ACTIVE_DWELL_TIME_24    (30)       /* all times in msec */
#define IWL_ACTIVE_DWELL_TIME_52    (20)

#define IWL_ACTIVE_DWELL_FACTOR_24GHZ (3)
#define IWL_ACTIVE_DWELL_FACTOR_52GHZ (2)

/* For passive scan, listen PASSIVE_DWELL_TIME (msec) on each channel.
 * Must be set longer than active dwell time.
 * For the most reliable scan, set > AP beacon interval (typically 100msec). */
#define IWL_PASSIVE_DWELL_TIME_24   (20)       /* all times in msec */
#define IWL_PASSIVE_DWELL_TIME_52   (10)
#define IWL_PASSIVE_DWELL_BASE      (100)
#define IWL_CHANNEL_TUNE_TIME       5
#define MAX_SCAN_CHANNEL        50

/* For reset radio, need minimal dwell time only */
#define IWL_RADIO_RESET_DWELL_TIME    5

// line 57
int IwlDvmOpMode::iwl_send_scan_abort(struct iwl_priv *priv)
{
    int ret;
    struct iwl_host_cmd cmd = {
        .id = REPLY_SCAN_ABORT_CMD,
        .flags = CMD_WANT_SKB,
    };
    __le32 *status;
    
    /* Exit instantly with error when device is not ready
     * to receive scan abort command or it does not perform
     * hardware scan currently */
    if (!test_bit(STATUS_READY, &priv->status) ||
        !test_bit(STATUS_SCAN_HW, &priv->status) ||
        test_bit(STATUS_FW_ERROR, &priv->status))
        return -EIO;
    
    ret = iwl_dvm_send_cmd(priv, &cmd);
    if (ret)
        return ret;
    
    status = (__le32 *)cmd.resp_pkt->data;
    if (*status != CAN_ABORT_STATUS) {
        /* The scan abort will return 1 for success or
         * 2 for "failure".  A failure condition can be
         * due to simply not being in an active scan which
         * can occur if we send the scan abort before we
         * the microcode has notified us that a scan is
         * completed. */
        IWL_DEBUG_SCAN(priv, "SCAN_ABORT ret %d.\n",
                       le32_to_cpu(*status));
        ret = -EIO;
    }
    
    iwl_free_resp(&cmd);
    return ret;
}

// line 95
static void iwl_complete_scan(struct iwl_priv *priv, bool aborted)
{
    // TODO: Implement
//    struct cfg80211_scan_info info = {
//        .aborted = aborted,
//    };
//
//    /* check if scan was requested from mac80211 */
//    if (priv->scan_request) {
//        IWL_DEBUG_SCAN(priv, "Complete scan in mac80211\n");
//        ieee80211_scan_completed(priv->hw, &info);
//    }
    
    priv->scan_type = IWL_SCAN_NORMAL;
    priv->scan_vif = NULL;
    //priv->scan_request = NULL;
}


// line 112
void IwlDvmOpMode::iwl_process_scan_complete(struct iwl_priv *priv)
{
    bool aborted;
    
    //lockdep_assert_held(&priv->mutex);
    
    if (!test_and_clear_bit(STATUS_SCAN_COMPLETE, &priv->status))
        return;
    
    IWL_DEBUG_SCAN(priv, "Completed scan.\n");
    
    //cancel_delayed_work(&priv->scan_check);
    
    aborted = test_and_clear_bit(STATUS_SCAN_ABORTING, &priv->status);
    if (aborted)
        IWL_DEBUG_SCAN(priv, "Aborted scan completed.\n");
    
    if (!test_and_clear_bit(STATUS_SCANNING, &priv->status)) {
        IWL_DEBUG_SCAN(priv, "Scan already completed.\n");
        goto out_settings;
    }
    
    if (priv->scan_type != IWL_SCAN_NORMAL && !aborted) {
        int err;
        
        /* Check if mac80211 requested scan during our internal scan */
        if (priv->scan_request == NULL)
            goto out_complete;
        
        /* If so request a new scan */
//        err = iwl_scan_initiate(priv, priv->scan_vif, IWL_SCAN_NORMAL,
//                                priv->scan_request->channels[0]->band);
        if (err) {
            IWL_DEBUG_SCAN(priv,
                           "failed to initiate pending scan: %d\n", err);
            aborted = true;
            goto out_complete;
        }
        
        return;
    }
    
out_complete:
    iwl_complete_scan(priv, aborted);
    
out_settings:
    /* Can we still talk to firmware ? */
    if (!iwl_is_ready_rf(priv))
        return;
    
    iwlagn_post_scan(priv);
}



// line 165
void IwlDvmOpMode::iwl_force_scan_end(struct iwl_priv *priv)
{
    //lockdep_assert_held(&priv->mutex);
    
    if (!test_bit(STATUS_SCANNING, &priv->status)) {
        IWL_DEBUG_SCAN(priv, "Forcing scan end while not scanning\n");
        return;
    }
    
    IWL_DEBUG_SCAN(priv, "Forcing scan end\n");
    clear_bit(STATUS_SCANNING, &priv->status);
    clear_bit(STATUS_SCAN_HW, &priv->status);
    clear_bit(STATUS_SCAN_ABORTING, &priv->status);
    clear_bit(STATUS_SCAN_COMPLETE, &priv->status);
    iwl_complete_scan(priv, true);
}



// line 182
void IwlDvmOpMode::iwl_do_scan_abort(struct iwl_priv *priv)
{
    int ret;
    
    //lockdep_assert_held(&priv->mutex);
    
    if (!test_bit(STATUS_SCANNING, &priv->status)) {
        IWL_DEBUG_SCAN(priv, "Not performing scan to abort\n");
        return;
    }
    
    if (test_and_set_bit(STATUS_SCAN_ABORTING, &priv->status)) {
        IWL_DEBUG_SCAN(priv, "Scan abort in progress\n");
        return;
    }
    
    ret = iwl_send_scan_abort(priv);
    if (ret) {
        IWL_DEBUG_SCAN(priv, "Send scan abort failed %d\n", ret);
        iwl_force_scan_end(priv);
    } else
        IWL_DEBUG_SCAN(priv, "Successfully send scan abort\n");
}


/** line 216
 * iwl_scan_cancel_timeout - Cancel any currently executing HW scan
 * @ms: amount of time to wait (in milliseconds) for scan to abort
 *
 */
void IwlDvmOpMode::iwl_scan_cancel_timeout(struct iwl_priv *priv, unsigned long ms)
{
    uint64_t uptime;
    clock_get_uptime(&uptime);
    unsigned long timeout = uptime + ms;
    
    //lockdep_assert_held(&priv->mutex);
    
    IWL_DEBUG_SCAN(priv, "Scan cancel timeout\n");
    
    iwl_do_scan_abort(priv);
    
    while (timeout <= uptime) {
        if (!test_bit(STATUS_SCAN_HW, &priv->status))
            goto finished;
        IODelay(20);
        clock_get_uptime(&uptime);
    }
    
    return;
    
finished:
    /*
     * Now STATUS_SCAN_HW is clear. This means that the
     * device finished, but the background work is going
     * to execute at best as soon as we release the mutex.
     * Since we need to be able to issue a new scan right
     * after this function returns, run the complete here.
     * The STATUS_SCAN_COMPLETE bit will then be cleared
     * and prevent the background work from "completing"
     * a possible new scan.
     */
    iwl_process_scan_complete(priv);
}

// line 357
void IwlDvmOpMode::iwl_setup_rx_scan_handlers(struct iwl_priv *priv)
{
    /* scan handlers */
    priv->rx_handlers[REPLY_SCAN_CMD] = iwl_rx_reply_scan;
    priv->rx_handlers[SCAN_START_NOTIFICATION] = iwl_rx_scan_start_notif;
    priv->rx_handlers[SCAN_RESULTS_NOTIFICATION] =
    iwl_rx_scan_results_notif;
    priv->rx_handlers[SCAN_COMPLETE_NOTIFICATION] =
    iwl_rx_scan_complete_notif;
}

void IwlDvmOpMode::iwl_rx_reply_scan(struct iwl_priv *priv,
                              struct iwl_rx_cmd_buffer *rxb)
{
    DebugLog("HANDLER: iwl_rx_reply_scan\n");
}

void IwlDvmOpMode::iwl_rx_scan_start_notif(struct iwl_priv *priv,
                              struct iwl_rx_cmd_buffer *rxb)
{
    DebugLog("HANDLER: iwl_rx_scan_start_notif\n");
}

void IwlDvmOpMode::iwl_rx_scan_results_notif(struct iwl_priv *priv,
                                    struct iwl_rx_cmd_buffer *rxb)
{
    DebugLog("HANDLER: iwl_rx_scan_results_notif\n");
}

void IwlDvmOpMode::iwl_rx_scan_complete_notif(struct iwl_priv *priv,
                                    struct iwl_rx_cmd_buffer *rxb)
{
    DebugLog("HANDLER: iwl_rx_scan_complete_notif\n");
}





// line 929
void IwlDvmOpMode::iwl_init_scan_params(struct iwl_priv *priv)
{
    u8 ant_idx = fls(priv->nvm_data->valid_tx_ant) - 1;
    if (!priv->scan_tx_ant[NL80211_BAND_5GHZ])
        priv->scan_tx_ant[NL80211_BAND_5GHZ] = ant_idx;
    if (!priv->scan_tx_ant[NL80211_BAND_2GHZ])
        priv->scan_tx_ant[NL80211_BAND_2GHZ] = ant_idx;
}


