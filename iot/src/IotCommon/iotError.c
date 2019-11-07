// ------------------------------------------------------------------
// IOT Error
// ------------------------------------------------------------------
// Error handling with an IoT global variable (like errno)
// Also offers a textual version of the error
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \file
 * \brief Iot error handling with an IoT global variable (like errno)
 */

#include <stdio.h>
#include "iotError.h"

int iotError = 0;

/**
 * \brief Converts an IoT error number in the corresponding error iotErrorString
 * \return Error string, or NULL when error number is unknown
 */
char * iotErrorStringRaw( int err ) {
    switch ( err ) {
    case IOT_ERROR_NONE:
        return( "No error" );
        break;

    case IOT_ERROR_AUTHORIZE_TIMEOUT:
        return( "Authorize timeout" );
        break;

    case IOT_ERROR_AUTHORIZE_RESPONSE:
        return( "Authorize response" );
        break;

    case IOT_ERROR_NO_TLS_RECEIVED:
        return( "No TLS received" );
        break;

    case IOT_ERROR_NO_MAC:
        return( "No MAC" );
        break;

    case IOT_ERROR_NON_EXISTING_MAC:
        return( "Non-existing MAC" );
        break;

    case IOT_ERROR_NON_TUNNEL_MAC:
        return( "No tunnel MAC" );
        break;

    case IOT_ERROR_ALREADY_EXISTING_MAC:
        return( "Already existing MAC" );
        break;

    case IOT_ERROR_DEVICE_HAS_NO_GET:
        return( "Device has no get" );
        break;

    case IOT_ERROR_DEVICE_HAS_NO_CONTROLS:
        return( "Device has no controls" );
        break;

    case IOT_ERROR_DEVICE_MISSING:
        return( "Device missing" );
        break;

    case IOT_ERROR_TOPO_ILLEGAL_CMD:
        return( "Topo - illegal command" );
        break;

    case IOT_ERROR_TOPO_MISSING_NAME:
        return( "Topo - missing name" );
        break;

    case IOT_ERROR_TOPO_TIMEOUT:
        return( "Topo - timeout" );
        break;

    case IOT_ERROR_TOPO_NOT_ACCEPTED:
        return( "Topo - not accepted" );
        break;

    case IOT_ERROR_TOPO_OVERRUN:
        return( "Topo - overrun" );
        break;

    case IOT_ERROR_QUEUE_CREATE:
        return( "Queue - create" );
        break;

    case IOT_ERROR_QUEUE_OPEN:
        return( "Queue - open" );
        break;

    case IOT_ERROR_QUEUE_READ:
        return( "Queue - read" );
        break;

    case IOT_ERROR_QUEUE_WRITE:
        return( "Queue - write" );
        break;

//    case IOT_ERROR_QUEUE_NAME:
//        return( "Queue - name" );
//        break;

//    case IOT_ERROR_QUEUE_NOT_FOUND:
//        return( "Queue - not found" );
//        break;

    case IOT_ERROR_QUEUE_BUFSIZE:
        return( "Queue - bufsize" );
        break;

    case IOT_ERROR_QUEUE_OVERRUN:
        return( "Queue - overrun" );
        break;

    case IOT_ERROR_SOCKET_OPEN:
        return( "Socket - open" );
        break;

    case IOT_ERROR_SOCKET_READ:
        return( "Socket - read" );
        break;

    case IOT_ERROR_SOCKET_WRITE:
        return( "Socket - write" );
        break;

    case IOT_ERROR_DB_OPEN:
        return( "DB-open error" );
        break;

    default:
        return( "Unknown IoT error" );
        break;
    }

    return( NULL );
}

/**
 * \brief Get string value for current IoT error number
 * \return IoT error string
 */
char * iotErrorString( void ) {
    return( iotErrorStringRaw( iotError ) );
}
