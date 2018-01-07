//
//  IntelEeprom.hpp
//  IntelWifi
//
//  Created by Roman Peshkov on 27/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef IntelEeprom_hpp
#define IntelEeprom_hpp

#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSMetaClass.h>

#include <sys/errno.h>

#include <IOKit/IOLib.h>

extern "C" {
#include "iwlwifi/iwl-eeprom-parse.h"
#include "iwl-config.h"
}


extern "C" {
//#include "porting/net/cfg80211.h"
}


#include "IntelIO.hpp"


class IntelEeprom : public OSObject {
    OSDeclareDefaultStructors(IntelEeprom)
    
public:
    static IntelEeprom* withIO(IntelIO *io, struct iwl_cfg *config, UInt32 hwRev);
    bool initWithIO(IntelIO *io, struct iwl_cfg *config, UInt32 hwRev);
    virtual void free() override;
    bool read();
    struct iwl_nvm_data* parse();
    int iwl_nvm_check_version(struct iwl_nvm_data *data, struct iwl_trans *trans);
    
private:
    // reading
    int iwl_eeprom_acquire_semaphore();
    void iwl_eeprom_release_semaphore();
    int iwl_nvm_is_otp();
    int iwl_eeprom_verify_signature(bool nvm_is_otp);
    int iwl_init_otp_access();
    int iwl_find_otp_image(UInt16 *validblockaddr);
    int iwl_read_otp_word(u16 addr, __le16 *eeprom_data);
    void iwl_set_otp_access_absolute();
    bool iwl_is_otp_empty();
    
    // parsing
    const UInt8 *iwl_eeprom_query_addr(UInt32 offset);
    UInt32 eeprom_indirect_address(UInt32 address);
    UInt16 iwl_eeprom_query16(int offset);
    int iwl_eeprom_read_calib(struct iwl_nvm_data *data);
    void iwl_init_sbands(struct iwl_nvm_data *data);
    int iwl_init_channel_map(struct iwl_nvm_data *data);
    int iwl_init_sband_channels(struct iwl_nvm_data *data,
                                struct ieee80211_supported_band *sband,
                                int n_channels, enum nl80211_band band);
    void iwl_init_ht_hw_capab(struct iwl_nvm_data *data,
                                           struct ieee80211_sta_ht_cap *ht_info,
                                           enum nl80211_band band,
                                           u8 tx_chains, u8 rx_chains);
    void iwl_init_band_reference(int eeprom_band, int *eeprom_ch_count,
                                 const struct iwl_eeprom_channel **ch_info,
                                 const u8 **eeprom_ch_array);
    void iwl_eeprom_enhanced_txpower(struct iwl_nvm_data *data,
                                     int n_channels);
    s8 iwl_get_max_txpwr_half_dbm(const struct iwl_nvm_data *data,
                                  struct iwl_eeprom_enhanced_txpwr *txp);
    void iwl_eeprom_enh_txp_read_element(struct iwl_nvm_data *data,
                                                      struct iwl_eeprom_enhanced_txpwr *txp,
                                                      int n_channels, s8 max_txpower_avg);
    void iwl_mod_ht40_chan_info(struct iwl_nvm_data *data, int n_channels,
                                             enum nl80211_band band, u16 channel,
                                             const struct iwl_eeprom_channel *eeprom_ch,
                                             u8 clear_ht40_extension_channel);

    
    
    // https://github.com/spotify/linux/blob/master/lib/hweight.c
    inline unsigned int hweight8(unsigned int w)
    {
        unsigned int res = w - ((w >> 1) & 0x55);
        res = (res & 0x33) + ((res >> 2) & 0x33);
        return (res + (res >> 4)) & 0x0F;
    };
    
    
    
    
    
    
    
    UInt8* fEeprom;
    
    IntelIO *fIO;
    struct iwl_cfg *fConfiguration;
    UInt32 fHwRev;
    int fEepromSize;
    struct iwl_nvm_data* nvmData;
};

/**
 * enum iwl_eeprom_channel_flags - channel flags in EEPROM
 * @EEPROM_CHANNEL_VALID: channel is usable for this SKU/geo
 * @EEPROM_CHANNEL_IBSS: usable as an IBSS channel
 * @EEPROM_CHANNEL_ACTIVE: active scanning allowed
 * @EEPROM_CHANNEL_RADAR: radar detection required
 * @EEPROM_CHANNEL_WIDE: 20 MHz channel okay (?)
 * @EEPROM_CHANNEL_DFS: dynamic freq selection candidate
 */
enum iwl_eeprom_channel_flags {
    EEPROM_CHANNEL_VALID = BIT(0),
    EEPROM_CHANNEL_IBSS = BIT(1),
    EEPROM_CHANNEL_ACTIVE = BIT(3),
    EEPROM_CHANNEL_RADAR = BIT(4),
    EEPROM_CHANNEL_WIDE = BIT(5),
    EEPROM_CHANNEL_DFS = BIT(7),
};

/**
 * struct iwl_eeprom_channel - EEPROM channel data
 * @flags: %EEPROM_CHANNEL_* flags
 * @max_power_avg: max power (in dBm) on this channel, at most 31 dBm
 */
struct iwl_eeprom_channel {
    u8 flags;
    s8 max_power_avg;
} __packed;


enum iwl_eeprom_enhanced_txpwr_flags {
    IWL_EEPROM_ENH_TXP_FL_VALID = BIT(0),
    IWL_EEPROM_ENH_TXP_FL_BAND_52G = BIT(1),
    IWL_EEPROM_ENH_TXP_FL_OFDM = BIT(2),
    IWL_EEPROM_ENH_TXP_FL_40MHZ = BIT(3),
    IWL_EEPROM_ENH_TXP_FL_HT_AP = BIT(4),
    IWL_EEPROM_ENH_TXP_FL_RES1 = BIT(5),
    IWL_EEPROM_ENH_TXP_FL_RES2 = BIT(6),
    IWL_EEPROM_ENH_TXP_FL_COMMON_TYPE = BIT(7),
};

/**
 * iwl_eeprom_enhanced_txpwr structure
 * @flags: entry flags
 * @channel: channel number
 * @chain_a_max_pwr: chain a max power in 1/2 dBm
 * @chain_b_max_pwr: chain b max power in 1/2 dBm
 * @chain_c_max_pwr: chain c max power in 1/2 dBm
 * @delta_20_in_40: 20-in-40 deltas (hi/lo)
 * @mimo2_max_pwr: mimo2 max power in 1/2 dBm
 * @mimo3_max_pwr: mimo3 max power in 1/2 dBm
 *
 * This structure presents the enhanced regulatory tx power limit layout
 * in an EEPROM image.
 */
struct iwl_eeprom_enhanced_txpwr {
    u8 flags;
    u8 channel;
    s8 chain_a_max;
    s8 chain_b_max;
    s8 chain_c_max;
    u8 delta_20_in_40;
    s8 mimo2_max;
    s8 mimo3_max;
} __packed;




#endif /* IntelEeprom_hpp */
