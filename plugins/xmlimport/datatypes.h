#ifndef DATATYPES_H
#define DATAYPES_H

#include <open62541/types.h>
#include <open62541/types_generated.h>

#define alignof(type) offsetof(struct { char c; type d; }, d)

static int getAlignment(const UA_DataType* type)
{
    switch(type->typeKind)
    {
        case UA_DATATYPEKIND_BOOLEAN:
            return alignof(UA_Boolean);
        case UA_DATATYPEKIND_SBYTE:
            return alignof(UA_SByte);
        case UA_DATATYPEKIND_BYTE:
            return alignof(UA_Byte);
        case UA_DATATYPEKIND_INT16:
            return alignof(UA_Int16);
        case UA_DATATYPEKIND_UINT16:
            return alignof(UA_UInt16);
        case UA_DATATYPEKIND_INT32:
            return alignof(UA_Int32);
        case UA_DATATYPEKIND_UINT32:
            return alignof(UA_UInt32);
        case UA_DATATYPEKIND_INT64:
            return alignof(UA_Int64);
        case UA_DATATYPEKIND_UINT64:
            return alignof(UA_UInt64);
        case UA_DATATYPEKIND_FLOAT:
            return alignof(UA_Float);
        case UA_DATATYPEKIND_DOUBLE:
            return alignof(UA_Double);
        default:
            return 0;
    }
    assert(false && "typeIndex invalid");
    return 0;
}
static void setPaddingMemsize(UA_DataType* type, const UA_DataType* ns0Types, const UA_DataType* customTypes)
{
    const UA_DataType *typelists[2] = {ns0Types, customTypes};
    int offset = 0;
    // everything should be there to calculate memsize, padding, etc
    for(UA_DataTypeMember *tm = type->members;
    tm < type->members + type->membersSize;tm++)
    {
        const UA_DataType *memberType =
            &typelists[!tm->namespaceZero][tm->memberTypeIndex];
        int align = getAlignment(memberType);
        if(align>0)
        {
            tm->padding = (UA_Byte)((align - (offset % align))%align);
        }
        else
        {
            tm->padding = 0;
        }

        offset = offset + tm->padding + memberType->memSize;
    }
    type->memSize = (UA_Byte)offset;
}
#endif