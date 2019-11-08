// ------------------------------------------------------------------
// dbEdit
// ------------------------------------------------------------------
// DB edits on GW dB
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup ci
 * \file
 * \brief JSON parser to "dbedit" messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "atoi.h"
#include "jsonCreate.h"
#include "parsing.h"
#include "newDb.h"
#include "newLog.h"
#include "dbEdit.h"
#include "plugUsage.h"
#include "strtoupper.h"
#include "iotError.h"
#include "socket.h"

// #define DBEDIT_DEBUG

#ifdef DBEDIT_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* DBEDIT_DEBUG */

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------

#define NUMINTATTRS  2
static char * intAttrs[NUMINTATTRS] = { "dev", "id" };

#define NUMSTRINGATTRS  7
static char * stringAttrs[NUMSTRINGATTRS] = { "add", "rem", "ty", "clr", "table", "col", "val" };
static int    stringMaxlens[NUMSTRINGATTRS] = { 16,   16,    8,    16,    10,      10,    16 };

// ------------------------------------------------------------------
// Socket helper
// ------------------------------------------------------------------

extern void sendResponse( char * jsonResponseString );

// ------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------

static int tableEdit( char * table, int id, char * col, char * val ) {
    int ok = 0;
    if ( strcmp( table, "system" ) == 0 ) {
    } else if ( strcmp( table, "devices" ) == 0 ) {
        newdb_dev_t dev;
        if ( newDbGetDeviceId( id, &dev ) ) {
            if ( strcmp( col, "par" ) == 0 ) dev.par = Atoi0( val );
            ok = newDbSetDevice( &dev );
        }
    } else if ( strcmp( table, "plughistory" ) == 0 ) {
    } else if ( strcmp( table, "zcb" ) == 0 ) {
    }
    return ok;
}

/**
 * \brief Init the JSON Parser for this type of commands
 */

void dbEditInit( void ) {
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        parsingAddIntAttr( intAttrs[i] );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        parsingAddStringAttr( stringAttrs[i], stringMaxlens[i] );
    }
}

#ifdef DBEDIT_DEBUG
static void dbEditDump ( void ) {
    printf( "dbEdit dump:\n" );
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        printf( "- %-12s = %d\n",
                intAttrs[i], parsingGetIntAttr( intAttrs[i] ) );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        printf( "- %-12s = %s\n",
                stringAttrs[i], parsingGetStringAttr( stringAttrs[i] ) );
    }
}
#endif

// ------------------------------------------------------------------
// Handle
// ------------------------------------------------------------------

/**
 * \brief JSON Parser for edit commands of the IoT database (e.g. add, rem, clr)
 * \retval 0 When OK
 * \retval iotError In case of an error
 */

int dbEditHandle( void ) {
#ifdef DBEDIT_DEBUG
    dbEditDump();
#endif

    int error = IOT_ERROR_NONE;

    char * mac, * type, * clr, * table;
    if ( ( mac = parsingGetStringAttr0( "add" ) ) != NULL ) {
        int dev = parsingGetIntAttr( "dev" );
        if ( dev > DEVICE_DEV_UNKNOWN ) {
            type = parsingGetStringAttr( "ty" );
            
            newdb_dev_t device;
            if ( newDbGetDevice( mac, &device ) ) {
                printf( "Already exists\n" );
                error = IOT_ERROR_ALREADY_EXISTING_MAC;
            } else if ( newDbGetNewDevice( mac, &device ) ) {
                device.dev = dev;
                device.par = 0;
                newDbStrNcpy( device.ty, type, LEN_TY );

                // Certain defaults
                switch ( dev ) {
                    case DEVICE_DEV_MANAGER:
                        if ( strcmp( type, "h" ) == 0 ) {
                            newDbStrNcpy( device.nm, "Heating Manager", LEN_NM );
                        } else {
                            newDbStrNcpy( device.nm, "Cooling Manager", LEN_NM );
                        }
                        break;
                    case DEVICE_DEV_UI:
                        newDbStrNcpy( device.nm, "UI", LEN_NM );
                        break;
                    case DEVICE_DEV_UISENSOR:
                        newDbStrNcpy( device.nm, "UI+Sensor", LEN_NM );
                        break;
                    case DEVICE_DEV_SENSOR:
                        newDbStrNcpy( device.nm, "Sensor", LEN_NM );
                        break;
                    case DEVICE_DEV_LAMP:
                        newDbStrNcpy( device.cmd, "on", LEN_CMD );
                        if ( strcmp( type, "dim" ) == 0 ) {
                            newDbStrNcpy( device.nm, "Dim-lamp", LEN_NM );
                            device.lvl = 100;
                        } else if ( strcmp( type, "col" ) == 0 ) {
                            newDbStrNcpy( device.nm, "RGB-lamp", LEN_NM );
                            device.lvl = 100;
                            device.rgb = 0xffffff;
                            device.kelvin = 6250;
                        } else if ( strcmp( type, "tw"  ) == 0 ) {
                            newDbStrNcpy( device.nm, "TW-lamp", LEN_NM );
                            device.lvl = 100;
                            device.kelvin = 6250;
                        } else {
                            newDbStrNcpy( device.nm, "OnOff-lamp", LEN_NM );
                        }
                        break;
                    case DEVICE_DEV_PLUG:
                        newDbStrNcpy( device.nm, "Plugmeter", LEN_NM );
                        newDbStrNcpy( device.cmd, "on", LEN_CMD );
                        break;
                    case DEVICE_DEV_PUMP:
                        newDbStrNcpy( device.nm, "Actuator", LEN_NM );
                        newDbStrNcpy( device.cmd, "off", LEN_CMD );
                        break;
                    case DEVICE_DEV_SWITCH:
                        newDbStrNcpy( device.nm, "Switch", LEN_NM );
                        break;
                }
                            
                device.flags = FLAG_ADDED_BY_GW;
                newDbSetDevice( &device );
            }

        } else {
            error = IOT_ERROR_DEVICE_MISSING;
        }
        
    } else if ( ( mac = parsingGetStringAttr0( "rem" ) ) != NULL ) {
        if ( !newDbDeleteDevice( mac ) ) {
            error = IOT_ERROR_NON_EXISTING_MAC;
        }
        
    } else if ( ( clr = parsingGetStringAttr0( "clr" ) ) != NULL ) {
        if ( strcmp( clr, "climate" ) == 0 ) {
            newDbEmptyDevices( MODE_DEV_EMPTY_CLIMATE );
        } else if ( strcmp( clr, "lamps" ) == 0 ) {
            newDbEmptyDevices( MODE_DEV_EMPTY_LAMPS );
        } else if ( strcmp( clr, "plugs" ) == 0 ) {
            newDbEmptyDevices( MODE_DEV_EMPTY_PLUGS );
        } else if ( strcmp( clr, "devices" ) == 0 ) {
            newDbEmptyDevices( MODE_DEV_EMPTY_ALL );
            newDbEmptyPlugHist();
        } else if ( strcmp( clr, "system" ) == 0 ) {
            newDbEmptySystem();
        } else if ( strcmp( clr, "zcb" ) == 0 ) {
            newDbEmptyZcb();
        } else if ( strcmp( clr, "plughistory" ) == 0 ) {
            newDbEmptyPlugHist();
        }

    } else if ( ( table = parsingGetStringAttr0( "table" ) ) != NULL ) {
        char * col = parsingGetStringAttr0( "col" );
        char * val = parsingGetStringAttr0( "val" );
        int id = parsingGetIntAttr( "id" );
        if ( col && val && ( id >= 0 ) ) {
            tableEdit( table, id, col, val );
        }
    }

    if ( error != IOT_ERROR_NONE ) {
        iotError = error;
    }

    return( iotError == IOT_ERROR_NONE );
}

