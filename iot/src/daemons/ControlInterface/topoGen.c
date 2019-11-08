// ------------------------------------------------------------------
// Topo Generator
// ------------------------------------------------------------------
// Generates JSON commands decribing the IoT topology 
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup ci
 * \file
 * \brief Topo helper that generates JSON commands decribing the IoT topology
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atoi.h"
#include "newDb.h"
#include "queue.h"
#include "jsonCreate.h"
#include "topoGen.h"
#include "iotError.h"

#define WAIT_NONE     0
#define WAIT_NORMAL   500    // For production system >= 48 msec per hop
#define WAIT_LONG     2500

// #define TOPOGEN_DEBUG

// -------------------------------------------------------------
// GLOBALS, used in topo.c
// -------------------------------------------------------------

char topoStrings[MAX_TOPOSTRINGS][MAX_JSONLENGTH+2];
int  topoAckWaits[MAX_TOPOSTRINGS];
int  topoManIds[MAX_TOPOSTRINGS];
int  numTopoStrings = 0;

// -------------------------------------------------------------
// GLOBALS, local
// -------------------------------------------------------------

static int  manId = 0;

// -------------------------------------------------------------
// Add string helper with error handling
// -------------------------------------------------------------

static void addString( char * string, int ackWait ) {
    if ( numTopoStrings >= MAX_TOPOSTRINGS ) {
        iotError = IOT_ERROR_TOPO_OVERRUN;
        printf( "---> Topo overrun - num\n" );
    } else if ( strlen( string ) > MAX_JSONLENGTH ) {
        iotError = IOT_ERROR_TOPO_OVERRUN;
        printf( "---> Topo overrun - length\n" );
    } else {
        strcpy( topoStrings[numTopoStrings], string );
        topoAckWaits[numTopoStrings] = ackWait;
        topoManIds[numTopoStrings] = manId;
        numTopoStrings++;
    }
}

// -------------------------------------------------------------
// Table handler
// -------------------------------------------------------------

#define MAXTOPODEVS 50
typedef struct topo_dev {
    int id;
    int dev;
    int par;
    int sid;
    char mac[20];
    char nm[20];
    char ty[10];
} topo_dev_t;

static topo_dev_t topodevs[MAXTOPODEVS];
static int topoDevCnt = 0;

static void saveStrcpy( char * dst, char * src ) {
    if ( src != NULL && *src != '\0' ) {
        strcpy( dst, src );
    } else {
        *dst = '\0';
    }
}

static int getTopoDevsCb( newdb_dev_t * pdev ) {
    // printf( "Table %s (%d)\n", data, argc );
    if ( topoDevCnt < MAXTOPODEVS ) {
        topodevs[topoDevCnt].id  = pdev->id;
        topodevs[topoDevCnt].dev = pdev->dev;
        topodevs[topoDevCnt].par = pdev->par;
        topodevs[topoDevCnt].sid = pdev->sid;
        saveStrcpy( topodevs[topoDevCnt].mac, pdev->mac );
        saveStrcpy( topodevs[topoDevCnt].nm,  pdev->nm );
        saveStrcpy( topodevs[topoDevCnt].ty,  pdev->ty );
        topoDevCnt++;
    } else {
        printf( "Too many topo devices\n" );
        return 0;
    }
    return 1;
}

// -------------------------------------------------------------
// Device handlers
// -------------------------------------------------------------

static void topoLoopPump( int par ) {
    int i;
    for ( i=0; i<topoDevCnt; i++ ) {
        if ( topodevs[i].dev == DEVICE_DEV_PUMP && topodevs[i].par == par ) {
            addString( jsonTopoAddPump( topodevs[i].mac, topodevs[i].nm, topodevs[i].sid ), WAIT_NORMAL );
        }
    }
}

static void topoLoopSensor( int par ) {
    int i;
    for ( i=0; i<topoDevCnt; i++ ) {
        if ( topodevs[i].dev == DEVICE_DEV_SENSOR && topodevs[i].par == par ) {
            addString( jsonTopoAddSensor( topodevs[i].mac, topodevs[i].nm ), WAIT_NORMAL );

            topoLoopPump( topodevs[i].id );
        }
    }
}

static void topoLoopUiSensor( int par ) {
    int i;
    for ( i=0; i<topoDevCnt; i++ ) {
        if ( topodevs[i].dev == DEVICE_DEV_UISENSOR && topodevs[i].par == par ) {
            addString( jsonTopoAddUI( topodevs[i].mac, topodevs[i].nm ), WAIT_NORMAL );
            addString( jsonTopoAddSensor( topodevs[i].mac, topodevs[i].nm ), WAIT_NORMAL );

            // There can be other sensors attached to this UISENSOR
            topoLoopSensor( topodevs[i].id );

            // There (normally) should be pumps attached to this UISENSOR
            topoLoopPump( topodevs[i].id );
        }
    }
}

static void topoLoopUi( int par ) {
    int i;
    for ( i=0; i<topoDevCnt; i++ ) {
        if ( topodevs[i].dev == DEVICE_DEV_UI && topodevs[i].par == par ) {
            addString( jsonTopoAddUI( topodevs[i].mac, topodevs[i].nm ), WAIT_NORMAL );

            topoLoopSensor( topodevs[i].id );
        }
    }
}

static void topoLoopMan( void ) {
    int i;
    for ( i=0; i<topoDevCnt; i++ ) {
        if ( topodevs[i].dev == DEVICE_DEV_MANAGER ) {

            manId = topodevs[i].id;
            
            addString( jsonTunnelOpen( topodevs[i].mac ), WAIT_NONE );
            addString( jsonTopoClear(), WAIT_LONG );

            // addString( jsonTopoAddManager( topodevs[i].mac, topodevs[i].nm, topodevs[i].ty ), WAIT_NORMAL );  // Removed 12 aug 2015

            topoLoopUiSensor( topodevs[i].id );
            topoLoopUi( topodevs[i].id );
        }
    }
}

// -------------------------------------------------------------
// MAIN
// -------------------------------------------------------------

/**
 * \brief Build a list of JSON topology commands based on the IoT device table.
 * \retval <N> Number of JSON topology commands in the list
 * \return Fills global JSON topology commands list, waiting times and manager IDs for topo.c
 */

int topoGenerate( void ) {

    if ( iotError == IOT_ERROR_TOPO_OVERRUN ) {
        iotError = IOT_ERROR_NONE;
    }
    numTopoStrings = 0;

    // Get the list of devices (column subset) into local memory
    
    // Loop through the list from the managere hierarchy down to the pumps
    topoLoopMan();

    // Add end markers
    if ( numTopoStrings > 0 ) {
        addString( jsonTopoEnd(), WAIT_NORMAL );
        addString( jsonTunnelClose(), WAIT_NONE );
    }

#ifdef TOPOGEN_DEBUG
    int i;
    printf( "TOPO-GENERATE (%d)\n", numTopoStrings );
    for ( i=0; i < numTopoStrings; i++ ) {
        printf( "%d: %s", i, topoStrings[i] );
    }
#endif /* TOPOGEN_DEBUG */

    return( numTopoStrings );
}

