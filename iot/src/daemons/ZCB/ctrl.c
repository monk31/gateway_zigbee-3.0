// ------------------------------------------------------------------
// CTRL
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup zb
 * \file
 * \brief JSON parser to "ctrl" messages
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
#include "ctrl.h"

// #define CTRL_DEBUG

#ifdef CTRL_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* CTRL_DEBUG */

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------
    
#define NUMINTATTRS  2
static char * intAttrs[NUMINTATTRS] = { "heat", "cool" };

#define NUMSTRINGATTRS  1
static char * stringAttrs[NUMSTRINGATTRS]   = { "mac" };
static int    stringMaxlens[NUMSTRINGATTRS] = {  16 };

// ------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------

/**
 * \brief Init the JSON Parser for this type of commands
 */

void ctrlInit( void ) {
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        parsingAddIntAttr( intAttrs[i] );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        parsingAddStringAttr( stringAttrs[i], stringMaxlens[i] );
    }
}

#ifdef CTRL_DEBUG
static void ctrlDump ( void ) {
    printf( "Ctrl dump:\n" );
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
 * \brief JSON Parser for ctrl commands, which are typically new setpoints (e.g. cool). 
 * Commands are typically sent to Zigbee devices directly or to managers via the ZCB-JenOS tunnel.
 */

void ctrlHandle( void ) {
#ifdef CTRL_DEBUG
    ctrlDump();
#endif

    char * mac = parsingGetStringAttr( "mac" );
    if ( !isEmptyString( mac ) ) {

        uint16_t u16ShortAddress;
        if ( ( u16ShortAddress = zcbNodeGetShortAddress( mac ) ) != 0xFFFF ) {
            DEBUG_PRINTF( "Node found, address = 0x%04x\n", u16ShortAddress );

            int heat = parsingGetIntAttr( "heat" );
            int cool = parsingGetIntAttr( "cool" );
            if ( heat >= 0 && cool >= 0 ) {
                DEBUG_PRINTF( "UI control %d, %d\n", heat, cool );
                tunnelSend( jsonUiCtrl( mac, heat, cool ) );
            }

        } else {
            printf( "Node not found\n" );
        }
    }
}

// ------------------------------------------------------------------
// End of file
// ------------------------------------------------------------------

