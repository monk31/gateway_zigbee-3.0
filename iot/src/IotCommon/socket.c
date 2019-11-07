// ------------------------------------------------------------------
// Socket
// ------------------------------------------------------------------
// Offers IoT socket connection and all routines for using it
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \file
 * \brief Offers IoT socket connection and all routines for using it
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>

#include "iotError.h"
#include "newLog.h"
#include "dump.h"
#include "socket.h"

#define BACKLOG 10

#define SOCKET_DEBUG
// #define SOCKET_DEBUG_DUMP

#ifdef SOCKET_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* SOCKET_DEBUG */

// ------------------------------------------------------------------------
// Open TCP socket connection
// ------------------------------------------------------------------------

/**
 * \brief Opens a TCP socket
 * \param host Host address (typically "0.0.0.0")
 * \param port Port number (e.g. "2000")
 * \param server Indicates if the caller is a server or a client
 * \returns A handle to a socket, or a negative number in case of an error (and sets the global iotError)
 */
int socketOpen( char * host, char * port, int server ) {

    struct addrinfo         sHints;
    struct addrinfo         *addrInfo;
    struct addrinfo         *p;
    int                     iYes = 1;
    int                     iResult;

    memset(&sHints, 0, sizeof(struct addrinfo));
    sHints.ai_family   = AF_UNSPEC;     // IPv4 or IPv6
    sHints.ai_socktype = SOCK_STREAM;   // TCP
    sHints.ai_flags    = AI_PASSIVE;

    int handle;

    if ((iResult = getaddrinfo(host, port, &sHints, &addrInfo)) != 0) {
        printf( "socketOpen: %s failed(%s)\n", "getaddrinfo", gai_strerror(iResult));
        iotError = IOT_ERROR_SOCKET_OPEN;
        return -1;
    }

    for(p = addrInfo; p != NULL; p = p->ai_next) {
        if (( handle = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            printf( "socketOpen: %s failed(%s)\n", "socket", strerror(errno));
            continue;
        }

        // A server does a bind, whereas a client does a connect...

        if ( server ) {
            if (setsockopt( handle, SOL_SOCKET, SO_REUSEADDR, &iYes, sizeof(int)) == -1) {
                printf( "socketOpen: %s failed(%s)\n", "setsockopt", strerror(errno));
            }

            if (bind( handle, p->ai_addr, p->ai_addrlen) == -1) {
                close( handle );
                printf( "socketOpen: %s failed(%s)\n", "bind", strerror(errno));
                continue;
            }

        } else {
            if (connect( handle, p->ai_addr, p->ai_addrlen) == -1) {
                close( handle );
                printf( "socketOpen: %s failed(%s)\n", "connect", strerror(errno));
                continue;
            }
        }

        break;
    }

    freeaddrinfo(addrInfo); // all done with this structure

    if (p == NULL) {
        iotError = IOT_ERROR_SOCKET_OPEN;
        return -2;
    }

    if ( server ) {
        // A server will start to listen for clients...

        if (listen( handle, BACKLOG) == -1) {
            printf( "socketOpen: %s failed(%s)\n", "listen", strerror(errno));
            iotError = IOT_ERROR_SOCKET_OPEN;
            return -3;
        }
    }

    return( handle );
}

// ------------------------------------------------------------------------
// Accept a client to connect
// ------------------------------------------------------------------------

static void *psGetSockAddr(struct sockaddr *psSockAddr)
{
    if (psSockAddr->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)psSockAddr)->sin_addr);
    }

    return &(((struct sockaddr_in6*)psSockAddr)->sin6_addr);
}

/**
 * \brief Accepts a socket connection to a client (typically called by a server)
 * \param handle Handle to a server socket
 * \returns Handle to a new socket that connects to a client, or -1 in case or an error
 */
int socketAccept( int handle ) {
    socklen_t               iSinSize = sizeof(struct sockaddr_storage);
    struct sockaddr_storage sClientAddress;
    char                    sClientAddressStr[INET6_ADDRSTRLEN];
    int                     iClientSocket;

    iClientSocket = accept(handle,
                           (struct sockaddr *)&sClientAddress, &iSinSize);
    if (iClientSocket == -1) {
        printf( "socketAccept: failed(%s)\n", strerror(errno));
    } else {
        inet_ntop(sClientAddress.ss_family,
                  psGetSockAddr((struct sockaddr *)&sClientAddress),
                  sClientAddressStr, INET6_ADDRSTRLEN);
        printf( "New client connection from %s\n", sClientAddressStr);
    }
   
    return( iClientSocket );
}
   
// ------------------------------------------------------------------------
// Read
// ------------------------------------------------------------------------

/**
 * \brief Reads a message from a socket. Note: this is a blocking read.
 * \param handle Handle to a socket
 * \param buffer User-allocated area to recieve the message that is read
 * \param size Size of the user-allocated area
 * \returns The length of the received message which should be > 0 on success
 */
int socketRead( int handle, char * buffer, int size ) {

    int len;
    len = read( handle, buffer, size ); 
    if ( len > 0 ) {
        buffer[len] = '\0';
        DEBUG_PRINTF( "SR (%d): %s", len, buffer );
#ifdef SOCKET_DEBUG_DUMP
        dump( buffer, len );
#endif
    } else {
        printf( "Socket reads %d bytes\n", len );
        iotError = IOT_ERROR_SOCKET_READ;
    }
    return( len );
}

/**
 * \brief Reads a message from a socket. Note: call unblocks after <sec> seconds.
 * \param handle Handle to a socket
 * \param buffer User-allocated area to recieve the message that is read
 * \param size Size of the user-allocated area
 * \param sec Timeout in seconds
 * \returns The length of the received message which should be > 0 on success
 */
int socketReadWithTimeout( int handle, char * buffer, int size, int sec ) {

    struct timeval tv;
    tv.tv_sec  = sec;
    tv.tv_usec = 0;

    if (setsockopt( handle, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) == -1) {
        if ( errno != EAGAIN )   // Timeout
            printf( "socketReadWithTimeout: %s failed(%s)\n", "setsockopt", strerror(errno));
        iotError = IOT_ERROR_SOCKET_READ;
    }

    int len;
    len = read( handle, buffer, size ); 
    if ( len > 0 ) {
        buffer[len] = '\0';
        DEBUG_PRINTF( "SRt (%d): %s", len, buffer );
#ifdef SOCKET_DEBUG_DUMP
        dump( buffer, len );
#endif
    }
    return( len );
}

// ------------------------------------------------------------------------
// Write
// ------------------------------------------------------------------------

/**
 * \brief Write a message to a socket
 * \param handle Handle to a socket
 * \param buffer Message tto write
 * \param size Size of the message
 * \returns The length of the written message, or -1 on error
 */
int socketWrite( int handle, char * buffer, int len ) {
    DEBUG_PRINTF( "SW (%d): %s", len, buffer );
    int iResult = send( handle, buffer, len, MSG_NOSIGNAL );
    if ((iResult <= 0) || (iResult != len) ) {
        if ( errno != EPIPE ) // Broken pipe
           printf( "socketWrite: failed(%s)\n", strerror(errno));
        iotError = IOT_ERROR_SOCKET_WRITE;
        return -1;
    }
    return( iResult );
}

/**
 * \brief Write a string to a socket
 * \param handle Handle to a socket
 * \param string String to write
 * \returns The length of the written message, or -1 on error
 */
int socketWriteString( int handle, char * string ) {
    int len = strlen( string );
    return( socketWrite( handle, string, len ) );
}

// ------------------------------------------------------------------------
// Close
// ------------------------------------------------------------------------

/**
 * \brief Close a socket
 * \param handle Handle to a socket
 */
void socketClose( int handle ) {
    close( handle );
}


