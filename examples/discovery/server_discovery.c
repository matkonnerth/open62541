/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
/*
 * Server representing a local discovery server as a central instance.
 * Any other server can register with this server (see server_register.c). Clients can then call the
 * find servers service to get all registered servers (see client_find_servers.c).
 */

#include <stdio.h>
#include <signal.h>

#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_server.h"
# include "ua_config_standard.h"
# include "ua_network_tcp.h"
#else
# include "open62541.h"
#endif

UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

int main(void) {
    signal(SIGINT,  stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig config = UA_ServerConfig_standard;
    config.applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;
    config.applicationDescription.applicationUri = UA_String_fromChars("open62541.example.local_discovery_server");
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 4048);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);

    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    return (int)retval;
}