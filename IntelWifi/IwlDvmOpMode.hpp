//
//  IwlDvmOpMode.hpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef IwlDvmOpMode_hpp
#define IwlDvmOpMode_hpp

extern "C" {
#include <linux/mac80211.h>
    #include <net/cfg80211.h>
#include <linux/jiffies.h>
}

#include <IOKit/IOBufferMemoryDescriptor.h>

#include "IwlOpModeOps.h"
#include "dev.h"

#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_BYTES(x) (x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5]


class IwlDvmOpMode : public IwlOpModeOps {
public:
    IwlDvmOpMode(IwlTransOps *ops);
    virtual struct ieee80211_hw *start(struct iwl_trans *trans,
                                const struct iwl_cfg *cfg,
                                const struct iwl_fw *fw,
                                struct dentry *dbgfs_dir) override;
    virtual void nic_config(struct iwl_priv *priv) override;
    
    virtual void stop(struct iwl_priv *priv) override;
    virtual void rx(struct iwl_priv *priv, struct napi_struct *napi,
                    struct iwl_rx_cmd_buffer *rxb) override;
    
    virtual void scan() override;

    
private:
    // main.c
    void iwl_update_chain_flags(struct iwl_priv *priv); // line 162
    int iwl_send_statistics_request(struct iwl_priv *priv, u8 flags, bool clear); // line 374
    void iwl_rf_kill_ct_config(struct iwl_priv *priv); // line 678
    int iwlagn_send_calib_cfg_rt(struct iwl_priv *priv, u32 cfg); // line 723
    int iwlagn_send_tx_ant_config(struct iwl_priv *priv, u8 valid_tx_ant); // line 740
    void iwl_send_bt_config(struct iwl_priv *priv); // line 757
    int iwl_alive_start(struct iwl_priv *priv); // line 780
    void iwl_down(struct iwl_priv *priv); // line 916
    int iwl_init_drv(struct iwl_priv *priv); // line 1112
    struct iwl_priv *iwl_op_mode_dvm_start(struct iwl_trans *trans,
                                           const struct iwl_cfg *cfg,
                                           const struct iwl_fw *fw,
                                           struct dentry *dbgfs_dir); // line 1232
    void iwl_op_mode_dvm_stop(struct iwl_priv* priv); // line 1524
    
    // mac80211.c
    int __iwl_up(struct iwl_priv *priv); // line 238
    int iwlagn_mac_start(struct iwl_priv *priv); // line 296
    void iwl_chswitch_done(struct iwl_priv *priv, bool is_success); // line 1048
    struct ieee80211_hw *iwl_alloc_all(void); // line 1637
    
    // ucode.c
    int iwl_init_alive_start(struct iwl_priv *priv);
    void iwl_send_prio_tbl(struct iwl_priv *priv);
    int iwl_send_bt_env(struct iwl_priv *priv, u8 action, u8 type);
    int iwl_load_ucode_wait_alive(struct iwl_priv *priv,
                                  enum iwl_ucode_type ucode_type);
    int iwl_run_init_ucode(struct iwl_priv *priv);
    int iwl_alive_notify(struct iwl_priv *priv);
    void iwl_nic_config(struct iwl_priv *priv);
    int iwl_set_Xtal_calib(struct iwl_priv *priv);
    int iwl_send_calib_cfg(struct iwl_priv *priv);
    int iwl_set_temperature_offset_calib(struct iwl_priv *priv);
    int iwl_set_temperature_offset_calib_v2(struct iwl_priv *priv);
    int iwl_send_wimax_coex(struct iwl_priv *priv);
    static bool iwlagn_wait_calib(struct iwl_notif_wait_data *notif_wait,
                           struct iwl_rx_packet *pkt, void *data);
    
    // sta.c
    int iwl_send_add_sta(struct iwl_priv *priv, struct iwl_addsta_cmd *sta, u8 flags); // line 102
    int iwl_sta_update_ht(struct iwl_priv *priv, struct iwl_rxon_context *ctx, struct ieee80211_sta *sta); // line 213
//    u8 iwl_prep_station(struct iwl_priv *priv, struct iwl_rxon_context *ctx,
//                                      const u8 *addr, bool is_ap, struct ieee80211_sta *sta); // line 252
    int iwl_add_station_common(struct iwl_priv *priv, struct iwl_rxon_context *ctx, const u8 *addr, bool is_ap, struct ieee80211_sta *sta, u8 *sta_id_r); // line 341
    int iwl_send_remove_station(struct iwl_priv *priv, const u8 *addr, int sta_id, bool temporary); // line 420
    int iwl_remove_station(struct iwl_priv *priv, const u8 sta_id, const u8 *addr); // line 469
    void iwl_clear_ucode_stations(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 619
    void iwl_restore_stations(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 654
    int iwl_send_lq_cmd(struct iwl_priv *priv, struct iwl_rxon_context *ctx,
                                      struct iwl_link_quality_cmd *lq, u8 flags, bool init); // line 825
    int iwlagn_add_bssid_station(struct iwl_priv *priv, struct iwl_rxon_context *ctx, const u8 *addr, u8 *sta_id_r); // line 898
    
    int iwl_send_static_wepkey_cmd(struct iwl_priv *priv,
                                          struct iwl_rxon_context *ctx,
                                          bool send_if_empty); // line 947
    
    int iwl_restore_default_wep_keys(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 1001
    
    
    
    int iwl_remove_default_wep_key(struct iwl_priv *priv,
                                   struct iwl_rxon_context *ctx,
                                   struct ieee80211_key_conf *keyconf); // line 1009
    
    
    
    int iwl_set_default_wep_key(struct iwl_priv *priv,
                                struct iwl_rxon_context *ctx,
                                struct ieee80211_key_conf *keyconf); // line 1034
    int iwlagn_send_sta_key(struct iwl_priv *priv, struct ieee80211_key_conf *keyconf, u8 sta_id, u32 tkip_iv32, u16 *tkip_p1k, u32 cmd_flags); // line 1097
    void iwl_update_tkip_key(struct iwl_priv *priv, struct ieee80211_vif *vif, struct ieee80211_key_conf *keyconf, struct ieee80211_sta *sta, u32 iv32, u16 *phase1key); // line 1150
    int iwl_remove_dynamic_key(struct iwl_priv *priv, struct iwl_rxon_context *ctx,
                               struct ieee80211_key_conf *keyconf,
                               struct ieee80211_sta *sta); // line 1170
    int iwl_set_dynamic_key(struct iwl_priv *priv, struct iwl_rxon_context *ctx, struct ieee80211_key_conf *keyconf, struct ieee80211_sta *sta); // line 1218
    int iwlagn_alloc_bcast_station(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 1276
    int iwl_update_bcast_station(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 1316
    
    // lib.c
    void iwlagn_set_rxon_chain(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 790
    int iwl_dvm_send_cmd(struct iwl_priv *priv, struct iwl_host_cmd *cmd); // line 1237
    int iwl_dvm_send_cmd_pdu(struct iwl_priv *priv, u8 id, u32 flags, u16 len, const void *data); // line 1271
    
    // rxon.c
    void iwl_connection_init_rx_config(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 35
    int iwlagn_disable_bss(struct iwl_priv *priv, struct iwl_rxon_context *ctx, struct iwl_rxon_cmd *send); // line 99
    int iwlagn_disable_pan(struct iwl_priv *priv, struct iwl_rxon_context *ctx, struct iwl_rxon_cmd *send); // line 119
    int iwlagn_disconn_pan(struct iwl_priv *priv, struct iwl_rxon_context *ctx, struct iwl_rxon_cmd *send); // line 156
    void iwlagn_update_qos(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 172
    int iwlagn_send_rxon_assoc(struct iwl_priv *priv,
                               struct iwl_rxon_context *ctx); // line 212
    int iwl_send_rxon_timing(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 292
    int iwlagn_rxon_disconn(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 360
    int iwl_set_tx_power(struct iwl_priv *priv, s8 tx_power, bool force); // line 402
    int iwlagn_set_pan_params(struct iwl_priv *priv); // line 529
    int iwlagn_rxon_connect(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 560
    void iwl_set_flags_for_band(struct iwl_priv *priv,
                                struct iwl_rxon_context *ctx,
                                enum nl80211_band band,
                                struct ieee80211_vif *vif); // line 736
    int iwlagn_commit_rxon(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 1025
    void iwlagn_post_scan(struct iwl_priv *priv); // line 1547
    
    // calib.c
    int iwl_send_calib_results(struct iwl_priv *priv); // line 93
    static int iwl_calib_set(struct iwl_priv *priv, const struct iwl_calib_hdr *cmd, int len); // line 117
    int iwl_sensitivity_write(struct iwl_priv *priv); // line 470
    int iwl_enhance_sensitivity_write(struct iwl_priv *priv); // line 505
    void iwl_init_sensitivity(struct iwl_priv *priv); // line 594
    void iwl_reset_run_time_calib(struct iwl_priv *priv); // line 1098
    
    // power.c
    int iwl_set_power(struct iwl_priv *priv, struct iwl_powertable_cmd *cmd);
    int iwl_power_update_mode(struct iwl_priv *priv, bool force);
    void iwl_power_initialize(struct iwl_priv *priv);
    int iwl_power_set_mode(struct iwl_priv *priv, struct iwl_powertable_cmd *cmd,
                           bool force);
    
    // lib.c
    int iwlagn_send_tx_power(struct iwl_priv *priv); // line 49
    void iwlagn_send_advance_bt_config(struct iwl_priv *priv); // line 224
    
    // rx.c
    void iwl_setup_rx_handlers(struct iwl_priv *priv); // line 945
    void iwl_rx_dispatch(struct iwl_priv *priv, struct napi_struct *napi,
                                       struct iwl_rx_cmd_buffer *rxb); // line 1001
    
    // scan.c
    int iwl_send_scan_abort(struct iwl_priv *priv); // line 57
    void iwl_process_scan_complete(struct iwl_priv *priv); // line 112
    void iwl_force_scan_end(struct iwl_priv *priv); // line 165
    void iwl_do_scan_abort(struct iwl_priv *priv); // line 182
    void iwl_scan_cancel_timeout(struct iwl_priv *priv, unsigned long ms); // line 216
    void iwl_setup_rx_scan_handlers(struct iwl_priv *priv); // line 357
    int iwlagn_request_scan(struct iwl_priv *priv, struct ieee80211_vif *vif); // line 632
    void iwl_init_scan_params(struct iwl_priv *priv); // line 929
    int iwl_scan_initiate(struct iwl_priv *priv,
                                        struct ieee80211_vif *vif,
                                        enum iwl_scan_type scan_type,
                                        enum nl80211_band band); // line 938

    
    static void iwl_rx_reply_scan(struct iwl_priv *priv, struct iwl_rx_cmd_buffer *rxb);
    static void iwl_rx_scan_start_notif(struct iwl_priv *priv, struct iwl_rx_cmd_buffer *rxb);
    static void iwl_rx_scan_results_notif(struct iwl_priv *priv, struct iwl_rx_cmd_buffer *rxb);
    static void iwl_rx_scan_complete_notif(struct iwl_priv *priv, struct iwl_rx_cmd_buffer *rxb);
    
    // agn.h
    void iwl_dvm_set_pmi(struct iwl_priv *priv, bool state);
    
    IwlTransOps *_ops;
    
    struct iwl_priv *priv;
};


#endif /* IwlDvmOpMode_hpp */
