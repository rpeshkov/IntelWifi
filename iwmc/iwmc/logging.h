//
//  error.h
//  iwmc
//
//  Created by Roman Peshkov on 05/02/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef error_h
#define error_h

#include <stdlib.h>

static inline void log(const char *message) {
    fputs(message, stdout);
}

static inline void error(const char *message) {
    fputs(message, stderr);
}


#endif /* error_h */
