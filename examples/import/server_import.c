/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <signal.h>
#include <stdlib.h>
#include "import.h"
#include "nodesetLoader.h"

UA_Boolean running = true;

static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(int argc, char** argv) {

    if(argc<2)
    {
        printf("specify nodesetfile as argument. E.g. server_import text.xml\n");
        return EXIT_FAILURE;
    }
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    
    UA_Server* server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_StatusCode retval;


    FileHandler handler;
    handler.callback = importNodesCallback;
    handler.addNamespace = addNamespaceCallback;
    handler.userContext = server;

    for(int cnt = 1; cnt < argc; cnt++)
    {
        handler.file = argv[cnt];
        if(!loadFile(&handler))
        {
            printf("nodeset could not be loaded, exit\n");
            return EXIT_FAILURE;
        }
    }

    retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
