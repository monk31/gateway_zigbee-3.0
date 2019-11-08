// ------------------------------------------------------------------
// Cmd
// ------------------------------------------------------------------
// Parses CMD JSON messages from the Control webpage
// Typically this handler re-assembles the incoming CMD JSON messages
// and sends them to the Zigbee Coordinator
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup ci
 * \file
 * \brief JSON parser to "cmd" messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "atoi.h"
#include "newLog.h"
#include "iotError.h"
#include "iotSleep.h"
#include "parsing.h"
#include "newDb.h"
#include "systemtable.h"
#include "queue.h"
#include "jsonCreate.h"
#include "gateway.h"
#include "cmd.h"

#define CMD_DEBUG

#ifdef CMD_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* CMD_DEBUG */

// #define AUTO_CHANSTART  1

// ------------------------------------------------------------------
// Globals
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------

#define NUMINTATTRS  6
static char * intAttrs[NUMINTATTRS] =
     { "reset", "resetrsp", "getversion", "duration", "nfcmode", "system" };

#define NUMSTRINGATTRS  3
static char * stringAttrs[NUMSTRINGATTRS] = { "chanmask", "setpermit", "strval" };
static int    stringMaxlens[NUMSTRINGATTRS] = { 8, 16, 38 };

// ------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------

/**
 * \brief Init the JSON Parser for this type of commands
 */

void cmdInit( void ) {
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        parsingAddIntAttr( intAttrs[i] );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        parsingAddStringAttr( stringAttrs[i], stringMaxlens[i] );
    }
}

#ifdef CMD_DEBUG
static void cmdDump ( void ) {
    printf( "Cmd dump:\n" );
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
 * \brief JSON Parser for GW (e.g. set nfcmode) or ZCB commands (e.g. reset, set-channel)
 * \retval 0 When OK
 * \retval iotError In case of an error
 */

int cmdHandle( void ) {
#ifdef CMD_DEBUG
    cmdDump();
#endif

    int val, ret;
    char * strval;
    char * message = NULL;

    if ( ( val = parsingGetIntAttr( "reset" ) ) >= 0 ) {
        DEBUG_PRINTF( "Command reset, val = %d\n", val );

        queueWriteOneMessage( QUEUE_KEY_ZCB_IN, jsonCmdReset( val ) );

    } else if ( ( val = parsingGetIntAttr( "resetrsp" ) ) >= 0 ) {
        DEBUG_PRINTF( "Command reset-response, val = %d\n", val );

        if ( val == 0 ) {
            // Wait a second to let the ZCB reboot
            IOT_SLEEP( 3 );   // 3 seconds

            // Factory reset: set (new) channel and start network
            int chanmask = newDbSystemGetInt( "chanmask" );
            if ( chanmask > 0 ) {
                char buf[20];
                sprintf( buf, "%08x", chanmask );
                queueWriteOneMessage( QUEUE_KEY_ZCB_IN,
                                        jsonCmdSetChannelMask( buf ) );
            }
            queueWriteOneMessage( QUEUE_KEY_ZCB_IN, jsonCmdStartNetwork() );
        }

    } else if ( ( val = parsingGetIntAttr( "getversion" ) ) >= 0 ) {
        DEBUG_PRINTF( "Command getversion, val = %d\n", val );
        message = jsonCmdGetVersion();

    } else if ( ( val = parsingGetIntAttr( "duration" ) ) >= 0 ) {
        DEBUG_PRINTF( "Command setpermit, val = %d\n", val );

        newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Set permit-join" );
        if ( val > 0 ) {
            int now = (int)time( NULL );
            newDbSystemSaveIntval( "permit", now + val );
        } else {
            newDbDeleteSystem( "permit" );
        }

        strval = parsingGetStringAttr0( "setpermit" );  // target-mac
        message = jsonCmdSetPermitJoin( strval, val );

    } else if ( ( val = parsingGetIntAttr( "nfcmode" ) ) >= 0 ) {
        newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Set NFC mode" );
        newDbSystemSaveIntval( "nfcmode", val );

    } else if ( ( val = parsingGetIntAttr( "system" ) ) >= 0 ) {
                
        switch ( val ) {
            
        case CMD_SYSTEM_SHUTDOWN:
            DEBUG_PRINTF( "Shutdown command (%d)\n", val );

            // Force DB save
            newDbFileLock();   // Lock but no unlock
            newDbSave();
            sleep( 3 );
        
#ifdef TARGET_RASPBIAN
            system( "sudo shutdown -h -P now" );
#else
            system( "halt" );
#endif
            break;
            
        case CMD_SYSTEM_REBOOT:
            DEBUG_PRINTF( "Reboot command (%d)\n", val );
            
            // Force DB save
            newDbFileLock();   // Lock but no unlock
            newDbSave();
            sleep( 3 );

#ifdef TARGET_RASPBIAN
            system( "sudo shutdown -r now" );
#else
            system( "reboot" );
#endif
            break;
            
            
        case CMD_SYSTEM_NETWORK_RESTART:
            DEBUG_PRINTF( "Network restart command (%d)\n", val );
#ifdef TARGET_RASPBERRYPI
            system( "/etc/init.d/network restart" );
#endif
            break;
            
        case CMD_SYSTEM_SET_HOSTNAME:
            strval = parsingGetStringAttr0( "strval" );
            if ( strval ) {
                if ( sethostname( strval, strlen( strval ) ) < 0 ) {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, strerror( errno ) );
                }
                ret = gwConfigEditOption( "/etc/config/system", "system", "hostname", strval );
                if ( ret == 1 ) {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Hostname option updated" );
                } else if ( ret == 0 ) {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, strerror( errno ) );
                } else {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Hostname option not found" );
                }
            }
            break;
            
        case CMD_SYSTEM_SET_SSID:
            strval = parsingGetStringAttr0( "strval" );
            if ( strval ) {
                ret = gwConfigEditOption( "/etc/config/wireless", "wifi-iface", "ssid", strval );
                if ( ret == 1 ) {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "SSID option updated" );
                } else if ( ret == 0 ) {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, strerror( errno ) );
                } else {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "SSID option not found" );
                }
            }
            break;
            
        case CMD_SYSTEM_SET_CHANNEL:
            strval = parsingGetStringAttr0( "strval" );
            if ( strval ) {
                ret = gwConfigEditOption( "/etc/config/wireless", "wifi-device", "channel", strval );
                if ( ret == 1 ) {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Wifi channel option updated" );
                } else if ( ret == 0 ) {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, strerror( errno ) );
                } else {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Wifi channel option not found" );
                }
            }
            break;
            
        case CMD_SYSTEM_SET_TIMEZONE:
            strval = parsingGetStringAttr0( "strval" );
            if ( strval ) {
                ret = gwConfigEditOption( "/etc/config/system", "system", "timezone", strval );
                if ( ret == 1 ) {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Timezone option updated" );
                } else if ( ret == 0 ) {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, strerror( errno ) );
                } else {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Timezone option not found" );
                }
            }
            break;
            
        case CMD_SYSTEM_SET_ZONENAME:
            strval = parsingGetStringAttr0( "strval" );
            if ( strval ) {
                ret = gwConfigEditOption( "/etc/config/system", "system", "zonename", strval );
                if ( ret == 1 ) {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Zonename option updated" );
                } else if ( ret == 0 ) {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, strerror( errno ) );
                } else {
                    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "System section not found" );
                }
            }
            break;
            
        }
      
    } else {
        strval = parsingGetStringAttr0( "chanmask" );
        if ( strval != NULL ) {
            DEBUG_PRINTF( "Command chanmask, val = %s\n", strval );

            sprintf( logbuffer, "Set channel mask to %s", strval );
            newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, logbuffer );
            newDbSystemSaveIntval( "chanmask", AtoiHex( strval ) );

            message = jsonCmdSetChannelMask( strval );
        }
    }
    if ( message ) {
        queueWriteOneMessage( QUEUE_KEY_ZCB_IN, message );
        return( iotError == IOT_ERROR_NONE );
    }
    return( 0 );
}

