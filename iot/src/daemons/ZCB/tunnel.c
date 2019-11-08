// ------------------------------------------------------------------
// TUNNEL
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup zb
 * \file
 * \brief Tunnel handler, using the Appliance Statistics Cluster
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zcb.h"
#include "SerialLink.h"

#include "nibbles.h"
#include "jsonCreate.h"
#include "parsing.h"
#include "dump.h"
#include "tunnel.h"

// #define TUNNEL_DEBUG

#ifdef TUNNEL_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* TUNNEL_DEBUG */

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------
    
#define NUMINTATTRS  1
static char * intAttrs[NUMINTATTRS] = { "close" };

#define NUMSTRINGATTRS  1
static char * stringAttrs[NUMSTRINGATTRS]   = { "open" };
static int    stringMaxlens[NUMSTRINGATTRS] = {  16 };

// ------------------------------------------------------------------
// Global
// ------------------------------------------------------------------

static uint16_t u16TunnelSaddr = 0;

// ------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------

/**
 * \brief Init the JSON Parser for this type of commands
 */

void tunnelInit( void ) {
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        parsingAddIntAttr( intAttrs[i] );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        parsingAddStringAttr( stringAttrs[i], stringMaxlens[i] );
    }
}

#ifdef TUNNEL_DEBUG
static void tunnelDump ( void ) {
    printf( "Tunnel dump:\n" );
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
// Handle
// ------------------------------------------------------------------

/**
 * \brief JSON Parser for tunnel commands (e.g. open, close).
 */

void tunnelHandle( void ) {
#ifdef TUNNEL_DEBUG
    tunnelDump();
#endif

    char * mac = parsingGetStringAttr( "open" );
    if ( !isEmptyString( mac ) ) {
        uint16_t u16ShortAddress;
        if ( ( u16ShortAddress = zcbNodeGetShortAddress( mac ) ) != 0xFFFF ) {
            u16TunnelSaddr = u16ShortAddress;
            DEBUG_PRINTF( "Tunnel opened to 0x%016llX\n", mac );
        } else {
            printf( "Tunnel short address not found for %s\n", mac );
            /** \todo Tunnel short address not found: Ignored for the time being since we only have 1 manager */
            // u16TunnelSaddr = 0;
            // End of TODO
        }
    }

    int close = parsingGetIntAttr( "close" );
    if ( close >= 0 ) {
        /** \todo Tunnel close: Ignored for the time being since we only have 1 manager */
        // u16TunnelSaddr = 0;
        // End of TODO
        DEBUG_PRINTF( "Tunnel closed\n" );
    }
}

// ------------------------------------------------------------------
// Tunnel Send message
// ------------------------------------------------------------------

/**
 * \brief Sends a message over the ZCB-JenOS tunnel using the Appliance Statistics Cluster
 */

int tunnelSend( char * message ) {
    int ret = 0;
    static uint32_t u32LogId = 0;
    uint8_t u8SequenceNo;

    int len = strlen( message );

    char buf[CLD_APPLIANCE_STATISTICS_ATTR_LOG_MAX_SIZE+2];
    if ( len >= CLD_APPLIANCE_STATISTICS_ATTR_LOG_MAX_SIZE ) {
        // Try to compress message in search for white spaces
        int i=0, j=0;
        char c;
        while ( i < len ) {
            c = message[i++];
            if ( c == ' ' || c == '\t' ) {
                // skip
            } else if ( j < CLD_APPLIANCE_STATISTICS_ATTR_LOG_MAX_SIZE ) {
                buf[j++] = c;
            }
        }
        if ( j <= CLD_APPLIANCE_STATISTICS_ATTR_LOG_MAX_SIZE ) {
	    buf[j] = '\0';
            message = buf;
            len = strlen( message );
	}
    }

    if ( len >= CLD_APPLIANCE_STATISTICS_ATTR_LOG_MAX_SIZE ) {
        printf( "Tunnel message too long for ASC cluster (%d > %d)\n",
                len, CLD_APPLIANCE_STATISTICS_ATTR_LOG_MAX_SIZE );

    } else if ( u16TunnelSaddr ) {
        DEBUG_PRINTF( "Sending following message over tunnel 0x%04X:\n\t%s",
            u16TunnelSaddr, message );

        typedef struct {
            uint32_t    utctTime;
            uint32_t    u32LogId;
            uint32_t    u32LogLength;
            uint8_t     pu8LogData[CLD_APPLIANCE_STATISTICS_ATTR_LOG_MAX_SIZE];
        } __attribute__((__packed__)) logData_t;

        struct {
            uint8_t     u8TargetAddressMode;
            uint16_t    u16TargetAddress;
            uint8_t     u8SourceEndpoint;
            uint8_t     u8DestinationEndpoint;
            logData_t   payload;
        } __attribute__((__packed__)) sLogMessage;


        // sLogMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        sLogMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        sLogMessage.u16TargetAddress      = htons(u16TunnelSaddr);
        sLogMessage.u8SourceEndpoint      = ZB_ENDPOINT_TUNNEL;
        sLogMessage.u8DestinationEndpoint = ZB_ENDPOINT_TUNNEL;

        sLogMessage.payload.utctTime     = 0xFFFFFFFF;              // insert an invalid UTC Time
        sLogMessage.payload.u32LogId     = htonl( u32LogId++ );
        sLogMessage.payload.u32LogLength = htonl( len );
        memset( (char *)sLogMessage.payload.pu8LogData, 0, CLD_APPLIANCE_STATISTICS_ATTR_LOG_MAX_SIZE );
        strcpy( (char *)sLogMessage.payload.pu8LogData, message );
	
#ifdef TUNNEL_DEBUG
	printf("sLogMessage:\n" );
	dump( (char *)&sLogMessage, sizeof(sLogMessage) );
#endif
	
	int err;
        if ( (err = eSL_SendMessageNoWait(E_SL_MSG_ASC_LOG_MSG, sizeof(sLogMessage),
                &sLogMessage, &u8SequenceNo) ) != E_SL_OK) {
            if ( err != E_SL_NOMESSAGE ) printf( "Error %d sending ASC log message\n", err );
        } else {
            ret = 1;
        }

    } else {
        printf( "Error: Tunnel not open\n" );
    }
    return( ret );
}

// ------------------------------------------------------------------
// End of file
// ------------------------------------------------------------------

