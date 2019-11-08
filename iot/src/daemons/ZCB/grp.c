// ------------------------------------------------------------------
// GROUP
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup zb
 * \file
 * \brief JSON parser to "grp" messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zcb.h"
#include "lmpgrp.h"
#include "SerialLink.h"

#include "nibbles.h"
#include "jsonCreate.h"
#include "parsing.h"
#include "dump.h"
#include "grp.h"

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------
    
#define NUMINTATTRS  5
static char * intAttrs[NUMINTATTRS] = { "lvl", "xcr", "ycr", "grpid", "scnid" };

#define NUMSTRINGATTRS  2
static char * stringAttrs[NUMSTRINGATTRS]   = { "cmd", "scn" };
static int    stringMaxlens[NUMSTRINGATTRS] = {   6,    8 };

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

void grpDump ( void ) {
    printf( "Group dump:\n" );
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
 * \brief JSON Parser for group/scene commands, which are typically sent to Zigbee lamps
 */

void grpHandle( void ) {
    grpDump();

    int gid = parsingGetIntAttr( "grpid" );
    if ( gid >= 0 ) {

        char * cmd = parsingGetStringAttr0( "cmd" );
        char * scn = parsingGetStringAttr0( "scn" );

        if ( cmd ) {
            // on, off
            printf( "Group cmd %s\n", cmd );
            if ( strcmp( cmd, "on" ) == 0 ) {
                lmpgrp_OnOff( 0xFFFF, gid, 1 );
            } else if ( strcmp( cmd, "off" ) == 0 ) {
                lmpgrp_OnOff( 0xFFFF, gid, 0 );
            }
        }

        if ( scn ) {
            // recall <sid>, remall
            if ( strcmp( scn, "recall" ) == 0 ) {
                int sid = parsingGetIntAttr( "scnid" );
                if ( sid >= 0 ) {
                    printf( "Group scn %s, %d\n", scn, sid );
                    eZCB_RecallScene( 0xFFFF, gid, sid );
                }
            } else if ( strcmp( scn, "remall" ) == 0 ) {
                printf( "Group scn %s\n", scn );
                eZCB_RemoveAllScenes( 0xFFFF, gid );
            }
        }

        int lvl = parsingGetIntAttr( "lvl" );
        if ( lvl >= 0 ) {
            printf( "Group level %d\n", lvl );
            
            // In JSON we specify level from 0-100
            // In Zigbee, however, level has to be specified from 0-255
            lvl = ( lvl * 255 ) / 100;
            lmpgrp_MoveToLevel( 0xFFFF, gid, lvl, 5 );
        }

        int xcr = parsingGetIntAttr( "xcr" );
        int ycr = parsingGetIntAttr( "ycr" );
        if ( xcr >= 0 && ycr >= 0 ) {
            printf( "Group color %d, %d\n", xcr, ycr );
            lmpgrp_MoveToColour( 0xFFFF, gid, xcr, ycr, 5 );
        }
    }
}

