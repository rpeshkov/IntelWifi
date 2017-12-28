//
//  ieee80211.h
//  IntelWifi
//
//  Created by Roman Peshkov on 28/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef ieee80211_h
#define ieee80211_h

/* line 1344
 * Maximum length of AMPDU that the STA can receive in high-throughput (HT).
 * Length = 2 ^ (13 + max_ampdu_length_exp) - 1 (octets)
 */
enum ieee80211_max_ampdu_length_exp {
    IEEE80211_HT_MAX_AMPDU_8K = 0,
    IEEE80211_HT_MAX_AMPDU_16K = 1,
    IEEE80211_HT_MAX_AMPDU_32K = 2,
    IEEE80211_HT_MAX_AMPDU_64K = 3
};

/* line 1355
 * Maximum length of AMPDU that the STA can receive in VHT.
 * Length = 2 ^ (13 + max_ampdu_length_exp) - 1 (octets)
 */
enum ieee80211_vht_max_ampdu_length_exp {
    IEEE80211_VHT_MAX_AMPDU_8K = 0,
    IEEE80211_VHT_MAX_AMPDU_16K = 1,
    IEEE80211_VHT_MAX_AMPDU_32K = 2,
    IEEE80211_VHT_MAX_AMPDU_64K = 3,
    IEEE80211_VHT_MAX_AMPDU_128K = 4,
    IEEE80211_VHT_MAX_AMPDU_256K = 5,
    IEEE80211_VHT_MAX_AMPDU_512K = 6,
    IEEE80211_VHT_MAX_AMPDU_1024K = 7
};

#define IEEE80211_HT_MAX_AMPDU_FACTOR 13




#endif /* ieee80211_h */
