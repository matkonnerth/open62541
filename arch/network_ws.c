/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Jose Cabral
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 */

#define UA_INTERNAL

#include <open62541/network_ws.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/util.h>

#include <libwebsockets.h>

#include "open62541_queue.h"

#include <string.h>  // memset
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/* one of these created for each message */

//insane?
static UA_Server *actServer;

struct msg {
	void *payload; /* is malloc'd */
	size_t len;
};

/* one of these is created for each client connecting to us */

struct per_session_data__minimal {
    UA_Connection *conn;
    struct per_session_data__minimal *pss_list;
    struct lws *wsi;
    int last; /* the last message number we sent */
};

/* one of these is created for each vhost our protocol is used with */

struct per_vhost_data__minimal {
	struct lws_context *context;
	struct lws_vhost *vhost;
	const struct lws_protocols *protocol;

	struct per_session_data__minimal *pss_list; /* linked-list of live pss*/

	struct msg amsg; /* the one pending message... */
	int current; /* the current message number we are caching */
};

/* destroys the message when everyone has had a copy of it */

/* 
static void
__minimal_destroy_message(void *_msg)
{
	struct msg *msg = (struct msg*) _msg;

	free(msg->payload);
	msg->payload = NULL;
	msg->len = 0;
}
*/

static UA_StatusCode
connection_getsendbuffer(UA_Connection *connection,
                         size_t length, UA_ByteString *buf) {
    if(length > connection->config.sendBufferSize)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    return UA_ByteString_allocBuffer(buf, length);
}

static UA_StatusCode
connection_write(UA_Connection *connection, UA_ByteString *buf) {
    if(connection->state == UA_CONNECTION_CLOSED) {
        UA_ByteString_deleteMembers(buf);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    struct lws *wsi = (struct lws *)connection->handle;
    struct per_vhost_data__minimal *vhd =
        (struct per_vhost_data__minimal *)lws_protocol_vh_priv_get(lws_get_vhost(wsi),
                                                                   lws_get_protocol(wsi));

    if(vhd->amsg.payload)
    {
        free(vhd->amsg.payload);
        vhd->amsg.payload = NULL;
        vhd->amsg.len = 0;
    }

    vhd->amsg.len = LWS_PRE + buf->length;
    vhd->amsg.payload = malloc(LWS_PRE + vhd->amsg.len);
    memcpy((unsigned char*)vhd->amsg.payload + LWS_PRE, buf->data, buf->length);
    lws_callback_on_writable((struct lws*)connection->handle);

    return UA_STATUSCODE_GOOD;
}

static void
ServerNetworkLayerWS_close(UA_Connection *connection) {
    if (connection->state == UA_CONNECTION_CLOSED)
        return;
    //UA_shutdown((UA_SOCKET)connection->sockfd, 2);
    //close connection in here
    connection->state = UA_CONNECTION_CLOSED;
}

static int
callback_minimal(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct per_session_data__minimal *pss =
			(struct per_session_data__minimal *)user;
	struct per_vhost_data__minimal *vhd =
			(struct per_vhost_data__minimal *)
			lws_protocol_vh_priv_get(lws_get_vhost(wsi),
					lws_get_protocol(wsi));
	int m;

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		vhd = (struct per_vhost_data__minimal*) lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
				lws_get_protocol(wsi),
				sizeof(struct per_vhost_data__minimal));
		vhd->context = lws_get_context(wsi);
		vhd->protocol = lws_get_protocol(wsi);
		vhd->vhost = lws_get_vhost(wsi);
		break;

	case LWS_CALLBACK_ESTABLISHED:
		/* add ourselves to the list of live pss held in the vhd */
		lws_ll_fwd_insert(pss, pss_list, vhd->pss_list);
		pss->wsi = wsi;
		pss->last = vhd->current;

        UA_Connection *c = pss->conn = (UA_Connection*)malloc(sizeof(UA_Connection));
        memset(c, 0, sizeof(UA_Connection));
        c->sockfd = 0;
        c->handle = wsi;
        // c->config = NULL;
        c->send = connection_write;
        c->close = ServerNetworkLayerWS_close;
        // c->free = ServerNetworkLayerTCP_freeConnection;
        c->getSendBuffer = connection_getsendbuffer;
        //c->releaseSendBuffer = connection_releasesendbuffer;
        //c->releaseRecvBuffer = connection_releaserecvbuffer;
        c->state = UA_CONNECTION_ESTABLISHED;
        c->openingDate = UA_DateTime_nowMonotonic();

        break;

	case LWS_CALLBACK_CLOSED:
		/* remove our closing pss from the list of live pss */
		lws_ll_fwd_remove(struct per_session_data__minimal, pss_list,
				  pss, vhd->pss_list);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		if (!vhd->amsg.payload)
			break;

		if (pss->last == vhd->current)
			break;

		/* notice we allowed for LWS_PRE in the payload already */
		m = lws_write(wsi, ((unsigned char *)vhd->amsg.payload) +
			      LWS_PRE, vhd->amsg.len, LWS_WRITE_TEXT);
		if (m < (int)vhd->amsg.len) {
			lwsl_err("ERROR %d writing to ws\n", m);
			return -1;
		}

		pss->last = vhd->current;
		break;


    /**< data has appeared for this server endpoint from a
	 * remote client, it can be found at *in and is
	 * len bytes long */
	case LWS_CALLBACK_RECEIVE:
        if(!actServer)
            break;

        //should we copy in here?
        UA_ByteString buf;
        buf.length = len;
        buf.data =(UA_Byte *)in;
        UA_Server_processBinaryMessage(actServer, pss->conn, &buf);
		break;

	default:
		break;
	}

	return 0;
}


static struct lws_protocols protocols[] = {
	{ "http", lws_callback_http_dummy, 0, 0, 0, NULL, 0 },
	{ "opcua", callback_minimal, sizeof(struct per_session_data__minimal), 128, 0, NULL, 0},
	{ NULL, NULL, 0, 0, 0, NULL, 0 } /* terminator */
};

const struct lws_protocol_vhost_options pvo_opt = {
    NULL, NULL, "default", "1"
};
const struct lws_protocol_vhost_options pvo = {
    NULL, &pvo_opt, "opcua", ""
};

static const struct lws_http_mount mount = {
    /* .mount_next */ NULL,         /* linked-list "next" */
    /* .mountpoint */ "/",          /* mountpoint URL */
    /* .origin */ "./mount-origin", /* serve from dir */
    /* .def */ "index.html",        /* default filename */
    /* .protocol */ NULL,
    /* .cgienv */ NULL,
    /* .extra_mimetypes */ NULL,
    /* .interpret */ NULL,
    /* .cgi_timeout */ 0,
    /* .cache_max_age */ 0,
    /* .auth_mask */ 0,
    /* .cache_reusable */ 0,
    /* .cache_revalidate */ 0,
    /* .cache_intermediaries */ 0,
    /* .origin_protocol */ LWSMPRO_FILE, /* files in a dir */
    /* .mountpoint_len */ 1,             /* char count */
    /* .basic_auth_login_file */ NULL, 
    {NULL}

};

/* 
static UA_StatusCode
connection_getsendbuffer(UA_Connection *connection,
                         size_t length, UA_ByteString *buf) {
    if(length > connection->config.sendBufferSize)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    return UA_ByteString_allocBuffer(buf, length);
}

static void
connection_releasesendbuffer(UA_Connection *connection,
                             UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

static void
connection_releaserecvbuffer(UA_Connection *connection,
                             UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

static UA_StatusCode
connection_write(UA_Connection *connection, UA_ByteString *buf) {
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
connection_recv(UA_Connection *connection, UA_ByteString *response,
                UA_UInt32 timeout) {    
    return UA_STATUSCODE_GOOD;
}
*/

/***************************/
/* Server NetworkLayer Websocket */
/***************************/

#define MAXBACKLOG     100
#define NOHELLOTIMEOUT 120000 /* timeout in ms before close the connection
                               * if server does not receive Hello Message */

typedef struct ConnectionEntry {
    UA_Connection connection;
    LIST_ENTRY(ConnectionEntry) pointers;
} ConnectionEntry;

typedef struct {
    const UA_Logger *logger;
    UA_UInt16 port;
    UA_SOCKET serverSockets[FD_SETSIZE];
    UA_UInt16 serverSocketsSize;
    struct lws_context *context;
    LIST_HEAD(, ConnectionEntry) connections;    
} ServerNetworkLayerWS;

/* 
static void
ServerNetworkLayerWS_freeConnection(UA_Connection *connection) {
    UA_Connection_deleteMembers(connection);
    UA_free(connection);
}
*/

/* This performs only 'shutdown'. 'close' is called when the shutdown
 * socket is returned from select. */
/* 
static void
ServerNetworkLayerWS_close(UA_Connection *connection) {
    if (connection->state == UA_CONNECTION_CLOSED)
        return;
    UA_shutdown((UA_SOCKET)connection->sockfd, 2);
    connection->state = UA_CONNECTION_CLOSED;
}
*/

static UA_StatusCode
ServerNetworkLayerWS_start(UA_ServerNetworkLayer *nl, const UA_String *customHostname) {
  UA_initialize_architecture_network();

    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)nl->handle;

    struct lws_context_creation_info info;
	struct lws_context *context;

    int logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
			/* | LLL_DEBUG */;

    lws_set_log_level(logs, NULL);
	lwsl_user("LWS minimal ws server | visit http://localhost:7681 (-s = use TLS / https)\n");


    memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
	info.port = 7681;
	info.mounts = &mount;
	info.protocols = protocols;
	info.vhost_name = "localhost";
	info.ws_ping_pong_interval = 10;
	//info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
    info.pvo = &pvo;

        // is this the only option??
        // info.user = malloc(sizeof(UA_Server *));

        context = lws_create_context(&info);
	if (!context) 
    {
		lwsl_err("lws init failed\n");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    layer->context = context;

    // 
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
ServerNetworkLayerWS_listen(UA_ServerNetworkLayer *nl, UA_Server *server,
                             UA_UInt16 timeout) {
    /* Every open socket can generate two jobs */
    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)nl->handle;

    actServer = server;
    lws_service(layer->context, timeout);
    return UA_STATUSCODE_GOOD;
}

static void
ServerNetworkLayerWS_stop(UA_ServerNetworkLayer *nl, UA_Server *server) {
    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)nl->handle;
    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "Shutting down the WS network layer");
    /* Run recv on client sockets. This picks up the closed sockets and frees
     * the connection. */
    ServerNetworkLayerWS_listen(nl, server, 0);

    lws_context_destroy(layer->context);

    UA_deinitialize_architecture_network();
}

/* run only when the server is stopped */
static void
ServerNetworkLayerWS_deleteMembers(UA_ServerNetworkLayer *nl) {

}

UA_ServerNetworkLayer
UA_ServerNetworkLayerWS(UA_ConnectionConfig config, UA_UInt16 port,
                         UA_Logger *logger) {
    UA_ServerNetworkLayer nl;
    memset(&nl, 0, sizeof(UA_ServerNetworkLayer));
    nl.deleteMembers = ServerNetworkLayerWS_deleteMembers;
    nl.localConnectionConfig = config;
    nl.start = ServerNetworkLayerWS_start;
    nl.listen = ServerNetworkLayerWS_listen;
    nl.stop = ServerNetworkLayerWS_stop;
    nl.handle = NULL;

    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS*)
        UA_calloc(1,sizeof(ServerNetworkLayerWS));
    if(!layer)
        return nl;
    nl.handle = layer;

    layer->logger = logger;
    layer->port = port;

    return nl;
}
