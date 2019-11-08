// ------------------------------------------------------------------
// Lamp commands (for group/scenes)
// ------------------------------------------------------------------
// Parses LMP JSON messages, re-assembles and sends them to the
// Zigbee Coordinator
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup ci
 * \file
 * \brief JSON parser to "lmp" messages
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "iotError.h"
#include "atoi.h"
#include "parsing.h"
#include "queue.h"
#include "jsonCreate.h"
#include "lmp.h"

// #define LMP_DEBUG

#ifdef LMP_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* LMP_DEBUG */

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------

#define NUMINTATTRS  2
static char * intAttrs[NUMINTATTRS] = { "grpid", "scnid" };

#define NUMSTRINGATTRS  3
static char * stringAttrs[NUMSTRINGATTRS] = { "mac", "grp", "scn" };
static int    stringMaxlens[NUMSTRINGATTRS] = { 16,   16,    16 };

// ------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------

/**
 * \brief Init the JSON Parser for this type of commands
 */

void lmpInit( void ) {
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        parsingAddIntAttr( intAttrs[i] );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        parsingAddStringAttr( stringAttrs[i], stringMaxlens[i] );
    }
}

#ifdef LMP_DEBUG
static void lmpDump ( void ) {
    printf( "Lamp dump:\n" );
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
 * \brief JSON Parser for commands to Zigbee lamp devices
 * \retval 0 When OK
 * \retval iotError In case of an error
 */

int lmpHandle( void ) {
#ifdef LMP_DEBUG
    lmpDump();
#endif

    char * mac = parsingGetStringAttr0( "mac" );
    char * grp = parsingGetStringAttr0( "grp" );
    char * scn = parsingGetStringAttr0( "scn" );
    int grpid  = parsingGetIntAttr( "grpid" );
    int scnid  = parsingGetIntAttr( "scnid" );

    DEBUG_PRINTF( "Lamp mac=%s, grp=%s, grpid=%d, scn=%s, scnid=%d\n", 
        ( mac != NULL ) ? mac : "NULL", 
        ( grp != NULL ) ? grp : "NULL",
        grpid,
        ( scn != NULL ) ? scn : "NULL",
        scnid );

    if ( grp || ( grpid > 0 ) || scn || ( scnid > 0 ) ) {
        queueWriteOneMessage( QUEUE_KEY_ZCB_IN,
                              jsonLampGroup( mac, grp, grpid, scn, scnid ) );
    }

    return( iotError == IOT_ERROR_NONE );
}

