//
//  IwlDvmOpMode_sta.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 17/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

#include <linux/etherdevice.h>

extern "C" {
#include "iwlwifi/dvm/agn.h"
#include "iwlwifi/dvm/dev.h"
}

const u8 iwl_bcast_addr[ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// line 37
static int iwl_sta_ucode_activate(struct iwl_priv *priv, u8 sta_id)
{
    //lockdep_assert_held(&priv->sta_lock);
    
    if (sta_id >= IWLAGN_STATION_COUNT) {
        IWL_ERR(priv, "invalid sta_id %u\n", sta_id);
        return -EINVAL;
    }
    if (!(priv->stations[sta_id].used & IWL_STA_DRIVER_ACTIVE))
        IWL_ERR(priv, "ACTIVATE a non DRIVER active station id %u "
                "addr %pM\n",
                sta_id, priv->stations[sta_id].sta.sta.addr);
    
    if (priv->stations[sta_id].used & IWL_STA_UCODE_ACTIVE) {
        IWL_DEBUG_ASSOC(priv,
                        "STA id %u addr %pM already present in uCode "
                        "(according to driver)\n",
                        sta_id, priv->stations[sta_id].sta.sta.addr);
    } else {
        priv->stations[sta_id].used |= IWL_STA_UCODE_ACTIVE;
        IWL_DEBUG_ASSOC(priv, "Added STA id %u addr " MAC_FMT " to uCode\n",
                        sta_id, MAC_BYTES(priv->stations[sta_id].sta.sta.addr));
    }
    return 0;
}


// line 63
static void iwl_process_add_sta_resp(struct iwl_priv *priv, struct iwl_rx_packet *pkt)
{
    struct iwl_add_sta_resp *add_sta_resp = (struct iwl_add_sta_resp *)pkt->data;
    
    IWL_DEBUG_INFO(priv, "Processing response for adding station\n");
    
    //spin_lock_bh(&priv->sta_lock);
    IOSimpleLockLock(priv->sta_lock);
    
    switch (add_sta_resp->status) {
        case ADD_STA_SUCCESS_MSK:
            IWL_DEBUG_INFO(priv, "REPLY_ADD_STA PASSED\n");
            break;
        case ADD_STA_NO_ROOM_IN_TABLE:
            IWL_ERR(priv, "Adding station failed, no room in table.\n");
            break;
        case ADD_STA_NO_BLOCK_ACK_RESOURCE:
            IWL_ERR(priv,
                    "Adding station failed, no block ack resource.\n");
            break;
        case ADD_STA_MODIFY_NON_EXIST_STA:
            IWL_ERR(priv, "Attempting to modify non-existing station\n");
            break;
        default:
            IWL_DEBUG_ASSOC(priv, "Received REPLY_ADD_STA:(0x%08X)\n",
                            add_sta_resp->status);
            break;
    }
    
    //spin_unlock_bh(&priv->sta_lock);
    IOSimpleLockUnlock(priv->sta_lock);
}

// line 95
void iwl_add_sta_callback(struct iwl_priv *priv, struct iwl_rx_cmd_buffer *rxb)
{
    struct iwl_rx_packet *pkt = (struct iwl_rx_packet *)rxb_addr(rxb);
    
    iwl_process_add_sta_resp(priv, pkt);
}

// line 102
int IwlDvmOpMode::iwl_send_add_sta(struct iwl_priv *priv, struct iwl_addsta_cmd *sta, u8 flags)
{
    int ret = 0;
    struct iwl_host_cmd cmd = {
        .id = REPLY_ADD_STA,
        .flags = flags,
        .data = { sta, },
        .len = { sizeof(*sta), },
    };
    u8 sta_id  = sta->sta.sta_id;
    struct iwl_rx_packet *pkt;
    struct iwl_add_sta_resp *add_sta_resp;
    
    IWL_DEBUG_INFO(priv, "Adding sta %u (" MAC_FMT ") %ssynchronously\n",
                   sta_id, MAC_BYTES(sta->sta.addr), flags & CMD_ASYNC ?  "a" : "");
    
    if (!(flags & CMD_ASYNC)) {
        cmd.flags |= CMD_WANT_SKB;
        might_sleep();
    }
    
    ret = iwl_dvm_send_cmd(priv, &cmd);
    
    if (ret || (flags & CMD_ASYNC))
        return ret;
    
    pkt = cmd.resp_pkt;
    add_sta_resp = (struct iwl_add_sta_resp *)pkt->data;
    
    /* debug messages are printed in the handler */
    if (add_sta_resp->status == ADD_STA_SUCCESS_MSK) {
        //spin_lock_bh(&priv->sta_lock);
        IOSimpleLockLock(priv->sta_lock);
        ret = iwl_sta_ucode_activate(priv, sta_id);
        //spin_unlock_bh(&priv->sta_lock);
        IOSimpleLockUnlock(priv->sta_lock);
    } else {
        ret = -EIO;
    }
    
    iwl_free_resp(&cmd);
    
    return ret;
}


// line 146
bool iwl_is_ht40_tx_allowed(struct iwl_priv *priv,
                            struct iwl_rxon_context *ctx,
                            struct ieee80211_sta *sta)
{
    if (!ctx->ht.enabled || !ctx->ht.is_40mhz)
        return false;
    
#ifdef CONFIG_IWLWIFI_DEBUGFS
    if (priv->disable_ht40)
        return false;
#endif
    
    /* special case for RXON */
    if (!sta)
        return true;
    
    return sta->bandwidth >= IEEE80211_STA_RX_BW_40;
}


// line 165
static void iwl_sta_calc_ht_flags(struct iwl_priv *priv,
                                  struct ieee80211_sta *sta,
                                  struct iwl_rxon_context *ctx,
                                  __le32 *flags, __le32 *mask)
{
    struct ieee80211_sta_ht_cap *sta_ht_inf = &sta->ht_cap;
    
    *mask = STA_FLG_RTS_MIMO_PROT_MSK |
            STA_FLG_MIMO_DIS_MSK |
            STA_FLG_HT40_EN_MSK |
            STA_FLG_MAX_AGG_SIZE_MSK |
            STA_FLG_AGG_MPDU_DENSITY_MSK;
    *flags = 0;
    
    if (!sta || !sta_ht_inf->ht_supported)
        return;
    
    IWL_DEBUG_INFO(priv, "STA %pM SM PS mode: %s\n",
                   sta->addr,
                   (sta->smps_mode == IEEE80211_SMPS_STATIC) ?
                   "static" :
                   (sta->smps_mode == IEEE80211_SMPS_DYNAMIC) ?
                   "dynamic" : "disabled");
    
    switch (sta->smps_mode) {
        case IEEE80211_SMPS_STATIC:
            *flags |= STA_FLG_MIMO_DIS_MSK;
            break;
        case IEEE80211_SMPS_DYNAMIC:
            *flags |= STA_FLG_RTS_MIMO_PROT_MSK;
            break;
        case IEEE80211_SMPS_OFF:
            break;
        default:
            IWL_WARN(priv, "Invalid MIMO PS mode %d\n", sta->smps_mode);
            break;
    }
    
    *flags |= cpu_to_le32(
            (u32)sta_ht_inf->ampdu_factor << STA_FLG_MAX_AGG_SIZE_POS);
    
    *flags |= cpu_to_le32(
            (u32)sta_ht_inf->ampdu_density << STA_FLG_AGG_MPDU_DENSITY_POS);
    
    if (iwl_is_ht40_tx_allowed(priv, ctx, sta))
        *flags |= STA_FLG_HT40_EN_MSK;
}


// line 239
static void iwl_set_ht_add_station(struct iwl_priv *priv, u8 index,
                                   struct ieee80211_sta *sta,
                                   struct iwl_rxon_context *ctx)
{
    __le32 flags, mask;
    
    iwl_sta_calc_ht_flags(priv, sta, ctx, &flags, &mask);
    
    //lockdep_assert_held(&priv->sta_lock);
    priv->stations[index].sta.station_flags &= ~mask;
    priv->stations[index].sta.station_flags |= flags;
}



/** line 252
 * iwl_prep_station - Prepare station information for addition
 *
 * should be called with sta_lock held
 */
u8 IwlDvmOpMode::iwl_prep_station(struct iwl_priv *priv, struct iwl_rxon_context *ctx,
                    const u8 *addr, bool is_ap, struct ieee80211_sta *sta)
{
    struct iwl_station_entry *station;
    int i;
    u8 sta_id = IWL_INVALID_STATION;
    
    if (is_ap)
        sta_id = ctx->ap_sta_id;
    else if (is_broadcast_ether_addr(addr))
        sta_id = ctx->bcast_sta_id;
    else
        for (i = IWL_STA_ID; i < IWLAGN_STATION_COUNT; i++) {
            if (ether_addr_equal(priv->stations[i].sta.sta.addr,
                                 addr)) {
                sta_id = i;
                break;
            }
            
            if (!priv->stations[i].used &&
                sta_id == IWL_INVALID_STATION)
                sta_id = i;
        }
    
    /*
     * These two conditions have the same outcome, but keep them
     * separate
     */
    if (unlikely(sta_id == IWL_INVALID_STATION))
        return sta_id;
    
    /*
     * uCode is not able to deal with multiple requests to add a
     * station. Keep track if one is in progress so that we do not send
     * another.
     */
    if (priv->stations[sta_id].used & IWL_STA_UCODE_INPROGRESS) {
        IWL_DEBUG_INFO(priv, "STA %d already in process of being "
                       "added.\n", sta_id);
        return sta_id;
    }
    
    if ((priv->stations[sta_id].used & IWL_STA_DRIVER_ACTIVE) &&
        (priv->stations[sta_id].used & IWL_STA_UCODE_ACTIVE) &&
        ether_addr_equal(priv->stations[sta_id].sta.sta.addr, addr)) {
        IWL_DEBUG_ASSOC(priv, "STA %d (%pM) already added, not "
                        "adding again.\n", sta_id, addr);
        return sta_id;
    }
    
    station = &priv->stations[sta_id];
    station->used = IWL_STA_DRIVER_ACTIVE;
    IWL_DEBUG_ASSOC(priv, "Add STA to driver ID %d: " MAC_FMT "\n", sta_id, MAC_BYTES(addr));
    priv->num_stations++;
    
    /* Set up the REPLY_ADD_STA command to send to device */
    memset(&station->sta, 0, sizeof(struct iwl_addsta_cmd));
    memcpy(station->sta.sta.addr, addr, ETH_ALEN);
    station->sta.mode = 0;
    station->sta.sta.sta_id = sta_id;
    station->sta.station_flags = ctx->station_flags;
    station->ctxid = ctx->ctxid;
    
    if (sta) {
//        struct iwl_station_priv *sta_priv;

        // TODO: Implement
//        sta_priv = (struct iwl_station_priv *)sta->drv_priv;
//        sta_priv->ctx = ctx;
    }
    
    /*
     * OK to call unconditionally, since local stations (IBSS BSSID
     * STA and broadcast STA) pass in a NULL sta, and mac80211
     * doesn't allow HT IBSS.
     */
    iwl_set_ht_add_station(priv, sta_id, sta, ctx);
    
    return sta_id;
    
}

// line 569
static void iwl_sta_fill_lq(struct iwl_priv *priv, struct iwl_rxon_context *ctx,
                            u8 sta_id, struct iwl_link_quality_cmd *link_cmd)
{
    int i, r;
    u32 rate_flags = 0;
    __le32 rate_n_flags = 0;
    
    //lockdep_assert_held(&priv->mutex);
    
    memset(link_cmd, 0, sizeof(*link_cmd));
    
    /* Set up the rate scaling to start at selected rate, fall back
     * all the way down to 1M in IEEE order, and then spin on 1M */
    if (priv->band == NL80211_BAND_5GHZ)
        r = IWL_RATE_6M_INDEX;
    else if (ctx && ctx->vif && ctx->vif->p2p)
        r = IWL_RATE_6M_INDEX;
    else
        r = IWL_RATE_1M_INDEX;
    
    if (r >= IWL_FIRST_CCK_RATE && r <= IWL_LAST_CCK_RATE)
        rate_flags |= RATE_MCS_CCK_MSK;
    
    rate_flags |= first_antenna(priv->nvm_data->valid_tx_ant) << RATE_MCS_ANT_POS;
    rate_n_flags = iwl_hw_set_rate_n_flags(iwl_rates[r].plcp, rate_flags);
    for (i = 0; i < LINK_QUAL_MAX_RETRY_NUM; i++)
        link_cmd->rs_table[i].rate_n_flags = rate_n_flags;
    
    link_cmd->general_params.single_stream_ant_msk =
            first_antenna(priv->nvm_data->valid_tx_ant);
    
    link_cmd->general_params.dual_stream_ant_msk =
            priv->nvm_data->valid_tx_ant &
            ~first_antenna(priv->nvm_data->valid_tx_ant);
    if (!link_cmd->general_params.dual_stream_ant_msk) {
        link_cmd->general_params.dual_stream_ant_msk = ANT_AB;
    } else if (num_of_ant(priv->nvm_data->valid_tx_ant) == 2) {
        link_cmd->general_params.dual_stream_ant_msk = priv->nvm_data->valid_tx_ant;
    }
    
    link_cmd->agg_params.agg_dis_start_th = LINK_QUAL_AGG_DISABLE_START_DEF;
    link_cmd->agg_params.agg_time_limit = cpu_to_le16(LINK_QUAL_AGG_TIME_LIMIT_DEF);
    
    link_cmd->sta_id = sta_id;
}

/** line 619
 * iwl_clear_ucode_stations - clear ucode station table bits
 *
 * This function clears all the bits in the driver indicating
 * which stations are active in the ucode. Call when something
 * other than explicit station management would cause this in
 * the ucode, e.g. unassociated RXON.
 */
void IwlDvmOpMode::iwl_clear_ucode_stations(struct iwl_priv *priv, struct iwl_rxon_context *ctx)
{
    int i;
    bool cleared = false;
    
    IWL_DEBUG_INFO(priv, "Clearing ucode stations in driver\n");
    
    //spin_lock_bh(&priv->sta_lock);
    IOSimpleLockLock(priv->sta_lock);
    for (i = 0; i < IWLAGN_STATION_COUNT; i++) {
        if (ctx && ctx->ctxid != priv->stations[i].ctxid)
            continue;
        
        if (priv->stations[i].used & IWL_STA_UCODE_ACTIVE) {
            IWL_DEBUG_INFO(priv, "Clearing ucode active for station %d\n", i);
            priv->stations[i].used &= ~IWL_STA_UCODE_ACTIVE;
            cleared = true;
        }
    }
    //spin_unlock_bh(&priv->sta_lock);
    IOSimpleLockUnlock(priv->sta_lock);
    
    if (!cleared)
        IWL_DEBUG_INFO(priv, "No active stations found to be cleared\n");
}

/** line 654
 * iwl_restore_stations() - Restore driver known stations to device
 *
 * All stations considered active by driver, but not present in ucode, is
 * restored.
 *
 * Function sleeps.
 */
void IwlDvmOpMode::iwl_restore_stations(struct iwl_priv *priv, struct iwl_rxon_context *ctx)
{
    struct iwl_addsta_cmd sta_cmd;
    static const struct iwl_link_quality_cmd zero_lq = {};
    struct iwl_link_quality_cmd lq;
    int i;
    bool found = false;
    int ret;
    bool send_lq;
    
    if (!iwl_is_ready(priv)) {
        IWL_DEBUG_INFO(priv, "Not ready yet, not restoring any stations.\n");
        return;
    }
    
    IWL_DEBUG_ASSOC(priv, "Restoring all known stations ... start.\n");
    //spin_lock_bh(&priv->sta_lock);
    IOSimpleLockLock(priv->sta_lock);
    for (i = 0; i < IWLAGN_STATION_COUNT; i++) {
        if (ctx->ctxid != priv->stations[i].ctxid)
            continue;
        if ((priv->stations[i].used & IWL_STA_DRIVER_ACTIVE) &&
            !(priv->stations[i].used & IWL_STA_UCODE_ACTIVE)) {
            IWL_DEBUG_ASSOC(priv, "Restoring sta " MAC_FMT "\n",
                            MAC_BYTES(priv->stations[i].sta.sta.addr));
            priv->stations[i].sta.mode = 0;
            priv->stations[i].used |= IWL_STA_UCODE_INPROGRESS;
            found = true;
        }
    }
    
    for (i = 0; i < IWLAGN_STATION_COUNT; i++) {
        if ((priv->stations[i].used & IWL_STA_UCODE_INPROGRESS)) {
            memcpy(&sta_cmd, &priv->stations[i].sta,
                   sizeof(struct iwl_addsta_cmd));
            send_lq = false;
            if (priv->stations[i].lq) {
                if (priv->wowlan)
                    iwl_sta_fill_lq(priv, ctx, i, &lq);
                else
                    memcpy(&lq, priv->stations[i].lq,
                           sizeof(struct iwl_link_quality_cmd));
                
                if (memcmp(&lq, &zero_lq, sizeof(lq)))
                    send_lq = true;
            }
            //spin_unlock_bh(&priv->sta_lock);
            IOSimpleLockUnlock(priv->sta_lock);
            ret = iwl_send_add_sta(priv, &sta_cmd, 0);
            if (ret) {
                //spin_lock_bh(&priv->sta_lock);
                IOSimpleLockLock(priv->sta_lock);
                IWL_ERR(priv, "Adding station %pM failed.\n",
                        priv->stations[i].sta.sta.addr);
                priv->stations[i].used &= ~IWL_STA_DRIVER_ACTIVE;
                priv->stations[i].used &= ~IWL_STA_UCODE_INPROGRESS;
                continue;
            }
            /*
             * Rate scaling has already been initialized, send
             * current LQ command
             */
            if (send_lq)
                iwl_send_lq_cmd(priv, ctx, &lq, 0, true);
            //spin_lock_bh(&priv->sta_lock);
            IOSimpleLockLock(priv->sta_lock);
            priv->stations[i].used &= ~IWL_STA_UCODE_INPROGRESS;
        }
    }
    
    //spin_unlock_bh(&priv->sta_lock);
    IOSimpleLockUnlock(priv->sta_lock);
    if (!found)
        IWL_DEBUG_INFO(priv, "Restoring all known stations .... "
                       "no stations to be restored.\n");
    else
        IWL_DEBUG_INFO(priv, "Restoring all known stations .... "
                       "complete.\n");
}

// line 770
#ifdef CONFIG_IWLWIFI_DEBUG
static void iwl_dump_lq_cmd(struct iwl_priv *priv,
                            struct iwl_link_quality_cmd *lq)
{
    int i;
    IWL_DEBUG_RATE(priv, "lq station id 0x%x\n", lq->sta_id);
    IWL_DEBUG_RATE(priv, "lq ant 0x%X 0x%X\n",
                   lq->general_params.single_stream_ant_msk,
                   lq->general_params.dual_stream_ant_msk);
    
    for (i = 0; i < LINK_QUAL_MAX_RETRY_NUM; i++)
        IWL_DEBUG_RATE(priv, "lq index %d 0x%X\n",
                       i, lq->rs_table[i].rate_n_flags);
}
#else
static inline void iwl_dump_lq_cmd(struct iwl_priv *priv,
                                   struct iwl_link_quality_cmd *lq)
{
}
#endif

/** line 791
 * is_lq_table_valid() - Test one aspect of LQ cmd for validity
 *
 * It sometimes happens when a HT rate has been in use and we
 * loose connectivity with AP then mac80211 will first tell us that the
 * current channel is not HT anymore before removing the station. In such a
 * scenario the RXON flags will be updated to indicate we are not
 * communicating HT anymore, but the LQ command may still contain HT rates.
 * Test for this to prevent driver from sending LQ command between the time
 * RXON flags are updated and when LQ command is updated.
 */
static bool is_lq_table_valid(struct iwl_priv *priv,
                              struct iwl_rxon_context *ctx,
                              struct iwl_link_quality_cmd *lq)
{
    int i;
    
    if (ctx->ht.enabled)
        return true;
    
    IWL_DEBUG_INFO(priv, "Channel %u is not an HT channel\n",
                   ctx->active.channel);
    for (i = 0; i < LINK_QUAL_MAX_RETRY_NUM; i++) {
        if (le32_to_cpu(lq->rs_table[i].rate_n_flags) & RATE_MCS_HT_MSK) {
            IWL_DEBUG_INFO(priv, "index %d of LQ expects HT channel\n", i);
            return false;
        }
    }
    return true;
}


/** line 825
 * iwl_send_lq_cmd() - Send link quality command
 * @init: This command is sent as part of station initialization right
 *        after station has been added.
 *
 * The link quality command is sent as the last step of station creation.
 * This is the special case in which init is set and we call a callback in
 * this case to clear the state indicating that station creation is in
 * progress.
 */
int IwlDvmOpMode::iwl_send_lq_cmd(struct iwl_priv *priv, struct iwl_rxon_context *ctx,
                    struct iwl_link_quality_cmd *lq, u8 flags, bool init)
{
    int ret = 0;
    struct iwl_host_cmd cmd = {
        .id = REPLY_TX_LINK_QUALITY_CMD,
        .len = { sizeof(struct iwl_link_quality_cmd), },
        .flags = flags,
        .data = { lq, },
    };
    
    if (WARN_ON(lq->sta_id == IWL_INVALID_STATION))
        return -EINVAL;
    
    
    //spin_lock_bh(&priv->sta_lock);
    IOSimpleLockLock(priv->sta_lock);
    if (!(priv->stations[lq->sta_id].used & IWL_STA_DRIVER_ACTIVE)) {
        //spin_unlock_bh(&priv->sta_lock);
        IOSimpleLockUnlock(priv->sta_lock);
        return -EINVAL;
    }
    //spin_unlock_bh(&priv->sta_lock);
    IOSimpleLockUnlock(priv->sta_lock);
    
    iwl_dump_lq_cmd(priv, lq);
    if (WARN_ON(init && (cmd.flags & CMD_ASYNC)))
        return -EINVAL;
    
    if (is_lq_table_valid(priv, ctx, lq))
        ret = iwl_dvm_send_cmd(priv, &cmd);
    else
        ret = -EINVAL;
    
    if (cmd.flags & CMD_ASYNC)
        return ret;
    
    if (init) {
        IWL_DEBUG_INFO(priv, "init LQ command complete, "
                       "clearing sta addition status for sta %d\n",
                       lq->sta_id);
        //spin_lock_bh(&priv->sta_lock);
        IOSimpleLockLock(priv->sta_lock);
        priv->stations[lq->sta_id].used &= ~IWL_STA_UCODE_INPROGRESS;
        //spin_unlock_bh(&priv->sta_lock);
        IOSimpleLockUnlock(priv->sta_lock);
    }
    return ret;
}





// line 881
static struct iwl_link_quality_cmd *
iwl_sta_alloc_lq(struct iwl_priv *priv, struct iwl_rxon_context *ctx,
                 u8 sta_id)
{
    struct iwl_link_quality_cmd *link_cmd;
    
    link_cmd = (struct iwl_link_quality_cmd *)IOMalloc(sizeof(struct iwl_link_quality_cmd));
    
    if (!link_cmd) {
        IWL_ERR(priv, "Unable to allocate memory for LQ cmd.\n");
        return NULL;
    }
    bzero(link_cmd, sizeof(struct iwl_link_quality_cmd));
    
    iwl_sta_fill_lq(priv, ctx, sta_id, link_cmd);
    
    return link_cmd;
}

/* line 947
 * static WEP keys
 *
 * For each context, the device has a table of 4 static WEP keys
 * (one for each key index) that is updated with the following
 * commands.
 */

int IwlDvmOpMode::iwl_send_static_wepkey_cmd(struct iwl_priv *priv,
                                      struct iwl_rxon_context *ctx,
                                      bool send_if_empty)
{
    int i, not_empty = 0;
    u8 buff[sizeof(struct iwl_wep_cmd) +
            sizeof(struct iwl_wep_key) * WEP_KEYS_MAX];
    struct iwl_wep_cmd *wep_cmd = (struct iwl_wep_cmd *)buff;
    size_t cmd_size  = sizeof(struct iwl_wep_cmd);
    struct iwl_host_cmd cmd = {
        .id = ctx->wep_key_cmd,
        .data = { wep_cmd, },
    };
    
    might_sleep();
    
    memset(wep_cmd, 0, cmd_size +
           (sizeof(struct iwl_wep_key) * WEP_KEYS_MAX));
    
    for (i = 0; i < WEP_KEYS_MAX ; i++) {
        wep_cmd->key[i].key_index = i;
        if (ctx->wep_keys[i].key_size) {
            wep_cmd->key[i].key_offset = i;
            not_empty = 1;
        } else {
            wep_cmd->key[i].key_offset = WEP_INVALID_OFFSET;
        }
        
        wep_cmd->key[i].key_size = ctx->wep_keys[i].key_size;
        memcpy(&wep_cmd->key[i].key[3], ctx->wep_keys[i].key,
               ctx->wep_keys[i].key_size);
    }
    
    wep_cmd->global_key_type = WEP_KEY_WEP_TYPE;
    wep_cmd->num_keys = WEP_KEYS_MAX;
    
    cmd_size += sizeof(struct iwl_wep_key) * WEP_KEYS_MAX;
    
    cmd.len[0] = cmd_size;
    
    if (not_empty || send_if_empty)
        return iwl_dvm_send_cmd(priv, &cmd);
    else
        return 0;
}

// line 1001
int IwlDvmOpMode::iwl_restore_default_wep_keys(struct iwl_priv *priv,
                                 struct iwl_rxon_context *ctx)
{
    //lockdep_assert_held(&priv->mutex);
    
    return iwl_send_static_wepkey_cmd(priv, ctx, false);
}

// line 1009
int IwlDvmOpMode::iwl_remove_default_wep_key(struct iwl_priv *priv,
                               struct iwl_rxon_context *ctx,
                               struct ieee80211_key_conf *keyconf)
{
    int ret;
    
    //lockdep_assert_held(&priv->mutex);
    
    IWL_DEBUG_WEP(priv, "Removing default WEP key: idx=%d\n", keyconf->keyidx);
    
    memset(&ctx->wep_keys[keyconf->keyidx], 0, sizeof(ctx->wep_keys[0]));
    if (iwl_is_rfkill(priv)) {
        IWL_DEBUG_WEP(priv, "Not sending REPLY_WEPKEY command due to RFKILL.\n");
        /* but keys in device are clear anyway so return success */
        return 0;
    }
    ret = iwl_send_static_wepkey_cmd(priv, ctx, 1);
    IWL_DEBUG_WEP(priv, "Remove default WEP key: idx=%d ret=%d\n", keyconf->keyidx, ret);
    
    return ret;
}

// line 1034
int IwlDvmOpMode::iwl_set_default_wep_key(struct iwl_priv *priv,
                            struct iwl_rxon_context *ctx,
                            struct ieee80211_key_conf *keyconf)
{
    int ret;
    
    //lockdep_assert_held(&priv->mutex);
    
    if (keyconf->keylen != WEP_KEY_LEN_128 &&
        keyconf->keylen != WEP_KEY_LEN_64) {
        IWL_DEBUG_WEP(priv,
                      "Bad WEP key length %d\n", keyconf->keylen);
        return -EINVAL;
    }
    
    keyconf->hw_key_idx = IWLAGN_HW_KEY_DEFAULT;
    
    ctx->wep_keys[keyconf->keyidx].key_size = keyconf->keylen;
    memcpy(&ctx->wep_keys[keyconf->keyidx].key, &keyconf->key,
           keyconf->keylen);
    
    ret = iwl_send_static_wepkey_cmd(priv, ctx, false);
    IWL_DEBUG_WEP(priv, "Set default WEP key: len=%d idx=%d ret=%d\n",
                  keyconf->keylen, keyconf->keyidx, ret);
    
    return ret;
}




/** line 1276
 * iwlagn_alloc_bcast_station - add broadcast station into driver's station table.
 *
 * This adds the broadcast station into the driver's station table
 * and marks it driver active, so that it will be restored to the
 * device at the next best time.
 */
int IwlDvmOpMode::iwlagn_alloc_bcast_station(struct iwl_priv *priv,
                               struct iwl_rxon_context *ctx)
{
    struct iwl_link_quality_cmd *link_cmd;
    u8 sta_id;
    
    //spin_lock_bh(&priv->sta_lock);
    IOSimpleLockLock(priv->sta_lock);
    sta_id = iwl_prep_station(priv, ctx, iwl_bcast_addr, false, NULL);
    if (sta_id == IWL_INVALID_STATION) {
        IWL_ERR(priv, "Unable to prepare broadcast station\n");
        //spin_unlock_bh(&priv->sta_lock);
        IOSimpleLockUnlock(priv->sta_lock);
        
        return -EINVAL;
    }
    
    priv->stations[sta_id].used |= IWL_STA_DRIVER_ACTIVE;
    priv->stations[sta_id].used |= IWL_STA_BCAST;
    //spin_unlock_bh(&priv->sta_lock);
    IOSimpleLockUnlock(priv->sta_lock);
    
    link_cmd = iwl_sta_alloc_lq(priv, ctx, sta_id);
    if (!link_cmd) {
        IWL_ERR(priv,
                "Unable to initialize rate scaling for bcast station.\n");
        return -ENOMEM;
    }
    
    //spin_lock_bh(&priv->sta_lock);
    IOSimpleLockLock(priv->sta_lock);
    priv->stations[sta_id].lq = link_cmd;
    //spin_unlock_bh(&priv->sta_lock);
    IOSimpleLockUnlock(priv->sta_lock);
    
    return 0;
}

/** line 1316
 * iwl_update_bcast_station - update broadcast station's LQ command
 *
 * Only used by iwlagn. Placed here to have all bcast station management
 * code together.
 */
int IwlDvmOpMode::iwl_update_bcast_station(struct iwl_priv *priv, struct iwl_rxon_context *ctx)
{
    struct iwl_link_quality_cmd *link_cmd;
    u8 sta_id = ctx->bcast_sta_id;
    
    link_cmd = iwl_sta_alloc_lq(priv, ctx, sta_id);
    if (!link_cmd) {
        IWL_ERR(priv, "Unable to initialize rate scaling for bcast station.\n");
        return -ENOMEM;
    }
    
    //spin_lock_bh(&priv->sta_lock);
    IOSimpleLockLock(priv->sta_lock);
    if (priv->stations[sta_id].lq)
        IOFree(priv->stations[sta_id].lq, sizeof(struct iwl_link_quality_cmd));
    else
        IWL_DEBUG_INFO(priv, "Bcast station rate scaling has not been initialized yet.\n");
    priv->stations[sta_id].lq = link_cmd;
    //spin_unlock_bh(&priv->sta_lock);
    IOSimpleLockUnlock(priv->sta_lock);
    
    return 0;
}

