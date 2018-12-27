/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Client disconnect handling
 * --------------------------
 * This example shows you how to handle a client disconnect, e.g., if the server
 * is shut down while the client is connected. You just need to call connect
 * again and the client will automatically reconnect.
 *
 * This example is very similar to the tutorial_client_firststeps.c. */

#include <ua_config_default.h>
#include <ua_client_subscriptions.h>
#include <ua_log_stdout.h>

#include <signal.h>

UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received Ctrl-C");
    running = 0;
}

static void
handler_currentTimeChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
                           UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "currentTime has changed!");
    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime *) value->value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "date is: %02u-%02u-%04u %02u:%02u:%02u.%03u",
                    dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    }
}

static void
handler_MonitoredItemChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
                         UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    printf("monitored item callback\n");
}

static void
deleteSubscriptionCallback(UA_Client *client, UA_UInt32 subscriptionId, void *subscriptionContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Subscription Id %u was deleted", subscriptionId);
}

static void
subscriptionInactivityCallback (UA_Client *client, UA_UInt32 subId, void *subContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Inactivity for subscription %u", subId);
}

static void
stateCallback (UA_Client *client, UA_ClientState clientState) {
    switch(clientState) {
        case UA_CLIENTSTATE_DISCONNECTED:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "The client is disconnected");
        break;
        case UA_CLIENTSTATE_WAITING_FOR_ACK:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Waiting for ack");
        break;
        case UA_CLIENTSTATE_CONNECTED:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "A TCP connection to the server is open");
        break;
        case UA_CLIENTSTATE_SECURECHANNEL:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "A SecureChannel to the server is open");
        break;
        case UA_CLIENTSTATE_SESSION:{
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "A session with the server is open");
            /* A new session was created. We need to create the subscription. */
            /* Create a subscription */
            UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
            UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                                    NULL, NULL, deleteSubscriptionCallback);

            if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                            "Create subscription succeeded, id %u", response.subscriptionId);
            else
                return;

            printf("Browsing nodes in objects folder:\n");
            UA_BrowseRequest bReq;
            UA_BrowseRequest_init(&bReq);
            bReq.requestedMaxReferencesPerNode = 0;
            bReq.nodesToBrowse = UA_BrowseDescription_new();
            bReq.nodesToBrowseSize = 1;
            bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); /* browse objects folder */
            bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */
            bReq.nodesToBrowse[0].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
            bReq.nodesToBrowse[0].includeSubtypes = true;
            UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
            printf("%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");
            for(size_t i = 0; i < bResp.resultsSize; ++i) {
                for(size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
                    UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
                    if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
                        printf("%-9d %-16d %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
                            ref->nodeId.nodeId.identifier.numeric, (int)ref->browseName.name.length,
                            ref->browseName.name.data, (int)ref->displayName.text.length,
                            ref->displayName.text.data);
                    } else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
                        printf("%-9d %-16.*s %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
                            (int)ref->nodeId.nodeId.identifier.string.length,
                            ref->nodeId.nodeId.identifier.string.data,
                            (int)ref->browseName.name.length, ref->browseName.name.data,
                            (int)ref->displayName.text.length, ref->displayName.text.data);
                    }

                    //create monitored item
                    for(int cnt=0; cnt<10; cnt++)
                    {
                        UA_MonitoredItemCreateRequest monRequest =
                            UA_MonitoredItemCreateRequest_default(ref->nodeId.nodeId);
                            monRequest.requestedParameters.samplingInterval = 100;

                            UA_MonitoredItemCreateResult monResponse =
                            UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                                    UA_TIMESTAMPSTORETURN_BOTH,
                                                                    monRequest, NULL, handler_MonitoredItemChanged, NULL);
                            if(monResponse.statusCode == UA_STATUSCODE_GOOD)
                                printf("Monitoring 'the.answer', id %u\n", monResponse.monitoredItemId);

                    }
                    


                    /* TODO: distinguish further types */
                }
            }
    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);
            

            /* Add a MonitoredItem */
            UA_MonitoredItemCreateRequest monRequest =
                UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));

            UA_MonitoredItemCreateResult monResponse =
                UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                          UA_TIMESTAMPSTORETURN_BOTH,
                                                          monRequest, NULL, handler_currentTimeChanged, NULL);
            if(monResponse.statusCode == UA_STATUSCODE_GOOD)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                            "Monitoring UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME', id %u",
                            monResponse.monitoredItemId);
        }
        break;
        case UA_CLIENTSTATE_SESSION_RENEWED:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "A session with the server is open (renewed)");
            /* The session was renewed. We don't need to recreate the subscription. */
        break;
        case UA_CLIENTSTATE_SESSION_DISCONNECTED:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Session disconnected");
        break;
    }
    return;
}

int
main(void) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ClientConfig config = UA_ClientConfig_default;
    /* Set stateCallback */
    config.stateCallback = stateCallback;
    config.subscriptionInactivityCallback = subscriptionInactivityCallback;

    UA_Client *client = UA_Client_new(config);    

    /* Endless loop runAsync */
    while(running) {
        /* if already connected, this will return GOOD and do nothing */
        /* if the connection is closed/errored, the connection will be reset and then reconnected */
        /* Alternatively you can also use UA_Client_getState to get the current state */
        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Not connected. Retrying to connect in 1 second");
            /* The connect may timeout after 1 second (see above) or it may fail immediately on network errors */
            /* E.g. name resolution errors or unreachable network. Thus there should be a small sleep here */
            UA_sleep_ms(1000);
            continue;
        }        
        
        UA_Client_run_iterate(client, 1000);
    };

    /* Clean up */
    UA_Client_delete(client); /* Disconnects the client internally */
    return UA_STATUSCODE_GOOD;
}
