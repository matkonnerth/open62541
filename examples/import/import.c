#include "import.h"
#include "nodesetLoader.h"
#include <open62541/server.h>


static UA_NodeId
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

static UA_NodeId getTypeDefinitionIdFromChars2(const TNode *node) 
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

static UA_NodeId
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
    if(ref==NULL)
    {
        return UA_NODEID_NULL;
    }
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

void importNodesCallback(void* userContext, const TNode* node)
{
    UA_Server *server = (UA_Server *)userContext;
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
                printf("adding method node %s failed\n", node->id.idString);
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
                printf("adding objecttype node %s failed\n", node->id.idString);
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

        case NODECLASS_VARIABLETYPE:
        {
            UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
            attr.valueRank = atoi(((const TVariableTypeNode *)node)->valueRank);
            attr.isAbstract = ((const TVariableTypeNode *)node)->isAbstract;
            if(attr.valueRank>=0)
            {
                if(!strcmp(((const TVariableTypeNode *)node)->arrayDimensions, ""))
                {
                    attr.arrayDimensionsSize = 1;
                    UA_UInt32 arrayDimensions[1];
                    arrayDimensions[0] = 0;
                    attr.arrayDimensions = &arrayDimensions[0];
                }
            }


            Reference *ref = getHierachicalInverseReference(node);            
            UA_NodeId parentId = getReferenceTarget(ref);
            UA_NodeId refId = getReferenceTypeId(ref);
            UA_NodeId typeDefId = getTypeDefinitionIdFromChars2(node);

            UA_StatusCode retval = UA_Server_addVariableTypeNode(
                server, id, parentId, refId, UA_QUALIFIEDNAME(1, node->browseName), typeDefId, attr,
                NULL, NULL);

            if(retval != UA_STATUSCODE_GOOD) {
                printf("adding variabletype node %s failed\n", node->id.idString);
            }
        } break;

        case NODECLASS_VARIABLE:
        {
            UA_VariableAttributes attr = UA_VariableAttributes_default;
            attr.dataType = getNodeIdFromChars(((const TVariableNode *)node)->datatype);
            attr.valueRank = atoi(((const TVariableNode *)node)->valueRank);
            if(attr.valueRank>=0)
            {
                if(!strcmp(((const TVariableNode *)node)->arrayDimensions, ""))
                {
                    attr.arrayDimensionsSize = 1;
                    UA_UInt32 arrayDimensions[1];
                    arrayDimensions[0] = 0;
                    attr.arrayDimensions = &arrayDimensions[0];
                }
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
                printf("adding datatype node %s failed\n", node->id.idString);
            }
        }
        break;
    }
}

int addNamespaceCallback(void* userContext, const char* namespaceUri)
{
    int idx = (int)UA_Server_addNamespace((UA_Server*) userContext, namespaceUri);
    return idx;
}