//
//  mac80211.h
//  IntelWifi
//
//  Created by Roman Peshkov on 18/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef mac80211_h
#define mac80211_h

#include <linux/kernel.h>
#include <net/cfg80211.h>

/* line 335
 * The maximum number of IPv4 addresses listed for ARP filtering. If the number
 * of addresses for an interface increase beyond this value, hardware ARP
 * filtering will be disabled.
 */
#define IEEE80211_BSS_ARP_ADDR_LIST_LEN 4


/** line 443
 * struct ieee80211_mu_group_data - STA's VHT MU-MIMO group data
 *
 * This structure describes the group id data of VHT MU-MIMO
 *
 * @membership: 64 bits array - a bit is set if station is member of the group
 * @position: 2 bits per group id indicating the position in the group
 */
struct ieee80211_mu_group_data {
    u8 membership[WLAN_MEMBERSHIP_LEN];
    u8 position[WLAN_USER_POSITION_LEN];
};


/** line 456
 * struct ieee80211_bss_conf - holds the BSS's changing parameters
 *
 * This structure keeps information about a BSS (and an association
 * to that BSS) that can change during the lifetime of the BSS.
 *
 * @assoc: association status
 * @ibss_joined: indicates whether this station is part of an IBSS
 *    or not
 * @ibss_creator: indicates if a new IBSS network is being created
 * @aid: association ID number, valid only when @assoc is true
 * @use_cts_prot: use CTS protection
 * @use_short_preamble: use 802.11b short preamble
 * @use_short_slot: use short slot time (only relevant for ERP)
 * @dtim_period: num of beacons before the next DTIM, for beaconing,
 *    valid in station mode only if after the driver was notified
 *    with the %BSS_CHANGED_BEACON_INFO flag, will be non-zero then.
 * @sync_tsf: last beacon's/probe response's TSF timestamp (could be old
 *    as it may have been received during scanning long ago). If the
 *    HW flag %IEEE80211_HW_TIMING_BEACON_ONLY is set, then this can
 *    only come from a beacon, but might not become valid until after
 *    association when a beacon is received (which is notified with the
 *    %BSS_CHANGED_DTIM flag.). See also sync_dtim_count important notice.
 * @sync_device_ts: the device timestamp corresponding to the sync_tsf,
 *    the driver/device can use this to calculate synchronisation
 *    (see @sync_tsf). See also sync_dtim_count important notice.
 * @sync_dtim_count: Only valid when %IEEE80211_HW_TIMING_BEACON_ONLY
 *    is requested, see @sync_tsf/@sync_device_ts.
 *    IMPORTANT: These three sync_* parameters would possibly be out of sync
 *    by the time the driver will use them. The synchronized view is currently
 *    guaranteed only in certain callbacks.
 * @beacon_int: beacon interval
 * @assoc_capability: capabilities taken from assoc resp
 * @basic_rates: bitmap of basic rates, each bit stands for an
 *    index into the rate table configured by the driver in
 *    the current band.
 * @beacon_rate: associated AP's beacon TX rate
 * @mcast_rate: per-band multicast rate index + 1 (0: disabled)
 * @bssid: The BSSID for this BSS
 * @enable_beacon: whether beaconing should be enabled or not
 * @chandef: Channel definition for this BSS -- the hardware might be
 *    configured a higher bandwidth than this BSS uses, for example.
 * @mu_group: VHT MU-MIMO group membership data
 * @ht_operation_mode: HT operation mode like in &struct ieee80211_ht_operation.
 *    This field is only valid when the channel is a wide HT/VHT channel.
 *    Note that with TDLS this can be the case (channel is HT, protection must
 *    be used from this field) even when the BSS association isn't using HT.
 * @cqm_rssi_thold: Connection quality monitor RSSI threshold, a zero value
 *    implies disabled. As with the cfg80211 callback, a change here should
 *    cause an event to be sent indicating where the current value is in
 *    relation to the newly configured threshold.
 * @cqm_rssi_low: Connection quality monitor RSSI lower threshold, a zero value
 *    implies disabled.  This is an alternative mechanism to the single
 *    threshold event and can't be enabled simultaneously with it.
 * @cqm_rssi_high: Connection quality monitor RSSI upper threshold.
 * @cqm_rssi_hyst: Connection quality monitor RSSI hysteresis
 * @arp_addr_list: List of IPv4 addresses for hardware ARP filtering. The
 *    may filter ARP queries targeted for other addresses than listed here.
 *    The driver must allow ARP queries targeted for all address listed here
 *    to pass through. An empty list implies no ARP queries need to pass.
 * @arp_addr_cnt: Number of addresses currently on the list. Note that this
 *    may be larger than %IEEE80211_BSS_ARP_ADDR_LIST_LEN (the arp_addr_list
 *    array size), it's up to the driver what to do in that case.
 * @qos: This is a QoS-enabled BSS.
 * @idle: This interface is idle. There's also a global idle flag in the
 *    hardware config which may be more appropriate depending on what
 *    your driver/device needs to do.
 * @ps: power-save mode (STA only). This flag is NOT affected by
 *    offchannel/dynamic_ps operations.
 * @ssid: The SSID of the current vif. Valid in AP and IBSS mode.
 * @ssid_len: Length of SSID given in @ssid.
 * @hidden_ssid: The SSID of the current vif is hidden. Only valid in AP-mode.
 * @txpower: TX power in dBm
 * @txpower_type: TX power adjustment used to control per packet Transmit
 *    Power Control (TPC) in lower driver for the current vif. In particular
 *    TPC is enabled if value passed in %txpower_type is
 *    NL80211_TX_POWER_LIMITED (allow using less than specified from
 *    userspace), whereas TPC is disabled if %txpower_type is set to
 *    NL80211_TX_POWER_FIXED (use value configured from userspace)
 * @p2p_noa_attr: P2P NoA attribute for P2P powersave
 * @allow_p2p_go_ps: indication for AP or P2P GO interface, whether it's allowed
 *    to use P2P PS mechanism or not. AP/P2P GO is not allowed to use P2P PS
 *    if it has associated clients without P2P PS support.
 * @max_idle_period: the time period during which the station can refrain from
 *    transmitting frames to its associated AP without being disassociated.
 *    In units of 1000 TUs. Zero value indicates that the AP did not include
 *    a (valid) BSS Max Idle Period Element.
 * @protected_keep_alive: if set, indicates that the station should send an RSN
 *    protected frame to the AP to reset the idle timer at the AP for the
 *    station.
 */
struct ieee80211_bss_conf {
    const u8 *bssid;
    /* association related data */
    bool assoc, ibss_joined;
    bool ibss_creator;
    u16 aid;
    /* erp related data */
    bool use_cts_prot;
    bool use_short_preamble;
    bool use_short_slot;
    bool enable_beacon;
    u8 dtim_period;
    u16 beacon_int;
    u16 assoc_capability;
    u64 sync_tsf;
    u32 sync_device_ts;
    u8 sync_dtim_count;
    u32 basic_rates;
    struct ieee80211_rate *beacon_rate;
    int mcast_rate[NUM_NL80211_BANDS];
    u16 ht_operation_mode;
    s32 cqm_rssi_thold;
    u32 cqm_rssi_hyst;
    s32 cqm_rssi_low;
    s32 cqm_rssi_high;
    struct cfg80211_chan_def chandef;
    struct ieee80211_mu_group_data mu_group;
    u32 arp_addr_list[IEEE80211_BSS_ARP_ADDR_LIST_LEN];
    int arp_addr_cnt;
    bool qos;
    bool idle;
    bool ps;
    u8 ssid[IEEE80211_MAX_SSID_LEN];
    size_t ssid_len;
    bool hidden_ssid;
    int txpower;
    enum nl80211_tx_power_setting txpower_type;
    struct ieee80211_p2p_noa_attr p2p_noa_attr;
    bool allow_p2p_go_ps;
    u16 max_idle_period;
    bool protected_keep_alive;
};


/**
 * struct ieee80211_vif - per-interface data
 *
 * Data in this structure is continually present for driver
 * use during the life of a virtual interface.
 *
 * @type: type of this virtual interface
 * @bss_conf: BSS configuration for this interface, either our own
 *    or the BSS we're associated to
 * @addr: address of this interface
 * @p2p: indicates whether this AP or STA interface is a p2p
 *    interface, i.e. a GO or p2p-sta respectively
 * @csa_active: marks whether a channel switch is going on. Internally it is
 *    write-protected by sdata_lock and local->mtx so holding either is fine
 *    for read access.
 * @mu_mimo_owner: indicates interface owns MU-MIMO capability
 * @driver_flags: flags/capabilities the driver has for this interface,
 *    these need to be set (or cleared) when the interface is added
 *    or, if supported by the driver, the interface type is changed
 *    at runtime, mac80211 will never touch this field
 * @hw_queue: hardware queue for each AC
 * @cab_queue: content-after-beacon (DTIM beacon really) queue, AP mode only
 * @chanctx_conf: The channel context this interface is assigned to, or %NULL
 *    when it is not assigned. This pointer is RCU-protected due to the TX
 *    path needing to access it; even though the netdev carrier will always
 *    be off when it is %NULL there can still be races and packets could be
 *    processed after it switches back to %NULL.
 * @debugfs_dir: debugfs dentry, can be used by drivers to create own per
 *    interface debug files. Note that it will be NULL for the virtual
 *    monitor interface (if that is requested.)
 * @probe_req_reg: probe requests should be reported to mac80211 for this
 *    interface.
 * @drv_priv: data area for driver use, will always be aligned to
 *    sizeof(void \*).
 * @txq: the multicast data TX queue (if driver uses the TXQ abstraction)
 */
struct ieee80211_vif {
    enum nl80211_iftype type;
    struct ieee80211_bss_conf bss_conf;
    u8 addr[ETH_ALEN] __aligned(2);
    bool p2p;
    bool csa_active;
    bool mu_mimo_owner;
    
    u8 cab_queue;
    u8 hw_queue[IEEE80211_NUM_ACS];
    
    struct ieee80211_txq *txq;
    
    struct ieee80211_chanctx_conf __rcu *chanctx_conf;
    
    u32 driver_flags;
    
#ifdef CONFIG_MAC80211_DEBUGFS
    struct dentry *debugfs_dir;
#endif
    
    unsigned int probe_req_reg;
    
    /* must be last */
    u8 drv_priv[0] __aligned(sizeof(void *));
};

/** line 1305
 * enum ieee80211_smps_mode - spatial multiplexing power save mode
 *
 * @IEEE80211_SMPS_AUTOMATIC: automatic
 * @IEEE80211_SMPS_OFF: off
 * @IEEE80211_SMPS_STATIC: static
 * @IEEE80211_SMPS_DYNAMIC: dynamic
 * @IEEE80211_SMPS_NUM_MODES: internal, don't use
 */
enum ieee80211_smps_mode {
    IEEE80211_SMPS_AUTOMATIC,
    IEEE80211_SMPS_OFF,
    IEEE80211_SMPS_STATIC,
    IEEE80211_SMPS_DYNAMIC,
    
    /* keep last */
    IEEE80211_SMPS_NUM_MODES,
};


/** line 1324
 * struct ieee80211_conf - configuration of the device
 *
 * This struct indicates how the driver shall configure the hardware.
 *
 * @flags: configuration flags defined above
 *
 * @listen_interval: listen interval in units of beacon interval
 * @ps_dtim_period: The DTIM period of the AP we're connected to, for use
 *    in power saving. Power saving will not be enabled until a beacon
 *    has been received and the DTIM period is known.
 * @dynamic_ps_timeout: The dynamic powersave timeout (in ms), see the
 *    powersave documentation below. This variable is valid only when
 *    the CONF_PS flag is set.
 *
 * @power_level: requested transmit power (in dBm), backward compatibility
 *    value only that is set to the minimum of all interfaces
 *
 * @chandef: the channel definition to tune to
 * @radar_enabled: whether radar detection is enabled
 *
 * @long_frame_max_tx_count: Maximum number of transmissions for a "long" frame
 *    (a frame not RTS protected), called "dot11LongRetryLimit" in 802.11,
 *    but actually means the number of transmissions not the number of retries
 * @short_frame_max_tx_count: Maximum number of transmissions for a "short"
 *    frame, called "dot11ShortRetryLimit" in 802.11, but actually means the
 *    number of transmissions not the number of retries
 *
 * @smps_mode: spatial multiplexing powersave mode; note that
 *    %IEEE80211_SMPS_STATIC is used when the device is not
 *    configured for an HT channel.
 *    Note that this is only valid if channel contexts are not used,
 *    otherwise each channel context has the number of chains listed.
 */
struct ieee80211_conf {
    u32 flags;
    int power_level, dynamic_ps_timeout;
    
    u16 listen_interval;
    u8 ps_dtim_period;
    
    u8 long_frame_max_tx_count, short_frame_max_tx_count;
    
    struct cfg80211_chan_def chandef;
    bool radar_enabled;
    enum ieee80211_smps_mode smps_mode;
};


/** line 1877
 * enum ieee80211_hw_flags - hardware flags
 *
 * These flags are used to indicate hardware capabilities to
 * the stack. Generally, flags here should have their meaning
 * done in a way that the simplest hardware doesn't need setting
 * any particular flags. There are some exceptions to this rule,
 * however, so you are advised to review these flags carefully.
 *
 * @IEEE80211_HW_HAS_RATE_CONTROL:
 *    The hardware or firmware includes rate control, and cannot be
 *    controlled by the stack. As such, no rate control algorithm
 *    should be instantiated, and the TX rate reported to userspace
 *    will be taken from the TX status instead of the rate control
 *    algorithm.
 *    Note that this requires that the driver implement a number of
 *    callbacks so it has the correct information, it needs to have
 *    the @set_rts_threshold callback and must look at the BSS config
 *    @use_cts_prot for G/N protection, @use_short_slot for slot
 *    timing in 2.4 GHz and @use_short_preamble for preambles for
 *    CCK frames.
 *
 * @IEEE80211_HW_RX_INCLUDES_FCS:
 *    Indicates that received frames passed to the stack include
 *    the FCS at the end.
 *
 * @IEEE80211_HW_HOST_BROADCAST_PS_BUFFERING:
 *    Some wireless LAN chipsets buffer broadcast/multicast frames
 *    for power saving stations in the hardware/firmware and others
 *    rely on the host system for such buffering. This option is used
 *    to configure the IEEE 802.11 upper layer to buffer broadcast and
 *    multicast frames when there are power saving stations so that
 *    the driver can fetch them with ieee80211_get_buffered_bc().
 *
 * @IEEE80211_HW_SIGNAL_UNSPEC:
 *    Hardware can provide signal values but we don't know its units. We
 *    expect values between 0 and @max_signal.
 *    If possible please provide dB or dBm instead.
 *
 * @IEEE80211_HW_SIGNAL_DBM:
 *    Hardware gives signal values in dBm, decibel difference from
 *    one milliwatt. This is the preferred method since it is standardized
 *    between different devices. @max_signal does not need to be set.
 *
 * @IEEE80211_HW_SPECTRUM_MGMT:
 *     Hardware supports spectrum management defined in 802.11h
 *     Measurement, Channel Switch, Quieting, TPC
 *
 * @IEEE80211_HW_AMPDU_AGGREGATION:
 *    Hardware supports 11n A-MPDU aggregation.
 *
 * @IEEE80211_HW_SUPPORTS_PS:
 *    Hardware has power save support (i.e. can go to sleep).
 *
 * @IEEE80211_HW_PS_NULLFUNC_STACK:
 *    Hardware requires nullfunc frame handling in stack, implies
 *    stack support for dynamic PS.
 *
 * @IEEE80211_HW_SUPPORTS_DYNAMIC_PS:
 *    Hardware has support for dynamic PS.
 *
 * @IEEE80211_HW_MFP_CAPABLE:
 *    Hardware supports management frame protection (MFP, IEEE 802.11w).
 *
 * @IEEE80211_HW_REPORTS_TX_ACK_STATUS:
 *    Hardware can provide ack status reports of Tx frames to
 *    the stack.
 *
 * @IEEE80211_HW_CONNECTION_MONITOR:
 *    The hardware performs its own connection monitoring, including
 *    periodic keep-alives to the AP and probing the AP on beacon loss.
 *
 * @IEEE80211_HW_NEED_DTIM_BEFORE_ASSOC:
 *    This device needs to get data from beacon before association (i.e.
 *    dtim_period).
 *
 * @IEEE80211_HW_SUPPORTS_PER_STA_GTK: The device's crypto engine supports
 *    per-station GTKs as used by IBSS RSN or during fast transition. If
 *    the device doesn't support per-station GTKs, but can be asked not
 *    to decrypt group addressed frames, then IBSS RSN support is still
 *    possible but software crypto will be used. Advertise the wiphy flag
 *    only in that case.
 *
 * @IEEE80211_HW_AP_LINK_PS: When operating in AP mode the device
 *    autonomously manages the PS status of connected stations. When
 *    this flag is set mac80211 will not trigger PS mode for connected
 *    stations based on the PM bit of incoming frames.
 *    Use ieee80211_start_ps()/ieee8021_end_ps() to manually configure
 *    the PS mode of connected stations.
 *
 * @IEEE80211_HW_TX_AMPDU_SETUP_IN_HW: The device handles TX A-MPDU session
 *    setup strictly in HW. mac80211 should not attempt to do this in
 *    software.
 *
 * @IEEE80211_HW_WANT_MONITOR_VIF: The driver would like to be informed of
 *    a virtual monitor interface when monitor interfaces are the only
 *    active interfaces.
 *
 * @IEEE80211_HW_NO_AUTO_VIF: The driver would like for no wlanX to
 *    be created.  It is expected user-space will create vifs as
 *    desired (and thus have them named as desired).
 *
 * @IEEE80211_HW_SW_CRYPTO_CONTROL: The driver wants to control which of the
 *    crypto algorithms can be done in software - so don't automatically
 *    try to fall back to it if hardware crypto fails, but do so only if
 *    the driver returns 1. This also forces the driver to advertise its
 *    supported cipher suites.
 *
 * @IEEE80211_HW_SUPPORT_FAST_XMIT: The driver/hardware supports fast-xmit,
 *    this currently requires only the ability to calculate the duration
 *    for frames.
 *
 * @IEEE80211_HW_QUEUE_CONTROL: The driver wants to control per-interface
 *    queue mapping in order to use different queues (not just one per AC)
 *    for different virtual interfaces. See the doc section on HW queue
 *    control for more details.
 *
 * @IEEE80211_HW_SUPPORTS_RC_TABLE: The driver supports using a rate
 *    selection table provided by the rate control algorithm.
 *
 * @IEEE80211_HW_P2P_DEV_ADDR_FOR_INTF: Use the P2P Device address for any
 *    P2P Interface. This will be honoured even if more than one interface
 *    is supported.
 *
 * @IEEE80211_HW_TIMING_BEACON_ONLY: Use sync timing from beacon frames
 *    only, to allow getting TBTT of a DTIM beacon.
 *
 * @IEEE80211_HW_SUPPORTS_HT_CCK_RATES: Hardware supports mixing HT/CCK rates
 *    and can cope with CCK rates in an aggregation session (e.g. by not
 *    using aggregation for such frames.)
 *
 * @IEEE80211_HW_CHANCTX_STA_CSA: Support 802.11h based channel-switch (CSA)
 *    for a single active channel while using channel contexts. When support
 *    is not enabled the default action is to disconnect when getting the
 *    CSA frame.
 *
 * @IEEE80211_HW_SUPPORTS_CLONED_SKBS: The driver will never modify the payload
 *    or tailroom of TX skbs without copying them first.
 *
 * @IEEE80211_HW_SINGLE_SCAN_ON_ALL_BANDS: The HW supports scanning on all bands
 *    in one command, mac80211 doesn't have to run separate scans per band.
 *
 * @IEEE80211_HW_TDLS_WIDER_BW: The device/driver supports wider bandwidth
 *    than then BSS bandwidth for a TDLS link on the base channel.
 *
 * @IEEE80211_HW_SUPPORTS_AMSDU_IN_AMPDU: The driver supports receiving A-MSDUs
 *    within A-MPDU.
 *
 * @IEEE80211_HW_BEACON_TX_STATUS: The device/driver provides TX status
 *    for sent beacons.
 *
 * @IEEE80211_HW_NEEDS_UNIQUE_STA_ADDR: Hardware (or driver) requires that each
 *    station has a unique address, i.e. each station entry can be identified
 *    by just its MAC address; this prevents, for example, the same station
 *    from connecting to two virtual AP interfaces at the same time.
 *
 * @IEEE80211_HW_SUPPORTS_REORDERING_BUFFER: Hardware (or driver) manages the
 *    reordering buffer internally, guaranteeing mac80211 receives frames in
 *    order and does not need to manage its own reorder buffer or BA session
 *    timeout.
 *
 * @IEEE80211_HW_USES_RSS: The device uses RSS and thus requires parallel RX,
 *    which implies using per-CPU station statistics.
 *
 * @IEEE80211_HW_TX_AMSDU: Hardware (or driver) supports software aggregated
 *    A-MSDU frames. Requires software tx queueing and fast-xmit support.
 *    When not using minstrel/minstrel_ht rate control, the driver must
 *    limit the maximum A-MSDU size based on the current tx rate by setting
 *    max_rc_amsdu_len in struct ieee80211_sta.
 *
 * @IEEE80211_HW_TX_FRAG_LIST: Hardware (or driver) supports sending frag_list
 *    skbs, needed for zero-copy software A-MSDU.
 *
 * @IEEE80211_HW_REPORTS_LOW_ACK: The driver (or firmware) reports low ack event
 *    by ieee80211_report_low_ack() based on its own algorithm. For such
 *    drivers, mac80211 packet loss mechanism will not be triggered and driver
 *    is completely depending on firmware event for station kickout.
 *
 * @IEEE80211_HW_SUPPORTS_TX_FRAG: Hardware does fragmentation by itself.
 *    The stack will not do fragmentation.
 *    The callback for @set_frag_threshold should be set as well.
 *
 * @NUM_IEEE80211_HW_FLAGS: number of hardware flags, used for sizing arrays
 */
enum ieee80211_hw_flags {
    IEEE80211_HW_HAS_RATE_CONTROL,
    IEEE80211_HW_RX_INCLUDES_FCS,
    IEEE80211_HW_HOST_BROADCAST_PS_BUFFERING,
    IEEE80211_HW_SIGNAL_UNSPEC,
    IEEE80211_HW_SIGNAL_DBM,
    IEEE80211_HW_NEED_DTIM_BEFORE_ASSOC,
    IEEE80211_HW_SPECTRUM_MGMT,
    IEEE80211_HW_AMPDU_AGGREGATION,
    IEEE80211_HW_SUPPORTS_PS,
    IEEE80211_HW_PS_NULLFUNC_STACK,
    IEEE80211_HW_SUPPORTS_DYNAMIC_PS,
    IEEE80211_HW_MFP_CAPABLE,
    IEEE80211_HW_WANT_MONITOR_VIF,
    IEEE80211_HW_NO_AUTO_VIF,
    IEEE80211_HW_SW_CRYPTO_CONTROL,
    IEEE80211_HW_SUPPORT_FAST_XMIT,
    IEEE80211_HW_REPORTS_TX_ACK_STATUS,
    IEEE80211_HW_CONNECTION_MONITOR,
    IEEE80211_HW_QUEUE_CONTROL,
    IEEE80211_HW_SUPPORTS_PER_STA_GTK,
    IEEE80211_HW_AP_LINK_PS,
    IEEE80211_HW_TX_AMPDU_SETUP_IN_HW,
    IEEE80211_HW_SUPPORTS_RC_TABLE,
    IEEE80211_HW_P2P_DEV_ADDR_FOR_INTF,
    IEEE80211_HW_TIMING_BEACON_ONLY,
    IEEE80211_HW_SUPPORTS_HT_CCK_RATES,
    IEEE80211_HW_CHANCTX_STA_CSA,
    IEEE80211_HW_SUPPORTS_CLONED_SKBS,
    IEEE80211_HW_SINGLE_SCAN_ON_ALL_BANDS,
    IEEE80211_HW_TDLS_WIDER_BW,
    IEEE80211_HW_SUPPORTS_AMSDU_IN_AMPDU,
    IEEE80211_HW_BEACON_TX_STATUS,
    IEEE80211_HW_NEEDS_UNIQUE_STA_ADDR,
    IEEE80211_HW_SUPPORTS_REORDERING_BUFFER,
    IEEE80211_HW_USES_RSS,
    IEEE80211_HW_TX_AMSDU,
    IEEE80211_HW_TX_FRAG_LIST,
    IEEE80211_HW_REPORTS_LOW_ACK,
    IEEE80211_HW_SUPPORTS_TX_FRAG,
    
    /* keep last, obviously */
    NUM_IEEE80211_HW_FLAGS
};


/** line 2106
 * struct ieee80211_hw - hardware information and state
 *
 * This structure contains the configuration and hardware
 * information for an 802.11 PHY.
 *
 * @wiphy: This points to the &struct wiphy allocated for this
 *    802.11 PHY. You must fill in the @perm_addr and @dev
 *    members of this structure using SET_IEEE80211_DEV()
 *    and SET_IEEE80211_PERM_ADDR(). Additionally, all supported
 *    bands (with channels, bitrates) are registered here.
 *
 * @conf: &struct ieee80211_conf, device configuration, don't use.
 *
 * @priv: pointer to private area that was allocated for driver use
 *    along with this structure.
 *
 * @flags: hardware flags, see &enum ieee80211_hw_flags.
 *
 * @extra_tx_headroom: headroom to reserve in each transmit skb
 *    for use by the driver (e.g. for transmit headers.)
 *
 * @extra_beacon_tailroom: tailroom to reserve in each beacon tx skb.
 *    Can be used by drivers to add extra IEs.
 *
 * @max_signal: Maximum value for signal (rssi) in RX information, used
 *    only when @IEEE80211_HW_SIGNAL_UNSPEC or @IEEE80211_HW_SIGNAL_DB
 *
 * @max_listen_interval: max listen interval in units of beacon interval
 *    that HW supports
 *
 * @queues: number of available hardware transmit queues for
 *    data packets. WMM/QoS requires at least four, these
 *    queues need to have configurable access parameters.
 *
 * @rate_control_algorithm: rate control algorithm for this hardware.
 *    If unset (NULL), the default algorithm will be used. Must be
 *    set before calling ieee80211_register_hw().
 *
 * @vif_data_size: size (in bytes) of the drv_priv data area
 *    within &struct ieee80211_vif.
 * @sta_data_size: size (in bytes) of the drv_priv data area
 *    within &struct ieee80211_sta.
 * @chanctx_data_size: size (in bytes) of the drv_priv data area
 *    within &struct ieee80211_chanctx_conf.
 * @txq_data_size: size (in bytes) of the drv_priv data area
 *    within @struct ieee80211_txq.
 *
 * @max_rates: maximum number of alternate rate retry stages the hw
 *    can handle.
 * @max_report_rates: maximum number of alternate rate retry stages
 *    the hw can report back.
 * @max_rate_tries: maximum number of tries for each stage
 *
 * @max_rx_aggregation_subframes: maximum buffer size (number of
 *    sub-frames) to be used for A-MPDU block ack receiver
 *    aggregation.
 *    This is only relevant if the device has restrictions on the
 *    number of subframes, if it relies on mac80211 to do reordering
 *    it shouldn't be set.
 *
 * @max_tx_aggregation_subframes: maximum number of subframes in an
 *    aggregate an HT driver will transmit. Though ADDBA will advertise
 *    a constant value of 64 as some older APs can crash if the window
 *    size is smaller (an example is LinkSys WRT120N with FW v1.0.07
 *    build 002 Jun 18 2012).
 *
 * @max_tx_fragments: maximum number of tx buffers per (A)-MSDU, sum
 *    of 1 + skb_shinfo(skb)->nr_frags for each skb in the frag_list.
 *
 * @offchannel_tx_hw_queue: HW queue ID to use for offchannel TX
 *    (if %IEEE80211_HW_QUEUE_CONTROL is set)
 *
 * @radiotap_mcs_details: lists which MCS information can the HW
 *    reports, by default it is set to _MCS, _GI and _BW but doesn't
 *    include _FMT. Use %IEEE80211_RADIOTAP_MCS_HAVE_\* values, only
 *    adding _BW is supported today.
 *
 * @radiotap_vht_details: lists which VHT MCS information the HW reports,
 *    the default is _GI | _BANDWIDTH.
 *    Use the %IEEE80211_RADIOTAP_VHT_KNOWN_\* values.
 *
 * @radiotap_timestamp: Information for the radiotap timestamp field; if the
 *    'units_pos' member is set to a non-negative value it must be set to
 *    a combination of a IEEE80211_RADIOTAP_TIMESTAMP_UNIT_* and a
 *    IEEE80211_RADIOTAP_TIMESTAMP_SPOS_* value, and then the timestamp
 *    field will be added and populated from the &struct ieee80211_rx_status
 *    device_timestamp. If the 'accuracy' member is non-negative, it's put
 *    into the accuracy radiotap field and the accuracy known flag is set.
 *
 * @netdev_features: netdev features to be set in each netdev created
 *    from this HW. Note that not all features are usable with mac80211,
 *    other features will be rejected during HW registration.
 *
 * @uapsd_queues: This bitmap is included in (re)association frame to indicate
 *    for each access category if it is uAPSD trigger-enabled and delivery-
 *    enabled. Use IEEE80211_WMM_IE_STA_QOSINFO_AC_* to set this bitmap.
 *    Each bit corresponds to different AC. Value '1' in specific bit means
 *    that corresponding AC is both trigger- and delivery-enabled. '0' means
 *    neither enabled.
 *
 * @uapsd_max_sp_len: maximum number of total buffered frames the WMM AP may
 *    deliver to a WMM STA during any Service Period triggered by the WMM STA.
 *    Use IEEE80211_WMM_IE_STA_QOSINFO_SP_* for correct values.
 *
 * @n_cipher_schemes: a size of an array of cipher schemes definitions.
 * @cipher_schemes: a pointer to an array of cipher scheme definitions
 *    supported by HW.
 * @max_nan_de_entries: maximum number of NAN DE functions supported by the
 *    device.
 */
struct ieee80211_hw {
    struct ieee80211_conf conf;
    struct wiphy *wiphy;
    const char *rate_control_algorithm;
    void *priv;
    unsigned long flags[BITS_TO_LONGS(NUM_IEEE80211_HW_FLAGS)];
    unsigned int extra_tx_headroom;
    unsigned int extra_beacon_tailroom;
    int vif_data_size;
    int sta_data_size;
    int chanctx_data_size;
    int txq_data_size;
    u16 queues;
    u16 max_listen_interval;
    s8 max_signal;
    u8 max_rates;
    u8 max_report_rates;
    u8 max_rate_tries;
    u8 max_rx_aggregation_subframes;
    u8 max_tx_aggregation_subframes;
    u8 max_tx_fragments;
    u8 offchannel_tx_hw_queue;
    u8 radiotap_mcs_details;
    u16 radiotap_vht_details;
    struct {
        int units_pos;
        s16 accuracy;
    } radiotap_timestamp;
    netdev_features_t netdev_features;
    u8 uapsd_queues;
    u8 uapsd_max_sp_len;
    u8 n_cipher_schemes;
    const struct ieee80211_cipher_scheme *cipher_schemes;
    u8 max_nan_de_entries;
};




#endif /* mac80211_h */
