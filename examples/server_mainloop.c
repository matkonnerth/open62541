/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <ua_server.h>
#include <ua_config_default.h>
#include <ua_log_stdout.h>

#include <signal.h>
#include <stdlib.h>

#include "statemachine.h"

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static UA_StatusCode requestRunningMethodCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
                                                  const UA_NodeId *methodId, void *methodContext,
                                                  const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                                                  const UA_Variant *input, size_t oucall_stopped_to_runningtputSize,
                                                  UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Request State Running was called");
    if (call_from_to(STOPPED, RUNNING) == true) {
        return UA_STATUSCODE_GOOD;
    } 
    else 
    {
        return UA_STATUSCODE_BADSTATENOTACTIVE;
    }
}

static UA_StatusCode requestStoppMethodCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
                                                  const UA_NodeId *methodId, void *methodContext,
                                                  const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                                                  const UA_Variant *input, size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Request State Stopped was called");
    if (call_from_to(RUNNING, STOPPED) == true) {
        return UA_STATUSCODE_GOOD;
    } else {
        return UA_STATUSCODE_BADSTATENOTACTIVE;
    }
}

static UA_StatusCode readCurrentState(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                                     const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
                                     const UA_NumericRange *range, UA_DataValue *dataValue) {
    UA_Int32 currentState = getCurrentState();
    UA_Variant_setScalarCopy(&dataValue->value, &currentState, &UA_TYPES[UA_TYPES_INT32]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writeCurrentState(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                                      const UA_NodeId *nodeId, void *nodeContext, const UA_NumericRange *range,
                                      const UA_DataValue *data) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Changing state with write not possible");
    return UA_STATUSCODE_BADINTERNALERROR;
}

static void addRequestRunningMethodNode(UA_Server *server) {

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US", "Request Running State Method");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Request Running State Method");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62541), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT), UA_QUALIFIEDNAME(1, "hello world"),
                            helloAttr, &requestRunningMethodCallback, 0, NULL, 0, NULL, NULL, NULL);
}

static void addRequestStoppMethodNode(UA_Server *server) {

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US", "Request Stopp State Method");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Request Stopp State Method");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62542), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT), UA_QUALIFIEDNAME(1, "request stopp"),
                            helloAttr, &requestStoppMethodCallback, 0, NULL, 0, NULL, NULL, NULL);
}

static void addCurrentStateVariable(UA_Server *server) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "current state");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "current-state");
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, "current-state");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource currentStateSource;
    currentStateSource.read = readCurrentState;
    currentStateSource.write = writeCurrentState;
    UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId, parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr, currentStateSource, NULL, NULL);
}

/* In this example, we integrate the server into an external "mainloop". This
   can be for example the event-loop used in GUI toolkits, such as Qt or GTK. */

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    /* Should the server networklayer block (with a timeout) until a message
       arrives or should it return immediately? */
    UA_Boolean waitInternal = false;

    addRequestRunningMethodNode(server);
    addRequestStoppMethodNode(server);
    addCurrentStateVariable(server);

    UA_StatusCode retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    while(running) {
        /* timeout is the maximum possible delay (in millisec) until the next
           _iterate call. Otherwise, the server might miss an internal timeout
           or cannot react to messages with the promised responsiveness. */
        /* If multicast discovery server is enabled, the timeout does not not consider new input data (requests) on the mDNS socket.
         * It will be handled on the next call, which may be too late for requesting clients.
         * if needed, the select with timeout on the multicast socket server->mdnsSocket (see example in mdnsd library)
         */

        run();
        UA_UInt16 timeout = UA_Server_run_iterate(server, waitInternal);

        /* Now we can use the max timeout to do something else. In this case, we
           just sleep. (select is used as a platform-independent sleep
           function.) */
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = timeout * 1000;
        select(0, NULL, NULL, NULL, &tv);
    }
    retval = UA_Server_run_shutdown(server);

 cleanup:
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
