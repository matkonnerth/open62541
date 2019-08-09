#ifndef DATATYPES_H
#define DATAYPES_H

#include <open62541/types.h>
#include <open62541/types_generated.h>

#define alignof(type) offsetof(struct { char c; type d; }, d)

const int SIZEOF[3] = { sizeof(int), sizeof(double), sizeof(float)};
const int ALIGNOF[3] = { alignof(int), alignof(double), alignof(float)};

static int getAlignment(size_t typeIndex)
{
    switch(typeIndex)
    {
        case UA_TYPES_BOOLEAN:
            return alignof(UA_Boolean);
        case UA_TYPES_INT16:
            return alignof(UA_Int16);
        case UA_TYPES_INT32:
            return alignof(UA_Int32);
        case UA_TYPES_INT64:
            return alignof(UA_Int64);
        case UA_TYPES_FLOAT:
            return alignof(UA_Float);
        default:
            return 0;
    }
    assert(false && "typeIndex invalid");
    return 0;
}
static void setPaddingMemsize(UA_DataType* type)
{
    int offset = 0;
    // everything should be there to calculate memsize, padding, etc
    for(UA_DataTypeMember *tm = type->members;
    tm < type->members + type->membersSize;tm++)
    {
        int align = getAlignment(tm->memberTypeIndex);
        if(align>0)
        {
            tm->padding = (UA_Byte)(offset % getAlignment(tm->memberTypeIndex));
        }
        else
        {
            tm->padding = 0;
        }

        offset = offset + tm->padding + UA_TYPES[tm->memberTypeIndex].memSize;
    }
    type->memSize = (UA_Byte)offset;
}
#endif