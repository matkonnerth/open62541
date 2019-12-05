/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/util.h>

#include <signal.h>

#include "statemachine.h"

#ifdef _MSC_VER
#pragma warning(disable:4996) // warning C4996: 'UA_Client_Subscriptions_addMonitoredEvent': was declared deprecated
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

static UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

struct Statemachine* sm = NULL;

#ifdef UA_ENABLE_SUBSCRIPTIONS

static void
handler_events(UA_Client *client, UA_UInt32 subId, void *subContext,
               UA_UInt32 monId, void *monContext,
               size_t nEventFields, UA_Variant *eventFields) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Event received");


    /* The context should point to the monId on the stack */
    UA_assert(*(UA_UInt32*)monContext == monId);

    //ugly
    if(UA_Variant_hasScalarType(&eventFields[0], &UA_TYPES[UA_TYPES_NODEID]))
    {
        UA_NodeId automatic = UA_NODEID_NUMERIC(1, 5000);
        UA_NodeId manual = UA_NODEID_NUMERIC(1, 5001);
        if(UA_NodeId_equal(&automatic, (UA_NodeId *)eventFields[0].data)) {
            struct Message m = {IN_TRANSITION_AUTOMATIC, *(int*)UA_Client_getContext(client)};
            Statemachine_setInputEvent(sm, m);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "EventType confirm automatic");
        }
        if(UA_NodeId_equal(&manual, (UA_NodeId *)eventFields[0].data)) {
            struct Message m = {IN_TRANSITION_MANUAL,
                                *(int *)UA_Client_getContext(client)};
            Statemachine_setInputEvent(sm, m);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "EventType confirm manual");
        }
    }
}

const size_t nSelectClauses = 2;

static UA_SimpleAttributeOperand *
setupSelectClauses(void) {
    UA_SimpleAttributeOperand *selectClauses = (UA_SimpleAttributeOperand*)
        UA_Array_new(nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if(!selectClauses)
        return NULL;

    for(size_t i =0; i<nSelectClauses; ++i) {
        UA_SimpleAttributeOperand_init(&selectClauses[i]);
    }

    selectClauses[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[0].browsePathSize = 1;
    selectClauses[0].browsePath = (UA_QualifiedName*)
        UA_Array_new(selectClauses[0].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[0].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[0].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "EventType");

    selectClauses[1].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[1].browsePathSize = 1;
    selectClauses[1].browsePath = (UA_QualifiedName*)
        UA_Array_new(selectClauses[1].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[1].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[1].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");

    return selectClauses;
}

#endif

static UA_StatusCode
requestAutomaticMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                               void *sessionHandle, const UA_NodeId *methodId,
                               void *methodContext, const UA_NodeId *objectId,
                               void *objectContext, size_t inputSize,
                               const UA_Variant *input, size_t outputSize,
                               UA_Variant *output) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Request automatic from user");
    struct Message m ={IN_REQUEST_AUTOMATIC,0};
    Statemachine_setInputEvent(sm, m);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
requestManualMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                               void *sessionHandle, const UA_NodeId *methodId,
                               void *methodContext, const UA_NodeId *objectId,
                               void *objectContext, size_t inputSize,
                               const UA_Variant *input, size_t outputSize,
                               UA_Variant *output) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Request manual from user");
    struct Message m = {IN_REQUEST_MANUAL, 0};
    Statemachine_setInputEvent(sm, m);

    return UA_STATUSCODE_GOOD;
}

static void
addRequestAutomaticMethod(UA_Server *server) {
    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description = UA_LOCALIZEDTEXT("en-US", "Request_Automatic");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Request_Automatic");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(
        server, UA_NODEID_NUMERIC(1, 6001), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
        UA_QUALIFIEDNAME(1, "Request_Automatic"), generateAttr,
        &requestAutomaticMethodCallback, 0, NULL, 0, NULL, NULL, NULL);
}

static void
addRequestManualMethod(UA_Server *server) {
    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description = UA_LOCALIZEDTEXT("en-US", "Request_Manual");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Request_Manual");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(
        server, UA_NODEID_NUMERIC(1, 6002), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
        UA_QUALIFIEDNAME(1, "Request_Manual"), generateAttr,
        &requestManualMethodCallback, 0, NULL, 0, NULL, NULL, NULL);
}

static UA_StatusCode
addNewEventType(UA_Server *server) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TransitionAutomaticEventType");
    attr.description = UA_LOCALIZEDTEXT("en-US", "signals transition to automatic");
    UA_StatusCode stat = UA_Server_addObjectTypeNode(
        server, UA_NODEID_NUMERIC(1, 5000), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(0, "TransitionAutomaticEventType"), attr, NULL, NULL);

    attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TransitionManualEventType");
    attr.description = UA_LOCALIZEDTEXT("en-US", "signals transition to manual");
    stat = UA_Server_addObjectTypeNode(
        server, UA_NODEID_NUMERIC(1, 5001), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(0, "TransitionAutomaticEventType"), attr, NULL, NULL);

    return stat;
}

static UA_StatusCode
setUpEvent(UA_Server *server, UA_NodeId *outId, const UA_NodeId eventType) {
    UA_StatusCode retval = UA_Server_createEvent(server, eventType, outId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "createEvent failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }

    /* Set the Event Attributes */
    /* Setting the Time is required or else the event will not show up in UAExpert! */
    UA_DateTime eventTime = UA_DateTime_now();
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Time"),
                                         &eventTime, &UA_TYPES[UA_TYPES_DATETIME]);

    UA_UInt16 eventSeverity = 100;
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Severity"),
                                         &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);

    UA_LocalizedText eventMessage =
        UA_LOCALIZEDTEXT("en-US", "An event has been generated.");
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Message"),
                                         &eventMessage,
                                         &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    UA_String eventSourceName = UA_STRING("Server");
    UA_Server_writeObjectProperty_scalar(server, *outId,
                                         UA_QUALIFIEDNAME(0, "SourceName"),
                                         &eventSourceName, &UA_TYPES[UA_TYPES_STRING]);

    return UA_STATUSCODE_GOOD;
}

static void generateAutomaticTransitionEventfinished(UA_Server* server)
{
    UA_NodeId eventNodeId;
    UA_StatusCode retval = setUpEvent(server, &eventNodeId, UA_NODEID_NUMERIC(1, 5001));
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Creating event failed. StatusCode %s",
                       UA_StatusCode_name(retval));
    }

    retval = UA_Server_triggerEvent(server, eventNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), NULL, UA_TRUE);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Triggering event failed. StatusCode %s",
                       UA_StatusCode_name(retval));
}

static void
generateManualTransitionEventfinished(UA_Server *server) {
    UA_NodeId eventNodeId;
    UA_StatusCode retval = setUpEvent(server, &eventNodeId, UA_NODEID_NUMERIC(1, 5000));
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Creating event failed. StatusCode %s",
                       UA_StatusCode_name(retval));
    }

    retval = UA_Server_triggerEvent(server, eventNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), NULL, UA_TRUE);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Triggering event failed. StatusCode %s",
                       UA_StatusCode_name(retval));
}

int main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    Statemachine_new(&sm);
    Statemachine_addSubstatemachine(sm);
    Statemachine_addSubstatemachine(sm);

    int ids[2] ={0,1};

    if(argc < 2) {
        printf("Usage: statemachine <opc.tcp://server-url>\n");
        return EXIT_FAILURE;
    }

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setMinimal(UA_Server_getConfig(server), 4844, NULL);
    addRequestAutomaticMethod(server);
    addRequestManualMethod(server);
    addNewEventType(server);
    UA_Server_run_startup(server);

    UA_Client* clients[2];
    for(size_t i=0;i<2;i++)
    {
        UA_Client *client = UA_Client_new();
        UA_ClientConfig_setDefault(UA_Client_getConfig(client));
        clients[i] = client;
        UA_Client_getConfig(client)->clientContext = &ids[i];

        UA_StatusCode retval = UA_Client_connect(client, argv[1]);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
            return EXIT_FAILURE;
        }
        /* Create a subscription */
        UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
        UA_CreateSubscriptionResponse response =
            UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
        if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_Client_disconnect(client);
            UA_Client_delete(client);
            return EXIT_FAILURE;
        }
        UA_UInt32 subId = response.subscriptionId;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Create subscription succeeded, id %u", subId);

        /* Add a MonitoredItem */
        UA_MonitoredItemCreateRequest item;
        UA_MonitoredItemCreateRequest_init(&item);
        item.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, 2253);  // Root->Objects->Server
        item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
        item.monitoringMode = UA_MONITORINGMODE_REPORTING;

        UA_EventFilter filter;
        UA_EventFilter_init(&filter);
        filter.selectClauses = setupSelectClauses();
        filter.selectClausesSize = nSelectClauses;

        item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
        item.requestedParameters.filter.content.decoded.data = &filter;
        item.requestedParameters.filter.content.decoded.type =
            &UA_TYPES[UA_TYPES_EVENTFILTER];

        UA_UInt32 monId = 0;

        UA_MonitoredItemCreateResult result = UA_Client_MonitoredItems_createEvent(
            client, subId, UA_TIMESTAMPSTORETURN_BOTH, item, &monId, handler_events,
            NULL);

        if(result.statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Could not add the MonitoredItem with %s",
                        UA_StatusCode_name(retval));
            goto cleanup;
        } else {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Monitoring 'Root->Objects->Server', id %u",
                        response.subscriptionId);
        }

        monId = result.monitoredItemId;
    }
    
    

    


    

    while(running)
    {
        UA_Client_run_iterate(clients[0], 10);
        UA_Client_run_iterate(clients[1], 10);
        UA_Server_run_iterate(server, false);
        Statemachine_process(sm);
        struct Message m = Statemachine_getOutputEvent(sm);
        while(m.id!=OUT_REQUEST_EMPTY)
        {            
            switch(m.id)
            {
                case OUT_REQUEST_AUTOMATIC:
                    UA_Client_call_async(clients[m.subId],
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                         UA_NODEID_NUMERIC(1, 6000), 0, NULL, NULL
                                         , NULL, NULL);
                    break;
                case OUT_REQUEST_MANUAL:
                    UA_Client_call_async(
                        clients[m.subId], UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                        UA_NODEID_NUMERIC(1, 6001), 0, NULL, NULL, NULL, NULL);
                    break;
                case OUT_TRANSITION_AUTOMATIC_FINISHED:
                    generateAutomaticTransitionEventfinished(server);
                    break;
                case OUT_TRANSITION_MANUAL_FINISHED:
                    generateManualTransitionEventfinished(server);
                case OUT_REQUEST_EMPTY:
                    break;
                case UNDEFINED:
                case IN_REQUEST_MANUAL:
                case IN_REQUEST_AUTOMATIC:
                case IN_TRANSITION_AUTOMATIC:
                case IN_TRANSITION_MANUAL:
                    break;
            }
            m = Statemachine_getOutputEvent(sm);
        }
    }

    /* Delete the subscription */
 cleanup:
    //UA_MonitoredItemCreateResult_clear(&result);
    //UA_Client_Subscriptions_deleteSingle(client, response.subscriptionId);
    //UA_Array_delete(filter.selectClauses, nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);


    UA_Client_disconnect(clients[0]);
    UA_Client_disconnect(clients[1]);
    UA_Client_delete(clients[0]);
    UA_Client_delete(clients[1]);
    return EXIT_SUCCESS;
}
