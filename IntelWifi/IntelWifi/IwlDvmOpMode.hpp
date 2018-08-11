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
    
    #include "iwl-debug.h"
    #include "iw_utils/allocation.h"
    
}

#include <IOKit/IOBufferMemoryDescriptor.h>

#include "IwlOpModeOps.h"
#include "TransOps.h"


extern "C" {

    #include "tt.h"
#include "dev.h"
#include "agn.h"
    #include "calib.h"
}


#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_BYTES(x) (x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5]


class IwlDvmOpMode : public IwlOpModeOps {
public:
    IwlDvmOpMode(TransOps *ops);
    struct ieee80211_hw *start(struct iwl_trans *trans, const struct iwl_cfg *cfg,
                               const struct iwl_fw *fw) override;
    void nic_config(struct iwl_priv *priv) override;
    
    void stop(struct iwl_priv *priv) override;
    void rx(struct iwl_priv *priv, struct napi_struct *napi, struct iwl_rx_cmd_buffer *rxb) override;
    
//    void add_interface(struct ieee80211_vif *vif) override;
//    void channel_switch(struct iwl_priv *priv, struct ieee80211_vif *vif, struct ieee80211_channel_switch *chsw) override;
    
    IOReturn getCARD_CAPABILITIES(IO80211Interface *interface, struct apple80211_capability_data *cd) override;
    
    IOReturn getPHY_MODE(IO80211Interface *interface, struct apple80211_phymode_data *pd) override;
    
    IOReturn getPOWER(IO80211Interface *intf, apple80211_power_data *power_data) override;
    IOReturn setPOWER(IO80211Interface *intf, apple80211_power_data *power_data) override;

    
private:
    // main.c
    void iwl_down(struct iwl_priv *priv); // line 916
    struct iwl_priv *iwl_op_mode_dvm_start(struct iwl_trans *trans,
                                           const struct iwl_cfg *cfg,
                                           const struct iwl_fw *fw); // line 1232
    void iwl_op_mode_dvm_stop(struct iwl_priv* priv); // line 1524
    
    // mac80211.c
    int __iwl_up(struct iwl_priv *priv); // line 238
    int iwlagn_mac_start(struct iwl_priv *priv); // line 296
    void iwlagn_mac_stop(struct iwl_priv *priv); // line 323
    void iwlagn_mac_channel_switch(struct iwl_priv *priv,
                                                 struct ieee80211_vif *vif,
                                                 struct ieee80211_channel_switch *ch_switch); // line 964

    int iwl_setup_interface(struct iwl_priv *priv, struct iwl_rxon_context *ctx); // line 1251
    int iwlagn_mac_add_interface(struct iwl_priv *priv, struct ieee80211_vif *vif); // line 1297
    
    // ucode.c
    int iwl_load_ucode_wait_alive(struct iwl_priv *priv,
                                  enum iwl_ucode_type ucode_type);
    int iwl_run_init_ucode(struct iwl_priv *priv);
    int iwl_alive_notify(struct iwl_priv *priv);
    void iwl_nic_config(struct iwl_priv *priv);
    static bool iwlagn_wait_calib(struct iwl_notif_wait_data *notif_wait,
                           struct iwl_rx_packet *pkt, void *data);
    

    
    TransOps *_ops;
    
    struct iwl_priv *priv;
};


#endif /* IwlDvmOpMode_hpp */
