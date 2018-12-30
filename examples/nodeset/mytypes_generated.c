/* Generated from My.Types.bsd with script /home/matzy/git/open62541/tools/generate_datatypes.py
 * on host debian by user matzy at 2018-12-29 09:04:23 */

#include "mytypes_generated.h"

/* Point */
static UA_DataTypeMember Point_members[2] = {
{
    UA_TYPENAME("X") /* .memberName */
    UA_TYPES_INT32, /* .memberTypeIndex */
    0, /* .padding */
    true, /* .namespaceZero */
    false /* .isArray */
},
{
    UA_TYPENAME("Y") /* .memberName */
    UA_TYPES_INT32, /* .memberTypeIndex */
    offsetof(UA_Point, y) - offsetof(UA_Point, x) - sizeof(UA_Int32), /* .padding */
    true, /* .namespaceZero */
    false /* .isArray */
},};
const UA_DataType UA_MYTYPES[UA_MYTYPES_COUNT] = {
/* Point */
{
    UA_TYPENAME("Point") /* .typeName */
    {2, UA_NODEIDTYPE_NUMERIC, {10001}}, /* .typeId */
    sizeof(UA_Point), /* .memSize */
    UA_MYTYPES_POINT, /* .typeIndex */
    2, /* .membersSize */
    false, /* .builtin */
    true, /* .pointerFree */
    true
		 && UA_BINARY_OVERLAYABLE_INTEGER
		 && UA_BINARY_OVERLAYABLE_INTEGER
		 && offsetof(UA_Point, y) == (offsetof(UA_Point, x) + sizeof(UA_Int32)), /* .overlayable */
    0, /* .binaryEncodingId */
    Point_members /* .members */
},
};

