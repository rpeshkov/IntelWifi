//
//  IwlDvmOpMode.hpp
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef IwlDvmOpMode_hpp
#define IwlDvmOpMode_hpp

#include "IwlOpModeOps.h"
#include "IntelEeprom.hpp"

extern "C" {
#include <linux/mac80211.h>
}


class IwlDvmOpMode : public IwlOpModeOps {
public:
    IwlDvmOpMode(IwlTransOps *ops, IntelIO *io, IntelEeprom *eeprom);
    virtual struct iwl_priv *start(struct iwl_trans *trans,
                                const struct iwl_cfg *cfg,
                                const struct iwl_fw *fw,
                                struct dentry *dbgfs_dir) override;
    virtual void nic_config(struct iwl_priv *priv) override;

    
private:
    struct iwl_priv *iwl_op_mode_dvm_start(struct iwl_trans *trans,
                                        const struct iwl_cfg *cfg,
                                        const struct iwl_fw *fw,
                                        struct dentry *dbgfs_dir); // line 1232
    
    // main.c
    int iwl_send_statistics_request(struct iwl_priv *priv, u8 flags, bool clear); // line 374
    void iwl_rf_kill_ct_config(struct iwl_priv *priv); // line 678
    int iwlagn_send_calib_cfg_rt(struct iwl_priv *priv, u32 cfg); // line 723
    int iwlagn_send_tx_ant_config(struct iwl_priv *priv, u8 valid_tx_ant); // line 740
    void iwl_send_bt_config(struct iwl_priv *priv); // line 757
    int iwl_alive_start(struct iwl_priv *priv); // line 780
    int iwl_init_drv(struct iwl_priv *priv); // line 1112
    
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
    u8 iwl_prep_station(struct iwl_priv *priv, struct iwl_rxon_context *ctx,
                                      const u8 *addr, bool is_ap, struct ieee80211_sta *sta); // line 252
    int iwlagn_alloc_bcast_station(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 1276
    
    // lib.c
    void iwlagn_set_rxon_chain(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 790
    int iwl_dvm_send_cmd(struct iwl_priv *priv, struct iwl_host_cmd *cmd); // line 1237
    int iwl_dvm_send_cmd_pdu(struct iwl_priv *priv, u8 id, u32 flags, u16 len, const void *data); // line 1271
    
    // rxon.c
    void iwl_connection_init_rx_config(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 35
    int iwlagn_send_rxon_assoc(struct iwl_priv *priv,
                               struct iwl_rxon_context *ctx); // line 212
    int iwl_set_tx_power(struct iwl_priv *priv, s8 tx_power, bool force); // line 402
    void iwl_set_flags_for_band(struct iwl_priv *priv,
                                struct iwl_rxon_context *ctx,
                                enum nl80211_band band,
                                struct ieee80211_vif *vif); // line 736
    int iwlagn_commit_rxon(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 1025
    
    // calib.c
    int iwl_send_calib_results(struct iwl_priv *priv); // line 93
    static int iwl_calib_set(struct iwl_priv *priv,
                      const struct iwl_calib_hdr *cmd, int len); // line 117
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
    //void iwl_setup_rx_handlers(struct iwl_priv *priv); // line 945

    
    
    

    
    IwlTransOps *_ops;
    IntelEeprom *_eeprom;
    IntelIO *_io;
    struct iwl_priv *priv;
};

#endif /* IwlDvmOpMode_hpp */
