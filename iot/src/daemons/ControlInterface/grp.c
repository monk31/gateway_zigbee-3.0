// ------------------------------------------------------------------
// Group commands
// ------------------------------------------------------------------
// Parses GRP JSON messages, re-assembles and sends them to the
// Zigbee Coordinator
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup ci
 * \file
 * \brief JSON parser to "grp" messages
 */

#include <stdio.h>
#include <string.h>

#include "iotError.h"
#include "parsing.h"
#include "queue.h"
#include "jsonCreate.h"
#include "grp.h"

// #define GRP_DEBUG

#ifdef GRP_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* GRP_DEBUG */

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------

#define NUMINTATTRS  7
static char * intAttrs[NUMINTATTRS] =
       { "grpid", "scnid", "lvl", "rgb", "kelvin", "xcr", "ycr" };

#define NUMSTRINGATTRS  3
static char * stringAttrs[NUMSTRINGATTRS] = { "mac", "cmd", "scn" };
static int    stringMaxlens[NUMSTRINGATTRS] = { 16,   16,    16 };

// ------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------

/**
 * \brief Init the JSON Parser for this type of commands
 */

void grpInit( void ) {
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        parsingAddIntAttr( intAttrs[i] );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        parsingAddStringAttr( stringAttrs[i], stringMaxlens[i] );
    }
}

#ifdef GRP_DEBUG
static void grpDump ( void ) {
    printf( "Grp dump:\n" );
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
 * \brief JSON Parser for group/scene commands towards Zigbee devices
 * \retval 0 When OK
 * \retval iotError In case of an error
 */

int grpHandle( void ) {
#ifdef GRP_DEBUG
    grpDump();
#endif

    char * cmd = parsingGetStringAttr0( "cmd" );
    char * scn = parsingGetStringAttr0( "scn" );
    int grpid  = parsingGetIntAttr( "grpid" );
    int scnid  = parsingGetIntAttr( "scnid" );
    int lvl    = parsingGetIntAttr( "lvl" );
    int rgb    = parsingGetIntAttr( "rgb" );
    int kelvin = parsingGetIntAttr( "kelvin" );
    int xcr    = parsingGetIntAttr( "xcr" );
    int ycr    = parsingGetIntAttr( "ycr" );

    DEBUG_PRINTF( "Grp grpid=%d, scn=%s, scnid=%d, cmd=%s, lvl=%d, rgb=%d, kelvin=%d, xcr=%d, ycr=%d\n", 
        grpid,
        ( scn != NULL ) ? scn : "NULL", scnid,
        ( cmd != NULL ) ? cmd : "NULL", lvl, rgb, kelvin, xcr, ycr );

    queueWriteOneMessage( QUEUE_KEY_ZCB_IN,
            jsonGroup( grpid, scn, scnid, cmd, lvl, rgb, kelvin, xcr, ycr ) );

    return( iotError == IOT_ERROR_NONE );
}

