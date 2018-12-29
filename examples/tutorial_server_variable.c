/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Adding Variables to a Server
 * ----------------------------
 *
 * This tutorial shows how to work with data types and how to add variable nodes
 * to a server. First, we add a new variable to the server. Take a look at the
 * definition of the ``UA_VariableAttributes`` structure to see the list of all
 * attributes defined for VariableNodes.
 *
 * Note that the default settings have the AccessLevel of the variable value as
 * read only. See below for making the variable writable.
 */

#include <ua_server.h>
#include <ua_config_default.h>
#include <ua_log_stdout.h>

#include <signal.h>


static void
addVariable(UA_Server *server) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","t");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","ka");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    //UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);   
    UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                        parentReferenceNodeId, myIntegerName,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);

    
   
}

/**
 * Now we change the value with the write service. This uses the same service
 * implementation that can also be reached over the network by an OPC UA client.
 */

static void
writeVariable(UA_Server *server) {
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");

    /* Write a different integer value */
    UA_Int32 myInteger = 43;
    UA_Variant myVar;
    UA_Variant_init(&myVar);
    UA_Variant_setScalar(&myVar, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, myIntegerNodeId, myVar);

    /* Set the status code of the value to an error code. The function
     * UA_Server_write provides access to the raw service. The above
     * UA_Server_writeValue is syntactic sugar for writing a specific node
     * attribute with the write service. */
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = myIntegerNodeId;
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.status = UA_STATUSCODE_BADNOTCONNECTED;
    wv.value.hasStatus = true;
    UA_Server_write(server, &wv);

    /* Reset the variable to a good statuscode with a value */
    wv.value.hasStatus = false;
    wv.value.value = myVar;
    wv.value.hasValue = true;
    UA_Server_write(server, &wv);
}

/**
 * Note how we initially set the DataType attribute of the variable node to the
 * NodeId of the Int32 data type. This forbids writing values that are not an
 * Int32. The following code shows how this consistency check is performed for
 * every write.
 */

static void
writeWrongVariable(UA_Server *server) {
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");

    /* Write a string */
    UA_String myString = UA_STRING("test");
    UA_Variant myVar;
    UA_Variant_init(&myVar);
    UA_Variant_setScalar(&myVar, &myString, &UA_TYPES[UA_TYPES_STRING]);
    UA_StatusCode retval = UA_Server_writeValue(server, myIntegerNodeId, myVar);
    printf("Writing a string returned statuscode %s\n", UA_StatusCode_name(retval));
}

/** It follows the main server code, making use of the above definitions. */

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static UA_NodeId addObject(UA_NodeId parent, UA_Server* server, int m, int n)
{

    char buf[256];    
    snprintf(buf, sizeof buf, "%s%d%s%d", "Node_", m, "_", n);

    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", buf);
    

    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, buf);    
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES); 
    
    UA_NodeId newNode;
    UA_Server_addObjectNode(server, UA_NODEID_NULL, parent, parentReferenceNodeId, browseName,  UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), attr, NULL, &newNode);
    return newNode;
}

static void addChilds(UA_NodeId parent, UA_Server* server)
{
    static int stack=0;    
    //printf("stack: %d \n", stack);
    stack++;     

    for(int i=0; i<3; i++)
    {
        UA_NodeId id = addObject(parent, server, stack, i);
        if(stack<5)
        {
            addChilds(id, server);
        }        
    }  
    stack--;  
}

static void deepHierachicalNodes(UA_Server* server)
{
    //add folder

    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US","BrowseHere");
    

    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "BrowseStart");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES); 
    UA_NodeId newNodeId = UA_NODEID_NUMERIC(0, 5000);

    UA_Server_addObjectNode(server, newNodeId, parentNodeId, parentReferenceNodeId, browseName,  UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), attr, NULL, NULL);

    addChilds(newNodeId, server);
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

   
    for(int i=0; i<10; i++)
    {
        addVariable(server);
    }

    deepHierachicalNodes(server);

    for(int i=0; i<1000; i++)
    {
        addObject(UA_NODEID_NUMERIC(0,5000), server, 10, i);
    }    
    writeVariable(server);
    writeWrongVariable(server);

    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)retval;
}
