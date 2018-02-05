//
//  main.c
//  iwmc
//
//  Created by Roman Peshkov on 05/02/2018.
//  Copyright © 2018 Roman Peshkov. All rights reserved.
//

#include <stdio.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "logging.h"
#include "constants.h"
#include "client.h"


int main(int argc, const char * argv[]) {
    
    if (argc < 2) {
        error("Provide command. Available commands: scan\n");
        return 1;
    }
    
    struct iwmc_client *client = iwmc_alloc(IWMC_SERVICE_NAME);
    if (!client) {
        error("Failed to initialize client. Check that service is available.\n");
        return 1;
    }
    
    const char *cmd_name = argv[1];
    
    if (strcmp(cmd_name, IWMC_CMD_SCAN) == 0) {
        iwmc_scan(client);
        log("Scan command sent to client");
    }
    
    iwmc_free(client);
    client = NULL;
    
    return 0;
}
