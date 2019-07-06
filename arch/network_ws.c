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

/****************************/
/* Generic Socket Functions */
/****************************/

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
    if(connection->state == UA_CONNECTION_CLOSED) {
        UA_ByteString_deleteMembers(buf);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Prevent OS signals when sending to a closed socket */
    int flags = 0;
    flags |= MSG_NOSIGNAL;

    /* Send the full buffer. This may require several calls to send */
    size_t nWritten = 0;
    do {
        ssize_t n = 0;
        do {
            size_t bytes_to_send = buf->length - nWritten;
            n = UA_send(connection->sockfd,
                     (const char*)buf->data + nWritten,
                     bytes_to_send, flags);
            if(n < 0 && UA_ERRNO != UA_INTERRUPTED && UA_ERRNO != UA_AGAIN) {
                connection->close(connection);
                UA_ByteString_deleteMembers(buf);
                return UA_STATUSCODE_BADCONNECTIONCLOSED;
            }
        } while(n < 0);
        nWritten += (size_t)n;
    } while(nWritten < buf->length);

    /* Free the buffer */
    UA_ByteString_deleteMembers(buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
connection_recv(UA_Connection *connection, UA_ByteString *response,
                UA_UInt32 timeout) {
    if(connection->state == UA_CONNECTION_CLOSED)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    /* Listen on the socket for the given timeout until a message arrives */
    if(timeout > 0) {
        fd_set fdset;
        FD_ZERO(&fdset);
        UA_fd_set(connection->sockfd, &fdset);
        UA_UInt32 timeout_usec = timeout * 1000;
        struct timeval tmptv = {(long int)(timeout_usec / 1000000),
                                (int)(timeout_usec % 1000000)};
        int resultsize = UA_select(connection->sockfd+1, &fdset, NULL,
                                NULL, &tmptv);

        /* No result */
        if(resultsize == 0)
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

        if(resultsize == -1) {
            /* The call to select was interrupted manually. Act as if it timed
             * out */
            if(UA_ERRNO == EINTR)
                return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

            /* The error cannot be recovered. Close the connection. */
            connection->close(connection);
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
        }
    }

    response->data = (UA_Byte*)UA_malloc(connection->config.recvBufferSize);
    if(!response->data) {
        response->length = 0;
        return UA_STATUSCODE_BADOUTOFMEMORY; /* not enough memory retry */
    }

#ifdef _WIN32
    // windows requires int parameter for length
    int offset = (int)connection->incompleteChunk.length;
    int remaining = connection->config.recvBufferSize - offset;
#else
    size_t offset = connection->incompleteChunk.length;
    size_t remaining = connection->config.recvBufferSize - offset;
#endif

    /* Get the received packet(s) */
    ssize_t ret = UA_recv(connection->sockfd, (char*)&response->data[offset],
                          remaining, 0);

    /* The remote side closed the connection */
    if(ret == 0) {
        UA_ByteString_deleteMembers(response);
        connection->close(connection);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Error case */
    if(ret < 0) {
        UA_ByteString_deleteMembers(response);
        if(UA_ERRNO == UA_INTERRUPTED || (timeout > 0) ?
           false : (UA_ERRNO == UA_EAGAIN || UA_ERRNO == UA_WOULDBLOCK))
            return UA_STATUSCODE_GOOD; /* statuscode_good but no data -> retry */
        connection->close(connection);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Preprend the last incompleteChunk into the buffer */
    if (connection->incompleteChunk.length > 0) {
        memcpy(response->data, connection->incompleteChunk.data,
               connection->incompleteChunk.length);
        UA_ByteString_deleteMembers(&connection->incompleteChunk);
    }

    /* Set the length of the received buffer */
    response->length = offset + (size_t)ret;
    return UA_STATUSCODE_GOOD;
}


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
    LIST_HEAD(, ConnectionEntry) connections;
} ServerNetworkLayerWS;

static void
ServerNetworkLayerWS_freeConnection(UA_Connection *connection) {
    UA_Connection_deleteMembers(connection);
    UA_free(connection);
}

/* This performs only 'shutdown'. 'close' is called when the shutdown
 * socket is returned from select. */
static void
ServerNetworkLayerWS_close(UA_Connection *connection) {
    if (connection->state == UA_CONNECTION_CLOSED)
        return;
    UA_shutdown((UA_SOCKET)connection->sockfd, 2);
    connection->state = UA_CONNECTION_CLOSED;
}

static UA_StatusCode
ServerNetworkLayerWS_add(UA_ServerNetworkLayer *nl, ServerNetworkLayerWS *layer,
                          UA_Int32 newsockfd, struct sockaddr_storage *remote) {
    /* Set nonblocking */
    UA_socket_set_nonblocking(newsockfd);//TODO: check return value

    /* Do not merge packets on the socket (disable Nagle's algorithm) */
    int dummy = 1;
    if(UA_setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY,
               (const char *)&dummy, sizeof(dummy)) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(layer->logger, UA_LOGCATEGORY_NETWORK,
                             "Cannot set socket option TCP_NODELAY. Error: %s",
                             errno_str));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

#if defined(UA_getnameinfo)
    /* Get the peer name for logging */
    char remote_name[100];
    int res = UA_getnameinfo((struct sockaddr*)remote,
                          sizeof(struct sockaddr_storage),
                          remote_name, sizeof(remote_name),
                          NULL, 0, NI_NUMERICHOST);
    if(res == 0) {
        UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | New connection over TCP from %s",
                    (int)newsockfd, remote_name);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                                                "Connection %i | New connection over TCP, "
                                                "getnameinfo failed with error: %s",
                                                (int)newsockfd, errno_str));
    }
#else
    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "Connection %i | New connection over TCP",
                (int)newsockfd);
#endif
    /* Allocate and initialize the connection */
    ConnectionEntry *e = (ConnectionEntry*)UA_malloc(sizeof(ConnectionEntry));
    if(!e){
        UA_close(newsockfd);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Connection *c = &e->connection;
    memset(c, 0, sizeof(UA_Connection));
    c->sockfd = newsockfd;
    c->handle = layer;
    c->config = nl->localConnectionConfig;
    c->send = connection_write;
    c->close = ServerNetworkLayerWS_close;
    c->free = ServerNetworkLayerWS_freeConnection;
    c->getSendBuffer = connection_getsendbuffer;
    c->releaseSendBuffer = connection_releasesendbuffer;
    c->releaseRecvBuffer = connection_releaserecvbuffer;
    c->state = UA_CONNECTION_OPENING;
    c->openingDate = UA_DateTime_nowMonotonic();

    /* Add to the linked list */
    LIST_INSERT_HEAD(&layer->connections, e, pointers);
    return UA_STATUSCODE_GOOD;
}

static void
addServerSocket(ServerNetworkLayerWS *layer, struct addrinfo *ai) {
    /* Create the server socket */
    UA_SOCKET newsock = UA_socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if(newsock == UA_INVALID_SOCKET)
    {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Error opening the server socket");
        return;
    }

    /* Some Linux distributions have net.ipv6.bindv6only not activated. So
     * sockets can double-bind to IPv4 and IPv6. This leads to problems. Use
     * AF_INET6 sockets only for IPv6. */

    int optval = 1;
#if UA_IPV6
    if(ai->ai_family == AF_INET6 &&
       UA_setsockopt(newsock, IPPROTO_IPV6, IPV6_V6ONLY,
                  (const char*)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not set an IPv6 socket to IPv6 only");
        UA_close(newsock);
        return;
    }
#endif
    if(UA_setsockopt(newsock, SOL_SOCKET, SO_REUSEADDR,
                  (const char *)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not make the socket reusable");
        UA_close(newsock);
        return;
    }


    if(UA_socket_set_nonblocking(newsock) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not set the server socket to nonblocking");
        UA_close(newsock);
        return;
    }

    /* Bind socket to address */
    if(UA_bind(newsock, ai->ai_addr, (socklen_t)ai->ai_addrlen) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                           "Error binding a server socket: %s", errno_str));
        UA_close(newsock);
        return;
    }

    /* Start listening */
    if(UA_listen(newsock, MAXBACKLOG) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Error listening on server socket: %s", errno_str));
        UA_close(newsock);
        return;
    }

    if (layer->port == 0) {
        /* Port was automatically chosen. Read it from the OS */
        struct sockaddr_in returned_addr;
        memset(&returned_addr, 0, sizeof(returned_addr));
        socklen_t len = sizeof(returned_addr);
        UA_getsockname(newsock, (struct sockaddr *)&returned_addr, &len);
        layer->port = ntohs(returned_addr.sin_port);
    }

    layer->serverSockets[layer->serverSocketsSize] = newsock;
    layer->serverSocketsSize++;
}

static UA_StatusCode
ServerNetworkLayerWS_start(UA_ServerNetworkLayer *nl, const UA_String *customHostname) {
  UA_initialize_architecture_network();

    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)nl->handle;


    //libwebsocket_create_context

    /* Get addrinfo of the server and create server sockets */
    char portno[6];
    UA_snprintf(portno, 6, "%d", layer->port);
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    if(UA_getaddrinfo(NULL, portno, &hints, &res) != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* There might be serveral addrinfos (for different network cards,
     * IPv4/IPv6). Add a server socket for all of them. */
    struct addrinfo *ai = res;
    for(layer->serverSocketsSize = 0;
        layer->serverSocketsSize < FD_SETSIZE && ai != NULL;
        ai = ai->ai_next)
        addServerSocket(layer, ai);
    UA_freeaddrinfo(res);

    /* Get the discovery url from the hostname */
    UA_String du = UA_STRING_NULL;
    char discoveryUrlBuffer[256];
    char hostnameBuffer[256];
    if (customHostname->length) {
        du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "opc.tcp://%.*s:%d/",
                                        (int)customHostname->length,
                                        customHostname->data,
                                        layer->port);
        du.data = (UA_Byte*)discoveryUrlBuffer;
    }else{
        if(UA_gethostname(hostnameBuffer, 255) == 0) {
            du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "opc.tcp://%s:%d/",
                                            hostnameBuffer, layer->port);
            du.data = (UA_Byte*)discoveryUrlBuffer;
        } else {
            UA_LOG_ERROR(layer->logger, UA_LOGCATEGORY_NETWORK, "Could not get the hostname");
        }
    }
    UA_String_copy(&du, &nl->discoveryUrl);

    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "TCP network layer listening on %.*s",
                (int)nl->discoveryUrl.length, nl->discoveryUrl.data);
    return UA_STATUSCODE_GOOD;
}

/* After every select, reset the sockets to listen on */
static UA_Int32
setFDSet(ServerNetworkLayerWS *layer, fd_set *fdset) {
    FD_ZERO(fdset);
    UA_Int32 highestfd = 0;
    for(UA_UInt16 i = 0; i < layer->serverSocketsSize; i++) {
        UA_fd_set(layer->serverSockets[i], fdset);
        if((UA_Int32)layer->serverSockets[i] > highestfd)
            highestfd = (UA_Int32)layer->serverSockets[i];
    }

    ConnectionEntry *e;
    LIST_FOREACH(e, &layer->connections, pointers) {
        UA_fd_set(e->connection.sockfd, fdset);
        if((UA_Int32)e->connection.sockfd > highestfd)
            highestfd = (UA_Int32)e->connection.sockfd;
    }

    return highestfd;
}

static UA_StatusCode
ServerNetworkLayerWS_listen(UA_ServerNetworkLayer *nl, UA_Server *server,
                             UA_UInt16 timeout) {
    /* Every open socket can generate two jobs */
    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)nl->handle;

    if (layer->serverSocketsSize == 0)
        return UA_STATUSCODE_GOOD;

    /* Listen on open sockets (including the server) */
    fd_set fdset, errset;
    UA_Int32 highestfd = setFDSet(layer, &fdset);
    setFDSet(layer, &errset);
    struct timeval tmptv = {0, timeout * 1000};
    if (UA_select(highestfd+1, &fdset, NULL, &errset, &tmptv) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_DEBUG(layer->logger, UA_LOGCATEGORY_NETWORK,
                           "Socket select failed with %s", errno_str));
        // we will retry, so do not return bad
        return UA_STATUSCODE_GOOD;
    }

    /* Accept new connections via the server sockets */
    for(UA_UInt16 i = 0; i < layer->serverSocketsSize; i++) {
        if(!UA_fd_isset(layer->serverSockets[i], &fdset))
            continue;

        struct sockaddr_storage remote;
        socklen_t remote_size = sizeof(remote);
        UA_SOCKET newsockfd = UA_accept((UA_SOCKET)layer->serverSockets[i],
                                  (struct sockaddr*)&remote, &remote_size);
        if(newsockfd == UA_INVALID_SOCKET)
            continue;

        UA_LOG_TRACE(layer->logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | New TCP connection on server socket %i",
                    (int)newsockfd, (int)(layer->serverSockets[i]));

        ServerNetworkLayerWS_add(nl, layer, (UA_Int32)newsockfd, &remote);
    }

    /* Read from established sockets */
    ConnectionEntry *e, *e_tmp;
    UA_DateTime now = UA_DateTime_nowMonotonic();
    LIST_FOREACH_SAFE(e, &layer->connections, pointers, e_tmp) {
        if ((e->connection.state == UA_CONNECTION_OPENING) &&
            (now > (e->connection.openingDate + (NOHELLOTIMEOUT * UA_DATETIME_MSEC)))){
            UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                        "Connection %i | Closed by the server (no Hello Message)",
                         (int)(e->connection.sockfd));
            LIST_REMOVE(e, pointers);
            UA_close(e->connection.sockfd);
            UA_Server_removeConnection(server, &e->connection);
            continue;
        }

        if(!UA_fd_isset(e->connection.sockfd, &errset) &&
           !UA_fd_isset(e->connection.sockfd, &fdset))
          continue;

        UA_LOG_TRACE(layer->logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Activity on the socket",
                    (int)(e->connection.sockfd));

        UA_ByteString buf = UA_BYTESTRING_NULL;
        UA_StatusCode retval = connection_recv(&e->connection, &buf, 0);

        if(retval == UA_STATUSCODE_GOOD) {
            /* Process packets */
            UA_Server_processBinaryMessage(server, &e->connection, &buf);
            connection_releaserecvbuffer(&e->connection, &buf);
        } else if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED) {
            /* The socket is shutdown but not closed */
            UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                        "Connection %i | Closed",
                        (int)(e->connection.sockfd));
            LIST_REMOVE(e, pointers);
            UA_close(e->connection.sockfd);
            UA_Server_removeConnection(server, &e->connection);
        }
    }
    return UA_STATUSCODE_GOOD;
}

static void
ServerNetworkLayerWS_stop(UA_ServerNetworkLayer *nl, UA_Server *server) {
    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)nl->handle;
    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "Shutting down the TCP network layer");

    /* Close the server sockets */
    for(UA_UInt16 i = 0; i < layer->serverSocketsSize; i++) {
        UA_shutdown(layer->serverSockets[i], 2);
        UA_close(layer->serverSockets[i]);
    }
    layer->serverSocketsSize = 0;

    /* Close open connections */
    ConnectionEntry *e;
    LIST_FOREACH(e, &layer->connections, pointers)
        ServerNetworkLayerWS_close(&e->connection);

    /* Run recv on client sockets. This picks up the closed sockets and frees
     * the connection. */
    ServerNetworkLayerWS_listen(nl, server, 0);

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
