// ------------------------------------------------------------------
// Topo
// ------------------------------------------------------------------
// Handle topology requests and send through tunnel mechanism
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup zb
 * \file
 * \brief JSON parser to "tp" and "tp+" messages
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "SerialLink.h"

#include "iotError.h"
#include "atoi.h"
#include "parsing.h"
#include "dump.h"
#include "tunnel.h"
#include "topo.h"

#include "queue.h"
#include "json.h"
#include "jsonCreate.h"
#include "newLog.h"

// #define TOPO_DEBUG
// #define TIMING_DEBUG

#ifdef TOPO_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* TOPO_DEBUG */


// ------------------------------------------------------------------
// Typing
// ------------------------------------------------------------------
    
#if 0
    typedef struct {
        uint32_t u32LogId;
    } tsCLD_ASC_Log_1;       // RequestPayload;

    typedef struct {
        uint32_t utctTime;
        uint32_t u32LogId;
        uint32_t u32LogLength;
        uint8_t  pu8LogData[CLD_APPLIANCE_STATISTICS_ATTR_LOG_MAX_SIZE];
    } tsCLD_ASC_Log_2;       // Notification OR ResponsePayload;

    typedef struct {
        uint8_t  u8LogQueueSize;
        uint32_t u32LogId;
    } tsCLD_ASC_Log_3;       // QueueResponse OR StatisticsAvailablePayload;

    typedef struct _AppStat {
        uint8_t u8CommandId;
        union {
            tsCLD_ASC_Log_1 sLog_1;
            tsCLD_ASC_Log_2 sLog_2;
            tsCLD_ASC_Log_3 sLog_3;
        } uMessage;
    } __attribute__((__packed__)) AppStat_t;
#endif
    

/* Server Command codes */    
typedef enum 
{
    E_CLD_APPLIANCE_STATISTICS_CMD_LOG_NOTIFICATION        = 0x00,    /* Mandatory */
    E_CLD_APPLIANCE_STATISTICS_CMD_LOG_RESPONSE,                      /* Mandatory */
    E_CLD_APPLIANCE_STATISTICS_CMD_LOG_QUEUE_RESPONSE,                /* Mandatory */
    E_CLD_APPLIANCE_STATISTICS_CMD_STATISTICS_AVAILABLE               /* Mandatory */
} teCLD_ApplianceStatistics_ServerCommandId;

/* Log Notification & Log Response Payload */
typedef struct
{
    uint32_t   utctTime;
    uint32_t   u32LogId;
    uint32_t   u32LogLength;
    uint8_t    pu8LogData[CLD_APPLIANCE_STATISTICS_ATTR_LOG_MAX_SIZE];
} tsCLD_ASC_LogNotificationORLogResponsePayload;

/* Log Queue Response  & Statistics Available Payload */
typedef struct
{
    uint8_t    u8LogQueueSize;
    uint32_t   u32LogId;
} tsCLD_ASC_LogQueueResponseORStatisticsAvailablePayload;

/* Log Request Payload */
typedef struct
{
    uint32_t   u32LogId;
} tsCLD_ASC_LogRequestPayload;

typedef struct
{
    uint8_t  u8SeqNo;
    uint8_t  u8EndPoint;
    uint16_t u16ClusterId;
    uint8_t  u8CommandId;
    union
    {
        tsCLD_ASC_LogNotificationORLogResponsePayload            sLogNotificationORLogResponsePayload;
        tsCLD_ASC_LogQueueResponseORStatisticsAvailablePayload   sLogQueueResponseORStatisticsAvailabePayload;
        tsCLD_ASC_LogRequestPayload                              sLogRequestPayload;
    } uMessage;
} __attribute__((__packed__)) tsCLD_ApplianceStatisticsCallBackMessage;

// ------------------------------------------------------------------
// External prototype
// ------------------------------------------------------------------

extern void dispatchMessage( int queue, char * message );

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------

#define NUMINTATTRS  1
static char * intAttrs[NUMINTATTRS] = { "sid" };

#define NUMSTRINGATTRS  9
static char * stringAttrs[NUMSTRINGATTRS] =
    { "nm", "ty", "cmd", "man", "ui", "sen", "pmp", "lmp", "plg" };
static int    stringMaxlens[NUMSTRINGATTRS] =
    {  16,   8,    12,    16,    16,   16,    16,    16,    16 };

// ------------------------------------------------------------------
// Globals
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// Topo Response Handler
// ------------------------------------------------------------------

/**
 * \brief Listener that gets called when topo responses come in. These messages are send to
 * the IoT Control Interface by the external "dispatchMessage" function.
 */

static void HandleAppStatResponse(void *pvUser, uint16_t u16Length, void *pvMessage) {
    DEBUG_PRINTF( "\n************ HandleAppStatResponse\n" );
    // dump( (char *)pvMessage, u16Length+4 );

    tsCLD_ApplianceStatisticsCallBackMessage *pAscResponse = (tsCLD_ApplianceStatisticsCallBackMessage *)pvMessage;

    struct timeval now;
    gettimeofday( &now, NULL );
 
    DEBUG_PRINTF( "Got appliance statistics message @ %d sec, %d msec:\n", (int)now.tv_sec, (int)now.tv_usec/1000 );
    DEBUG_PRINTF( "- seqno     %d\n",     pAscResponse->u8SeqNo );
    DEBUG_PRINTF( "- endpoint  %d\n",     pAscResponse->u8EndPoint );
    DEBUG_PRINTF( "- clusterid 0x%04x\n", ntohs( pAscResponse->u16ClusterId ) );
    DEBUG_PRINTF( "- command   %d\n",     pAscResponse->u8CommandId );

    if ( pAscResponse->u8CommandId == E_CLD_APPLIANCE_STATISTICS_CMD_LOG_NOTIFICATION ) {
        char * mess = (char *)pAscResponse->uMessage.sLogNotificationORLogResponsePayload.pu8LogData;

#ifdef TOPO_DEBUG
        uint32_t u32LogLength =
            ntohl( pAscResponse->uMessage.sLogNotificationORLogResponsePayload.u32LogLength );
        DEBUG_PRINTF( "Log notification:\n" );
        DEBUG_PRINTF( "- timestamp 0x%08x\n", ntohl( pAscResponse->uMessage.sLogNotificationORLogResponsePayload.utctTime ) );
        DEBUG_PRINTF( "- id        %d\n", ntohl( pAscResponse->uMessage.sLogNotificationORLogResponsePayload.u32LogId ) );
        DEBUG_PRINTF( "- length    %d\n", u32LogLength );
        dump( mess, u32LogLength );
#endif

        dispatchMessage( QUEUE_KEY_CONTROL_INTERFACE, mess );
    } 
}

// ------------------------------------------------------------------
// Init
// ------------------------------------------------------------------

/**
 * \brief Init the JSON Parser for this type of commands. Also adds a listener for the topo responses.
 */

void topoInit( void ) {
    // printf( "Topo init\n" );
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        parsingAddIntAttr( intAttrs[i] );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        parsingAddStringAttr( stringAttrs[i], stringMaxlens[i] );
    }

    eSL_AddListener(E_SL_MSG_ASC_LOG_MSG_RESPONSE, HandleAppStatResponse,  NULL);
}

// ------------------------------------------------------------------
// Dump
// ------------------------------------------------------------------

#ifdef TOPO_DEBUG
static void topoDump( void ) {
    printf( "Topo dump:\n" );
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
// Handler
// ------------------------------------------------------------------

/**
 * \brief JSON Parser for topo commands, which are typically sent to managers via the ZCB-JenOS tunnel.
 * Responses are caught by the listener that was added during init.
 */

int topoHandle( void ) {
    int status = 0;
    int error  = IOT_ERROR_NONE;

#ifdef TOPO_DEBUG
    topoDump();
#endif
    
#ifdef TIMING_DEBUG
    struct timeval start;
    struct timeval now;
    double elapsed;

    gettimeofday( &start, NULL );
#endif
    
    char * cmd = parsingGetStringAttr0( "cmd" );
    if ( cmd != NULL ) {

        if ( strcmp( cmd, "clearall" ) == 0 ) {
            DEBUG_PRINTF( "Drop topo\n" );
            if ( !tunnelSend( jsonTopoClear() ) ) error = IOT_ERROR_TUNNEL_SEND;

        } else if ( strcmp( cmd, "endconf" ) == 0 ) {
            DEBUG_PRINTF( "Accept topo\n" );
            if ( !tunnelSend( jsonTopoEnd() ) ) error = IOT_ERROR_TUNNEL_SEND;

        } else {
            error = IOT_ERROR_TOPO_ILLEGAL_CMD;
        }

    } else {
        char * nm = parsingGetStringAttr0( "nm" );

        // Check if the name is set
        if ( nm != NULL ) {

            // printf( "Topo - nm = %d\n", nm );

            char * mac;

            if ( ( mac = parsingGetStringAttr0( "man" ) ) != NULL ) {
                char * type = parsingGetStringAttr0( "ty" );
                if ( !tunnelSend( jsonTopoAddManager( mac, nm, type ) ) ) error = IOT_ERROR_TUNNEL_SEND;

            } else if ( ( mac = parsingGetStringAttr0( "ui" ) ) != NULL ) {
                if ( !tunnelSend( jsonTopoAddUI( mac, nm ) ) ) error = IOT_ERROR_TUNNEL_SEND;

            } else if ( ( mac = parsingGetStringAttr0( "sen" ) ) != NULL ) {
                if ( !tunnelSend( jsonTopoAddSensor( mac, nm ) ) ) error = IOT_ERROR_TUNNEL_SEND;

            } else if ( ( mac = parsingGetStringAttr0( "pmp" ) ) != NULL ) {
                int sid = parsingGetIntAttr( "sid" );
                if ( !tunnelSend( jsonTopoAddPump( mac, nm, sid ) ) ) error = IOT_ERROR_TUNNEL_SEND;

            } else if ( ( mac = parsingGetStringAttr0( "lmp" ) ) != NULL ) {
                char * type = parsingGetStringAttr0( "ty" );
                if ( !tunnelSend( jsonTopoAddLamp( mac, nm, type ) ) ) error = IOT_ERROR_TUNNEL_SEND;

            } else if ( ( mac = parsingGetStringAttr0( "plg" ) ) != NULL ) {
                if ( !tunnelSend( jsonTopoAddPlug( mac, nm ) ) ) error = IOT_ERROR_TUNNEL_SEND;

            }
        } else {
            printf( "No name specified\n" );
            error = IOT_ERROR_TOPO_MISSING_NAME;
        }
    }

#ifdef TIMING_DEBUG
    gettimeofday( &now, NULL );
    elapsed = (double)(now.tv_sec - start.tv_sec) + 
        ((double)(now.tv_usec - start.tv_usec) / 1000000.0F);
    printf( "TOPO handle: Elapsed REAL time: %.2f seconds @ %d sec, %d msec\n\n", elapsed, (int)now.tv_sec, (int)now.tv_usec/1000 );
#endif
    
    if ( error != IOT_ERROR_NONE ) {
        iotError = error;
        return( -1 );
    }

    return( status );
}

// ------------------------------------------------------------------
// End of file
// ------------------------------------------------------------------

