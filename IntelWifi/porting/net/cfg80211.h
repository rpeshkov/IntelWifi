//
//  cfg80211.h
//  IntelWifi
//
//  Created by Roman Peshkov on 30/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef cfg80211_h
#define cfg80211_h

#include <linux/types.h>
#include <linux/ieee80211.h>
#include <linux/nl80211.h>

/**
 * enum ieee80211_channel_flags - channel flags
 *
 * Channel flags set by the regulatory control code.
 *
 * @IEEE80211_CHAN_DISABLED: This channel is disabled.
 * @IEEE80211_CHAN_NO_IR: do not initiate radiation, this includes
 *     sending probe requests or beaconing.
 * @IEEE80211_CHAN_RADAR: Radar detection is required on this channel.
 * @IEEE80211_CHAN_NO_HT40PLUS: extension channel above this channel
 *     is not permitted.
 * @IEEE80211_CHAN_NO_HT40MINUS: extension channel below this channel
 *     is not permitted.
 * @IEEE80211_CHAN_NO_OFDM: OFDM is not allowed on this channel.
 * @IEEE80211_CHAN_NO_80MHZ: If the driver supports 80 MHz on the band,
 *    this flag indicates that an 80 MHz channel cannot use this
 *    channel as the control or any of the secondary channels.
 *    This may be due to the driver or due to regulatory bandwidth
 *    restrictions.
 * @IEEE80211_CHAN_NO_160MHZ: If the driver supports 160 MHz on the band,
 *    this flag indicates that an 160 MHz channel cannot use this
 *    channel as the control or any of the secondary channels.
 *    This may be due to the driver or due to regulatory bandwidth
 *    restrictions.
 * @IEEE80211_CHAN_INDOOR_ONLY: see %NL80211_FREQUENCY_ATTR_INDOOR_ONLY
 * @IEEE80211_CHAN_IR_CONCURRENT: see %NL80211_FREQUENCY_ATTR_IR_CONCURRENT
 * @IEEE80211_CHAN_NO_20MHZ: 20 MHz bandwidth is not permitted
 *    on this channel.
 * @IEEE80211_CHAN_NO_10MHZ: 10 MHz bandwidth is not permitted
 *    on this channel.
 *
 */
enum ieee80211_channel_flags {
    IEEE80211_CHAN_DISABLED        = 1<<0,
    IEEE80211_CHAN_NO_IR        = 1<<1,
    /* hole at 1<<2 */
    IEEE80211_CHAN_RADAR        = 1<<3,
    IEEE80211_CHAN_NO_HT40PLUS    = 1<<4,
    IEEE80211_CHAN_NO_HT40MINUS    = 1<<5,
    IEEE80211_CHAN_NO_OFDM        = 1<<6,
    IEEE80211_CHAN_NO_80MHZ        = 1<<7,
    IEEE80211_CHAN_NO_160MHZ    = 1<<8,
    IEEE80211_CHAN_INDOOR_ONLY    = 1<<9,
    IEEE80211_CHAN_IR_CONCURRENT    = 1<<10,
    IEEE80211_CHAN_NO_20MHZ        = 1<<11,
    IEEE80211_CHAN_NO_10MHZ        = 1<<12,
};

#define IEEE80211_CHAN_NO_HT40 \
(IEEE80211_CHAN_NO_HT40PLUS | IEEE80211_CHAN_NO_HT40MINUS)

#define IEEE80211_DFS_MIN_CAC_TIME_MS        60000
#define IEEE80211_DFS_MIN_NOP_TIME_MS        (30 * 60 * 1000)

/**
 * struct ieee80211_channel - channel definition
 *
 * This structure describes a single channel for use
 * with cfg80211.
 *
 * @center_freq: center frequency in MHz
 * @hw_value: hardware-specific value for the channel
 * @flags: channel flags from &enum ieee80211_channel_flags.
 * @orig_flags: channel flags at registration time, used by regulatory
 *    code to support devices with additional restrictions
 * @band: band this channel belongs to.
 * @max_antenna_gain: maximum antenna gain in dBi
 * @max_power: maximum transmission power (in dBm)
 * @max_reg_power: maximum regulatory transmission power (in dBm)
 * @beacon_found: helper to regulatory code to indicate when a beacon
 *    has been found on this channel. Use regulatory_hint_found_beacon()
 *    to enable this, this is useful only on 5 GHz band.
 * @orig_mag: internal use
 * @orig_mpwr: internal use
 * @dfs_state: current state of this channel. Only relevant if radar is required
 *    on this channel.
 * @dfs_state_entered: timestamp (jiffies) when the dfs state was entered.
 * @dfs_cac_ms: DFS CAC time in milliseconds, this is valid for DFS channels.
 */
struct ieee80211_channel {
    enum nl80211_band band;
    u16 center_freq;
    u16 hw_value;
    u32 flags;
    int max_antenna_gain;
    int max_power;
    int max_reg_power;
    bool beacon_found;
    u32 orig_flags;
    int orig_mag, orig_mpwr;
    enum nl80211_dfs_state dfs_state;
    unsigned long dfs_state_entered;
    unsigned int dfs_cac_ms;
};

/**
 * line 165
 * enum ieee80211_rate_flags - rate flags
 *
 * Hardware/specification flags for rates. These are structured
 * in a way that allows using the same bitrate structure for
 * different bands/PHY modes.
 *
 * @IEEE80211_RATE_SHORT_PREAMBLE: Hardware can send with short
 *    preamble on this bitrate; only relevant in 2.4GHz band and
 *    with CCK rates.
 * @IEEE80211_RATE_MANDATORY_A: This bitrate is a mandatory rate
 *    when used with 802.11a (on the 5 GHz band); filled by the
 *    core code when registering the wiphy.
 * @IEEE80211_RATE_MANDATORY_B: This bitrate is a mandatory rate
 *    when used with 802.11b (on the 2.4 GHz band); filled by the
 *    core code when registering the wiphy.
 * @IEEE80211_RATE_MANDATORY_G: This bitrate is a mandatory rate
 *    when used with 802.11g (on the 2.4 GHz band); filled by the
 *    core code when registering the wiphy.
 * @IEEE80211_RATE_ERP_G: This is an ERP rate in 802.11g mode.
 * @IEEE80211_RATE_SUPPORTS_5MHZ: Rate can be used in 5 MHz mode
 * @IEEE80211_RATE_SUPPORTS_10MHZ: Rate can be used in 10 MHz mode
 */
enum ieee80211_rate_flags {
    IEEE80211_RATE_SHORT_PREAMBLE    = 1<<0,
    IEEE80211_RATE_MANDATORY_A    = 1<<1,
    IEEE80211_RATE_MANDATORY_B    = 1<<2,
    IEEE80211_RATE_MANDATORY_G    = 1<<3,
    IEEE80211_RATE_ERP_G        = 1<<4,
    IEEE80211_RATE_SUPPORTS_5MHZ    = 1<<5,
    IEEE80211_RATE_SUPPORTS_10MHZ    = 1<<6,
};



/**
 * struct ieee80211_rate - bitrate definition
 *
 * This structure describes a bitrate that an 802.11 PHY can
 * operate with. The two values @hw_value and @hw_value_short
 * are only for driver use when pointers to this structure are
 * passed around.
 *
 * @flags: rate-specific flags
 * @bitrate: bitrate in units of 100 Kbps
 * @hw_value: driver/hardware value for this rate
 * @hw_value_short: driver/hardware value for this rate when
 *    short preamble is used
 */
struct ieee80211_rate {
    u32 flags;
    u16 bitrate;
    u16 hw_value, hw_value_short;
};

/**
 * struct ieee80211_sta_ht_cap - STA's HT capabilities
 *
 * This structure describes most essential parameters needed
 * to describe 802.11n HT capabilities for an STA.
 *
 * @ht_supported: is HT supported by the STA
 * @cap: HT capabilities map as described in 802.11n spec
 * @ampdu_factor: Maximum A-MPDU length factor
 * @ampdu_density: Minimum A-MPDU spacing
 * @mcs: Supported MCS rates
 */
struct ieee80211_sta_ht_cap {
    u16 cap; /* use IEEE80211_HT_CAP_ */
    bool ht_supported;
    u8 ampdu_factor;
    u8 ampdu_density;
    struct ieee80211_mcs_info mcs;
};

/**
 * struct ieee80211_sta_vht_cap - STA's VHT capabilities
 *
 * This structure describes most essential parameters needed
 * to describe 802.11ac VHT capabilities for an STA.
 *
 * @vht_supported: is VHT supported by the STA
 * @cap: VHT capabilities map as described in 802.11ac spec
 * @vht_mcs: Supported VHT MCS rates
 */
struct ieee80211_sta_vht_cap {
    bool vht_supported;
    u32 cap; /* use IEEE80211_VHT_CAP_ */
    struct ieee80211_vht_mcs_info vht_mcs;
};

/**
 * struct ieee80211_supported_band - frequency band definition
 *
 * This structure describes a frequency band a wiphy
 * is able to operate in.
 *
 * @channels: Array of channels the hardware can operate in
 *    in this band.
 * @band: the band this structure represents
 * @n_channels: Number of channels in @channels
 * @bitrates: Array of bitrates the hardware can operate with
 *    in this band. Must be sorted to give a valid "supported
 *    rates" IE, i.e. CCK rates first, then OFDM.
 * @n_bitrates: Number of bitrates in @bitrates
 * @ht_cap: HT capabilities in this band
 * @vht_cap: VHT capabilities in this band
 */
struct ieee80211_supported_band {
    struct ieee80211_channel *channels;
    struct ieee80211_rate *bitrates;
    enum nl80211_band band;
    int n_channels;
    int n_bitrates;
    struct ieee80211_sta_ht_cap ht_cap;
    struct ieee80211_sta_vht_cap vht_cap;
};

int ieee80211_channel_to_frequency(int chan, enum nl80211_band band);


#endif /* cfg80211_h */
