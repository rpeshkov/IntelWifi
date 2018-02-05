//
//  client.h
//  iwmc
//
//  Created by Roman Peshkov on 05/02/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#ifndef client_h
#define client_h

#include <stdio.h>

struct iwmc_client {
    void *priv;
};


/*
 * Allocation/freeing
 */
struct iwmc_client *iwmc_alloc(const char *name);
void iwmc_free(struct iwmc_client* client);

/*
 * Commands
 */
void iwmc_scan(struct iwmc_client* client);


#endif /* client_h */
