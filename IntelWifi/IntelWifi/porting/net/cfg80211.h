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

#include <net/regulatory.h>

/**
 * enum ieee80211_band - supported frequency bands
 *
 * The bands are assigned this way because the supported
 * bitrates differ in these bands.
 *
 * @IEEE80211_BAND_2GHZ: 2.4GHz ISM band
 * @IEEE80211_BAND_5GHZ: around 5GHz band (4.9-5.7)
 * @IEEE80211_NUM_BANDS: number of defined bands
 */
enum ieee80211_band {
    IEEE80211_BAND_2GHZ = NL80211_BAND_2GHZ,
    IEEE80211_BAND_5GHZ = NL80211_BAND_5GHZ,
    
    /* keep last */
    IEEE80211_NUM_BANDS
};

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


/** line 409
 * struct cfg80211_chan_def - channel definition
 * @chan: the (control) channel
 * @width: channel width
 * @center_freq1: center frequency of first segment
 * @center_freq2: center frequency of second segment
 *    (only with 80+80 MHz)
 */
struct cfg80211_chan_def {
    struct ieee80211_channel *chan;
    enum nl80211_chan_width width;
    u32 center_freq1;
    u32 center_freq2;
};

/** line 424
 * cfg80211_get_chandef_type - return old channel type from chandef
 * @chandef: the channel definition
 *
 * Return: The old channel type (NOHT, HT20, HT40+/-) from a given
 * chandef, which must have a bandwidth allowing this conversion.
 */
static inline enum nl80211_channel_type
cfg80211_get_chandef_type(const struct cfg80211_chan_def *chandef)
{
    switch (chandef->width) {
        case NL80211_CHAN_WIDTH_20_NOHT:
            return NL80211_CHAN_NO_HT;
        case NL80211_CHAN_WIDTH_20:
            return NL80211_CHAN_HT20;
        case NL80211_CHAN_WIDTH_40:
            if (chandef->center_freq1 > chandef->chan->center_freq)
                return NL80211_CHAN_HT40PLUS;
            return NL80211_CHAN_HT40MINUS;
        default:
            // WARN_ON(1);
            return NL80211_CHAN_NO_HT;
    }
}


/**
 * enum rate_info_bw - rate bandwidth information
 *
 * Used by the driver to indicate the rate bandwidth.
 *
 * @RATE_INFO_BW_5: 5 MHz bandwidth
 * @RATE_INFO_BW_10: 10 MHz bandwidth
 * @RATE_INFO_BW_20: 20 MHz bandwidth
 * @RATE_INFO_BW_40: 40 MHz bandwidth
 * @RATE_INFO_BW_80: 80 MHz bandwidth
 * @RATE_INFO_BW_160: 160 MHz bandwidth
 */
enum rate_info_bw {
    RATE_INFO_BW_20 = 0,
    RATE_INFO_BW_5,
    RATE_INFO_BW_10,
    RATE_INFO_BW_40,
    RATE_INFO_BW_80,
    RATE_INFO_BW_160,
};


// line 1094
#define IEEE80211_MAX_CHAINS    4

/** line 1501
 * DOC: Scanning and BSS list handling
 *
 * The scanning process itself is fairly simple, but cfg80211 offers quite
 * a bit of helper functionality. To start a scan, the scan operation will
 * be invoked with a scan definition. This scan definition contains the
 * channels to scan, and the SSIDs to send probe requests for (including the
 * wildcard, if desired). A passive scan is indicated by having no SSIDs to
 * probe. Additionally, a scan request may contain extra information elements
 * that should be added to the probe request. The IEs are guaranteed to be
 * well-formed, and will not exceed the maximum length the driver advertised
 * in the wiphy structure.
 *
 * When scanning finds a BSS, cfg80211 needs to be notified of that, because
 * it is responsible for maintaining the BSS list; the driver should not
 * maintain a list itself. For this notification, various functions exist.
 *
 * Since drivers do not maintain a BSS list, there are also a number of
 * functions to search for a BSS and obtain information about it from the
 * BSS structure cfg80211 maintains. The BSS list is also made available
 * to userspace.
 */

/** line 1524
 * struct cfg80211_ssid - SSID description
 * @ssid: the SSID
 * @ssid_len: length of the ssid
 */
struct cfg80211_ssid {
    u8 ssid[IEEE80211_MAX_SSID_LEN];
    u8 ssid_len;
};

/** line 1534
 * struct cfg80211_scan_info - information about completed scan
 * @scan_start_tsf: scan start time in terms of the TSF of the BSS that the
 *    wireless device that requested the scan is connected to. If this
 *    information is not available, this field is left zero.
 * @tsf_bssid: the BSSID according to which %scan_start_tsf is set.
 * @aborted: set to true if the scan was aborted for any reason,
 *    userspace will be notified of that
 */
struct cfg80211_scan_info {
    u64 scan_start_tsf;
    u8 tsf_bssid[ETH_ALEN] __aligned(2);
    bool aborted;
};

/** line 1549
 * struct cfg80211_scan_request - scan request description
 *
 * @ssids: SSIDs to scan for (active scan only)
 * @n_ssids: number of SSIDs
 * @channels: channels to scan on.
 * @n_channels: total number of channels to scan
 * @scan_width: channel width for scanning
 * @ie: optional information element(s) to add into Probe Request or %NULL
 * @ie_len: length of ie in octets
 * @duration: how long to listen on each channel, in TUs. If
 *    %duration_mandatory is not set, this is the maximum dwell time and
 *    the actual dwell time may be shorter.
 * @duration_mandatory: if set, the scan duration must be as specified by the
 *    %duration field.
 * @flags: bit field of flags controlling operation
 * @rates: bitmap of rates to advertise for each band
 * @wiphy: the wiphy this was for
 * @scan_start: time (in jiffies) when the scan started
 * @wdev: the wireless device to scan for
 * @info: (internal) information about completed scan
 * @notified: (internal) scan request was notified as done or aborted
 * @no_cck: used to send probe requests at non CCK rate in 2GHz band
 * @mac_addr: MAC address used with randomisation
 * @mac_addr_mask: MAC address mask used with randomisation, bits that
 *    are 0 in the mask should be randomised, bits that are 1 should
 *    be taken from the @mac_addr
 * @bssid: BSSID to scan for (most commonly, the wildcard BSSID)
 */
struct cfg80211_scan_request {
    struct cfg80211_ssid *ssids;
    int n_ssids;
    u32 n_channels;
    enum nl80211_bss_scan_width scan_width;
    const u8 *ie;
    size_t ie_len;
    u16 duration;
    bool duration_mandatory;
    u32 flags;
    
    u32 rates[NUM_NL80211_BANDS];
    
    struct wireless_dev *wdev;
    
    u8 mac_addr[ETH_ALEN] __aligned(2);
    u8 mac_addr_mask[ETH_ALEN] __aligned(2);
    u8 bssid[ETH_ALEN] __aligned(2);
    
    /* internal */
    struct wiphy *wiphy;
    unsigned long scan_start;
    struct cfg80211_scan_info info;
    bool notified;
    bool no_cck;
    
    /* keep last */
    struct ieee80211_channel *channels[0];
};




/** line 1746
 * enum cfg80211_signal_type - signal type
 *
 * @CFG80211_SIGNAL_TYPE_NONE: no signal strength information available
 * @CFG80211_SIGNAL_TYPE_MBM: signal strength in mBm (100*dBm)
 * @CFG80211_SIGNAL_TYPE_UNSPEC: signal strength, increasing from 0 through 100
 */
enum cfg80211_signal_type {
    CFG80211_SIGNAL_TYPE_NONE,
    CFG80211_SIGNAL_TYPE_MBM,
    CFG80211_SIGNAL_TYPE_UNSPEC,
};


/** line 3211
 * enum wiphy_flags - wiphy capability flags
 *
 * @WIPHY_FLAG_NETNS_OK: if not set, do not allow changing the netns of this
 *    wiphy at all
 * @WIPHY_FLAG_PS_ON_BY_DEFAULT: if set to true, powersave will be enabled
 *    by default -- this flag will be set depending on the kernel's default
 *    on wiphy_new(), but can be changed by the driver if it has a good
 *    reason to override the default
 * @WIPHY_FLAG_4ADDR_AP: supports 4addr mode even on AP (with a single station
 *    on a VLAN interface)
 * @WIPHY_FLAG_4ADDR_STATION: supports 4addr mode even as a station
 * @WIPHY_FLAG_CONTROL_PORT_PROTOCOL: This device supports setting the
 *    control port protocol ethertype. The device also honours the
 *    control_port_no_encrypt flag.
 * @WIPHY_FLAG_IBSS_RSN: The device supports IBSS RSN.
 * @WIPHY_FLAG_MESH_AUTH: The device supports mesh authentication by routing
 *    auth frames to userspace. See @NL80211_MESH_SETUP_USERSPACE_AUTH.
 * @WIPHY_FLAG_SUPPORTS_SCHED_SCAN: The device supports scheduled scans.
 * @WIPHY_FLAG_SUPPORTS_FW_ROAM: The device supports roaming feature in the
 *    firmware.
 * @WIPHY_FLAG_AP_UAPSD: The device supports uapsd on AP.
 * @WIPHY_FLAG_SUPPORTS_TDLS: The device supports TDLS (802.11z) operation.
 * @WIPHY_FLAG_TDLS_EXTERNAL_SETUP: The device does not handle TDLS (802.11z)
 *    link setup/discovery operations internally. Setup, discovery and
 *    teardown packets should be sent through the @NL80211_CMD_TDLS_MGMT
 *    command. When this flag is not set, @NL80211_CMD_TDLS_OPER should be
 *    used for asking the driver/firmware to perform a TDLS operation.
 * @WIPHY_FLAG_HAVE_AP_SME: device integrates AP SME
 * @WIPHY_FLAG_REPORTS_OBSS: the device will report beacons from other BSSes
 *    when there are virtual interfaces in AP mode by calling
 *    cfg80211_report_obss_beacon().
 * @WIPHY_FLAG_AP_PROBE_RESP_OFFLOAD: When operating as an AP, the device
 *    responds to probe-requests in hardware.
 * @WIPHY_FLAG_OFFCHAN_TX: Device supports direct off-channel TX.
 * @WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL: Device supports remain-on-channel call.
 * @WIPHY_FLAG_SUPPORTS_5_10_MHZ: Device supports 5 MHz and 10 MHz channels.
 * @WIPHY_FLAG_HAS_CHANNEL_SWITCH: Device supports channel switch in
 *    beaconing mode (AP, IBSS, Mesh, ...).
 * @WIPHY_FLAG_HAS_STATIC_WEP: The device supports static WEP key installation
 *    before connection.
 */
enum wiphy_flags {
    /* use hole at 0 */
    /* use hole at 1 */
    /* use hole at 2 */
    WIPHY_FLAG_NETNS_OK            = BIT(3),
    WIPHY_FLAG_PS_ON_BY_DEFAULT        = BIT(4),
    WIPHY_FLAG_4ADDR_AP            = BIT(5),
    WIPHY_FLAG_4ADDR_STATION        = BIT(6),
    WIPHY_FLAG_CONTROL_PORT_PROTOCOL    = BIT(7),
    WIPHY_FLAG_IBSS_RSN            = BIT(8),
    WIPHY_FLAG_MESH_AUTH            = BIT(10),
    /* use hole at 11 */
    /* use hole at 12 */
    WIPHY_FLAG_SUPPORTS_FW_ROAM        = BIT(13),
    WIPHY_FLAG_AP_UAPSD            = BIT(14),
    WIPHY_FLAG_SUPPORTS_TDLS        = BIT(15),
    WIPHY_FLAG_TDLS_EXTERNAL_SETUP        = BIT(16),
    WIPHY_FLAG_HAVE_AP_SME            = BIT(17),
    WIPHY_FLAG_REPORTS_OBSS            = BIT(18),
    WIPHY_FLAG_AP_PROBE_RESP_OFFLOAD    = BIT(19),
    WIPHY_FLAG_OFFCHAN_TX            = BIT(20),
    WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL    = BIT(21),
    WIPHY_FLAG_SUPPORTS_5_10_MHZ        = BIT(22),
    WIPHY_FLAG_HAS_CHANNEL_SWITCH        = BIT(23),
    WIPHY_FLAG_HAS_STATIC_WEP        = BIT(24),
};


/** line 3280
 * struct ieee80211_iface_limit - limit on certain interface types
 * @max: maximum number of interfaces of these types
 * @types: interface types (bits)
 */
struct ieee80211_iface_limit {
    u16 max;
    u16 types;
};

/**
 * struct ieee80211_iface_combination - possible interface combination
 *
 * With this structure the driver can describe which interface
 * combinations it supports concurrently.
 *
 * Examples:
 *
 * 1. Allow #STA <= 1, #AP <= 1, matching BI, channels = 1, 2 total:
 *
 *    .. code-block:: c
 *
 *    struct ieee80211_iface_limit limits1[] = {
 *        { .max = 1, .types = BIT(NL80211_IFTYPE_STATION), },
 *        { .max = 1, .types = BIT(NL80211_IFTYPE_AP}, },
 *    };
 *    struct ieee80211_iface_combination combination1 = {
 *        .limits = limits1,
 *        .n_limits = ARRAY_SIZE(limits1),
 *        .max_interfaces = 2,
 *        .beacon_int_infra_match = true,
 *    };
 *
 *
 * 2. Allow #{AP, P2P-GO} <= 8, channels = 1, 8 total:
 *
 *    .. code-block:: c
 *
 *    struct ieee80211_iface_limit limits2[] = {
 *        { .max = 8, .types = BIT(NL80211_IFTYPE_AP) |
 *                     BIT(NL80211_IFTYPE_P2P_GO), },
 *    };
 *    struct ieee80211_iface_combination combination2 = {
 *        .limits = limits2,
 *        .n_limits = ARRAY_SIZE(limits2),
 *        .max_interfaces = 8,
 *        .num_different_channels = 1,
 *    };
 *
 *
 * 3. Allow #STA <= 1, #{P2P-client,P2P-GO} <= 3 on two channels, 4 total.
 *
 *    This allows for an infrastructure connection and three P2P connections.
 *
 *    .. code-block:: c
 *
 *    struct ieee80211_iface_limit limits3[] = {
 *        { .max = 1, .types = BIT(NL80211_IFTYPE_STATION), },
 *        { .max = 3, .types = BIT(NL80211_IFTYPE_P2P_GO) |
 *                     BIT(NL80211_IFTYPE_P2P_CLIENT), },
 *    };
 *    struct ieee80211_iface_combination combination3 = {
 *        .limits = limits3,
 *        .n_limits = ARRAY_SIZE(limits3),
 *        .max_interfaces = 4,
 *        .num_different_channels = 2,
 *    };
 *
 */
struct ieee80211_iface_combination {
    /**
     * @limits:
     * limits for the given interface types
     */
    const struct ieee80211_iface_limit *limits;
    
    /**
     * @num_different_channels:
     * can use up to this many different channels
     */
    u32 num_different_channels;
    
    /**
     * @max_interfaces:
     * maximum number of interfaces in total allowed in this group
     */
    u16 max_interfaces;
    
    /**
     * @n_limits:
     * number of limitations
     */
    u8 n_limits;
    
    /**
     * @beacon_int_infra_match:
     * In this combination, the beacon intervals between infrastructure
     * and AP types must match. This is required only in special cases.
     */
    bool beacon_int_infra_match;
    
    /**
     * @radar_detect_widths:
     * bitmap of channel widths supported for radar detection
     */
    u8 radar_detect_widths;
    
    /**
     * @radar_detect_regions:
     * bitmap of regions supported for radar detection
     */
    u8 radar_detect_regions;
    
    /**
     * @beacon_int_min_gcd:
     * This interface combination supports different beacon intervals.
     *
     * = 0
     *   all beacon intervals for different interface must be same.
     * > 0
     *   any beacon interval for the interface part of this combination AND
     *   GCD of all beacon intervals from beaconing interfaces of this
     *   combination must be greater or equal to this value.
     */
    u32 beacon_int_min_gcd;
};



/** line 3547
 * struct wiphy - wireless hardware description
 * @reg_notifier: the driver's regulatory notification callback,
 *    note that if your driver uses wiphy_apply_custom_regulatory()
 *    the reg_notifier's request can be passed as NULL
 * @regd: the driver's regulatory domain, if one was requested via
 *     the regulatory_hint() API. This can be used by the driver
 *    on the reg_notifier() if it chooses to ignore future
 *    regulatory domain changes caused by other drivers.
 * @signal_type: signal type reported in &struct cfg80211_bss.
 * @cipher_suites: supported cipher suites
 * @n_cipher_suites: number of supported cipher suites
 * @retry_short: Retry limit for short frames (dot11ShortRetryLimit)
 * @retry_long: Retry limit for long frames (dot11LongRetryLimit)
 * @frag_threshold: Fragmentation threshold (dot11FragmentationThreshold);
 *    -1 = fragmentation disabled, only odd values >= 256 used
 * @rts_threshold: RTS threshold (dot11RTSThreshold); -1 = RTS/CTS disabled
 * @_net: the network namespace this wiphy currently lives in
 * @perm_addr: permanent MAC address of this device
 * @addr_mask: If the device supports multiple MAC addresses by masking,
 *    set this to a mask with variable bits set to 1, e.g. if the last
 *    four bits are variable then set it to 00-00-00-00-00-0f. The actual
 *    variable bits shall be determined by the interfaces added, with
 *    interfaces not matching the mask being rejected to be brought up.
 * @n_addresses: number of addresses in @addresses.
 * @addresses: If the device has more than one address, set this pointer
 *    to a list of addresses (6 bytes each). The first one will be used
 *    by default for perm_addr. In this case, the mask should be set to
 *    all-zeroes. In this case it is assumed that the device can handle
 *    the same number of arbitrary MAC addresses.
 * @registered: protects ->resume and ->suspend sysfs callbacks against
 *    unregister hardware
 * @debugfsdir: debugfs directory used for this wiphy, will be renamed
 *    automatically on wiphy renames
 * @dev: (virtual) struct device for this wiphy
 * @registered: helps synchronize suspend/resume with wiphy unregister
 * @wext: wireless extension handlers
 * @priv: driver private data (sized according to wiphy_new() parameter)
 * @interface_modes: bitmask of interfaces types valid for this wiphy,
 *    must be set by driver
 * @iface_combinations: Valid interface combinations array, should not
 *    list single interface types.
 * @n_iface_combinations: number of entries in @iface_combinations array.
 * @software_iftypes: bitmask of software interface types, these are not
 *    subject to any restrictions since they are purely managed in SW.
 * @flags: wiphy flags, see &enum wiphy_flags
 * @regulatory_flags: wiphy regulatory flags, see
 *    &enum ieee80211_regulatory_flags
 * @features: features advertised to nl80211, see &enum nl80211_feature_flags.
 * @ext_features: extended features advertised to nl80211, see
 *    &enum nl80211_ext_feature_index.
 * @bss_priv_size: each BSS struct has private data allocated with it,
 *    this variable determines its size
 * @max_scan_ssids: maximum number of SSIDs the device can scan for in
 *    any given scan
 * @max_sched_scan_reqs: maximum number of scheduled scan requests that
 *    the device can run concurrently.
 * @max_sched_scan_ssids: maximum number of SSIDs the device can scan
 *    for in any given scheduled scan
 * @max_match_sets: maximum number of match sets the device can handle
 *    when performing a scheduled scan, 0 if filtering is not
 *    supported.
 * @max_scan_ie_len: maximum length of user-controlled IEs device can
 *    add to probe request frames transmitted during a scan, must not
 *    include fixed IEs like supported rates
 * @max_sched_scan_ie_len: same as max_scan_ie_len, but for scheduled
 *    scans
 * @max_sched_scan_plans: maximum number of scan plans (scan interval and number
 *    of iterations) for scheduled scan supported by the device.
 * @max_sched_scan_plan_interval: maximum interval (in seconds) for a
 *    single scan plan supported by the device.
 * @max_sched_scan_plan_iterations: maximum number of iterations for a single
 *    scan plan supported by the device.
 * @coverage_class: current coverage class
 * @fw_version: firmware version for ethtool reporting
 * @hw_version: hardware version for ethtool reporting
 * @max_num_pmkids: maximum number of PMKIDs supported by device
 * @privid: a pointer that drivers can use to identify if an arbitrary
 *    wiphy is theirs, e.g. in global notifiers
 * @bands: information about bands/channels supported by this device
 *
 * @mgmt_stypes: bitmasks of frame subtypes that can be subscribed to or
 *    transmitted through nl80211, points to an array indexed by interface
 *    type
 *
 * @available_antennas_tx: bitmap of antennas which are available to be
 *    configured as TX antennas. Antenna configuration commands will be
 *    rejected unless this or @available_antennas_rx is set.
 *
 * @available_antennas_rx: bitmap of antennas which are available to be
 *    configured as RX antennas. Antenna configuration commands will be
 *    rejected unless this or @available_antennas_tx is set.
 *
 * @probe_resp_offload:
 *     Bitmap of supported protocols for probe response offloading.
 *     See &enum nl80211_probe_resp_offload_support_attr. Only valid
 *     when the wiphy flag @WIPHY_FLAG_AP_PROBE_RESP_OFFLOAD is set.
 *
 * @max_remain_on_channel_duration: Maximum time a remain-on-channel operation
 *    may request, if implemented.
 *
 * @wowlan: WoWLAN support information
 * @wowlan_config: current WoWLAN configuration; this should usually not be
 *    used since access to it is necessarily racy, use the parameter passed
 *    to the suspend() operation instead.
 *
 * @ap_sme_capa: AP SME capabilities, flags from &enum nl80211_ap_sme_features.
 * @ht_capa_mod_mask:  Specify what ht_cap values can be over-ridden.
 *    If null, then none can be over-ridden.
 * @vht_capa_mod_mask:  Specify what VHT capabilities can be over-ridden.
 *    If null, then none can be over-ridden.
 *
 * @wdev_list: the list of associated (virtual) interfaces; this list must
 *    not be modified by the driver, but can be read with RTNL/RCU protection.
 *
 * @max_acl_mac_addrs: Maximum number of MAC addresses that the device
 *    supports for ACL.
 *
 * @extended_capabilities: extended capabilities supported by the driver,
 *    additional capabilities might be supported by userspace; these are
 *    the 802.11 extended capabilities ("Extended Capabilities element")
 *    and are in the same format as in the information element. See
 *    802.11-2012 8.4.2.29 for the defined fields. These are the default
 *    extended capabilities to be used if the capabilities are not specified
 *    for a specific interface type in iftype_ext_capab.
 * @extended_capabilities_mask: mask of the valid values
 * @extended_capabilities_len: length of the extended capabilities
 * @iftype_ext_capab: array of extended capabilities per interface type
 * @num_iftype_ext_capab: number of interface types for which extended
 *    capabilities are specified separately.
 * @coalesce: packet coalescing support information
 *
 * @vendor_commands: array of vendor commands supported by the hardware
 * @n_vendor_commands: number of vendor commands
 * @vendor_events: array of vendor events supported by the hardware
 * @n_vendor_events: number of vendor events
 *
 * @max_ap_assoc_sta: maximum number of associated stations supported in AP mode
 *    (including P2P GO) or 0 to indicate no such limit is advertised. The
 *    driver is allowed to advertise a theoretical limit that it can reach in
 *    some cases, but may not always reach.
 *
 * @max_num_csa_counters: Number of supported csa_counters in beacons
 *    and probe responses.  This value should be set if the driver
 *    wishes to limit the number of csa counters. Default (0) means
 *    infinite.
 * @max_adj_channel_rssi_comp: max offset of between the channel on which the
 *    frame was sent and the channel on which the frame was heard for which
 *    the reported rssi is still valid. If a driver is able to compensate the
 *    low rssi when a frame is heard on different channel, then it should set
 *    this variable to the maximal offset for which it can compensate.
 *    This value should be set in MHz.
 * @bss_select_support: bitmask indicating the BSS selection criteria supported
 *    by the driver in the .connect() callback. The bit position maps to the
 *    attribute indices defined in &enum nl80211_bss_select_attr.
 *
 * @cookie_counter: unique generic cookie counter, used to identify objects.
 * @nan_supported_bands: bands supported by the device in NAN mode, a
 *    bitmap of &enum nl80211_band values.  For instance, for
 *    NL80211_BAND_2GHZ, bit 0 would be set
 *    (i.e. BIT(NL80211_BAND_2GHZ)).
 */
struct wiphy {
    /* assign these fields before you register the wiphy */
    
    /* permanent MAC address(es) */
    u8 perm_addr[ETH_ALEN];
    u8 addr_mask[ETH_ALEN];
    
    struct mac_address *addresses;
    
    const struct ieee80211_txrx_stypes *mgmt_stypes;
    
    const struct ieee80211_iface_combination *iface_combinations;
    int n_iface_combinations;
    u16 software_iftypes;
    
    u16 n_addresses;
    
    /* Supported interface modes, OR together BIT(NL80211_IFTYPE_...) */
    u16 interface_modes;
    
    u16 max_acl_mac_addrs;
    
    u32 flags, regulatory_flags, features;
    u8 ext_features[DIV_ROUND_UP(NUM_NL80211_EXT_FEATURES, 8)];
    
    u32 ap_sme_capa;
    
    enum cfg80211_signal_type signal_type;
    
    int bss_priv_size;
    u8 max_scan_ssids;
    u8 max_sched_scan_reqs;
    u8 max_sched_scan_ssids;
    u8 max_match_sets;
    u16 max_scan_ie_len;
    u16 max_sched_scan_ie_len;
    u32 max_sched_scan_plans;
    u32 max_sched_scan_plan_interval;
    u32 max_sched_scan_plan_iterations;
    
    int n_cipher_suites;
    const u32 *cipher_suites;
    
    u8 retry_short;
    u8 retry_long;
    u32 frag_threshold;
    u32 rts_threshold;
    u8 coverage_class;
    
    char fw_version[ETHTOOL_FWVERS_LEN];
    u32 hw_version;
    
#ifdef CONFIG_PM
    const struct wiphy_wowlan_support *wowlan;
    struct cfg80211_wowlan *wowlan_config;
#endif
    
    u16 max_remain_on_channel_duration;
    
    u8 max_num_pmkids;
    
    u32 available_antennas_tx;
    u32 available_antennas_rx;
    
    /*
     * Bitmap of supported protocols for probe response offloading
     * see &enum nl80211_probe_resp_offload_support_attr. Only valid
     * when the wiphy flag @WIPHY_FLAG_AP_PROBE_RESP_OFFLOAD is set.
     */
    u32 probe_resp_offload;
    
    const u8 *extended_capabilities, *extended_capabilities_mask;
    u8 extended_capabilities_len;
    
    const struct wiphy_iftype_ext_capab *iftype_ext_capab;
    unsigned int num_iftype_ext_capab;
    
    /* If multiple wiphys are registered and you're handed e.g.
     * a regular netdev with assigned ieee80211_ptr, you won't
     * know whether it points to a wiphy your driver has registered
     * or not. Assign this to something global to your driver to
     * help determine whether you own this wiphy or not. */
    const void *privid;
    
    struct ieee80211_supported_band *bands[NUM_NL80211_BANDS];
    
    /* Lets us get back the wiphy on the callback */
//    void (*reg_notifier)(struct wiphy *wiphy,
//                         struct regulatory_request *request);
    
    /* fields below are read-only, assigned by cfg80211 */
    
    const struct ieee80211_regdomain __rcu *regd;
    
    /* the item in /sys/class/ieee80211/ points to this,
     * you need use set_wiphy_dev() (see below) */
    //struct device dev;
    
    /* protects ->resume, ->suspend sysfs callbacks against unregister hw */
    bool registered;
    
    /* dir in debugfs: ieee80211/<wiphyname> */
    struct dentry *debugfsdir;
    
    const struct ieee80211_ht_cap *ht_capa_mod_mask;
    const struct ieee80211_vht_cap *vht_capa_mod_mask;
    
//    struct list_head wdev_list;
    
    /* the network namespace this phy lives in currently */
    //possible_net_t _net;
    
#ifdef CONFIG_CFG80211_WEXT
    const struct iw_handler_def *wext;
#endif
    
    const struct wiphy_coalesce_support *coalesce;
    
    const struct wiphy_vendor_command *vendor_commands;
    const struct nl80211_vendor_cmd_info *vendor_events;
    int n_vendor_commands, n_vendor_events;
    
    u16 max_ap_assoc_sta;
    
    u8 max_num_csa_counters;
    u8 max_adj_channel_rssi_comp;
    
    u32 bss_select_support;
    
    u64 cookie_counter;
    
    u8 nan_supported_bands;
    
    //char priv[0] __aligned(NETDEV_ALIGN);
};



#endif /* cfg80211_h */
