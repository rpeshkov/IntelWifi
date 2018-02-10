//
//  macro_stubs.h
//  IntelWifi
//
//  This file contains macros that are not usable in MacOS and you just want the compiler to ignore them
//
//  Created by Roman Peshkov on 08/02/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef macro_stubs_h
#define macro_stubs_h

#define MODULE_FIRMWARE(fw)
#define MODULE_DESCRIPTION(x)
#define MODULE_AUTHOR(x)
#define MODULE_LICENSE(x)

#define __init
#define __exit

#define module_init(x)
#define module_exit(x)

#define pr_info(x)

#define __rcu

# define __acquires(x)
# define __releases(x)
# define __acquire(x) (void)0
# define __release(x) (void)0

#define might_sleep()

#define __bitwise
#define __force

#define WARN_ON(x) (x)
#define WARN_ON_ONCE(x) (x)

#define __stringify OS_STRINGIFY

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

#endif /* macro_stubs_h */
