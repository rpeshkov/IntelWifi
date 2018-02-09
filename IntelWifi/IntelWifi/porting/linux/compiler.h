//
//  compiler.h
//  IntelWifi
//
//  Created by Roman Peshkov on 06/01/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef compiler_h
#define compiler_h

#define __packed __attribute__((packed))
#define __aligned(x)        __attribute__((aligned(x)))
#define __must_check        __attribute__((warn_unused_result))

#endif /* compiler_h */
