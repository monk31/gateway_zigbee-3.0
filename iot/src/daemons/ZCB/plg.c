// ------------------------------------------------------------------
// PLUG
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup zb
 * \file
 * \brief JSON parser to "plg" messages
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
#include "plg.h"

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------
    
#define NUMINTATTRS  0
static char * intAttrs[NUMINTATTRS] = { };

#define NUMSTRINGATTRS  2
static char * stringAttrs[NUMSTRINGATTRS]   = { "mac", "cmd" };
static int    stringMaxlens[NUMSTRINGATTRS] = {   16,    6 };

// ------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------

/**
 * \brief Init the JSON Parser for this type of commands
 */

void plgInit( void ) {
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        parsingAddIntAttr( intAttrs[i] );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        parsingAddStringAttr( stringAttrs[i], stringMaxlens[i] );
    }
}

static void plgDump ( void ) {
    printf( "Plug dump:\n" );
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        printf( "- %-12s = %d\n", intAttrs[i], parsingGetIntAttr( intAttrs[i] ) );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        printf( "- %-12s = %s\n", stringAttrs[i], parsingGetStringAttr( stringAttrs[i] ) );
    }
}

// ------------------------------------------------------------------
// Handle
// ------------------------------------------------------------------

/**
 * \brief JSON Parser for plg commands (e.g. on/off), which are typically sent to Zigbee plug meters
 */

void plgHandle(void) {
    plgDump();

    char * mac = parsingGetStringAttr( "mac" ); //YB pou essai
    if ( !isEmptyString( mac ) ) {

        uint16_t u16ShortAddress;
        if ( ( u16ShortAddress = zcbNodeGetShortAddress( mac ) ) != 0xFFFF ) {
            printf( "Node found, address = 0x%04x\n", u16ShortAddress );

            char * cmd = parsingGetStringAttr( "cmd" );
            if ( !isEmptyString( cmd ) ) {

                int mode = ( strcmp( cmd, "on" ) == 0 );

                eOnOff( u16ShortAddress, mode );
            }

        } else {
            printf( "Node not found\n" );
        }
    }
}

// ------------------------------------------------------------------
// End of file
// ------------------------------------------------------------------

