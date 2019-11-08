// ------------------------------------------------------------------
// Sensor
// ------------------------------------------------------------------
// Parse sensor messages
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup ci
 * \file
 * \brief JSON parser to "sensor" messages
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "iotError.h"
#include "parsing.h"
#include "strtoupper.h"
#include "newLog.h"
#include "jsonCreate.h"
#include "newDb.h"
#include "sensor.h"

// #define SENSOR_DEBUG

#ifdef SENSOR_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* SENSOR_DEBUG */

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------

#define NUMINTATTRS  3
static char * intAttrs[NUMINTATTRS] =
     { "xloc", "yloc", "zloc" };

#define NUMSTRINGATTRS  2
static char * stringAttrs[NUMSTRINGATTRS] = { "mac", "nm" };
static int    stringMaxlens[NUMSTRINGATTRS] = { 16,   16 };

// ------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------

/**
 * \brief Init the JSON Parser for this type of commands
 */

void sensorInit( void ) {
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        parsingAddIntAttr( intAttrs[i] );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        parsingAddStringAttr( stringAttrs[i], stringMaxlens[i] );
    }
}

#ifdef SENSOR_DEBUG
static void sensorDump( void ) {
    printf( "Sensor dump:\n" );
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
 * \brief JSON Parser for local sensor commands towards the IoT database.
 * This interface is typically used by the DBP daemon to set sensor location and name.
 * \retval 0 When OK
 * \retval iotError In case of an error
 */

int sensorHandle( void ) {
    int error = IOT_ERROR_NONE;

#ifdef SENSOR_DEBUG
    sensorDump();
#endif

    char * mac = parsingGetStringAttr0( "mac" );

    // Check if the MAC is valid
    if ( mac != NULL ) {

        int xloc = parsingGetIntAttr( "xloc" );
        int yloc = parsingGetIntAttr( "yloc" );
        int zloc = parsingGetIntAttr( "zloc" );
        char * nm = parsingGetStringAttr0( "nm" );
        
        newdb_dev_t device;
        if ( newDbGetDevice( mac, &device ) ) {
            newDbStrNcpy( device.nm, nm, LEN_NM );
            if ( xloc != INT_MIN ) device.xloc = xloc;
            if ( yloc != INT_MIN ) device.yloc = yloc;
            if ( zloc != INT_MIN ) device.zloc = zloc;
            newDbSetDevice( &device );
        }
        
    } else {
        printf( "NO sensor MAC " );
    }

    if ( error != IOT_ERROR_NONE ) {
      iotError = error;
      sprintf( logbuffer, "Sensor error: %s", iotErrorString() );
      newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, logbuffer );
    }

    return( iotError == IOT_ERROR_NONE );

}
