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
#include "ua_config_default.h"
#include "ua_log_stdout.h"
#include "ua_network_pubsub_udp.h"
#include "ua_pubsub.h"
#include "ua_pubsub_networkmessage.h"
#include "ua_server.h"
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include "ua_network_pubsub_ethernet.h"
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <ua_client_highlevel.h>

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static void subscriptionPollingCallback(UA_Server *server, UA_PubSubConnection *connection) {
    UA_ByteString buffer;
    if (UA_ByteString_allocBuffer(&buffer, 512) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Message buffer allocation failed!");
        return;
    }

    /* Receive the message. Blocks for 5ms */
    UA_StatusCode retval = connection->channel->receive(connection->channel, &buffer, NULL, 5);
    if (retval != UA_STATUSCODE_GOOD || buffer.length == 0) {
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
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
    //            "Message length: %lu", (unsigned long) buffer.length);
    UA_NetworkMessage networkMessage;
    memset(&networkMessage, 0, sizeof(UA_NetworkMessage));
    size_t currentPosition = 0;
    UA_NetworkMessage_decodeBinary(&buffer, &currentPosition, &networkMessage);
    UA_ByteString_clear(&buffer);

    /* Is this the correct message type? */
    if (networkMessage.networkMessageType != UA_NETWORKMESSAGE_DATASET)
        goto cleanup;

    /* At least one DataSetMessage in the NetworkMessage? */
    if (networkMessage.payloadHeaderEnabled && networkMessage.payloadHeader.dataSetPayloadHeader.count < 1)
        goto cleanup;

    /* Is this a KeyFrame-DataSetMessage? */
    UA_DataSetMessage *dsm = &networkMessage.payload.dataSetPayload.dataSetMessages[0];
    if (dsm->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME)
        goto cleanup;

    //UA_Float *value = (UA_Float *)dsm->data.keyFrameData.dataSetFields[0].value.data;

    //for (size_t i = 0; i < dsm->data.keyFrameData.dataSetFields[0].value.arrayLength; i++) {
    //    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
    //                        "Message content: [Some position] \t"
    //                        "Received date: %f",
    //                        value[i]);
    //}

    UA_UInt16 writeIdCount = 4000;
    
    //TODO: look for different data and change the data only when they are changed -> deltaFrameData ??
    for (size_t count = 0; count < dsm->data.keyFrameData.fieldCount; count++) {
        retval = UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, writeIdCount++), dsm->data.keyFrameData.dataSetFields[count].value);
        if (retval != UA_STATUSCODE_GOOD) {
            printf("Server not able to write value with nodeId %d\n", writeIdCount);
        }
    }

    //TODO: react on a change of the data structure in the publisher side

    /* Loop over the fields and print well-known content types */
    //for (int i = 0; i < dsm->data.keyFrameData.fieldCount; i++) {
    //    
    //    //printf("%.*s",(int)dsm->data.keyFrameData.fieldNames[i].length,dsm->data.keyFrameData.fieldNames[i].data);
    //    //fwrite(dsm->data.keyFrameData.fieldNames[i].data,1,dsm->data.keyFrameData.fieldNames[i].length,stdout);
    //    const UA_DataType *currentType = dsm->data.keyFrameData.dataSetFields[i].value.type;
    //    if(currentType == &UA_TYPES[UA_TYPES_BYTE]) {
    //        UA_Byte value = *(UA_Byte *)dsm->data.keyFrameData.dataSetFields[i].value.data;
    //        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
    //                    "Message content: [Byte] \tReceived data: %i", value);
    //    } else if (currentType == &UA_TYPES[UA_TYPES_DATETIME]) {
    //        UA_DateTime value = *(UA_DateTime *)dsm->data.keyFrameData.dataSetFields[i].value.data;
    //        UA_DateTimeStruct receivedTime = UA_DateTime_toStruct(value);

    //        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
    //                    "Message content: [DateTime] \t"
    //                    "Received date: %02i-%02i-%02i Received time: %02i:%02i:%02i",
    //                    receivedTime.year, receivedTime.month, receivedTime.day,
    //                    receivedTime.hour, receivedTime.min, receivedTime.sec);

    //    } else if (currentType == &UA_TYPES[UA_TYPES_FLOAT]) {
    //        UA_Float value = *(UA_Float *)dsm->data.keyFrameData.dataSetFields[i].value.data;
    //        //fwrite(buf,1,len,stdout);

    //        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
    //                    "Message content: [Some position] \t"
    //                    "Received date: %f",
    //                    value);
    //    }
    //
    //}

cleanup:
    UA_NetworkMessage_clear(&networkMessage);
}

#define pathSize 1
#define startIdCount 4000
static UA_UInt16 idCount = startIdCount;
static UA_NodeId connectionIdent;
static UA_Boolean subscriptionPollingCallbackConfigured = UA_FALSE;

//Delete all nodes added by setup
static void deleteNodesAddedBySetupCallback(UA_Server *server) {
    if (subscriptionPollingCallbackConfigured) {
        idCount--;
        UA_StatusCode retval;

        for (size_t i = idCount; i >= startIdCount; i--) {
            retval = UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, i), UA_TRUE);
            if (retval != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Delete nodes added by setup callback failed: %s\n",
                             UA_StatusCode_name(retval));
            }
        }

        idCount = startIdCount;
    }
}
 
//Create a client to read the data from the publisher
static UA_StatusCode triggerSetupCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
                                          const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId,
                                          void *objectContext, size_t inputSize, const UA_Variant *input,
                                          size_t outputSize, UA_Variant *output) {

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    /* default timeout is 5 seconds. Set it to 1 second here for demo */
    cc->timeout = 1000;

#define lenNetworkadress 50

    UA_String *networkadressPubServer = (UA_String *)input->data;
    char networkadressClientConnect[lenNetworkadress];

    snprintf(networkadressClientConnect, lenNetworkadress, "opc.tcp://%s:4843", (char *)networkadressPubServer->data);

    UA_StatusCode retval = UA_Client_connect(client, networkadressClientConnect);  //take localhost for test internal -> IP 192.168.120.2 for CP test make this configurable
    if (retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return UA_STATUSCODE_BADNOCOMMUNICATION;
    }

    // browse the pub/sub model
    // not implemented at the moment
    /* Browse the publishedDataSets -> pub/sub model find the published data */
    deleteNodesAddedBySetupCallback(server);

    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS); /* browse objects folder */
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;            /* return everything */
    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    //printf("%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");
    for (size_t i = 0; i < bResp.resultsSize; ++i) {
        for (size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
            UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
            if (ref->typeDefinition.nodeId.identifier.numeric == UA_NS0ID_PUBLISHEDDATAITEMSTYPE) {
                //tempNodeIdPubDataSets = ref->nodeId.nodeId.identifier.numeric;
                
                //printf("%-9d %-16zu %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex, tempNodeIdPubDataSets,
                //      (int)ref->browseName.name.length, ref->browseName.name.data,
                //      (int)ref->displayName.text.length, ref->displayName.text.data);

                char path[] = "PublishedData";
                
                UA_BrowsePath browsePath;
                UA_BrowsePath_init(&browsePath);
                browsePath.startingNode = ref->nodeId.nodeId;
                browsePath.relativePath.elements = (UA_RelativePathElement *)UA_Array_new(pathSize, &UA_TYPES[UA_TYPES_RELATIVEPATHELEMENT]);
                browsePath.relativePath.elementsSize = pathSize;
                browsePath.relativePath.elements->referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
                browsePath.relativePath.elements->targetName = UA_QUALIFIEDNAME_ALLOC(0, path);
                
                UA_TranslateBrowsePathsToNodeIdsRequest request;
                UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
                request.browsePaths = &browsePath;
                request.browsePathsSize = 1;

                UA_TranslateBrowsePathsToNodeIdsResponse response =
                    UA_Client_Service_translateBrowsePathsToNodeIds(client, request);

                //printf("Found NodeId: %zu\n", response.results[0].targets[0].targetId.nodeId.identifier.numeric);
                
                printf("\nReading the value of published data with correct nodeId:\n");
                UA_Variant valPubData;
                UA_Variant_init(&valPubData);

                retval = UA_Client_readValueAttribute(client, response.results[0].targets[0].targetId.nodeId , &valPubData);
                if (retval == UA_STATUSCODE_GOOD) {
                    printf("array length: %zu\n-------------------------------------------------\n", valPubData.arrayLength);

                    // iterate over array
                    for (size_t k = 0; k < valPubData.arrayLength; k++) {
                        UA_ExtensionObject *eoData = &((UA_ExtensionObject *)valPubData.data)[k];
                        UA_PublishedVariableDataType *publishedVariablesData =
                            (UA_PublishedVariableDataType *)eoData->content.decoded.data;
                        printf("the node id is: %zu\n", publishedVariablesData->publishedVariable.identifier.numeric);
                        printf("the node namespace is: %zu\n", publishedVariablesData->publishedVariable.namespaceIndex);
                        //printf("the attribute id is: %i\n", publishedVariablesData->attributeId);
                        
                        // Get the data type from the node to add the node
                        UA_NodeId getDataType;  //NodeId for the add Node data type

                        retval = UA_Client_readDataTypeAttribute(client, publishedVariablesData->publishedVariable,
                                                                 &getDataType);
                        if (retval == UA_STATUSCODE_GOOD) {
                            printf("Data type name: %s\n", UA_findDataType(&getDataType)->typeName);
                        } else {
                            printf("cannot read the data type\n");
                            //return UA_STATUSCODE_BADNOTFOUND;
                        }

                        // Get the browse name from the node to add the node
                        UA_QualifiedName getBrowseName;

                        retval = UA_Client_readBrowseNameAttribute(client, publishedVariablesData->publishedVariable,
                                                                 &getBrowseName);
                        if (retval == UA_STATUSCODE_GOOD) {
                            printf("Browse name: %s\n", getBrowseName.name.data);
                        } else {
                            printf("cannot read the browse name\n");
                            //return UA_STATUSCODE_BADNOTFOUND;
                        }

                        // Get the display name from the node to add the node
                        UA_LocalizedText getDisplayName;

                        retval = UA_Client_readDisplayNameAttribute(client, publishedVariablesData->publishedVariable,
                                                                    &getDisplayName);
                        if (retval == UA_STATUSCODE_GOOD) {
                            printf("Display name: %s\n", getDisplayName.text.data);
                        } else {
                            printf("cannot read the display name\n");
                            //return UA_STATUSCODE_BADNOTFOUND;
                        }

                         // Get the description from the node to add the node
                        UA_LocalizedText getDescription;

                        retval = UA_Client_readDescriptionAttribute(client, publishedVariablesData->publishedVariable,
                                                                    &getDescription);
                        if (retval == UA_STATUSCODE_GOOD) {
                            if (getDescription.text.length != 0) {
                                printf("Description: %s\n", getDescription.text.data);
                            } else {
                                getDescription = UA_LOCALIZEDTEXT("en-US", "");
                            }                            
                        } else {
                            printf("cannot read the desciption\n");
                            //return UA_STATUSCODE_BADNOTFOUND;
                        }

                        // TODO: read and check hastypedefinition to support also other types
                        // Add the current node to the server
                        UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
                        UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
                        UA_VariableAttributes attr = UA_VariableAttributes_default;
                        attr.description = getDescription;
                        attr.displayName = getDisplayName;
                        attr.dataType = getDataType;
                        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

                        retval = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, idCount++), parentNodeId,
                                                           parentReferenceNodeId, getBrowseName,
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);

                        if (retval != UA_STATUSCODE_GOOD) {
                            printf("Server not able to add Variable");
                        }

                        printf("-------------------------------------------------\n\n");
                    }
                } else {
                    printf("cannot read the node\n");
                    //return UA_STATUSCODE_BADNOTFOUND;
                }
                
                UA_BrowsePath_deleteMembers(&browsePath);
                UA_TranslateBrowsePathsToNodeIdsResponse_deleteMembers(&response);
            }
        }
    }
    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);

    // read the published data
    /* Read attribute */
    printf("\nReading the value of published data:\n");
    UA_Variant val;
    UA_Variant_init(&val);

    // this reads the manually added node from pub_sub_runtime_config
    // only for showing how to read struct array
    retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(1, 8000), &val);
    if (retval == UA_STATUSCODE_GOOD) {
        printf("array length: %zu\n", val.arrayLength);

        // iterate over array
        for (size_t i = 0; i < val.arrayLength; i++) {
            UA_ExtensionObject *eo = &((UA_ExtensionObject *)val.data)[i];
            UA_PublishedVariableDataType *publishedVariables = (UA_PublishedVariableDataType *)eo->content.decoded.data;
            printf("the attribute id is: %i\n", publishedVariables->attributeId);
        }
    } else {
        printf("cannot read the node\n");
    }

    /* The following lines register the listening on the configured multicast
     * address and configure a repeated job, which is used to handle received
     * messages. */
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdent);
    if (connection != NULL && !subscriptionPollingCallbackConfigured) {
        UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
        if (rv == UA_STATUSCODE_GOOD) {
            UA_UInt64 subscriptionCallbackId = 0;
            UA_Server_addRepeatedCallback(server, (UA_ServerCallback)subscriptionPollingCallback, connection, 100,
                                          &subscriptionCallbackId);
            if (subscriptionCallbackId != 0) {
                subscriptionPollingCallbackConfigured = UA_TRUE;
            }
        } else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "register channel failed: %s!",
                           UA_StatusCode_name(rv));
        }
    }

    UA_Client_delete(client);
    return UA_STATUSCODE_GOOD;
}


static void addTriggerBuildUpMethod(UA_Server *server) {

    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "Networkadress Pub server");
    inputArgument.name = UA_STRING("Networkadress Pub server");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US", "");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US", "trigger setup");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62541), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT), UA_QUALIFIEDNAME(1, "add dateTime"),
                            helloAttr, &triggerSetupCallback, 1, &inputArgument, 0, NULL, NULL, NULL);
}

static int run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_minimal(4844, NULL);
    /* Details about the PubSubTransportLayer can be found inside the
     * tutorial_pubsub_connection */
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *)UA_calloc(2, sizeof(UA_PubSubTransportLayer));
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
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    //UA_NodeId connectionIdent;
    UA_StatusCode retval = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    if (retval == UA_STATUSCODE_GOOD)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The PubSub Connection was created successfully!");

    // add a method for triggering reading the subscriber config and setting up model
    addTriggerBuildUpMethod(server);

    /* The following lines register the listening on the configured multicast
     * address and configure a repeated job, which is used to handle received
     * messages. */
    //UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdent);
    //if (connection != NULL) {
    //    UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
    //    if (rv == UA_STATUSCODE_GOOD) {
    //        UA_UInt64 subscriptionCallbackId;
    //        UA_Server_addRepeatedCallback(server, (UA_ServerCallback)subscriptionPollingCallback, connection, 100,
    //                                      &subscriptionCallbackId);
    //    } else {
    //        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "register channel failed: %s!",
    //                       UA_StatusCode_name(rv));
    //    }
    //}

    retval |= UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
    ;
}

static void usage(char *progname) { printf("usage: %s <uri> [device]\n", progname); }

int main(int argc, char **argv) {
    UA_String transportProfile = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING("192.168.120.1"), UA_STRING("opc.udp://224.0.0.1:4840/")};     //interface normal UA_STRING_NULL internal

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
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
