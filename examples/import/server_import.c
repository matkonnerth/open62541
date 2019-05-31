/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <signal.h>
#include <stdlib.h>
#include <xmlparser.h>

UA_Boolean running = true;

UA_Server *server;

void
myCallback(const TNode *node);

UA_NodeId
getTypeDefinitionIdFromChars2(const TNode *node);

UA_NodeId
getNodeIdFromChars(TNodeId id);

static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

UA_NodeId
getNodeIdFromChars(TNodeId id)
{
    if (id.id==0)
    {
        return UA_NODEID_NULL;
    }
    UA_UInt16 nsidx = (UA_UInt16)id.nsIdx;

    switch(id.id[0]) {
        // integer
        case 'i': {
            UA_UInt32 nodeId = (UA_UInt32)atoi(&id.id[2]);               
            return UA_NODEID_NUMERIC(nsidx, nodeId);
            break;
        }
        case 's':
        {
            return UA_NODEID_STRING_ALLOC(nsidx, &id.id[2]);
            break;
        }
    }
    return UA_NODEID_NULL;
}

UA_NodeId getTypeDefinitionIdFromChars2(const TNode *node) 
{
    Reference *hierachicalRef = node->nonHierachicalRefs;
    while(hierachicalRef)
    {
        if(!strcmp("HasTypeDefinition", hierachicalRef->refType.idString)) {
            return getNodeIdFromChars(hierachicalRef->target);
        }
        hierachicalRef = hierachicalRef->next;
    }
    return UA_NODEID_NULL;
    return UA_NODEID_NULL;
}
UA_NodeId
getOrganizesId(const TNode *node);

UA_NodeId
getOrganizesId(const TNode *node) {
    Reference *hierachicalRef = node->hierachicalRefs;
    while(hierachicalRef)
    {
        if(!hierachicalRef->isForward &&
           !strcmp("Organizes", hierachicalRef->refType.idString)) {
            return getNodeIdFromChars(hierachicalRef->target);
        }
        hierachicalRef = hierachicalRef->next;
    }
    return UA_NODEID_NULL;
}

UA_NodeId
getHasComponentId(const TNode *node);

UA_NodeId
getHasComponentId(const TNode *node) {
    Reference *hierachicalRef = node->hierachicalRefs;
    while(hierachicalRef)
    {
        if(!hierachicalRef->isForward &&
           !strcmp("HasComponent", hierachicalRef->refType.idString)) {
            return getNodeIdFromChars(hierachicalRef->target);
        }
        hierachicalRef = hierachicalRef->next;
    }
    return UA_NODEID_NULL;
}

UA_NodeId
getHasPropertyId(const TNode *node);

UA_NodeId
getHasPropertyId(const TNode *node)
{
    Reference *hierachicalRef = node->hierachicalRefs;
    while(hierachicalRef)
    {
        if(!hierachicalRef->isForward &&
           !strcmp("HasProperty", hierachicalRef->refType.idString)) {
            return getNodeIdFromChars(hierachicalRef->target);
        }
        hierachicalRef = hierachicalRef->next;
    }
    return UA_NODEID_NULL;
}

UA_NodeId
getHasSubType(const TNode *node);

UA_NodeId
getHasSubType(const TNode *node) 
{
    Reference *hierachicalRef = node->hierachicalRefs;
    while(hierachicalRef)
    {        
        if(!hierachicalRef->isForward &&
           !strcmp("HasSubtype", hierachicalRef->refType.idString)) {
            return getNodeIdFromChars(hierachicalRef->target);
        }        
        hierachicalRef = hierachicalRef->next;
    }       
    return UA_NODEID_NULL;
}
UA_NodeId
getReferenceTypeId(const Reference *ref);

UA_NodeId
    getReferenceTypeId(const Reference *ref) {
    if(ref==NULL)
    {
        return UA_NODEID_NULL;
    }
    if(!strcmp(ref->refType.idString, "HasProperty"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    }
    else if (!strcmp(ref->refType.idString, "HasComponent"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    }
    else if (!strcmp(ref->refType.idString, "Organizes"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    }
    else if (!strcmp(ref->refType.idString, "HasTypeDefinition"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    }
    else if(!strcmp(ref->refType.idString, "HasSubtype")) 
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    } 
    else if(!strcmp(ref->refType.idString, "HasEncoding")) 
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASENCODING);
    } 
    else {
        return getNodeIdFromChars(ref->refType);
    }
    return UA_NODEID_NULL;
}
UA_NodeId
getReferenceTarget(const Reference *ref);

UA_NodeId getReferenceTarget(const Reference *ref) {
    return getNodeIdFromChars(ref->target);
}

Reference *
getHierachicalInverseReference(const TNode *node);

Reference *
getHierachicalInverseReference(const TNode *node) {

    Reference *hierachicalRef = node->hierachicalRefs;
    while(hierachicalRef)
    {
        if(!hierachicalRef->isForward)
        {
            return hierachicalRef;
        }        
        hierachicalRef = hierachicalRef->next;
    }   
    return NULL;
}

void myCallback(const TNode* node)
{
    UA_NodeId id = getNodeIdFromChars(node->id);
    switch(node->nodeClass) {
        case NODECLASS_OBJECT:
        {
        
            UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
            oAttr.displayName = UA_LOCALIZEDTEXT("en-US", node->displayName);
            UA_NodeId parentId =
                getNodeIdFromChars(((const TObjectNode *)node)->parentNodeId);
            Reference *ref = getHierachicalInverseReference(node);
            if(UA_NodeId_equal(&parentId, &UA_NODEID_NULL)) {
                parentId = getReferenceTarget(ref);
            }

            UA_NodeId refId = getReferenceTypeId(ref);
            UA_NodeId typeDefId = getTypeDefinitionIdFromChars2(node);

            UA_StatusCode retval = UA_Server_addObjectNode(
                server, id, parentId,refId,
                UA_QUALIFIEDNAME(1, node->browseName),
                typeDefId, oAttr, NULL, NULL);
            if(retval != UA_STATUSCODE_GOOD)
            {
                printf("adding object node %s failed\n", node->id.idString);
            }            
            break;
        }

        case NODECLASS_METHOD:
        {
            
            UA_MethodAttributes attr = UA_MethodAttributes_default;
            attr.executable = true;
            attr.userExecutable = true;
            attr.displayName = UA_LOCALIZEDTEXT("", node->displayName);


            UA_NodeId parentId =
                getNodeIdFromChars(((const TMethodNode *)node)->parentNodeId);
            Reference *ref = getHierachicalInverseReference(node);
            if(UA_NodeId_equal(&parentId, &UA_NODEID_NULL)) {
                parentId = getReferenceTarget(ref);
            }

            UA_NodeId refId = getReferenceTypeId(ref);

            UA_StatusCode retval = UA_Server_addMethodNode(
                server, id, parentId, refId, UA_QUALIFIEDNAME(1, node->browseName), attr,
                NULL, 0, NULL, 0, NULL, NULL, NULL);

            if(retval != UA_STATUSCODE_GOOD) {
                printf("adding object node %s failed\n", node->id.idString);
            }

            break;
        }

        case NODECLASS_OBJECTTYPE: {

            UA_ObjectTypeAttributes oAttr = UA_ObjectTypeAttributes_default;
            oAttr.displayName = UA_LOCALIZEDTEXT("en-US", node->displayName);
            
            Reference *ref = getHierachicalInverseReference(node);            
            UA_NodeId parentId = getReferenceTarget(ref);
            UA_NodeId refId = getReferenceTypeId(ref);

            UA_StatusCode retval = UA_Server_addObjectTypeNode(
                server, id, parentId, refId, UA_QUALIFIEDNAME(1, node->browseName), oAttr,
                NULL, NULL);

            if(retval != UA_STATUSCODE_GOOD) {
                printf("adding object node %s failed\n", node->id.idString);
            }

            break;
        }

        case NODECLASS_REFERENCETYPE: {

            UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
            attr.symmetric = true;
            attr.displayName = UA_LOCALIZEDTEXT("en-US", node->displayName);

            Reference *ref = getHierachicalInverseReference(node);
            UA_NodeId parentId = getReferenceTarget(ref);
            UA_NodeId refId = getReferenceTypeId(ref);

            UA_StatusCode retval = UA_Server_addReferenceTypeNode(
                server, id, parentId, refId, UA_QUALIFIEDNAME(1, node->browseName), attr,
                NULL, NULL);

            if(retval != UA_STATUSCODE_GOOD) {
                printf("adding reftype node %s failed\n", node->id.idString);
            }
            break;
        }

        case NODECLASS_VARIABLE:
        {
            UA_VariableAttributes attr = UA_VariableAttributes_default;
            attr.dataType = getNodeIdFromChars(((const TVariableNode *)node)->datatype);
            attr.valueRank = atoi(((const TVariableNode *)node)->valueRank);
            char *tmp = "";
            if(strcmp(((const TVariableNode *)node)->arrayDimensions, tmp))
            {
                attr.arrayDimensionsSize = 1;
                UA_UInt32 dim = (UA_UInt32)atoi(((const TVariableNode *)node)->arrayDimensions);
                attr.arrayDimensions = &dim;
            }

            UA_NodeId parentId =
                getNodeIdFromChars(((const TVariableNode *)node)->parentNodeId);
            Reference *ref = getHierachicalInverseReference(node);
            if(UA_NodeId_equal(&parentId, &UA_NODEID_NULL)) {
                parentId = getReferenceTarget(ref);
            }

            UA_NodeId refId = getReferenceTypeId(ref);
            UA_NodeId typeDefId = getTypeDefinitionIdFromChars2(node);
            UA_StatusCode retval = UA_Server_addVariableNode(
                server, id, parentId, refId, UA_QUALIFIEDNAME(1, node->browseName),
                typeDefId, attr, NULL, NULL);
            if(retval != UA_STATUSCODE_GOOD) {
                printf("adding variable node %s failed\n", node->id.idString);
            }
            break;
        }
        case NODECLASS_DATATYPE:
        {
            UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
            attr.displayName = UA_LOCALIZEDTEXT("", node->displayName);

            Reference *ref = getHierachicalInverseReference(node);            
            UA_NodeId parentId = getReferenceTarget(ref);
            UA_NodeId refId = getReferenceTypeId(ref);

            UA_StatusCode retval = UA_Server_addDataTypeNode(
                server, id, parentId, refId, UA_QUALIFIEDNAME(1, node->browseName), attr,
                NULL, NULL);
            if(retval != UA_STATUSCODE_GOOD) {
                printf("adding reftype node %s failed\n", node->id.idString);
            }
        }
        break;
    }
}

static int addNamespace(const char *namespaceUri);
static int addNamespace(const char* namespaceUri)
{
    int idx = (int)UA_Server_addNamespace(server, namespaceUri);
    return idx;
}

int main(int argc, char** argv) {

     if(argc!=2)
    {
        printf("specify nodesetfile as argument. E.g. xmlLoader text.xml\n");
        return -1;
    }
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_StatusCode retval;

    FileHandler handler;
    //handler.file = "/home/matzy/git/open62541/examples/import/system.txt";
    //handler.file = "/mnt/c/c2k/git/mkOpen62541/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml";
    //handler.file = "/home/matzy/git/open62541/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml";
    //handler.file = "/home/matzy/opcua/sdk/bin/testNodeset.xml";
    handler.file = argv[1];
    handler.callback = myCallback;
    handler.addNamespace = addNamespace;

    for(int cnt = 1; cnt < argc; cnt++)
    {
        handler.file = argv[cnt];
        loadFile(&handler);
    }


    retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
