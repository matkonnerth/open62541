/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/util.h>

#include <signal.h>
#include <stdio.h>

static UA_Boolean running = true;
static void
stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

static int kbhit(void);

static int
kbhit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int
main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, argv[1]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    const UA_NodeId moldFwdId = UA_NODEID_NUMERIC(0, 1000);
    //const UA_NodeId moldBwdId = UA_NODEID_NUMERIC(0,1001);
    UA_UInt16 val = 10;
    UA_Variant var10pct;
    UA_Variant_setScalarCopy(&var10pct, &val, &UA_TYPES[UA_TYPES_UINT16]);
    val =0;
    UA_Variant var0pct;
    UA_Variant_setScalarCopy(&var0pct, &val, &UA_TYPES[UA_TYPES_UINT16]);

    while(running) {
        retval = UA_Client_run_iterate(client, 100);

        //keep alive stuff
        retval = UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC(1, 6001), 0, NULL, NULL, NULL);

        if(UA_STATUSCODE_GOOD!=retval)
            break;

        //get key pressed
        if(kbhit())
        {
            int ch = fgetc(stdin);
            if(ch == 65) {
                printf("mold forward\n");                
                UA_Client_writeValueAttribute(client, moldFwdId,
                                              &var10pct);
            }
        }
        else
        {
            UA_Client_writeValueAttribute(client, moldFwdId, &var0pct);
        }
        
        
        printf("loop\n");
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
