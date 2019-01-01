/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <ua_server.h>
#include <ua_config_default.h>
#include <ua_log_stdout.h>
#include <ua_client_highlevel.h>

#include <signal.h>

#include "ua_namespace_mynodeset.h"


UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}


static void
addVariable(UA_Server *server) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_String myString = UA_STRING("Hello World !");
    UA_Variant_setScalar(&attr.value, &myString, &UA_TYPES[UA_TYPES_STRING]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                              parentReferenceNodeId, UA_QUALIFIEDNAME(1, "myString"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}


int main(int argc, char** argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    UA_StatusCode retval;
    /* create nodes from nodeset */
    if( ua_namespace_mynodeset (server) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Could not add the example nodeset. "
        "Check previous output for any error.");
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
        //retval = UA_Server_run(server, &running);
    } else {

        UA_Variant out;
        UA_Variant_init(&out);
        UA_Server_readValue(server, UA_NODEID_NUMERIC(2,10002), &out);
        UA_Point* p = (UA_Point*)out.data;      
        printf("point 2d x: %d y: %d \n", p->x, p->y);

        for(int i=0; i<1000; i++)
        {
             p->y++;
            p->x++;
        }
       

        UA_Server_readValue(server, UA_NODEID_NUMERIC(2,6002), &out);
            
        printf("point 3d x: %f y: %f z: %f \n", ((UA_Point3D*)out.data)->x, ((UA_Point3D*)out.data)->y, ((UA_Point3D*)out.data)->z);   


        addVariable(server);
        //UA_Variant var;
        //UA_

        //UA_Server_writeValue

        retval = UA_Server_run(server, &running);
    }

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)retval;
}
