// ------------------------------------------------------------------
// CTRL
// ------------------------------------------------------------------
// Parse Control messages that lead to new setpoints or direct device
// control
// Typically JSON messages are re-assembled and sent into the network
// over a tunnel
// If the controlled device is a manager then the mac is directly used
// as command tunnel
// If the controlled device is not a manager, then the topology is
// scanned to find its manager, and the manager's mac is used for the
// tunnel
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup ci
 * \file
 * \brief JSON parser to "ctrl" messages
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "iotError.h"
#include "atoi.h"
#include "newDb.h"
#include "queue.h"
#include "jsonCreate.h"
#include "parsing.h"
#include "strtoupper.h"
#include "newLog.h"
#include "ctrl.h"

// #define CTRL_DEBUG

// ------------------------------------------------------------------
// Globals
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------

#define NUMINTATTRS  6
static char * intAttrs[NUMINTATTRS] =
     { "heat", "cool", "sid", "lvl", "rgb", "kelvin" };

#define NUMSTRINGATTRS  2
static char * stringAttrs[NUMSTRINGATTRS] = { "mac", "cmd" };
static int    stringMaxlens[NUMSTRINGATTRS] = { 16,   4 };

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
static void ctrlDump( void ) {
    printf( "CTRL dump:\n" );
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
 * \brief JSON Parser for control commands to Zigbee devices
 * \retval 0 When OK
 * \retval iotError In case of an error
 */

int ctrlHandle( void ) {
    iotError = IOT_ERROR_NONE;

#ifdef CTRL_DEBUG
    ctrlDump();
#endif
    
    char * mac = parsingGetStringAttr0( "mac" );

    // Check if the MAC is valid
    if ( mac != NULL ) {
        mac = strtoupper( mac );
        printf( "MAC = %s\n", mac );

        // Now see what device it was, perform DB changes and determine tunnel
        int changed = 0, tunnelManager = -1;
        char tunnelMessage[80];

        newdb_dev_t device;
        if ( newDbGetDevice( mac, &device ) ) {

            int heat, cool, lvl, rgb, kelvin;
            char * cmd;

            switch ( device.dev ) {
            case DEVICE_DEV_MANAGER:
            case DEVICE_DEV_SENSOR:
            case DEVICE_DEV_PUMP:
                // iotError = IOT_ERROR_DEVICE_HAS_NO_CONTROLS;
                break;

            case DEVICE_DEV_UI:
            case DEVICE_DEV_UISENSOR:
                heat = parsingGetIntAttr( "heat" );
                cool = parsingGetIntAttr( "cool" );
                if ( heat >= 0 ) {
                    printf( "Set heat to %d\n", heat );
                    device.heat = heat;
                    device.flags |= FLAG_UI_IGNORENEXT_HEAT;
                    changed = 1;
                }
                if ( cool >= 0 ) {
                    printf( "Set cool to %d\n", cool );
                    device.cool = cool;
                    device.flags |= FLAG_UI_IGNORENEXT_COOL;
                    changed = 1;
                }

                // Prepare tunnel message
                tunnelManager = device.par;
                strcpy( tunnelMessage,
                        jsonUiCtrl( mac, heat, cool ) );

                break;

            case DEVICE_DEV_PLUG:
                cmd = parsingGetStringAttr0( "cmd" );

                if ( cmd != NULL ) {
                    newDbStrNcpy( device.cmd, cmd, LEN_CMD );
                    changed = 1;

                    // Write to ZCB
                    queueWriteOneMessage( QUEUE_KEY_ZCB_IN,
                                            jsonPlugCmd( mac, cmd ) );
                }
                break;

            case DEVICE_DEV_LAMP:
                lvl    = parsingGetIntAttr( "lvl" );
                rgb    = parsingGetIntAttr( "rgb" );
                kelvin = parsingGetIntAttr( "kelvin" );
                cmd    = parsingGetStringAttr0( "cmd" );

                /* \todo Hack: new control bridge does not always propagate announce messages */
                device.flags |= FLAG_DEV_JOINED;
                changed = 1;

                if ( lvl >= 0 ) {
                    device.lvl = lvl;
                }
                if ( rgb >= 0 ) {
                    device.flags &= ~FLAG_LASTKELVIN;
                    device.rgb = rgb;
                }
                if ( kelvin >= 0 ) {
                    device.flags |= FLAG_LASTKELVIN;
                    device.kelvin = kelvin;
                }
                if ( cmd != NULL ) {
                    newDbStrNcpy( device.cmd, cmd, LEN_CMD );
                } else if ( kelvin < 0 && ( device.flags & FLAG_LASTKELVIN ) ) {
                    // When there was no new color and previous control was Kelvin, then re-use that one
                    kelvin = device.kelvin;
                }
                if ( changed ) {
                    // Write to ZCB
                    queueWriteOneMessage( QUEUE_KEY_ZCB_IN,
                            jsonLampToZigbee( mac, cmd, lvl, rgb, kelvin ) );
                }
                break;
            }

            if ( changed ) {
                newDbSetDevice( &device );
            }

            if ( tunnelManager >= 0 ) {
                char macBuf[20];
                char * tunnelMac = newDbDeviceGetMac( tunnelManager, macBuf );
                if ( tunnelMac == NULL ) {
                    iotError = IOT_ERROR_NON_EXISTING_MAC;
                } else {
                    int q;
                    if ( ( q = queueOpen( QUEUE_KEY_ZCB_IN, 1 ) ) != -1 ) {
                        queueWrite( q, jsonTunnelOpen( tunnelMac ) );
                        queueWrite( q, tunnelMessage );
                        queueWrite( q, jsonTunnelClose() );
                        queueClose( q );
                    }
                }
            }
        } else {
            iotError = IOT_ERROR_NON_EXISTING_MAC;
        }

    } else {
	
	// YB essai
	  // Write to ZCB
                    queueWriteOneMessage( QUEUE_KEY_ZCB_IN,
                                            jsonPlugCmd( mac, "on" ) );
    //  printf( "NO MAC " );
    //  iotError = IOT_ERROR_NO_MAC;
	iotError = IOT_ERROR_NONE; 
    }

    if ( iotError != IOT_ERROR_NONE ) {
      sprintf( logbuffer, "Ctrl error: %s", iotErrorString() );
      printf( logbuffer );
      newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, logbuffer );
    }

    return( iotError == IOT_ERROR_NONE );
}

