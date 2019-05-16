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
myCallback(const TNode *node);

UA_NodeId
getTypeDefinitionIdFromChars2(const TNode *node);

UA_NodeId
getNodeIdFromChars(const char *id);

static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

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
            case 's':
            {
                idxSemi[0] = '\0';
                UA_UInt16 nsidx = (UA_UInt16)atoi(&id[3]);
                return UA_NODEID_STRING(nsidx, &idxSemi[3]);
                break;
            }
        }
        
    }
    return UA_NODEID_NULL;
}


UA_NodeId getTypeDefinitionIdFromChars2(const TNode *node) 
{
    Reference *hierachicalRef = node->nonHierachicalRefs;
    while(hierachicalRef)
    {
        if(!strcmp("HasTypeDefinition", hierachicalRef->refType)) {
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
           !strcmp("Organizes", hierachicalRef->refType)) {
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
           !strcmp("HasComponent", hierachicalRef->refType)) {
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
           !strcmp("HasProperty", hierachicalRef->refType)) {
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
           !strcmp("HasSubtype", hierachicalRef->refType)) {
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
    if(!strcmp(ref->refType, "HasProperty"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    }
    else if (!strcmp(ref->refType, "HasComponent"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    }
    else if (!strcmp(ref->refType, "Organizes"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    }
    else if (!strcmp(ref->refType, "HasTypeDefinition"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    }
    else if(!strcmp(ref->refType, "HasSubtype")) 
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    } 
    else if(!strcmp(ref->refType, "HasEncoding")) 
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

void printOrderedNodes(const TNode* node);
void printOrderedNodes(const TNode* node)
{
    printf("NodeId: %s BrowseName: %s DisplayName: %s\n", node->nodeId, node->browseName, node->displayName);
    
    switch (node->nodeClass)
    {
    case NODECLASS_OBJECT:
        //printf("\tparentNodeId: %s\n", ((const TObjectNode *)node)->parentNodeId);
        //printf("\teventNotifier: %s\n", ((const TObjectNode *)node)->eventNotifier);
        break;
    case NODECLASS_OBJECTTYPE:
        //printf("\tparentNodeId: %s\n", ((const TObjectType *)node)->
        
        break;
    case NODECLASS_DATATYPE:
    //printf("\tparentNodeId: %s\n", ((const TObjectType *)node)->
    
        break;
    case NODECLASS_VARIABLE:
        //printf("\tparentNodeId: %s\n", ((const TVariableNode *)node)->parentNodeId);
        //printf("\tdatatype: %s\n", ((const TVariableNode *)node)->datatype);
        //printf("\tvalueRank: %s\n", ((const TVariableNode *)node)->valueRank);
        //printf("\tarrayDimensions: %s\n", ((const TVariableNode *)node)->valueRank);
        break;

    case NODECLASS_METHOD:
        //printf("\tparentNodeId: %s\n", ((const TVariableNode *)node)->parentNodeId);
        //printf("\tdatatype: %s\n", ((const TVariableNode *)node)->datatype);
        //printf("\tvalueRank: %s\n", ((const TVariableNode *)node)->valueRank);
        //printf("\tarrayDimensions: %s\n", ((const TVariableNode *)node)->valueRank);
        break;
    case NODECLASS_REFERENCETYPE:
        break;
    }
    Reference *hierachicalRef = node->hierachicalRefs;
    while(hierachicalRef)
    {
        //printf("\treftype: %s target: %s\n", hierachicalRef->refType, hierachicalRef->target);
        hierachicalRef = hierachicalRef->next;
    }

    Reference *nonHierRef = node->nonHierachicalRefs;
    while (nonHierRef)
    {
        //printf("\treftype: %s target: %s\n", nonHierRef->refType, nonHierRef->target);
        nonHierRef = nonHierRef->next;
    }
}

void myCallback(const TNode* node)
{
    UA_NodeId id = getNodeIdFromChars(node->nodeId);
    //printf("NodeId: %s BrowseName: %s DisplayName %s\n", node->nodeId, node->browseName,
    //       node->displayName);
    //for(size_t i = 0; i < node->references->size; i++) {
    //    printf("\treftype: %s target: %s\n isForward: %i\n", node->references->refs[i]->refType, node->references->refs[i]->target, node->references->refs[i]->isForward);
    //}
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
                printf("adding object node %s failed\n", node->nodeId);
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
           // UA_NodeId typeDefId = getTypeDefinitionIdFromChars2(node);

            UA_StatusCode retval = UA_Server_addMethodNode(
                server, id, parentId, refId, UA_QUALIFIEDNAME(1, node->browseName), attr,
                NULL, 0, NULL, 0, NULL, NULL, NULL);

            if(retval != UA_STATUSCODE_GOOD) {
                printf("adding object node %s failed\n", node->nodeId);
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
                printf("adding object node %s failed\n", node->nodeId);
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
                printf("adding reftype node %s failed\n", node->nodeId);
            }
            break;
        }

        case NODECLASS_VARIABLE:
        {
            UA_VariableAttributes attr = UA_VariableAttributes_default;
            attr.dataType = getNodeIdFromChars(((const TVariableNode *)node)->datatype);
            attr.valueRank = atoi(((const TVariableNode *)node)->valueRank);

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
                printf("adding variable node %s failed\n", node->nodeId);
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
                printf("adding reftype node %s failed\n", node->nodeId);
            }
        }
        break;
    }
}

int main(int argc, char** argv) {

     if(argc!=2)
    {
        printf("specify nodesetfile as argument. E.g. xmlLoader text.xml\n");
        //return -1;
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
    handler.file = "/home/matzy/opcua/sdk/bin/testNodeset.xml";
    //handler.file = argv[1];
    handler.callback = myCallback;
    //handler.callback = printOrderedNodes;
    for(int i = 0; i < 1; i++)
    {
        loadFile(&handler);
    }
        

    retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
