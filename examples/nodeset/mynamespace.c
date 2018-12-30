/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#include "mynamespace.h"


/* Point - ns=1;i=10001 */

static UA_StatusCode function_mynamespace_0_begin(UA_Server *server, UA_UInt16* ns) {
UA_StatusCode retVal = UA_STATUSCODE_GOOD;
UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
attr.displayName = UA_LOCALIZEDTEXT("", "Point");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
attr.description = UA_LOCALIZEDTEXT("", "");
#endif
attr.writeMask = 0;
attr.userWriteMask = 0;
retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_DATATYPE,
UA_NODEID_NUMERIC(ns[1], 10001),
UA_NODEID_NUMERIC(ns[0], 22),
UA_NODEID_NUMERIC(ns[0], 45),
UA_QUALIFIEDNAME(ns[1], "Point"),
 UA_NODEID_NULL,
(const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES],NULL, NULL);
return retVal;
}

static UA_StatusCode function_mynamespace_0_finish(UA_Server *server, UA_UInt16* ns) {
return UA_Server_addNode_finish(server, 
UA_NODEID_NUMERIC(ns[1], 10001)
);
}

/* testType - ns=1;i=1001 */

static UA_StatusCode function_mynamespace_1_begin(UA_Server *server, UA_UInt16* ns) {
UA_StatusCode retVal = UA_STATUSCODE_GOOD;
UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
attr.displayName = UA_LOCALIZEDTEXT("", "testType");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
attr.description = UA_LOCALIZEDTEXT("", "");
#endif
attr.writeMask = 0;
attr.userWriteMask = 0;
retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECTTYPE,
UA_NODEID_NUMERIC(ns[1], 1001),
UA_NODEID_NUMERIC(ns[0], 58),
UA_NODEID_NUMERIC(ns[0], 45),
UA_QUALIFIEDNAME(ns[1], "testType"),
 UA_NODEID_NULL,
(const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],NULL, NULL);
return retVal;
}

static UA_StatusCode function_mynamespace_1_finish(UA_Server *server, UA_UInt16* ns) {
return UA_Server_addNode_finish(server, 
UA_NODEID_NUMERIC(ns[1], 1001)
);
}

/* Var1 - ns=1;i=6001 */

static UA_StatusCode function_mynamespace_2_begin(UA_Server *server, UA_UInt16* ns) {
UA_StatusCode retVal = UA_STATUSCODE_GOOD;
UA_VariableAttributes attr = UA_VariableAttributes_default;
attr.minimumSamplingInterval = 0.000000;
attr.userAccessLevel = 3;
attr.accessLevel = 3;
attr.valueRank = -1;
attr.dataType = UA_NODEID_NUMERIC(ns[0], 11);
UA_Double *variablenode_ns_1_i_6001_variant_DataContents =  UA_Double_new();
if (!variablenode_ns_1_i_6001_variant_DataContents) return UA_STATUSCODE_BADOUTOFMEMORY;
*variablenode_ns_1_i_6001_variant_DataContents = (UA_Double) 42.0;
UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_6001_variant_DataContents, &UA_TYPES[UA_TYPES_DOUBLE]);
attr.displayName = UA_LOCALIZEDTEXT("", "Var1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
attr.description = UA_LOCALIZEDTEXT("", "");
#endif
attr.writeMask = 0;
attr.userWriteMask = 0;
retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE,
UA_NODEID_NUMERIC(ns[1], 6001),
UA_NODEID_NUMERIC(ns[1], 1001),
UA_NODEID_NUMERIC(ns[0], 47),
UA_QUALIFIEDNAME(ns[1], "Var1"),
UA_NODEID_NUMERIC(ns[0], 63),
(const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],NULL, NULL);
UA_Double_delete(variablenode_ns_1_i_6001_variant_DataContents);
retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 6001), UA_NODEID_NUMERIC(ns[0], 37), UA_EXPANDEDNODEID_NUMERIC(ns[0], 78), true);
return retVal;
}

static UA_StatusCode function_mynamespace_2_finish(UA_Server *server, UA_UInt16* ns) {
return UA_Server_addNode_finish(server, 
UA_NODEID_NUMERIC(ns[1], 6001)
);
}

/* testFolder - ns=1;i=5002 */

static UA_StatusCode function_mynamespace_3_begin(UA_Server *server, UA_UInt16* ns) {
UA_StatusCode retVal = UA_STATUSCODE_GOOD;
UA_ObjectAttributes attr = UA_ObjectAttributes_default;
attr.displayName = UA_LOCALIZEDTEXT("", "testFolder");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
attr.description = UA_LOCALIZEDTEXT("", "");
#endif
attr.writeMask = 0;
attr.userWriteMask = 0;
retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT,
UA_NODEID_NUMERIC(ns[1], 5002),
UA_NODEID_NUMERIC(ns[0], 85),
UA_NODEID_NUMERIC(ns[0], 35),
UA_QUALIFIEDNAME(ns[1], "testFolder"),
UA_NODEID_NUMERIC(ns[0], 61),
(const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],NULL, NULL);
return retVal;
}

static UA_StatusCode function_mynamespace_3_finish(UA_Server *server, UA_UInt16* ns) {
return UA_Server_addNode_finish(server, 
UA_NODEID_NUMERIC(ns[1], 5002)
);
}

/* testInstance - ns=1;i=5001 */

static UA_StatusCode function_mynamespace_4_begin(UA_Server *server, UA_UInt16* ns) {
UA_StatusCode retVal = UA_STATUSCODE_GOOD;
UA_ObjectAttributes attr = UA_ObjectAttributes_default;
attr.displayName = UA_LOCALIZEDTEXT("", "testInstance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
attr.description = UA_LOCALIZEDTEXT("", "");
#endif
attr.writeMask = 0;
attr.userWriteMask = 0;
retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT,
UA_NODEID_NUMERIC(ns[1], 5001),
UA_NODEID_NUMERIC(ns[1], 5002),
UA_NODEID_NUMERIC(ns[0], 35),
UA_QUALIFIEDNAME(ns[1], "testInstance"),
UA_NODEID_NUMERIC(ns[1], 1001),
(const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],NULL, NULL);
return retVal;
}

static UA_StatusCode function_mynamespace_4_finish(UA_Server *server, UA_UInt16* ns) {
return UA_Server_addNode_finish(server, 
UA_NODEID_NUMERIC(ns[1], 5001)
);
}

/* PointA - ns=1;i=10002 */

static UA_StatusCode function_mynamespace_5_begin(UA_Server *server, UA_UInt16* ns) {
UA_StatusCode retVal = UA_STATUSCODE_GOOD;
UA_VariableAttributes attr = UA_VariableAttributes_default;
attr.minimumSamplingInterval = 0.000000;
attr.userAccessLevel = 3;
attr.accessLevel = 3;
attr.valueRank = -1;
attr.dataType = UA_NODEID_NUMERIC(ns[1], 10001);
UA_STACKARRAY(UA_Point, variablenode_ns_1_i_10002_variant_DataContents, 1);
UA_init(variablenode_ns_1_i_10002_variant_DataContents, &UA_MYTYPES[UA_MYTYPES_POINT]);
UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_10002_variant_DataContents, &UA_MYTYPES[UA_MYTYPES_POINT]);
attr.displayName = UA_LOCALIZEDTEXT("", "PointA");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
attr.description = UA_LOCALIZEDTEXT("", "");
#endif
attr.writeMask = 0;
attr.userWriteMask = 0;
retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE,
UA_NODEID_NUMERIC(ns[1], 10002),
UA_NODEID_NUMERIC(ns[1], 5001),
UA_NODEID_NUMERIC(ns[0], 47),
UA_QUALIFIEDNAME(ns[1], "PointA"),
UA_NODEID_NUMERIC(ns[0], 63),
(const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],NULL, NULL);
return retVal;
}

static UA_StatusCode function_mynamespace_5_finish(UA_Server *server, UA_UInt16* ns) {
return UA_Server_addNode_finish(server, 
UA_NODEID_NUMERIC(ns[1], 10002)
);
}

/* Var2 - ns=1;i=6002 */

static UA_StatusCode function_mynamespace_6_begin(UA_Server *server, UA_UInt16* ns) {
UA_StatusCode retVal = UA_STATUSCODE_GOOD;
UA_VariableAttributes attr = UA_VariableAttributes_default;
attr.minimumSamplingInterval = 0.000000;
attr.userAccessLevel = 3;
attr.accessLevel = 3;
attr.valueRank = -1;
attr.dataType = UA_NODEID_NUMERIC(ns[0], 7);
UA_UInt32 *variablenode_ns_1_i_6002_variant_DataContents =  UA_UInt32_new();
if (!variablenode_ns_1_i_6002_variant_DataContents) return UA_STATUSCODE_BADOUTOFMEMORY;
*variablenode_ns_1_i_6002_variant_DataContents = (UA_UInt32) 1;
UA_Variant_setScalar(&attr.value, variablenode_ns_1_i_6002_variant_DataContents, &UA_TYPES[UA_TYPES_UINT32]);
attr.displayName = UA_LOCALIZEDTEXT("", "Var2");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
attr.description = UA_LOCALIZEDTEXT("", "");
#endif
attr.writeMask = 0;
attr.userWriteMask = 0;
retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE,
UA_NODEID_NUMERIC(ns[1], 6002),
UA_NODEID_NUMERIC(ns[1], 5001),
UA_NODEID_NUMERIC(ns[0], 47),
UA_QUALIFIEDNAME(ns[1], "Var1"),
UA_NODEID_NUMERIC(ns[0], 63),
(const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],NULL, NULL);
UA_UInt32_delete(variablenode_ns_1_i_6002_variant_DataContents);
return retVal;
}

static UA_StatusCode function_mynamespace_6_finish(UA_Server *server, UA_UInt16* ns) {
return UA_Server_addNode_finish(server, 
UA_NODEID_NUMERIC(ns[1], 6002)
);
}

UA_StatusCode mynamespace(UA_Server *server) {
UA_StatusCode retVal = UA_STATUSCODE_GOOD;
/* Use namespace ids generated by the server */
UA_UInt16 ns[2];
ns[0] = UA_Server_addNamespace(server, "http://opcfoundation.org/UA/");
ns[1] = UA_Server_addNamespace(server, "http://yourorganisation.org/test/");
retVal |= function_mynamespace_0_begin(server, ns);
retVal |= function_mynamespace_1_begin(server, ns);
retVal |= function_mynamespace_2_begin(server, ns);
retVal |= function_mynamespace_3_begin(server, ns);
retVal |= function_mynamespace_4_begin(server, ns);
retVal |= function_mynamespace_5_begin(server, ns);
retVal |= function_mynamespace_6_begin(server, ns);
retVal |= function_mynamespace_6_finish(server, ns);
retVal |= function_mynamespace_5_finish(server, ns);
retVal |= function_mynamespace_4_finish(server, ns);
retVal |= function_mynamespace_3_finish(server, ns);
retVal |= function_mynamespace_2_finish(server, ns);
retVal |= function_mynamespace_1_finish(server, ns);
retVal |= function_mynamespace_0_finish(server, ns);
return retVal;
}
