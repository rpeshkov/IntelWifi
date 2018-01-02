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
#define DebugLog(args...) IOLog(args)
#else
#define DebugLog(args...)
#endif

#define TraceLog(args...) IOLog(args)

#define IWL_DEBUG_INFO(trans, args...) DebugLog("DEBUG INFO: " args)
#define IWL_ERR(trans, args...) TraceLog("ERROR: " args)
#define IWL_ERR_DEV(dev, args...) TraceLog("ERROR DEV: " args)
#define IWL_INFO(trans, args...) TraceLog("INFO: " args)
#define IWL_DEBUG_FW(drv, args...) DebugLog("DEBUG FW: " args)
#define IWL_WARN(drv, args...) TraceLog("WARN: " args)
#define IWL_DEBUG_EEPROM(dev, args...) DebugLog("DEBUG EEPROM: " args)
#define IWL_DEBUG_ISR(trans, args...) DebugLog("IntelWifi DEBUG ISR: " args)

#define IWL_DEBUG_RF_KILL(trans, args...) DebugLog("DEBUG RFKILL: " args)


#endif /* Logging_h */
