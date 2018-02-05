//
//  regulatory.h
//  IntelWifi
//
//  Created by Roman Peshkov on 01/02/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef regulatory_h
#define regulatory_h

#include <linux/bitfield.h>

/**
 * enum ieee80211_regulatory_flags - device regulatory flags
 *
 * @REGULATORY_CUSTOM_REG: tells us the driver for this device
 *    has its own custom regulatory domain and cannot identify the
 *    ISO / IEC 3166 alpha2 it belongs to. When this is enabled
 *    we will disregard the first regulatory hint (when the
 *    initiator is %REGDOM_SET_BY_CORE). Drivers that use
 *    wiphy_apply_custom_regulatory() should have this flag set
 *    or the regulatory core will set it for the wiphy.
 *    If you use regulatory_hint() *after* using
 *    wiphy_apply_custom_regulatory() the wireless core will
 *    clear the REGULATORY_CUSTOM_REG for your wiphy as it would be
 *    implied that the device somehow gained knowledge of its region.
 * @REGULATORY_STRICT_REG: tells us that the wiphy for this device
 *    has regulatory domain that it wishes to be considered as the
 *    superset for regulatory rules. After this device gets its regulatory
 *    domain programmed further regulatory hints shall only be considered
 *    for this device to enhance regulatory compliance, forcing the
 *    device to only possibly use subsets of the original regulatory
 *    rules. For example if channel 13 and 14 are disabled by this
 *    device's regulatory domain no user specified regulatory hint which
 *    has these channels enabled would enable them for this wiphy,
 *    the device's original regulatory domain will be trusted as the
 *    base. You can program the superset of regulatory rules for this
 *    wiphy with regulatory_hint() for cards programmed with an
 *    ISO3166-alpha2 country code. wiphys that use regulatory_hint()
 *    will have their wiphy->regd programmed once the regulatory
 *    domain is set, and all other regulatory hints will be ignored
 *    until their own regulatory domain gets programmed.
 * @REGULATORY_DISABLE_BEACON_HINTS: enable this if your driver needs to
 *    ensure that passive scan flags and beaconing flags may not be lifted by
 *    cfg80211 due to regulatory beacon hints. For more information on beacon
 *    hints read the documenation for regulatory_hint_found_beacon()
 * @REGULATORY_COUNTRY_IE_FOLLOW_POWER:  for devices that have a preference
 *    that even though they may have programmed their own custom power
 *    setting prior to wiphy registration, they want to ensure their channel
 *    power settings are updated for this connection with the power settings
 *    derived from the regulatory domain. The regulatory domain used will be
 *    based on the ISO3166-alpha2 from country IE provided through
 *    regulatory_hint_country_ie()
 * @REGULATORY_COUNTRY_IE_IGNORE: for devices that have a preference to ignore
 *     all country IE information processed by the regulatory core. This will
 *     override %REGULATORY_COUNTRY_IE_FOLLOW_POWER as all country IEs will
 *     be ignored.
 * @REGULATORY_ENABLE_RELAX_NO_IR: for devices that wish to allow the
 *      NO_IR relaxation, which enables transmissions on channels on which
 *      otherwise initiating radiation is not allowed. This will enable the
 *      relaxations enabled under the CFG80211_REG_RELAX_NO_IR configuration
 *      option
 * @REGULATORY_IGNORE_STALE_KICKOFF: the regulatory core will _not_ make sure
 *    all interfaces on this wiphy reside on allowed channels. If this flag
 *    is not set, upon a regdomain change, the interfaces are given a grace
 *    period (currently 60 seconds) to disconnect or move to an allowed
 *    channel. Interfaces on forbidden channels are forcibly disconnected.
 *    Currently these types of interfaces are supported for enforcement:
 *    NL80211_IFTYPE_ADHOC, NL80211_IFTYPE_STATION, NL80211_IFTYPE_AP,
 *    NL80211_IFTYPE_AP_VLAN, NL80211_IFTYPE_MONITOR,
 *    NL80211_IFTYPE_P2P_CLIENT, NL80211_IFTYPE_P2P_GO,
 *    NL80211_IFTYPE_P2P_DEVICE. The flag will be set by default if a device
 *    includes any modes unsupported for enforcement checking.
 * @REGULATORY_WIPHY_SELF_MANAGED: for devices that employ wiphy-specific
 *    regdom management. These devices will ignore all regdom changes not
 *    originating from their own wiphy.
 *    A self-managed wiphys only employs regulatory information obtained from
 *    the FW and driver and does not use other cfg80211 sources like
 *    beacon-hints, country-code IEs and hints from other devices on the same
 *    system. Conversely, a self-managed wiphy does not share its regulatory
 *    hints with other devices in the system. If a system contains several
 *    devices, one or more of which are self-managed, there might be
 *    contradictory regulatory settings between them. Usage of flag is
 *    generally discouraged. Only use it if the FW/driver is incompatible
 *    with non-locally originated hints.
 *    This flag is incompatible with the flags: %REGULATORY_CUSTOM_REG,
 *    %REGULATORY_STRICT_REG, %REGULATORY_COUNTRY_IE_FOLLOW_POWER,
 *    %REGULATORY_COUNTRY_IE_IGNORE and %REGULATORY_DISABLE_BEACON_HINTS.
 *    Mixing any of the above flags with this flag will result in a failure
 *    to register the wiphy. This flag implies
 *    %REGULATORY_DISABLE_BEACON_HINTS and %REGULATORY_COUNTRY_IE_IGNORE.
 */
enum ieee80211_regulatory_flags {
    REGULATORY_CUSTOM_REG            = BIT(0),
    REGULATORY_STRICT_REG            = BIT(1),
    REGULATORY_DISABLE_BEACON_HINTS        = BIT(2),
    REGULATORY_COUNTRY_IE_FOLLOW_POWER    = BIT(3),
    REGULATORY_COUNTRY_IE_IGNORE        = BIT(4),
    REGULATORY_ENABLE_RELAX_NO_IR           = BIT(5),
    REGULATORY_IGNORE_STALE_KICKOFF         = BIT(6),
    REGULATORY_WIPHY_SELF_MANAGED        = BIT(7),
};


#endif /* regulatory_h */
