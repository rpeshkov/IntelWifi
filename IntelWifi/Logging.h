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

#define IWL_DEBUG_INFO(trans, args...) DebugLog(args)
#define IWL_ERR(trans, args...) TraceLog("ERROR: " args)
#define IWL_INFO(trans, args...) TraceLog("INFO: " args)
#define IWL_DEBUG_FW(drv, args...) DebugLog("DEBUG FW: " args)


#endif /* Logging_h */
