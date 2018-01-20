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
    IWL_DEBUG_ASSOC(priv, "Add STA to driver ID %d: %pM\n",
                    sta_id, addr);
    priv->num_stations++;
    
    /* Set up the REPLY_ADD_STA command to send to device */
    memset(&station->sta, 0, sizeof(struct iwl_addsta_cmd));
    memcpy(station->sta.sta.addr, addr, ETH_ALEN);
    station->sta.mode = 0;
    station->sta.sta.sta_id = sta_id;
    station->sta.station_flags = ctx->station_flags;
    station->ctxid = ctx->ctxid;
    
    if (sta) {
        struct iwl_station_priv *sta_priv;

        // TODO: Implement
//        sta_priv = (struct iwl_station_priv *)sta->drv_priv;
//        sta_priv->ctx = ctx;
    }
    
    /*
     * OK to call unconditionally, since local stations (IBSS BSSID
     * STA and broadcast STA) pass in a NULL sta, and mac80211
     * doesn't allow HT IBSS.
     */
    // TODO: Implement
    //iwl_set_ht_add_station(priv, sta_id, sta, ctx);
    
    return sta_id;
    
}

// line 569
static void iwl_sta_fill_lq(struct iwl_priv *priv, struct iwl_rxon_context *ctx,
                            u8 sta_id, struct iwl_link_quality_cmd *link_cmd)
{
    int i, r;
    u32 rate_flags = 0;
    __le32 rate_n_flags;
    
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
    
    rate_flags |= first_antenna(priv->nvm_data->valid_tx_ant) <<
    RATE_MCS_ANT_POS;
    //rate_n_flags = iwl_hw_set_rate_n_flags(iwl_rates[r].plcp, rate_flags);
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
        link_cmd->general_params.dual_stream_ant_msk =
        priv->nvm_data->valid_tx_ant;
    }
    
    link_cmd->agg_params.agg_dis_start_th =
    LINK_QUAL_AGG_DISABLE_START_DEF;
    link_cmd->agg_params.agg_time_limit =
    cpu_to_le16(LINK_QUAL_AGG_TIME_LIMIT_DEF);
    
    link_cmd->sta_id = sta_id;
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
