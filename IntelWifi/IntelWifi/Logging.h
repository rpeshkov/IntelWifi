//
//  Logging.h
//  IntelWifi
//
//  Created by Roman Peshkov on 26/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#include <IOKit/IOLib.h>

#ifndef Logging_h
#define Logging_h

#ifdef DEBUG
#define DebugLog(args...) IOLog("IntelWifi: " args)
#else
#define DebugLog(args...)
#endif

#define TraceLog(args...) IOLog("IntelWifi: " args)

#define IWL_DEBUG_INFO(trans, args...) DebugLog("DEBUG INFO: " args)
#define IWL_ERR(trans, args...) TraceLog("ERROR: " args)
#define IWL_ERR_DEV(dev, args...) TraceLog("ERROR DEV: " args)
#define IWL_INFO(trans, args...) TraceLog("INFO: " args)
#define IWL_DEBUG_FW(drv, args...) DebugLog("DEBUG FW: " args)
#define IWL_WARN(drv, args...) TraceLog("WARN: " args)
#define IWL_DEBUG_EEPROM(dev, args...) DebugLog("DEBUG EEPROM: " args)
#define IWL_DEBUG_ISR(trans, args...) DebugLog("DEBUG ISR: " args)

#define IWL_DEBUG_RF_KILL(trans, args...) DebugLog("DEBUG RFKILL: " args)

#define IWL_DEBUG_POWER(trans, args...) DebugLog("DEBUG POWER: " args)

#define IWL_DEBUG_TX(trans, args...) DebugLog("DEBUG TX: " args)

#define IWL_DEBUG_RPM(trans, args...) DebugLog("DEBUG RPM: " args)
#define IWL_DEBUG_TX_QUEUES(trans, args...) DebugLog("DEBUG TXQ: " args)

#define IWL_DEBUG_CALIB(priv, args...) DebugLog("DEBUG CALIB: " args)

#define IWL_DEBUG_MAC80211(priv, args...) DebugLog("DEBUG MAC80211: " args)

#define IWL_DEBUG_ASSOC(priv, args...) DebugLog("DEBUG ASSOC: " args)
#define IWL_DEBUG_HC(priv, args...) DebugLog("DEBUG HC: " args)
#define IWL_DEBUG_11H(priv, args...) DebugLog("DEBUG 11H: " args)
#define IWL_DEBUG_RATE(priv, args...) DebugLog("DEBUG RATE: " args)
#define IWL_DEBUG_COEX(priv, args...) DebugLog("DEBUG COEX: " args)

#define IWL_DEBUG_RADIO(priv, args...) DebugLog("DEBUG RADIO: " args)
#define IWL_DEBUG_SCAN(priv, args...) DebugLog("DEBUG SCAN: " args)
#define IWL_DEBUG_RX(priv, args...) DebugLog("DEBUG RX: " args)
#define IWL_DEBUG_DROP_LIMIT(priv, args...) DebugLog("DEBUG DROP LIMIT: " args)
#define IWL_DEBUG_QUIET_RFKILL(priv, args...) DebugLog("DEBUG QUIET RFKILL: " args)
#define IWL_DEBUG_STATS(priv, args...) DebugLog("DEBUG STATS: " args)
#define IWL_DEBUG_DROP(priv, args...) DebugLog("DEBUG DROP: " args)
#define IWL_DEBUG_STATS_LIMIT(priv, args...) DebugLog("DEBUG STATS LIMIT: " args)
#define IWL_DEBUG_WEP(priv, args...) DebugLog("DEBUG WEP: " args)
#define IWL_DEBUG_TEMP(priv, args...) DebugLog("DEBUG TEMP: " args)

#define IWL_DEBUG_FRAME(priv, args...) DebugLog("DEBUG FRAME: " args)


#endif /* Logging_h */
