//
//  client.c
//  iwmc
//
//  Created by Roman Peshkov on 05/02/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include <IOKit/IOKitLib.h>

#include <stdio.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>


#include "client.h"

#include "kext_user_shared.h"

/**
 * Private data of the client
 */
struct iwmc_priv {
    mach_port_t master_port;
    io_service_t service;
    io_connect_t data_port;
};

#define IWMC_PRIV(client) ((struct iwmc_priv*)client->priv)

/**
 * Allocate new client and open gate to service
 */
struct iwmc_client* iwmc_alloc(const char *name) {
    kern_return_t kern_result;
    CFMutableDictionaryRef matching_dict;
    
    struct iwmc_priv *priv = malloc(sizeof(struct iwmc_priv));
    if (!priv) {
        return NULL;
    }
    
    kern_result = IOMasterPort(MACH_PORT_NULL, &priv->master_port);
    if (kern_result != KERN_SUCCESS) {
        goto free_priv;
    }
    
    matching_dict = IOServiceNameMatching(name);
    if (matching_dict == NULL) {
        goto free_master_port;
    }
    
    priv->service = IOServiceGetMatchingService(priv->master_port, matching_dict);
    if (!priv->service) {
        goto free_master_port;
    }
    
    kern_result = IOServiceOpen(priv->service, mach_task_self(), 0, &priv->data_port);
    if (kern_result != KERN_SUCCESS) {
        goto free_service;
    }
    
    struct iwmc_client *client = malloc(sizeof(struct iwmc_client));
    client->priv = priv;
    return client;
free_service:
    IOObjectRelease(priv->service);
free_master_port:
    IOObjectRelease(priv->master_port);
free_priv:
    free(priv);
    return NULL;
}

/**
 * Close the gate to service and free all resources used by client
 */
void iwmc_free(struct iwmc_client* client) {
    struct iwmc_priv *priv = IWMC_PRIV(client);
    
    IOServiceClose(priv->service);
    IOObjectRelease(priv->service);
    
    IOObjectRelease(priv->master_port);
    
    free(priv);
    client->priv = NULL;
    
    free(client);
}

/**
 * Initiate scan routing in service
 */
void iwmc_scan(struct iwmc_client* client) {
    struct iwmc_priv *priv = IWMC_PRIV(client);
    IOConnectCallScalarMethod(priv->data_port, kIwlClientScan, 0, 0, 0, 0);
}
