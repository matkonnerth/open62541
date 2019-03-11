/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * .. _pubsub-tutorial:
 *
 * Working with Publish/Subscribe
 * ------------------------------
 *
 * Work in progress: This Tutorial will be continuously extended during the next
 * PubSub batches. More details about the PubSub extension and corresponding
 * open62541 API are located here: :ref:`pubsub`.
 *
 * Publishing Fields
 * ^^^^^^^^^^^^^^^^^
 * The PubSub publish example demonstrate the simplest way to publish
 * informations from the information model over UDP multicast using the UADP
 * encoding.
 *
 * **Connection handling**
 *
 * PubSubConnections can be created and deleted on runtime. More details about
 * the system preconfiguration and connection can be found in
 * ``tutorial_pubsub_connection.c``.
 */

#include <ua_config_default.h>
#include <ua_log_stdout.h>
#include <ua_network_pubsub_ethernet.h>
#include <ua_network_pubsub_udp.h>
#include <ua_server.h>

#include <signal.h>

#include <ua_client_highlevel.h>

UA_Float actPos = 100.0;

UA_UInt32 idCount = 4000;

UA_NodeId connectionIdent, publishedDataSetIdent, writerGroupIdent;

static void addFloatVariable(UA_Server *server, char *name) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Float myInteger = 0.0;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_FLOAT]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, idCount++), parentNodeId, parentReferenceNodeId,
                              myIntegerName, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

// Update a float variable with a defined nodeId
static void updateFloatVariable(UA_Server *server, UA_NodeId *nodeId, UA_Float *newValue) {

    UA_Variant value;
    UA_Variant_setScalarCopy(&value, newValue, &UA_TYPES[UA_TYPES_FLOAT]);

    UA_StatusCode retval;
    retval = UA_Server_writeValue(server, *nodeId, value);

    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Update float variable failed: %s\n",
                     UA_StatusCode_name(retval));
    }
}

static void addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                                UA_NetworkAddressUrlDataType *networkAddressUrl) {
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

/**
 * **PublishedDataSet handling**
 *
 * The PublishedDataSet (PDS) and PubSubConnection are the toplevel entities and
 * can exist alone. The PDS contains the collection of the published fields. All
 * other PubSub elements are directly or indirectly linked with the PDS or
 * connection. */
static void addPublishedDataSet(UA_Server *server) {
    /* The PublishedDataSetConfig contains all necessary public
     * informations for the creation of a new PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
}

/**
 * **DataSetField handling**
 *
 * The DataSetField (DSF) is part of the PDS and describes exactly one published
 * field. */
static void addDataSetField(UA_Server *server) {
    /* Add a field to the previous created PublishedDataSet */
    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldConfig, &dataSetFieldIdent);
}

static void addDataSetFieldVariable(UA_Server *server, UA_NodeId id) {
    /* Add a field to the previous created PublishedDataSet */
    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Mould Position");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable = id;
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldConfig, &dataSetFieldIdent);
}

/**
 * **WriterGroup handling**
 *
 * The WriterGroup (WG) is part of the connection and contains the primary
 * configuration parameters for the message creation. */
static void addWriterGroup(UA_Server *server) {
    /* Now we create a new WriterGroupConfig and add the group to the existing
     * PubSubConnection. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    /* The configuration flags for the messages are encapsulated inside the
     * message- and transport settings extension objects. These extension
     * objects are defined by the standard. e.g.
     * UadpWriterGroupMessageDataType */
    UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig, &writerGroupIdent);
}

/**
 * **DataSetWriter handling**
 *
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional informations for the
 * message generation. */
static void addDataSetWriter(UA_Server *server) {
    /* We need now a DataSetWriter within the WriterGroup. This means we must
     * create a new DataSetWriterConfig and add call the addWriterGroup function. */
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 10;
    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig,
                               &dataSetWriterIdent);
}

/**
 * That's it! You're now publishing the selected fields. Open a packet
 * inspection tool of trust e.g. wireshark and take a look on the outgoing
 * packages. The following graphic figures out the packages created by this
 * tutorial.
 *
 * .. figure:: ua-wireshark-pubsub.png
 *     :figwidth: 100 %
 *     :alt: OPC UA PubSub communication in wireshark
 *
 * The open62541 subscriber API will be released later. If you want to process
 * the the datagrams, take a look on the ua_network_pubsub_networkmessage.c
 * which already contains the decoding code for UADP messages.
 *
 * It follows the main server code, making use of the above definitions. */
UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

// Data structure test data
typedef struct {
    UA_String name;
    const UA_DataType *type;
    size_t arrayLength;
    void *data;
} tTestData;

//Add given test data to the server and add them to the published data
static void addTestData(UA_Server *server, tTestData *testData, size_t const lenTestData) {
    // Add an object node where the test data are placed
    UA_NodeId testDataId;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Test Data");
    UA_Server_addObjectNode(server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Test Data"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &testDataId);

    // Add the test data in the constructed object
    UA_VariableAttributes attr;
    UA_QualifiedName testName;
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId currentAddedNodeId;
    UA_StatusCode retval;

    for (size_t i = 0; i < lenTestData; i++) {
        if (testData[i].arrayLength == 0) {
            attr = UA_VariableAttributes_default;
            UA_Variant_setScalar(&attr.value, testData[i].data, testData[i].type);
            attr.displayName = UA_LOCALIZEDTEXT("en-US", (char *)testData[i].name.data);
            attr.dataType = testData[i].type->typeId;
            attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

            testName = UA_QUALIFIEDNAME(1, (char *)testData[i].name.data);

            retval = UA_Server_addVariableNode(
                server, UA_NODEID_NUMERIC(1, idCount++), testDataId, parentReferenceNodeId, testName,
                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, &currentAddedNodeId);
            if (retval != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Add test data \"%s\" failed",
                             testData[i].name.data);
            } else {
                // Add the test data to the published data
                addDataSetFieldVariable(server, currentAddedNodeId);
            }
        }
    }
}

static UA_StatusCode helloWorldMethodCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
                                              const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId,
                                              void *objectContext, size_t inputSize, const UA_Variant *input,
                                              size_t outputSize, UA_Variant *output) {

    UA_NodeId *id = (UA_NodeId *)input->data;
    addDataSetFieldVariable(server, *id);
    return UA_STATUSCODE_GOOD;
}

static void addDataSetFielddMethod(UA_Server *server) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    inputArgument.name = UA_STRING("MyInput");
    inputArgument.dataType = UA_TYPES[UA_TYPES_NODEID].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US", "");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US", "add Variable to Published Dataset");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62541), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT), UA_QUALIFIEDNAME(1, "add dateTime"),
                            helloAttr, &helloWorldMethodCallback, 1, &inputArgument, 0, NULL, NULL, NULL);
}

static UA_StatusCode addFloatVariableMethodCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
                                                    const UA_NodeId *methodId, void *methodContext,
                                                    const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                                                    const UA_Variant *input, size_t outputSize, UA_Variant *output) {

    UA_String *inputStr = (UA_String *)input->data;
    char *name = (char *)calloc(inputStr->length + 1, sizeof(char));
    memcpy(name, inputStr->data, inputStr->length);
    addFloatVariable(server, name);

    return UA_STATUSCODE_GOOD;
}

static void addFloatVariableMethod(UA_Server *server) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    inputArgument.name = UA_STRING("name of variable");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US", "Say `Hello World`");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US", "addFloatVariable");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62542), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT), UA_QUALIFIEDNAME(1, "addFloatVariable"),
                            helloAttr, &addFloatVariableMethodCallback, 1, &inputArgument, 0, NULL, NULL, NULL);
}

static UA_StatusCode updateFloatVariableMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                                                       void *sessionHandle, const UA_NodeId *methodId,
                                                       void *methodContext, const UA_NodeId *objectId,
                                                       void *objectContext, size_t inputSize, const UA_Variant *input,
                                                       size_t outputSize, UA_Variant *output) {

    UA_NodeId *inputUpdateId = (UA_NodeId *)input[0].data;
    UA_Float *inputTestVal = (UA_Float *)input[1].data;

    updateFloatVariable(server, inputUpdateId, inputTestVal);

    return UA_STATUSCODE_GOOD;
}

static void updateFloatVariableMethod(UA_Server *server) {

#define lenInputArgs 2

    UA_Argument inputArguments[lenInputArgs];
    UA_Argument_init(&inputArguments[0]);
    inputArguments[0].description = UA_LOCALIZEDTEXT("en-US", "NodeId update");
    inputArguments[0].name = UA_STRING("NodeId of variable");
    inputArguments[0].dataType = UA_TYPES[UA_TYPES_NODEID].typeId;
    inputArguments[0].valueRank = UA_VALUERANK_SCALAR;

    UA_Argument_init(&inputArguments[1]);
    inputArguments[1].description = UA_LOCALIZEDTEXT("en-US", "New float value");
    inputArguments[1].name = UA_STRING("float value");
    inputArguments[1].dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
    inputArguments[1].valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes floatAttr = UA_MethodAttributes_default;
    floatAttr.description = UA_LOCALIZEDTEXT("en-US", "Update a float variable");
    floatAttr.displayName = UA_LOCALIZEDTEXT("en-US", "updateFloatVariable");
    floatAttr.executable = true;
    floatAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62543), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "updateFloatVariable"), floatAttr, &updateFloatVariableMethodCallback,
                            2, inputArguments, 0, NULL, NULL, NULL);
}

static UA_StatusCode readActMouldPosition(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                                          const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
                                          const UA_NumericRange *range, UA_DataValue *dataValue) {
    UA_Variant_setScalarCopy(&dataValue->value, &actPos, &UA_TYPES[UA_TYPES_FLOAT]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writeActMouldPosition(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                                           const UA_NodeId *nodeId, void *nodeContext, const UA_NumericRange *range,
                                           const UA_DataValue *data) {
    actPos = *(UA_Float *)data->value.data;
    return UA_STATUSCODE_GOOD;
}

static void addCurrentMouldPosition(UA_Server *server) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Mould Position");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId currentNodeId = UA_NODEID_NUMERIC(1, 7000);
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, "current-time-datasource");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource timeDataSource;
    timeDataSource.read = readActMouldPosition;
    timeDataSource.write = writeActMouldPosition;
    UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId, parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr, timeDataSource, NULL, NULL);
}

static void addVariable(UA_Server *server) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;

    UA_Variant var;
    var.storageType = UA_VARIANT_DATA;
    var.type = &UA_TYPES[UA_TYPES_FLOAT];
    var.data = NULL;
    var.arrayLength = 0;
    var.arrayDimensionsSize = 0;
    var.arrayDimensions = NULL;

    UA_PublishedVariableDataType pubVar[] = {
        {UA_NODEID_NUMERIC(1, 4000), (UA_UInt32)13, 0.0, 0u, 0.0, UA_STRING("test"), var, 0, NULL},
        {UA_NODEID_NUMERIC(1, 4000), (UA_UInt32)14, 0.0, 0u, 0.0, UA_STRING("test"), var, 0, NULL}};
    UA_Variant_setArray(&attr.value, &pubVar, 2, &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);
    // UA_Variant_setScalar(&attr.value, &pubVar, &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "demo publishedData");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "demo publishedData");
    attr.dataType = UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_NUMERIC(1, 8000);
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "publishedVariabled");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

static void addFloatArrayVariable(UA_Server *server) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;

    UA_Float var[] = {1.1f, 2.2f};
    UA_Variant_setArray(&attr.value, &var, 2, &UA_TYPES[UA_TYPES_FLOAT]);
    // UA_Variant_setScalar(&attr.value, &pubVar, &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "demo publishedData");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "demo publishedData");
    attr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_NUMERIC(1, 8001);
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "float array");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

static int run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ServerConfig *config = UA_ServerConfig_new_minimal(4843, NULL); // UA_ServerConfig_new_default();
    /* Details about the connection configuration and handling are located in
     * the pubsub connection tutorial */
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

    addFloatVariableMethod(server);
    addFloatVariable(server, "Core1_ActPos");
    addCurrentMouldPosition(server);
    addPubSubConnection(server, transportProfile, networkAddressUrl);
    addPublishedDataSet(server);
    addDataSetField(server);
    addWriterGroup(server);
    addDataSetWriter(server);
    addDataSetFielddMethod(server);
    updateFloatVariableMethod(server);

    //Add some test data to check the performance
#define lenTestData 3
    UA_Float dataTest1 = (UA_Float)13.23;
    UA_Float dataTest2 = (UA_Float)231.50;
    UA_Float dataTest3 = (UA_Float)52.80;

    tTestData testData[lenTestData] = {{UA_STRING("Test1"), &UA_TYPES[UA_TYPES_FLOAT], 0, &dataTest1},
                                       {UA_STRING("Test2"), &UA_TYPES[UA_TYPES_FLOAT], 0, &dataTest2},
                                       {UA_STRING("Test3"), &UA_TYPES[UA_TYPES_FLOAT], 0, &dataTest3}};

    addTestData(server, testData, lenTestData);

    //TODO: Update some variables during the runtime

    addVariable(server);
    addFloatArrayVariable(server);

    // get published dataset
    printf("Published dataset NodeId demo pds: %u %u\n", publishedDataSetIdent.namespaceIndex,
           publishedDataSetIdent.identifier.numeric);
    // get node Id of publishedData
    UA_RelativePathElement rel = {UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), UA_FALSE, UA_TRUE,
                                  UA_QUALIFIEDNAME(0, "PublishedData")};

    UA_BrowsePath bp;
    bp.startingNode = publishedDataSetIdent;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rel;

    UA_BrowsePathResult res = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    UA_NodeId publishedDataId;
    if (res.statusCode == UA_STATUSCODE_GOOD) {
        publishedDataId = res.targets[0].targetId.nodeId;
        printf("Published data id : %u %u\n", publishedDataId.namespaceIndex, publishedDataId.identifier.numeric);

        UA_Variant out;
        UA_Variant_init(&out);
        if (UA_Server_readValue(server, publishedDataId, &out) == UA_STATUSCODE_GOOD) {
            UA_PublishedVariableDataType *p = (UA_PublishedVariableDataType *)out.data;
            printf("attributeid: %u \n", p->attributeId);
        }
    }

    retval |= UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void usage(char *progname) { printf("usage: %s <uri> [device]\n", progname); }

int main(int argc, char **argv) {
    UA_String transportProfile = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};

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
