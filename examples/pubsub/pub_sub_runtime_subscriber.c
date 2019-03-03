/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

/**
 * IMPORTANT ANNOUNCEMENT
 * The PubSub subscriber API is currently not finished. This examples can be used to receive
 * and print the values, which are published by the tutorial_pubsub_publish example.
 * The following code uses internal API which will be later replaced by the higher-level
 * PubSub subscriber API.
*/
#include "ua_pubsub_networkmessage.h"
#include "ua_log_stdout.h"
#include "ua_server.h"
#include "ua_config_default.h"
#include "ua_pubsub.h"
#include "ua_network_pubsub_udp.h"
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include "ua_network_pubsub_ethernet.h"
#endif
#include "src_generated/ua_types_generated.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include <ua_client_highlevel.h>

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static void
subscriptionPollingCallback(UA_Server *server, UA_PubSubConnection *connection) {
    UA_ByteString buffer;
    if (UA_ByteString_allocBuffer(&buffer, 512) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Message buffer allocation failed!");
        return;
    }

    /* Receive the message. Blocks for 5ms */
    UA_StatusCode retval =
        connection->channel->receive(connection->channel, &buffer, NULL, 5);
    if(retval != UA_STATUSCODE_GOOD || buffer.length == 0) {
        /* Workaround!! Reset buffer length. Receive can set the length to zero.
         * Then the buffer is not deleted because no memory allocation is
         * assumed.
         * TODO: Return an error code in 'receive' instead of setting the buf
         * length to zero. */
        buffer.length = 512;
        UA_ByteString_clear(&buffer);
        return;
    }

    /* Decode the message */
    //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
    //            "Message length: %lu", (unsigned long) buffer.length);
    UA_NetworkMessage networkMessage;
    memset(&networkMessage, 0, sizeof(UA_NetworkMessage));
    size_t currentPosition = 0;
    UA_NetworkMessage_decodeBinary(&buffer, &currentPosition, &networkMessage);
    UA_ByteString_clear(&buffer);

    /* Is this the correct message type? */
    if(networkMessage.networkMessageType != UA_NETWORKMESSAGE_DATASET)
        goto cleanup;

    /* At least one DataSetMessage in the NetworkMessage? */
    if(networkMessage.payloadHeaderEnabled &&
       networkMessage.payloadHeader.dataSetPayloadHeader.count < 1)
        goto cleanup;

    /* Is this a KeyFrame-DataSetMessage? */
    UA_DataSetMessage *dsm = &networkMessage.payload.dataSetPayload.dataSetMessages[0];
    if(dsm->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME)
        goto cleanup;

    /* Loop over the fields and print well-known content types */
    for(int i = 0; i < dsm->data.keyFrameData.fieldCount; i++) {
        /*
       // printf("%.*s",(int)dsm->data.keyFrameData.fieldNames[i].length,dsm->data.keyFrameData.fieldNames[i].data);
        //fwrite(dsm->data.keyFrameData.fieldNames[i].data,1,dsm->data.keyFrameData.fieldNames[i].length,stdout);
        const UA_DataType *currentType = dsm->data.keyFrameData.dataSetFields[i].value.type;
        if(currentType == &UA_TYPES[UA_TYPES_BYTE]) {
            UA_Byte value = *(UA_Byte *)dsm->data.keyFrameData.dataSetFields[i].value.data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Message content: [Byte] \tReceived data: %i", value);
        } else if (currentType == &UA_TYPES[UA_TYPES_DATETIME]) {
            UA_DateTime value = *(UA_DateTime *)dsm->data.keyFrameData.dataSetFields[i].value.data;
            UA_DateTimeStruct receivedTime = UA_DateTime_toStruct(value);

            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Message content: [DateTime] \t"
                        "Received date: %02i-%02i-%02i Received time: %02i:%02i:%02i",
                        receivedTime.year, receivedTime.month, receivedTime.day,
                        receivedTime.hour, receivedTime.min, receivedTime.sec);

        } else if (currentType == &UA_TYPES[UA_TYPES_FLOAT]) {
            UA_Float value = *(UA_Float *)dsm->data.keyFrameData.dataSetFields[i].value.data;
            //fwrite(buf,1,len,stdout);

            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Message content: [Some position] \t"
                        "Received date: %f",
                        value);
        }
    */
    }

 cleanup:
    UA_NetworkMessage_clear(&networkMessage);
}

static UA_StatusCode triggerSetupCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
                                              const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId,
                                              void *objectContext, size_t inputSize, const UA_Variant *input,
                                              size_t outputSize, UA_Variant *output) {

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    /* default timeout is 5 seconds. Set it to 1 second here for demo */
    cc->timeout = 1000;

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_Client_delete(client);
        return UA_STATUSCODE_BADNOCOMMUNICATION;
    }

    //browse the pub/sub model
    //not implemented at the moment

    //read the published data
    /* Read attribute */
    printf("\nReading the value of published data:\n");
    UA_Variant val;
    UA_Variant_init(&val);

    //this reads the manually added node from pub_sub_runtime_config
    //only for showing how to read struct array
    retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(1, 8000), &val);
    if (retval == UA_STATUSCODE_GOOD) {
        printf("array length: %zu\n",    val.arrayLength);
        
        //iterate over array
        for (size_t i = 0; i < val.arrayLength; i++)
        {
            UA_ExtensionObject *eo = &((UA_ExtensionObject *)val.data)[i];
            UA_PublishedVariableDataType *publishedVariables = (UA_PublishedVariableDataType *)eo->content.decoded.data;
            printf("the attribute id is: %i\n", publishedVariables->attributeId);
        }
    }
    else
    {
        printf("cannot read the node\n");
    }

    UA_Client_delete(client);
    return UA_STATUSCODE_GOOD;
}

static void addTriggerBuildUpMethod(UA_Server *server) {   

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US", "");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US", "trigger setup");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62541), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT), UA_QUALIFIEDNAME(1, "add dateTime"),
                            helloAttr, &triggerSetupCallback, 0, NULL, 0, NULL, NULL, NULL);
}


static int
run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_minimal(4801, NULL);
    /* Details about the PubSubTransportLayer can be found inside the
     * tutorial_pubsub_connection */
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *)
        UA_calloc(2, sizeof(UA_PubSubTransportLayer));
    if (!config->pubsubTransportLayers) {
        UA_ServerConfig_delete(config);
        return EXIT_FAILURE;
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernet();
    config->pubsubTransportLayersSize++;
#endif
    UA_Server *server = UA_Server_new(config);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_NodeId connectionIdent;
    UA_StatusCode retval =
        UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    if(retval == UA_STATUSCODE_GOOD)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "The PubSub Connection was created successfully!");


    //add a method for triggering reading the subscriber config and setting up model
    addTriggerBuildUpMethod(server);

    /* The following lines register the listening on the configured multicast
     * address and configure a repeated job, which is used to handle received
     * messages. */
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, connectionIdent);
    if(connection != NULL) {
        UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
        if (rv == UA_STATUSCODE_GOOD) {
            UA_UInt64 subscriptionCallbackId;
            UA_Server_addRepeatedCallback(server, (UA_ServerCallback)subscriptionPollingCallback,
                                          connection, 100, &subscriptionCallbackId);
        } else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "register channel failed: %s!",
                           UA_StatusCode_name(rv));
        }
    }

    retval |= UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;;
}


static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}

int main(int argc, char **argv) {
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile =
                UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if (argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }
            networkAddressUrl.networkInterface = UA_STRING(argv[2]);
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else {
            printf("Error: unknown URI\n");
            return EXIT_FAILURE;
        }
    }

    return run(&transportProfile, &networkAddressUrl);
}
