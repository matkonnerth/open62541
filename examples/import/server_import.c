/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <xmlLoader.h>



#include <signal.h>
#include <stdlib.h>

UA_Boolean running = true;

UA_Server *server;

void
myCallback(TNodeClass nodeClass, const TNode *node);

UA_NodeId
getTypeDefinitionIdFromChars2(const TNode *node);

UA_NodeId
getNodeIdFromChars(const char *id);

static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/* Default Binary - ns=0;i=298 */
/*
static UA_StatusCode
function_namespace0_generated_51_begin(UA_Server *server, UA_UInt16 *ns) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "Default Binary");
    retVal |= UA_Server_addNode_begin(
        server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[0], 298),
        UA_NODEID_NUMERIC(ns[0], 0), UA_NODEID_NUMERIC(ns[0], 0),
        UA_QUALIFIEDNAME(ns[0], "Default Binary"), UA_NODEID_NUMERIC(ns[0], 76),
        (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
        NULL);
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[0], 298),
                                     UA_NODEID_NUMERIC(ns[0], 38),
                                     UA_EXPANDEDNODEID_NUMERIC(ns[0], 296), false);
    return retVal;
}

static UA_StatusCode
function_namespace0_generated_51_finish(UA_Server *server, UA_UInt16 *ns) {
    return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[0], 298));
}
*/



UA_NodeId
getNodeIdFromChars(const char *id)
{
    if (id==NULL)
    {
        return UA_NODEID_NULL;
    }
    char* idxSemi = strchr(id, ';');
    if(idxSemi==NULL)
    {
        switch(id[0])
        {
            //integer
            case 'i':
            {
                UA_UInt32 idx = (UA_UInt32) atoi(&id[2]);
                return UA_NODEID_NUMERIC(0, idx);
                break;
            }
                
            
        }
    }
    else
    {        
        switch(idxSemi[1]) {
            // integer
            case 'i': {
                UA_UInt32 nodeId = (UA_UInt32)atoi(&idxSemi[3]);
                idxSemi[0] = '\0';
                UA_UInt16 nsidx = (UA_UInt16)atoi(&id[3]);
                return UA_NODEID_NUMERIC(nsidx, nodeId);
                break;
            }
        }
        
    }
    return UA_NODEID_NULL;
}


UA_NodeId getTypeDefinitionIdFromChars2(const TNode *node) 
{
    for(size_t i = 0; i < node->references->size; i++) {
        if(!strcmp("HasTypeDefinition", node->references->refs[i]->refType))
        {
            return getNodeIdFromChars(node->references->refs[i]->target);
        }
    }
    return UA_NODEID_NULL;
}
UA_NodeId
getOrganizesId(const TNode *node);

UA_NodeId
getOrganizesId(const TNode *node) {
    for(size_t i = 0; i < node->references->size; i++) {
        if(!node->references->refs[i]->isForward && !strcmp("Organizes",
                                            node->references->refs[i]->refType)) {
            return getNodeIdFromChars(node->references->refs[i]->target);
        }
    }
    return UA_NODEID_NULL;
}

UA_NodeId
getHasSubType(const TNode *node);

UA_NodeId
getHasSubType(const TNode *node) {
    for(size_t i = 0; i < node->references->size; i++) {
        if(!node->references->refs[i]->isForward &&
           !strcmp("HasSubtype", node->references->refs[i]->refType)) {
            return getNodeIdFromChars(node->references->refs[i]->target);
        }
    }
    return UA_NODEID_NULL;
}

void myCallback(TNodeClass nodeClass, const TNode* node)
{
    UA_NodeId id = getNodeIdFromChars(node->nodeId);
    printf("BrowseName: %s\n", node->browseName);
    printf("DisplayName: %s\n", node->displayName);
    for(size_t i = 0; i < node->references->size; i++) {
        printf("reftype: %s target: %s\n", node->references->refs[i]->refType, node->references->refs[i]->target);
    }
    switch(nodeClass) {
        case NODECLASS_OBJECT:
        {
        
                UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
                oAttr.displayName = UA_LOCALIZEDTEXT("en-US", node->displayName);
                UA_NodeId parentId =
                    getNodeIdFromChars(((const TObjectNode *)node)->parentNodeId);
                if(UA_NodeId_equal(&parentId, &UA_NODEID_NULL)) {
                    parentId = getOrganizesId(node);
                }
                UA_NodeId refId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
                UA_NodeId typeDefId = getTypeDefinitionIdFromChars2(node);

                UA_Server_addObjectNode(
                    server, id, parentId,refId,
                    UA_QUALIFIEDNAME(1, node->browseName),
                    typeDefId, oAttr, NULL, NULL);
            
                break;
        }
        case NODECLASS_OBJECTTYPE: {

            UA_ObjectTypeAttributes oAttr = UA_ObjectTypeAttributes_default;
            oAttr.displayName = UA_LOCALIZEDTEXT("en-US", node->displayName);

            UA_NodeId parentId = getHasSubType(node);
            UA_NodeId refId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
            

            UA_Server_addObjectTypeNode(
                server, id, parentId, refId, UA_QUALIFIEDNAME(1, node->browseName), 
                 oAttr, NULL, NULL);

            break;
        }


        case NODECLASS_VARIABLE:
        {
            UA_VariableAttributes attr = UA_VariableAttributes_default;
            attr.dataType = getNodeIdFromChars(((const TVariableNode *)node)->datatype);

            UA_NodeId parentId =
                getNodeIdFromChars(((const TVariableNode *)node)->parentNodeId);
            if(UA_NodeId_equal(&parentId, &UA_NODEID_NULL)) {
                parentId = getOrganizesId(node);
            }
            UA_NodeId refId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
            UA_NodeId typeDefId = getTypeDefinitionIdFromChars2(node);
            UA_Server_addVariableNode(server, id, parentId, refId,
                                      UA_QUALIFIEDNAME(1, node->browseName), typeDefId,
                                      attr, NULL, NULL);
            break;
        }
        case NODECLASS_DATATYPE:
        {
            UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
            attr.displayName = UA_LOCALIZEDTEXT("", node->displayName);

            UA_NodeId parentId = getHasSubType(node);
            UA_NodeId refId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);

            UA_Server_addDataTypeNode(server, id, parentId, refId,
                                      UA_QUALIFIEDNAME(1, node->browseName), attr, NULL,
                                      NULL);

        }
        break;
    }
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_StatusCode retval;

    FileHandler handler;
    //handler.file = "/home/matzy/git/open62541/examples/import/system.txt";
    //handler.file = "/home/matzy/git/open62541/examples/nodeset/testnodeset.xml";
    handler.file = "/home/matzy/git/open62541/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml";
    handler.callback = myCallback;
    loadFile(&handler);

    retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
