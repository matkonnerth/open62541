/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <ua_server.h>
#include <ua_config_default.h>
#include <ua_log_stdout.h>
#include <ua_client_highlevel.h>

#include <signal.h>

#include "ua_namespace_testnodeset.h"


UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}


int main(int argc, char** argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    //config->

    UA_StatusCode retval;
    /* create nodes from nodeset */
    if( ua_namespace_testnodeset (server) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Could not add the example nodeset. "
        "Check previous output for any error.");
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;        
    } else {

        UA_Variant out;
        UA_Variant_init(&out);
        UA_Server_readValue(server, UA_NODEID_NUMERIC(2,10002), &out);
        UA_Point* p = (UA_Point*)out.data;      
        printf("point 2d x: %d y: %d \n", p->x, p->y);       

        UA_Server_readValue(server, UA_NODEID_NUMERIC(2,6002), &out);            
        printf("point 3d x: %f y: %f z: %f \n", ((UA_Point3D*)out.data)->x, ((UA_Point3D*)out.data)->y, ((UA_Point3D*)out.data)->z);

        UA_StructureDefinition structdef;
        structdef.defaultEncodingId = UA_NODEID_NUMERIC(2,3002);
        structdef.baseDataType = UA_NODEID_NUMERIC(0,22);
        structdef.structureType = UA_STRUCTURETYPE_STRUCTURE;
        UA_StructureField fields[3];
        fields[0].name = UA_STRING("x");
        fields[0].description = UA_LOCALIZEDTEXT("de", "x");
        fields[0].dataType = UA_NODEID_NUMERIC(0, 7);
        fields[0].valueRank = -1;
        fields[0].arrayDimensionsSize = 0;
        fields[0].arrayDimensions = NULL;
        fields[0].isOptional = false;
        


        retval = UA_Server_run(server, &running);
    }

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)retval;
}
