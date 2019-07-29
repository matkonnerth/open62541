#ifndef NODESETLOADER_H
#define NODESETLOADER_H
#include <open62541/types.h>
struct UA_Server;
typedef UA_UInt16 (*addNamespaceCb)(struct UA_Server *server, const char *);

typedef struct {
    int loadTimeMs;
} Statistics;

typedef struct {
    const char *file;
    addNamespaceCb addNamespace;
    const Statistics *stat;
    void *userContext;
} FileHandler;

struct Nodeset;
UA_StatusCode
UA_Nodestore_Xml_load(const FileHandler *fileHandler);
#endif