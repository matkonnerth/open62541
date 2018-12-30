/* Generated from My.Types.bsd with script /home/matzy/git/open62541/tools/generate_datatypes.py
 * on host debian by user matzy at 2018-12-29 09:04:23 */

#ifdef UA_ENABLE_AMALGAMATION
# include "open62541.h"
#else
# include "ua_types_encoding_binary.h"
# include "mytypes_generated.h"
#endif



/* Point */
static UA_INLINE size_t
UA_Point_calcSizeBinary(const UA_Point *src) {
    return UA_calcSizeBinary(src, &UA_MYTYPES[UA_MYTYPES_POINT]);
}
static UA_INLINE UA_StatusCode
UA_Point_encodeBinary(const UA_Point *src, UA_Byte **bufPos, const UA_Byte *bufEnd) {
    return UA_encodeBinary(src, &UA_MYTYPES[UA_MYTYPES_POINT], bufPos, &bufEnd, NULL, NULL);
}
static UA_INLINE UA_StatusCode
UA_Point_decodeBinary(const UA_ByteString *src, size_t *offset, UA_Point *dst) {
    return UA_decodeBinary(src, offset, dst, &UA_MYTYPES[UA_MYTYPES_POINT], NULL);
}
