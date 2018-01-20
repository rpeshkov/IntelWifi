//
//  IwlDvmOpMode_rxon.cpp
//  IntelWifi
//
//  Created by Roman Peshkov on 18/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include "IwlDvmOpMode.hpp"

#include <linux/etherdevice.h>

extern "C" {
#include "agn.h"
#include "dev.h"
}


/* line 35
 * initialize rxon structure with default values from eeprom
 */
void IwlDvmOpMode::iwl_connection_init_rx_config(struct iwl_priv *priv,
                                   struct iwl_rxon_context *ctx)
{
    memset(&ctx->staging, 0, sizeof(ctx->staging));
    
    if (!ctx->vif) {
        ctx->staging.dev_type = ctx->unused_devtype;
    } else
        switch (ctx->vif->type) {
            case NL80211_IFTYPE_AP:
                ctx->staging.dev_type = ctx->ap_devtype;
                break;
                
            case NL80211_IFTYPE_STATION:
                ctx->staging.dev_type = ctx->station_devtype;
                ctx->staging.filter_flags = RXON_FILTER_ACCEPT_GRP_MSK;
                break;
                
            case NL80211_IFTYPE_ADHOC:
                ctx->staging.dev_type = ctx->ibss_devtype;
                ctx->staging.flags = RXON_FLG_SHORT_PREAMBLE_MSK;
                ctx->staging.filter_flags = RXON_FILTER_BCON_AWARE_MSK |
                RXON_FILTER_ACCEPT_GRP_MSK;
                break;
                
            case NL80211_IFTYPE_MONITOR:
                ctx->staging.dev_type = RXON_DEV_TYPE_SNIFFER;
                break;
                
            default:
                IWL_ERR(priv, "Unsupported interface type %d\n",
                        ctx->vif->type);
                break;
        }
    
#if 0
    /* TODO:  Figure out when short_preamble would be set and cache from
     * that */
    if (!hw_to_local(priv->hw)->short_preamble)
        ctx->staging.flags &= ~RXON_FLG_SHORT_PREAMBLE_MSK;
    else
        ctx->staging.flags |= RXON_FLG_SHORT_PREAMBLE_MSK;
#endif
    
    ctx->staging.channel =
    cpu_to_le16(priv->hw->conf.chandef.chan->hw_value);
    priv->band = priv->hw->conf.chandef.chan->band;
    
    iwl_set_flags_for_band(priv, ctx, priv->band, ctx->vif);
    
    /* clear both MIX and PURE40 mode flag */
    ctx->staging.flags &= ~(RXON_FLG_CHANNEL_MODE_MIXED |
                            RXON_FLG_CHANNEL_MODE_PURE_40);
    if (ctx->vif)
        memcpy(ctx->staging.node_addr, ctx->vif->addr, ETH_ALEN);
    
    ctx->staging.ofdm_ht_single_stream_basic_rates = 0xff;
    ctx->staging.ofdm_ht_dual_stream_basic_rates = 0xff;
    ctx->staging.ofdm_ht_triple_stream_basic_rates = 0xff;
}

// line 212
int IwlDvmOpMode::iwlagn_send_rxon_assoc(struct iwl_priv *priv,
                                  struct iwl_rxon_context *ctx)
{
    int ret = 0;
    struct iwl_rxon_assoc_cmd rxon_assoc;
    const struct iwl_rxon_cmd *rxon1 = &ctx->staging;
    const struct iwl_rxon_cmd *rxon2 = &ctx->active;
    
    if ((rxon1->flags == rxon2->flags) &&
        (rxon1->filter_flags == rxon2->filter_flags) &&
        (rxon1->cck_basic_rates == rxon2->cck_basic_rates) &&
        (rxon1->ofdm_ht_single_stream_basic_rates ==
         rxon2->ofdm_ht_single_stream_basic_rates) &&
        (rxon1->ofdm_ht_dual_stream_basic_rates ==
         rxon2->ofdm_ht_dual_stream_basic_rates) &&
        (rxon1->ofdm_ht_triple_stream_basic_rates ==
         rxon2->ofdm_ht_triple_stream_basic_rates) &&
        (rxon1->acquisition_data == rxon2->acquisition_data) &&
        (rxon1->rx_chain == rxon2->rx_chain) &&
        (rxon1->ofdm_basic_rates == rxon2->ofdm_basic_rates)) {
        IWL_DEBUG_INFO(priv, "Using current RXON_ASSOC.  Not resending.\n");
        return 0;
    }
    
    rxon_assoc.flags = ctx->staging.flags;
    rxon_assoc.filter_flags = ctx->staging.filter_flags;
    rxon_assoc.ofdm_basic_rates = ctx->staging.ofdm_basic_rates;
    rxon_assoc.cck_basic_rates = ctx->staging.cck_basic_rates;
    rxon_assoc.reserved1 = 0;
    rxon_assoc.reserved2 = 0;
    rxon_assoc.reserved3 = 0;
    rxon_assoc.ofdm_ht_single_stream_basic_rates =
    ctx->staging.ofdm_ht_single_stream_basic_rates;
    rxon_assoc.ofdm_ht_dual_stream_basic_rates =
    ctx->staging.ofdm_ht_dual_stream_basic_rates;
    rxon_assoc.rx_chain_select_flags = ctx->staging.rx_chain;
    rxon_assoc.ofdm_ht_triple_stream_basic_rates =
    ctx->staging.ofdm_ht_triple_stream_basic_rates;
    rxon_assoc.acquisition_data = ctx->staging.acquisition_data;
    
    ret = iwl_dvm_send_cmd_pdu(priv, ctx->rxon_assoc_cmd,
                               CMD_ASYNC, sizeof(rxon_assoc), &rxon_assoc);
    return ret;
}

// line 402
int IwlDvmOpMode::iwl_set_tx_power(struct iwl_priv *priv, s8 tx_power, bool force)
{
    int ret;
    s8 prev_tx_power;
    bool defer;
    struct iwl_rxon_context *ctx = &priv->contexts[IWL_RXON_CTX_BSS];
    
    if (priv->calib_disabled & IWL_TX_POWER_CALIB_DISABLED)
        return 0;
    
    //lockdep_assert_held(&priv->mutex);
    
    if (priv->tx_power_user_lmt == tx_power && !force)
        return 0;
    
    if (tx_power < IWLAGN_TX_POWER_TARGET_POWER_MIN) {
        IWL_WARN(priv,
                 "Requested user TXPOWER %d below lower limit %d.\n",
                 tx_power,
                 IWLAGN_TX_POWER_TARGET_POWER_MIN);
        return -EINVAL;
    }
    
    if (tx_power > DIV_ROUND_UP(priv->nvm_data->max_tx_pwr_half_dbm, 2)) {
        IWL_WARN(priv,
                 "Requested user TXPOWER %d above upper limit %d.\n",
                 tx_power, priv->nvm_data->max_tx_pwr_half_dbm);
        return -EINVAL;
    }
    
    if (!iwl_is_ready_rf(priv))
        return -EIO;
    
    /* scan complete and commit_rxon use tx_power_next value,
     * it always need to be updated for newest request */
    priv->tx_power_next = tx_power;
    
    /* do not set tx power when scanning or channel changing */
    defer = test_bit(STATUS_SCANNING, &priv->status) ||
    memcmp(&ctx->active, &ctx->staging, sizeof(ctx->staging));
    if (defer && !force) {
        IWL_DEBUG_INFO(priv, "Deferring tx power set\n");
        return 0;
    }
    
    prev_tx_power = priv->tx_power_user_lmt;
    priv->tx_power_user_lmt = tx_power;
    
    ret = iwlagn_send_tx_power(priv);
    
    /* if fail to set tx_power, restore the orig. tx power */
    if (ret) {
        priv->tx_power_user_lmt = prev_tx_power;
        priv->tx_power_next = prev_tx_power;
    }
    return ret;
}



// line 736
void IwlDvmOpMode::iwl_set_flags_for_band(struct iwl_priv *priv,
                            struct iwl_rxon_context *ctx,
                            enum nl80211_band band,
                            struct ieee80211_vif *vif)
{
    if (band == NL80211_BAND_5GHZ) {
        ctx->staging.flags &=
        ~(RXON_FLG_BAND_24G_MSK | RXON_FLG_AUTO_DETECT_MSK
          | RXON_FLG_CCK_MSK);
        ctx->staging.flags |= RXON_FLG_SHORT_SLOT_MSK;
    } else {
        /* Copied from iwl_post_associate() */
        if (vif && vif->bss_conf.use_short_slot)
            ctx->staging.flags |= RXON_FLG_SHORT_SLOT_MSK;
        else
            ctx->staging.flags &= ~RXON_FLG_SHORT_SLOT_MSK;
        
        ctx->staging.flags |= RXON_FLG_BAND_24G_MSK;
        ctx->staging.flags |= RXON_FLG_AUTO_DETECT_MSK;
        ctx->staging.flags &= ~RXON_FLG_CCK_MSK;
    }
}

// line 759
static void iwl_set_rxon_hwcrypto(struct iwl_priv *priv,
                                  struct iwl_rxon_context *ctx, int hw_decrypt)
{
    struct iwl_rxon_cmd *rxon = &ctx->staging;
    
    if (hw_decrypt)
        rxon->filter_flags &= ~RXON_FILTER_DIS_DECRYPT_MSK;
    else
        rxon->filter_flags |= RXON_FILTER_DIS_DECRYPT_MSK;
    
}


// line 771
/* validate RXON structure is valid */
static int iwl_check_rxon_cmd(struct iwl_priv *priv,
                              struct iwl_rxon_context *ctx)
{
    struct iwl_rxon_cmd *rxon = &ctx->staging;
    u32 errors = 0;
    
    if (rxon->flags & RXON_FLG_BAND_24G_MSK) {
        if (rxon->flags & RXON_FLG_TGJ_NARROW_BAND_MSK) {
            IWL_WARN(priv, "check 2.4G: wrong narrow\n");
            errors |= BIT(0);
        }
        if (rxon->flags & RXON_FLG_RADAR_DETECT_MSK) {
            IWL_WARN(priv, "check 2.4G: wrong radar\n");
            errors |= BIT(1);
        }
    } else {
        if (!(rxon->flags & RXON_FLG_SHORT_SLOT_MSK)) {
            IWL_WARN(priv, "check 5.2G: not short slot!\n");
            errors |= BIT(2);
        }
        if (rxon->flags & RXON_FLG_CCK_MSK) {
            IWL_WARN(priv, "check 5.2G: CCK!\n");
            errors |= BIT(3);
        }
    }
    if ((rxon->node_addr[0] | rxon->bssid_addr[0]) & 0x1) {
        IWL_WARN(priv, "mac/bssid mcast!\n");
        errors |= BIT(4);
    }
    
    /* make sure basic rates 6Mbps and 1Mbps are supported */
    if ((rxon->ofdm_basic_rates & IWL_RATE_6M_MASK) == 0 &&
        (rxon->cck_basic_rates & IWL_RATE_1M_MASK) == 0) {
        IWL_WARN(priv, "neither 1 nor 6 are basic\n");
        errors |= BIT(5);
    }
    
    if (le16_to_cpu(rxon->assoc_id) > 2007) {
        IWL_WARN(priv, "aid > 2007\n");
        errors |= BIT(6);
    }
    
    if ((rxon->flags & (RXON_FLG_CCK_MSK | RXON_FLG_SHORT_SLOT_MSK))
        == (RXON_FLG_CCK_MSK | RXON_FLG_SHORT_SLOT_MSK)) {
        IWL_WARN(priv, "CCK and short slot\n");
        errors |= BIT(7);
    }
    
    if ((rxon->flags & (RXON_FLG_CCK_MSK | RXON_FLG_AUTO_DETECT_MSK))
        == (RXON_FLG_CCK_MSK | RXON_FLG_AUTO_DETECT_MSK)) {
        IWL_WARN(priv, "CCK and auto detect\n");
        errors |= BIT(8);
    }
    
    if ((rxon->flags & (RXON_FLG_AUTO_DETECT_MSK |
                        RXON_FLG_TGG_PROTECT_MSK)) ==
        RXON_FLG_TGG_PROTECT_MSK) {
        IWL_WARN(priv, "TGg but no auto-detect\n");
        errors |= BIT(9);
    }
    
    if (rxon->channel == 0) {
        IWL_WARN(priv, "zero channel is invalid\n");
        errors |= BIT(10);
    }
    
//    WARN(errors, "Invalid RXON (%#x), channel %d",
//         errors, le16_to_cpu(rxon->channel));
    
    return errors ? -EINVAL : 0;
}

/** line 844
 * iwl_full_rxon_required - check if full RXON (vs RXON_ASSOC) cmd is needed
 * @priv: staging_rxon is compared to active_rxon
 *
 * If the RXON structure is changing enough to require a new tune,
 * or is clearing the RXON_FILTER_ASSOC_MSK, then return 1 to indicate that
 * a new tune (full RXON command, rather than RXON_ASSOC cmd) is required.
 */
static int iwl_full_rxon_required(struct iwl_priv *priv,
                                  struct iwl_rxon_context *ctx)
{
    const struct iwl_rxon_cmd *staging = &ctx->staging;
    const struct iwl_rxon_cmd *active = &ctx->active;
    
#define CHK(cond)                            \
if ((cond)) {                            \
IWL_DEBUG_INFO(priv, "need full RXON - " #cond "\n");    \
return 1;                        \
}
    
#define CHK_NEQ(c1, c2)                        \
if ((c1) != (c2)) {                    \
IWL_DEBUG_INFO(priv, "need full RXON - "    \
#c1 " != " #c2 " - %d != %d\n",    \
(c1), (c2));            \
return 1;                    \
}
    
    /* These items are only settable from the full RXON command */
    CHK(!iwl_is_associated_ctx(ctx));
    CHK(!ether_addr_equal(staging->bssid_addr, active->bssid_addr));
    CHK(!ether_addr_equal(staging->node_addr, active->node_addr));
    CHK(!ether_addr_equal(staging->wlap_bssid_addr,
                          active->wlap_bssid_addr));
    CHK_NEQ(staging->dev_type, active->dev_type);
    CHK_NEQ(staging->channel, active->channel);
    CHK_NEQ(staging->air_propagation, active->air_propagation);
    CHK_NEQ(staging->ofdm_ht_single_stream_basic_rates,
            active->ofdm_ht_single_stream_basic_rates);
    CHK_NEQ(staging->ofdm_ht_dual_stream_basic_rates,
            active->ofdm_ht_dual_stream_basic_rates);
    CHK_NEQ(staging->ofdm_ht_triple_stream_basic_rates,
            active->ofdm_ht_triple_stream_basic_rates);
    CHK_NEQ(staging->assoc_id, active->assoc_id);
    
    /* flags, filter_flags, ofdm_basic_rates, and cck_basic_rates can
     * be updated with the RXON_ASSOC command -- however only some
     * flag transitions are allowed using RXON_ASSOC */
    
    /* Check if we are not switching bands */
    CHK_NEQ(staging->flags & RXON_FLG_BAND_24G_MSK,
            active->flags & RXON_FLG_BAND_24G_MSK);
    
    /* Check if we are switching association toggle */
    CHK_NEQ(staging->filter_flags & RXON_FILTER_ASSOC_MSK,
            active->filter_flags & RXON_FILTER_ASSOC_MSK);
    
#undef CHK
#undef CHK_NEQ
    
    return 0;
}



// line 934
static void iwl_calc_basic_rates(struct iwl_priv *priv,
                                 struct iwl_rxon_context *ctx)
{
    int lowest_present_ofdm = 100;
    int lowest_present_cck = 100;
    u8 cck = 0;
    u8 ofdm = 0;
    
    if (ctx->vif) {
        struct ieee80211_supported_band *sband;
//        unsigned long basic = ctx->vif->bss_conf.basic_rates;
//        int i;
        
        sband = priv->hw->wiphy->bands[priv->hw->conf.chandef.chan->band];
        
        // TODO: Implement
//        for_each_set_bit(i, &basic, BITS_PER_LONG) {
//            int hw = sband->bitrates[i].hw_value;
//            if (hw >= IWL_FIRST_OFDM_RATE) {
//                ofdm |= BIT(hw - IWL_FIRST_OFDM_RATE);
//                if (lowest_present_ofdm > hw)
//                    lowest_present_ofdm = hw;
//            } else {
//                //BUILD_BUG_ON(IWL_FIRST_CCK_RATE != 0);
//
//                cck |= BIT(hw);
//                if (lowest_present_cck > hw)
//                    lowest_present_cck = hw;
//            }
//        }
    }
    
    /*
     * Now we've got the basic rates as bitmaps in the ofdm and cck
     * variables. This isn't sufficient though, as there might not
     * be all the right rates in the bitmap. E.g. if the only basic
     * rates are 5.5 Mbps and 11 Mbps, we still need to add 1 Mbps
     * and 6 Mbps because the 802.11-2007 standard says in 9.6:
     *
     *    [...] a STA responding to a received frame shall transmit
     *    its Control Response frame [...] at the highest rate in the
     *    BSSBasicRateSet parameter that is less than or equal to the
     *    rate of the immediately previous frame in the frame exchange
     *    sequence ([...]) and that is of the same modulation class
     *    ([...]) as the received frame. If no rate contained in the
     *    BSSBasicRateSet parameter meets these conditions, then the
     *    control frame sent in response to a received frame shall be
     *    transmitted at the highest mandatory rate of the PHY that is
     *    less than or equal to the rate of the received frame, and
     *    that is of the same modulation class as the received frame.
     *
     * As a consequence, we need to add all mandatory rates that are
     * lower than all of the basic rates to these bitmaps.
     */
    
    if (IWL_RATE_24M_INDEX < lowest_present_ofdm)
        ofdm |= IWL_RATE_24M_MASK >> IWL_FIRST_OFDM_RATE;
    if (IWL_RATE_12M_INDEX < lowest_present_ofdm)
        ofdm |= IWL_RATE_12M_MASK >> IWL_FIRST_OFDM_RATE;
    /* 6M already there or needed so always add */
    ofdm |= IWL_RATE_6M_MASK >> IWL_FIRST_OFDM_RATE;
    
    /*
     * CCK is a bit more complex with DSSS vs. HR/DSSS vs. ERP.
     * Note, however:
     *  - if no CCK rates are basic, it must be ERP since there must
     *    be some basic rates at all, so they're OFDM => ERP PHY
     *    (or we're in 5 GHz, and the cck bitmap will never be used)
     *  - if 11M is a basic rate, it must be ERP as well, so add 5.5M
     *  - if 5.5M is basic, 1M and 2M are mandatory
     *  - if 2M is basic, 1M is mandatory
     *  - if 1M is basic, that's the only valid ACK rate.
     * As a consequence, it's not as complicated as it sounds, just add
     * any lower rates to the ACK rate bitmap.
     */
    if (IWL_RATE_11M_INDEX < lowest_present_cck)
        cck |= IWL_RATE_11M_MASK >> IWL_FIRST_CCK_RATE;
    if (IWL_RATE_5M_INDEX < lowest_present_cck)
        cck |= IWL_RATE_5M_MASK >> IWL_FIRST_CCK_RATE;
    if (IWL_RATE_2M_INDEX < lowest_present_cck)
        cck |= IWL_RATE_2M_MASK >> IWL_FIRST_CCK_RATE;
    /* 1M already there or needed so always add */
    cck |= IWL_RATE_1M_MASK >> IWL_FIRST_CCK_RATE;
    
    IWL_DEBUG_RATE(priv, "Set basic rates cck:0x%.2x ofdm:0x%.2x\n",
                   cck, ofdm);
    
    /* "basic_rates" is a misnomer here -- should be called ACK rates */
    ctx->staging.cck_basic_rates = cck;
    ctx->staging.ofdm_basic_rates = ofdm;
}


/** line 1025
 * iwlagn_commit_rxon - commit staging_rxon to hardware
 *
 * The RXON command in staging_rxon is committed to the hardware and
 * the active_rxon structure is updated with the new data.  This
 * function correctly transitions out of the RXON_ASSOC_MSK state if
 * a HW tune is required based on the RXON structure changes.
 *
 * The connect/disconnect flow should be as the following:
 *
 * 1. make sure send RXON command with association bit unset if not connect
 *    this should include the channel and the band for the candidate
 *    to be connected to
 * 2. Add Station before RXON association with the AP
 * 3. RXON_timing has to send before RXON for connection
 * 4. full RXON command - associated bit set
 * 5. use RXON_ASSOC command to update any flags changes
 */
int IwlDvmOpMode::iwlagn_commit_rxon(struct iwl_priv *priv, struct iwl_rxon_context *ctx)
{
    /* cast away the const for active_rxon in this function */
    struct iwl_rxon_cmd *active = (struct iwl_rxon_cmd *)&ctx->active;
    bool new_assoc = !!(ctx->staging.filter_flags & RXON_FILTER_ASSOC_MSK);
    int ret;
    
    //lockdep_assert_held(&priv->mutex);
    
    if (!iwl_is_alive(priv))
        return -EBUSY;
    
    /* This function hardcodes a bunch of dual-mode assumptions */
    //BUILD_BUG_ON(NUM_IWL_RXON_CTX != 2);
    
    if (!ctx->is_active)
        return 0;
    
    /* always get timestamp with Rx frame */
    ctx->staging.flags |= RXON_FLG_TSF2HOST_MSK;
    
    /* recalculate basic rates */
    iwl_calc_basic_rates(priv, ctx);
    
    /*
     * force CTS-to-self frames protection if RTS-CTS is not preferred
     * one aggregation protection method
     */
    if (!priv->hw_params.use_rts_for_aggregation)
        ctx->staging.flags |= RXON_FLG_SELF_CTS_EN;
    
    if ((ctx->vif && ctx->vif->bss_conf.use_short_slot) ||
        !(ctx->staging.flags & RXON_FLG_BAND_24G_MSK))
        ctx->staging.flags |= RXON_FLG_SHORT_SLOT_MSK;
    else
        ctx->staging.flags &= ~RXON_FLG_SHORT_SLOT_MSK;
    
    iwl_print_rx_config_cmd(priv, ctx->ctxid);
    ret = iwl_check_rxon_cmd(priv, ctx);
    if (ret) {
        IWL_ERR(priv, "Invalid RXON configuration. Not committing.\n");
        return -EINVAL;
    }
    
    /*
     * receive commit_rxon request
     * abort any previous channel switch if still in process
     */
    if (test_bit(STATUS_CHANNEL_SWITCH_PENDING, &priv->status) &&
        (priv->switch_channel != ctx->staging.channel)) {
        IWL_DEBUG_11H(priv, "abort channel switch on %d\n",
                      le16_to_cpu(priv->switch_channel));
        iwl_chswitch_done(priv, false);
    }
    
    /*
     * If we don't need to send a full RXON, we can use
     * iwl_rxon_assoc_cmd which is used to reconfigure filter
     * and other flags for the current radio configuration.
     */
    if (!iwl_full_rxon_required(priv, ctx)) {
        ret = iwlagn_send_rxon_assoc(priv, ctx);
        if (ret) {
            IWL_ERR(priv, "Error setting RXON_ASSOC (%d)\n", ret);
            return ret;
        }
        
        memcpy(active, &ctx->staging, sizeof(*active));
        /*
         * We do not commit tx power settings while channel changing,
         * do it now if after settings changed.
         */
        iwl_set_tx_power(priv, priv->tx_power_next, false);
        
        /* make sure we are in the right PS state */
        iwl_power_update_mode(priv, true);
        
        return 0;
    }
    
    iwl_set_rxon_hwcrypto(priv, ctx, !iwlwifi_mod_params.swcrypto);
    
    IWL_DEBUG_INFO(priv,
                   "Going to commit RXON\n"
                   "  * with%s RXON_FILTER_ASSOC_MSK\n"
                   "  * channel = %d\n"
                   "  * bssid = %pM\n",
                   (new_assoc ? "" : "out"),
                   le16_to_cpu(ctx->staging.channel),
                   ctx->staging.bssid_addr);
    
    /*
     * Always clear associated first, but with the correct config.
     * This is required as for example station addition for the
     * AP station must be done after the BSSID is set to correctly
     * set up filters in the device.
     */
    // TODO: Implement
//    ret = iwlagn_rxon_disconn(priv, ctx);
//    if (ret)
//        return ret;
//
//    ret = iwlagn_set_pan_params(priv);
//    if (ret)
//        return ret;
//
//    if (new_assoc)
//        return iwlagn_rxon_connect(priv, ctx);
    
    return 0;
}


