// ------------------------------------------------------------------
// dbGet
// ------------------------------------------------------------------
// Parses DBGET requests and returns requested info out of the DB
// back over the client socket
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup ci
 * \file
 * \brief JSON parser to "dbget" messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "atoi.h"
#include "jsonCreate.h"
#include "parsing.h"
#include "newDb.h"
#include "newLog.h"
#include "dbget.h"
#include "plugUsage.h"
#include "strtoupper.h"
#include "iotError.h"
#include "socket.h"

// #define DBGET_DEBUG
// #define DBGET_DEBUG_TIMING

#ifdef DBGET_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* DBGET_DEBUG */

#define MAXBUF    6000

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------

#define NUMINTATTRS  2
static char * intAttrs[NUMINTATTRS] = { "room", "ts" };

#define NUMSTRINGATTRS  1
static char * stringAttrs[NUMSTRINGATTRS] = { "mac" };
static int    stringMaxlens[NUMSTRINGATTRS] = { 16 };
static char   responseBuf[MAXBUF];
static int    responseBufSize;

// ------------------------------------------------------------------
// Socket helper
// ------------------------------------------------------------------

extern void sendResponse( char * jsonResponseString );

// ------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------

/**
 * \brief Init the JSON Parser for this type of commands
 */

void dbgetInit( void ) {
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        parsingAddIntAttr( intAttrs[i] );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        parsingAddStringAttr( stringAttrs[i], stringMaxlens[i] );
    }
}

#ifdef DBGET_DEBUG
static void dbgetDump ( void ) {
    printf( "dbGet dump:\n" );
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        printf( "- %-12s = %d\n", intAttrs[i], parsingGetIntAttr( intAttrs[i] ) );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        printf( "- %-12s = %s\n", stringAttrs[i], parsingGetStringAttr( stringAttrs[i] ) );
    }
}
#endif

// ------------------------------------------------------------------
// Add string to response message
// ------------------------------------------------------------------

void clearResponse( void ) {
    responseBuf[0] = '\0';
    responseBufSize = 0;
}

int addToResponse( char * str ) {
    int len = strlen( str );
    if ( ( len + responseBufSize ) < MAXBUF ) {
        strcat( responseBuf, str );
        responseBufSize += len;
        return( 1 );
    }
    return( 0 );
}

// ------------------------------------------------------------------
// Callbacks
// ------------------------------------------------------------------
/*
static int cbDevs( newdb_dev_t * pdev ) {

    int joined = -1;
    if ( pdev->flags >= 0 ) joined = ( pdev->flags & FLAG_DEV_JOINED ) != 0;

    devCount++;

    switch ( pdev->dev ) {
    case DEVICE_DEV_MANAGER:
        DEBUG_PRINTF( "Manager = %s\n", pdev->mac );
        addToResponse( jsonManager( pdev->id, pdev->mac, pdev->nm, pdev->rm, pdev->ty, joined ) );
        break;

    case DEVICE_DEV_UISENSOR:
        DEBUG_PRINTF( "UI-Sensor = %s\n", pdev->mac );
        addToResponse( jsonUI( pdev->id, pdev->mac, pdev->nm, pdev->rm, pdev->par, pdev->heat, pdev->cool, joined ) );
        addToResponse( jsonSensor( pdev->id, pdev->mac, pdev->nm, pdev->rm, pdev->par,
                                  pdev->tmp, pdev->hum, pdev->prs, pdev->co2, pdev->bat, pdev->batl, pdev->als,
                                  pdev->xloc, pdev->yloc, pdev->zloc, joined ) );
        break;

    case DEVICE_DEV_UI:
        DEBUG_PRINTF( "UI = %s\n", pdev->mac );
        addToResponse( jsonUI( pdev->id, pdev->mac, pdev->nm, pdev->rm, pdev->par, pdev->heat, pdev->cool, joined ) );
        break;

    case DEVICE_DEV_SENSOR:
        DEBUG_PRINTF( "Sensor = %s\n", pdev->mac );
        addToResponse( jsonSensor( pdev->id, pdev->mac, pdev->nm, pdev->rm, pdev->par,
                                  pdev->tmp, pdev->hum, pdev->prs, pdev->co2, pdev->bat, pdev->batl, pdev->als,
                                  pdev->xloc, pdev->yloc, pdev->zloc, joined ) );
        break;

    case DEVICE_DEV_PUMP:
        DEBUG_PRINTF( "Pump = %s\n", pdev->mac );
        addToResponse( jsonPump( pdev->id, pdev->mac, pdev->nm, pdev->rm, pdev->par, pdev->sid, pdev->cmd, pdev->lvl, joined ) );
        break;

    case DEVICE_DEV_LAMP:
        DEBUG_PRINTF( "Lamp = %s\n", pdev->mac );
        addToResponse( jsonLamp( pdev->id, pdev->mac, pdev->nm, pdev->rm, pdev->ty,
                                pdev->cmd, pdev->lvl, pdev->rgb, pdev->kelvin, joined ) );
        break;

    case DEVICE_DEV_PLUG:
        DEBUG_PRINTF( "Plug = %s\n", pdev->mac );
        addToResponse( jsonPlug( pdev->id, pdev->mac, pdev->nm, pdev->rm, pdev->cmd, pdev->act, pdev->sum, -1, joined, -1 ) );
        break;
        
    case DEVICE_DEV_SWITCH:
        // Ignore
        break;

    default:
        printf( "DbGet - unknown device (%d)\n", pdev->dev );
        devCount--;
        break;
    }
    return( 1 );
}
*/
// ------------------------------------------------------------------
// Handle
// ------------------------------------------------------------------

/**
 * \brief JSON Parser for get commands of the IoT database (e.g. per room or device).
 * Builds a response message which is sent to the caller via the (external) sendResponse-function.
 * \retval 0 When OK
 * \retval iotError In case of an error
 */

int dbgetHandle( void ) {
#ifdef DBGET_DEBUG
    dbgetDump();
#endif

    int error = IOT_ERROR_NONE;
    int dayHistory[24];
    
#ifdef DBGET_DEBUG_TIMING
    struct timeval start;
    struct timeval end;
    double elapsed;

    gettimeofday( &start, NULL );
#endif
    
    clearResponse();

    {
        // Single device mode

        char * mac = parsingGetStringAttr0( "mac" );

        if ( mac != NULL ) {
            mac = strtoupper( mac );
            DEBUG_PRINTF( "MAC = %s\n", mac );
            
            newdb_dev_t device;
            if ( newDbGetDevice( mac, &device ) ) {

                int joined = -1;
                if ( device.flags >= 0 ) joined = ( device.flags & FLAG_DEV_JOINED ) != 0;

                // Build the response
                switch ( device.dev ) {
                case DEVICE_DEV_MANAGER:
                    addToResponse( jsonManager( -1, device.mac, NULL, -1, NULL, joined ) );
                    break;

                case DEVICE_DEV_UISENSOR:
                    addToResponse( jsonUI( -1, device.mac, NULL, -1, -1,
                                        device.heat, device.cool, joined ) );
                    addToResponse( jsonSensor( -1, device.mac, NULL, -1, -1,
                                            device.tmp, device.hum, device.prs,
                                            device.co2, device.bat, device.batl, device.als,
                                            device.xloc, device.yloc, device.zloc, joined ) );
                    break;

                case DEVICE_DEV_UI:
                    addToResponse( jsonUI( -1, device.mac, NULL, -1, -1,
                                        device.heat, device.cool, joined ) );
                    break;

                case DEVICE_DEV_SENSOR:
                    addToResponse( jsonSensor( -1, device.mac, NULL, -1, -1,
                                            device.tmp, device.hum, device.prs,
                                            device.co2, device.bat, device.batl, device.als,
                                            device.xloc, device.yloc, device.zloc, joined ) );
                    break;

                case DEVICE_DEV_PUMP:
                    addToResponse( jsonPump( -1, device.mac, NULL, -1, -1,
                                            device.sid, device.cmd, device.lvl, joined ) );
                    break;

                case DEVICE_DEV_LAMP:
                    addToResponse( jsonLamp( -1, device.mac, NULL, -1,
                                            NULL, device.cmd, device.lvl, device.rgb, device.kelvin, joined ) );
                    break;

                case DEVICE_DEV_PLUG:
                    plugGetHistory( device.mac, (int)time(NULL), 60, 24, dayHistory );
                    int plug_h24 = dayHistory[0] - dayHistory[23];
                    addToResponse( jsonPlug( -1, device.mac, NULL, -1,
                                            device.cmd, device.act, device.sum, plug_h24, joined, -1 ) );
                    break;
                    
                case DEVICE_DEV_SWITCH:
                    // Ignore
                    break;
    
                default:
                    // Error handling
                    if ( device.dev < 0 ) {
                        error = IOT_ERROR_NON_EXISTING_MAC;
                    } else {
                        error = IOT_ERROR_DEVICE_HAS_NO_GET;
                    }
                    break;
                }
            }

        } else {
          error = IOT_ERROR_NO_MAC;
        }
    }

#ifdef DBGET_DEBUG_TIMING
    gettimeofday( &end, NULL );

    elapsed = (double)(end.tv_sec - start.tv_sec) + 
            ((double)(end.tv_usec - start.tv_usec) / 1000000.0F);

    printf( "DBGET Elapsed REAL time: %.2f seconds (before send)\n\n", elapsed );
#endif
                        
    if ( error == IOT_ERROR_NONE ) {
        // Now send the responses
        if ( responseBufSize > 0 ) {
            // printf( "Sending dbget responses (%d):\n%s\n", responseBufSize, responseBuf );
            sendResponse( responseBuf );
        }
    } else {
      iotError = error;
    }

#ifdef DBGET_DEBUG_TIMING
    gettimeofday( &end, NULL );

    elapsed = (double)(end.tv_sec - start.tv_sec) + 
            ((double)(end.tv_usec - start.tv_usec) / 1000000.0F);

    printf( "DBGET Elapsed REAL time: %.2f seconds (after send)\n\n", elapsed );
#endif
                        
    return( iotError == IOT_ERROR_NONE );
}

