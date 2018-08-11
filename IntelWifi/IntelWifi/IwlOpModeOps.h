//
//  IwlOpMode.h
//  IntelWifi
//
//  Created by Roman Peshkov on 07/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef IwlOpModeOps_h
#define IwlOpModeOps_h



extern "C" {
#include "iwl-trans.h"
}

#include <IOKit/IOLib.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOFilterInterruptEventSource.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/network/IOPacketQueue.h>
#include <IOKit/IOMemoryCursor.h>


#include "apple80211/IO80211Controller.h"
#include "apple80211/IO80211Interface.h"




class IwlOpModeOps {
public:
    
    virtual struct ieee80211_hw *start(struct iwl_trans *trans,
                                 const struct iwl_cfg *cfg,
                                 const struct iwl_fw *fw) = 0;
    virtual void nic_config(struct iwl_priv *priv) = 0;
    virtual void stop(struct iwl_priv *priv) = 0;
    virtual void rx(struct iwl_priv *priv, struct napi_struct *napi, struct iwl_rx_cmd_buffer *rxb) = 0;
    
    
    // IOCTLs
    // 12
    virtual IOReturn getCARD_CAPABILITIES(IO80211Interface *interface, struct apple80211_capability_data *cd) = 0;
    // 14
    virtual IOReturn getPHY_MODE(IO80211Interface *interface, struct apple80211_phymode_data *pd) = 0;
    // 19
    virtual IOReturn getPOWER(IO80211Interface *intf, apple80211_power_data *power_data) = 0;
    virtual IOReturn setPOWER(IO80211Interface *intf, apple80211_power_data *power_data) = 0;
    
    
    // Linux calls
    
//    virtual void add_interface(struct ieee80211_vif *vif) = 0;
//    virtual void channel_switch(struct iwl_priv *priv, struct ieee80211_vif *vif, struct ieee80211_channel_switch *chsw) = 0;

//    void (*rx_rss)(struct iwl_op_mode *op_mode, struct napi_struct *napi,
//                   struct iwl_rx_cmd_buffer *rxb, unsigned int queue);
//    void (*async_cb)(struct iwl_op_mode *op_mode,
//                     const struct iwl_device_cmd *cmd);
//    void (*queue_full)(struct iwl_op_mode *op_mode, int queue);
//    void (*queue_not_full)(struct iwl_op_mode *op_mode, int queue);
//    bool (*hw_rf_kill)(struct iwl_op_mode *op_mode, bool state);
//    void (*free_skb)(struct iwl_op_mode *op_mode, struct sk_buff *skb);
//    void (*nic_error)(struct iwl_op_mode *op_mode);
//    void (*cmd_queue_full)(struct iwl_op_mode *op_mode);
//    void (*nic_config)(struct iwl_op_mode *op_mode);
//    void (*wimax_active)(struct iwl_op_mode *op_mode);
//    int (*enter_d0i3)(struct iwl_op_mode *op_mode);
//    int (*exit_d0i3)(struct iwl_op_mode *op_mode);

};

#endif /* IwlOpModeOps_h */
