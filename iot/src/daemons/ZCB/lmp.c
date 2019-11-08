// ------------------------------------------------------------------
// LAMP
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup zb
 * \file
 * \brief JSON parser to "lmp" messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zcb.h"
#include "lmpgrp.h"

#include "nibbles.h"
#include "jsonCreate.h"
#include "parsing.h"
#include "dump.h"
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
    
#define NUMINTATTRS  6
static char * intAttrs[NUMINTATTRS] = { "lvl", "xcr", "ycr", "grpid", "scnid", "kelvin" };

#define NUMSTRINGATTRS  4
static char * stringAttrs[NUMSTRINGATTRS]   = { "mac", "cmd", "grp", "scn" };
static int    stringMaxlens[NUMSTRINGATTRS] = {   16,    6,     8,     8 };

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
 * \brief JSON Parser for lmp commands (e.g. color), which are typically sent to Zigbee lamps
 */

void lmpHandle( void ) {
#ifdef LMP_DEBUG
    lmpDump();
#endif

    char * mac = parsingGetStringAttr( "mac" );
    if ( !isEmptyString( mac ) ) {

        uint16_t u16ShortAddress;
        if ( ( u16ShortAddress = zcbNodeGetShortAddress( mac ) ) != 0xFFFF ) {
            DEBUG_PRINTF( "Node found, address = 0x%04x\n", u16ShortAddress );

            char * cmd = parsingGetStringAttr0( "cmd" );
            if ( cmd ) {
                DEBUG_PRINTF( "Lamp command %s\n", cmd );

                int mode = ( strcmp( cmd, "on" ) == 0 );

                lmpgrp_OnOff( u16ShortAddress, 0, mode );
            }

            int lvl = parsingGetIntAttr( "lvl" );
            if ( lvl >= 0 ) {
                DEBUG_PRINTF( "Lamp level %d\n", lvl );

                // In JSON we specify level from 0-100
                // In Zigbee, however, level has to be specified from 0-255
                lvl = ( lvl * 255 ) / 100;
                lmpgrp_MoveToLevel( u16ShortAddress, 0, lvl, 5 );
            }

            int xcr = parsingGetIntAttr( "xcr" );
            int ycr = parsingGetIntAttr( "ycr" );
            if ( xcr >= 0 && ycr >= 0 ) {
                DEBUG_PRINTF( "Lamp color %d, %d\n", xcr, ycr );

                lmpgrp_MoveToColour( u16ShortAddress, 0, xcr, ycr, 5 );
            }
            
            int kelvin = parsingGetIntAttr( "kelvin" );
            if ( kelvin > 0 ) {
                DEBUG_PRINTF( "Lamp kelvin %d\n", kelvin );
		// According to Zigbee, the temp is 1000000/kelvin
                lmpgrp_MoveToColourTemperature( u16ShortAddress, 0, 1000000 / kelvin, 5 );
	    }

            char * grp = parsingGetStringAttr0( "grp" );
            if ( grp ) {
                // add <gid>, rem <gid>, remall
                int gid = parsingGetIntAttr( "grpid" );

                if ( strcmp( grp, "remall" ) == 0 ) {
                    DEBUG_PRINTF( "Lamp group remall\n" );
                    eZCB_ClearGroupMemberships( u16ShortAddress );
                } else if ( gid >= 0 ) {
                    DEBUG_PRINTF( "Lamp group %s %d\n", grp, gid );
                    if ( strcmp( grp, "add" ) == 0 ) {
                        eZCB_AddGroupMembership( u16ShortAddress, gid );
                    } else if ( strcmp( grp, "rem" ) == 0 ) {
                        eZCB_RemoveGroupMembership( u16ShortAddress, gid );
                    }
                }
            }

            char * scn = parsingGetStringAttr0( "scn" );
            if ( scn ) {
                // add <sid> <gid>, rem <sid> <gid>, remall <gid>, store <sid> <gid>, recall <sid> <gid>
                int gid = parsingGetIntAttr( "grpid" );
                int sid = parsingGetIntAttr( "scnid" );

                if ( gid >= 0 ) {
                    if ( strcmp( scn, "remall" ) == 0 ) {
                        DEBUG_PRINTF( "Scene: remove all for group 0x%04x\n", gid );
                        eZCB_RemoveAllScenes( u16ShortAddress, gid );
                    } else if ( sid >= 0 ) {
                        if ( strcmp( scn, "add" ) == 0 ) {
                            DEBUG_PRINTF( "Scene: add scene %d for group 0x%04x\n", sid, gid );
                            // Same effect as Store-Scene
                            eZCB_StoreScene( u16ShortAddress, gid, sid );
                        } else if ( strcmp( scn, "rem" ) == 0 ) {
                            DEBUG_PRINTF( "Scene: remove scene %d for group 0x%04x\n", sid, gid );
                            eZCB_RemoveScene( u16ShortAddress, gid, sid );
                        } else if ( strcmp( scn, "store" ) == 0 ) {
                            DEBUG_PRINTF( "Scene: store scene %d for group 0x%04x\n", sid, gid );
                            eZCB_StoreScene( u16ShortAddress, gid, sid );
                        } else if ( strcmp( scn, "recall" ) == 0 ) {
                            DEBUG_PRINTF( "Scene: recall scene %d for group 0x%04x\n", sid, gid );
                            eZCB_RecallScene( u16ShortAddress, gid, sid );
                        }
                    }
                }
            }

        } else {
            printf( "Node not found\n" );
        }
    }
}

