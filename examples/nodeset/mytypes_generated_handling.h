/* Generated from My.Types.bsd with script /home/matzy/git/open62541/tools/generate_datatypes.py
 * on host debian by user matzy at 2018-12-29 09:04:23 */

#ifndef MYTYPES_GENERATED_HANDLING_H_
#define MYTYPES_GENERATED_HANDLING_H_

#include "mytypes_generated.h"

_UA_BEGIN_DECLS

#if defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmissing-field-initializers"
# pragma GCC diagnostic ignored "-Wmissing-braces"
#endif


/* Point */
static UA_INLINE void
UA_Point_init(UA_Point *p) {
    memset(p, 0, sizeof(UA_Point));
}

static UA_INLINE UA_Point *
UA_Point_new(void) {
    return (UA_Point*)UA_new(&UA_MYTYPES[UA_MYTYPES_POINT]);
}

static UA_INLINE UA_StatusCode
UA_Point_copy(const UA_Point *src, UA_Point *dst) {
    *dst = *src;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE void
UA_Point_deleteMembers(UA_Point *p) {
    memset(p, 0, sizeof(UA_Point));
}

static UA_INLINE void
UA_Point_clear(UA_Point *p) {
    memset(p, 0, sizeof(UA_Point));
}

static UA_INLINE void
UA_Point_delete(UA_Point *p) {
    UA_delete(p, &UA_MYTYPES[UA_MYTYPES_POINT]);
}

#if defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
# pragma GCC diagnostic pop
#endif

_UA_END_DECLS

#endif /* MYTYPES_GENERATED_HANDLING_H_ */
