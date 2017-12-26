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


#endif /* Logging_h */
