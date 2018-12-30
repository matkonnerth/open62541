/* Generated from My.Types.bsd with script /home/matzy/git/open62541/tools/generate_datatypes.py
 * on host debian by user matzy at 2018-12-29 09:04:23 */

#ifndef MYTYPES_GENERATED_H_
#define MYTYPES_GENERATED_H_

#ifdef UA_ENABLE_AMALGAMATION
#include "open62541.h"
#else
#include "ua_types.h"
#include "ua_types_generated.h"

#endif

_UA_BEGIN_DECLS


/**
 * Every type is assigned an index in an array containing the type descriptions.
 * These descriptions are used during type handling (copying, deletion,
 * binary encoding, ...). */
#define UA_MYTYPES_COUNT 1
extern UA_EXPORT const UA_DataType UA_MYTYPES[UA_MYTYPES_COUNT];

/**
 * Point
 * ^^^^^
 */
typedef struct {
    UA_Int32 x;
    UA_Int32 y;
} UA_Point;

#define UA_MYTYPES_POINT 0


_UA_END_DECLS

#endif /* MYTYPES_GENERATED_H_ */
