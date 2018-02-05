//
//  util.c
//  IntelWifi
//
//  Created by Roman Peshkov on 30/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#include <net/cfg80211.h>

int ieee80211_channel_to_frequency(int chan, enum nl80211_band band)
{
    /* see 802.11 17.3.8.3.2 and Annex J
     * there are overlapping channel numbers in 5GHz and 2GHz bands */
    if (chan <= 0)
        return 0; /* not supported */
    switch (band) {
        case NL80211_BAND_2GHZ:
            if (chan == 14)
                return 2484;
            else if (chan < 14)
                return 2407 + chan * 5;
            break;
        case NL80211_BAND_5GHZ:
            if (chan >= 182 && chan <= 196)
                return 4000 + chan * 5;
            else
                return 5000 + chan * 5;
            break;
        case NL80211_BAND_60GHZ:
            if (chan < 5)
                return 56160 + chan * 2160;
            break;
        default:
            ;
    }
    return 0; /* not supported */
}

