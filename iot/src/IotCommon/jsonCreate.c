// ------------------------------------------------------------------
// JSON Message Constructors
// ------------------------------------------------------------------
// Contruct valid IoT JSON messages of all types
// Uses a global character buffer so result must be used immediately
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \file
 * \brief JSON Message Constructors
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "colorConv.h"

#define MAXJSONMESSAGE    500
#define MAXNAME           30
#define MAXNAMEVALUE      160   // was 80

// #define JSON_DEBUG

#ifdef JSON_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* MAIN_DEBUG */

// ------------------------------------------------------------------
// Global buffer
// ------------------------------------------------------------------

static char jsonMessage[MAXJSONMESSAGE+2];

// ------------------------------------------------------------------
// Constructors
// ------------------------------------------------------------------

static void catString( char * String ) {
    if ( ( strlen( jsonMessage ) + strlen( String ) ) < MAXJSONMESSAGE ) {
        strcat( jsonMessage, String );
    } else {
        printf( "Error: overflow in jsonCreate\n" );
    }
}

static void catName( char * name ) {
    char buf[MAXNAME+2];
    sprintf( buf, "\"%s\"", name );
    catString( buf );
}

static void catNameValueInt( char * name, int value ) {
    char buf[MAXNAMEVALUE+2];
    sprintf( buf, "\"%s\":%d", name, value );
    catString( buf );
}

static void catNameValueString( char * name, char * value ) {
    char buf[MAXNAMEVALUE+2];
    sprintf( buf, "\"%s\":\"%s\"", name, value );
    catString( buf );
}

// ------------------------------------------------------------------------
// Command / ZCB helpers
// ------------------------------------------------------------------------

static char * jsonCmd( char * name, int value ) {
    jsonMessage[0] = '\0';
    catName( "cmd" );
    catString( " : { " );
    catNameValueInt( name, value );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

static char * jsonCmdString( char * name, char * value ) {
    jsonMessage[0] = '\0';
    catName( "cmd" );
    catString( " : { " );
    catNameValueString( name, value );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

static char * jsonZcb( char * name, int value ) {
    jsonMessage[0] = '\0';
    catName( "zcb" );
    catString( " : { " );
    catNameValueInt( name, value );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

static char * jsonZcbString( char * name, char * value ) {
    jsonMessage[0] = '\0';
    catName( "zcb" );
    catString( " : { " );
    catNameValueString( name, value );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Status
// ------------------------------------------------------------------------

char * jsonCmdGetStatus( void ) {
    return( jsonCmd( "getstatus", 1 ) );
}

char * jsonZcbStatus( int status ) {
    return( jsonZcb( "status", status ) );
}

// ------------------------------------------------------------------------
// Logging
// ------------------------------------------------------------------------

char * jsonZcbLog( char * text ) {
    return( jsonZcbString( "log", text ) );
}

// ------------------------------------------------------------------------
// Restart
// ------------------------------------------------------------------------

char * jsonCmdReset( int mode ) {
    return( jsonCmd( "reset", mode ) );
}

char * jsonZcbResetRsp( int ret ) {
    return( jsonZcb( "resetrsp", ret ) );
}

char * jsonCmdResetRsp( int ret ) {
    return( jsonCmd( "resetrsp", ret ) );
}

char * jsonCmdStartNetwork( void ) {
    return( jsonCmd( "startnwk", 1 ) );
}

char * jsonZcbStartRsp( int ret ) {
    return( jsonZcb( "startrsp", ret ) );
}

char * jsonCmdErase( void ) {
    return( jsonCmd( "erase", 1 ) );
}

char * jsonCmdSystem( int system, char * strval ) {
    jsonMessage[0] = '\0';
    catName( "cmd" );
    catString( " : { " );
    catNameValueInt( "system", system );
    if ( strval ) {
        catString( ", " );
        catNameValueString( "strval", strval );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Device Announce
// ------------------------------------------------------------------------

char * jsonZcbAnnounce( char * mac, char * dev, char * ty ) {
    jsonMessage[0] = '\0';
    catName( "zcb" );
    catString( " : { " );
    catNameValueString( "joined", mac );
    catString( ", " );
    catNameValueString( "dev", dev );
    if ( ty ) {
        catString( ", " );
        catNameValueString( "ty", ty );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Version
// ------------------------------------------------------------------------

char * jsonCmdGetVersion( void ) {
    return( jsonCmd( "getversion", 1 ) );
}

char * jsonZcbVersion( int major, int minor ) {
    char buf[20];
    sprintf( buf, "r%d.%d", major, minor );
    return( jsonZcbString( "version", buf ) );
}

// ------------------------------------------------------------------------
// NFC mode
// ------------------------------------------------------------------------

char * jsonCmdNfcmode( int nfcMode ) {
    return( jsonCmd( "nfcmode", nfcMode ) );
}

// ------------------------------------------------------------------------
// Set Extended PANID
// ------------------------------------------------------------------------

char * jsonCmdSetExtPan( char * extPanID ) {
    return( jsonCmdString( "setextpanid", extPanID ) );
}

// ------------------------------------------------------------------------
// Set Channel Mask
// ------------------------------------------------------------------------

char * jsonCmdSetChannelMask( char * channelMask ) {
    return( jsonCmdString( "chanmask", channelMask ) );
}

char * jsonZcbChanMaskRsp( char * channelMask ) {
    return( jsonZcbString( "chanrsp", channelMask ) );
}

char * jsonZcbChanRsp( int channel ) {
    return( jsonZcb( "chanrsp", channel ) );
}

// ------------------------------------------------------------------------
// Set Device Type
// ------------------------------------------------------------------------

char * jsonCmdSetDeviceType( int devType ) {
    return( jsonCmd( "setdevicetype", devType ) );
}

// ------------------------------------------------------------------------
// Permit Join
// ------------------------------------------------------------------------

char * jsonCmdGetPermitJoin( void ) {
    return( jsonCmd( "getpermit", 1 ) );
}

char * jsonZcbRspPermitJoin( int status ) {
    return( jsonZcb( "permit", status ) );
}

char * jsonCmdSetPermitJoin( char * target, int duration ) {
    jsonMessage[0] = '\0';
    catName( "cmd" );
    catString( " : { " );
    catNameValueString( "setpermit", ( target != NULL ) ? target : "1" );
    catString( ", " );
    catNameValueInt( "duration", duration );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Authentication
// ------------------------------------------------------------------------

char * jsonCmdAuthorizeRequest( char * mac, char * linkkey ) {
    jsonMessage[0] = '\0';
    catName( "cmd" );
    catString( " : { " );
    catNameValueString( "authorize", mac );
    catString( ", " );
    catNameValueString( "linkkey", linkkey );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonZcbAuthorizeResponse( char * mac, char * nwKey, char * mic, char * tcAddress, 
                                 int keySeq, int channel, char * pan, char * extPan  ) {
    jsonMessage[0] = '\0';
    catName( "zcb" );
    catString( " : { " );
    catNameValueString( "authorize", mac );
    catString( ", " );
    catNameValueString( "key", nwKey );
    catString( ", " );
    catNameValueString( "mic", mic );
    catString( ", " );
    catNameValueString( "tcaddr", tcAddress );
    catString( ", " );
    catNameValueInt( "keyseq", keySeq );
    catString( ", " );
    catNameValueInt( "chan", channel );
    catString( ", " );
    catNameValueString( "pan", pan );
    catString( ", " );
    catNameValueString( "extpan", extPan );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Authentication with Out-of-band (ICode)
// ------------------------------------------------------------------------

char * jsonCmdAuthorizeOobRequest( char * mac, char * key ) {
    jsonMessage[0] = '\0';
    catName( "cmd" );
    catString( " : { " );
    catNameValueString( "authorize_oob", mac );
    catString( ", " );
    catNameValueString( "key", key );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonZcbAuthorizeOobResponse( char * mac, char * nwKey, char * mic, char * tcAddress,
                                    int keySeq, int channel, char * pan, char * extPan,
                                    int tcShortAddr, int deviceId) {
    jsonMessage[0] = '\0';
    catName( "zcb" );
    catString( " : { " );
    catNameValueString( "authorize_oob", mac );
    catString( ", " );
    catNameValueString( "key", nwKey );
    catString( ", " );
    catNameValueString( "mic", mic );
    catString( ", " );
    catNameValueString( "tcaddr", tcAddress );
    catString( ", " );
    catNameValueInt( "keyseq", keySeq );
    catString( ", " );
    catNameValueInt( "chan", channel );
    catString( ", " );
    catNameValueString( "pan", pan );
    catString( ", " );
    catNameValueString( "extpan", extPan );
    catString( ", " );
    catNameValueInt( "tcshaddr", tcShortAddr );
    catString( ", " );
    catNameValueInt( "devid", deviceId );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Single touch commissioning
// ------------------------------------------------------------------------

char * jsonTls( char * label, char * val ) {
    jsonMessage[0] = '\0';
    catName( "tls" );
    catString( " : { " );
    catNameValueString( label, val );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonLinkInfo( int nw_version, int nw_type, int nw_profile,
                     char * mac, char * linkkey ) {
    jsonMessage[0] = '\0';
    catName( "linkinfo" );
    catString( " : { " );
    catNameValueInt( "version", nw_version );
    catString( ", " );
    catNameValueInt( "type", nw_type );
    catString( ", " );
    catNameValueInt( "profile", nw_profile );
    catString( ", " );
    catNameValueString( "mac", mac );
    catString( ", " );
    catNameValueString( "linkkey", linkkey );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonJoinSecure( int channel, int keysequence, char * pan,
                       char * extendedPan, char * networkKey,
                       char * mic, char * tcAddress ) {
    jsonMessage[0] = '\0';
    catName( "joinsecure" );
    catString( " : { " );

    catNameValueInt( "chan", channel );
    catString( ", " );
    catNameValueInt( "keyseq", keysequence );
    catString( ", " );
    catNameValueString( "pan", pan );
    catString( ", " );
    catNameValueString( "extpan", extendedPan );
    catString( ", " );
    catNameValueString( "nwkey", networkKey );
    catString( ", " );
    catNameValueString( "mic", mic );
    catString( ", " );
    catNameValueString( "tcaddr", tcAddress );

    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonOobCommissioningRequest( int nw_version, int nw_type, int nw_profile,
                                    char * mac, char * key ) {
    jsonMessage[0] = '\0';
    catName( "oobrequest" );
    catString( " : { " );
    catNameValueInt( "version", nw_version );
    catString( ", " );
    catNameValueInt( "type", nw_type );
    catString( ", " );
    catNameValueInt( "profile", nw_profile );
    catString( ", " );
    catNameValueString( "mac", mac );
    catString( ", " );
    catNameValueString( "key", key );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonOobCommissioningResponse( int channel, int keysequence, char * pan,
                                     char * extendedPan, char * key,
                                     char * mic, char * tcExtAddress,
                                     int tcShortAddress, int deviceId ) {
    jsonMessage[0] = '\0';
    catName( "oobresponse" );
    catString( " : { " );

    catNameValueInt( "chan", channel );
    catString( ", " );
    catNameValueInt( "keyseq", keysequence );
    catString( ", " );
    catNameValueString( "pan", pan );
    catString( ", " );
    catNameValueString( "extpan", extendedPan );
    catString( ", " );
    catNameValueString( "key", key );
    catString( ", " );
    catNameValueString( "mic", mic );
    catString( ", " );
    catNameValueString( "tcaddr", tcExtAddress );
    catString( ", " );
    catNameValueInt( "tcshaddr", tcShortAddress );
    catString( ", " );
    catNameValueInt( "devid", deviceId );

    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Manager
// ------------------------------------------------------------------------

char * jsonManager( int id, char * mac, char * nm, int rm,
                    char * ty, int joined ) {
    jsonMessage[0] = '\0';
    catName( "manager" );
    catString( " : { " );
    if ( id >= 0 ) {
        catNameValueInt( "id", id );
        catString( ", " );
    }
    catNameValueString( "mac", mac );
    if ( nm != NULL ) {
        catString( ", " );
        catNameValueString( "nm", nm );
    }
    if ( rm >= 0 ) {
        catString( ", " );
        catNameValueInt( "rm", rm );
    }
    if ( ty != NULL ) {
        catString( ", " );
        catNameValueString( "ty", ty );
    }
    if ( joined >= 0 ) {
        catString( ", " );
        catNameValueInt( "jnd", joined );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// UI
// ------------------------------------------------------------------------

char * jsonUI( int id, char * mac, char * nm, int rm, int man,
               int heat, int cool, int joined ) {
    jsonMessage[0] = '\0';
    catName( "ui" );
    catString( " : { " );
    if ( id >= 0 ) {
        catNameValueInt( "id", id );
        catString( ", " );
    }
    catNameValueString( "mac", mac );
    if ( nm != NULL ) {
        catString( ", " );
        catNameValueString( "nm", nm );
    }
    if ( rm >= 0 ) {
        catString( ", " );
        catNameValueInt( "rm", rm );
    }
    if ( man >= 0 ) {
        catString( ", " );
        catNameValueInt( "man", man );
    }
    if ( heat >= 0 ) {
        catString( ", " );
        catNameValueInt( "heat", heat );
    }
    if ( cool >= 0 ) {
        catString( ", " );
        catNameValueInt( "cool", cool );
    }
    if ( joined >= 0 ) {
        catString( ", " );
        catNameValueInt( "jnd", joined );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonUiCtrl( char * mac, int heat, int cool ) {
    jsonMessage[0] = '\0';
    catName( "ctrl" );
    catString( " : { " );
    catNameValueString( "mac", mac );
    if ( heat >= 0 ) {
        catString( ", " );
        catNameValueInt( "heat", heat );
    }
    if ( cool >= 0 ) {
        catString( ", " );
        catNameValueInt( "cool", cool );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Sensor
// ------------------------------------------------------------------------

char * jsonSensor( int id, char * mac, char * nm, int rm, int ui,
                   int tmp, int hum, int prs, int co2, int bat, int batl, int als,
                   int xloc, int yloc, int zloc, int joined ) {
    jsonMessage[0] = '\0';
    catName( "sensor" );
    catString( " : { " );
    if ( id >= 0 ) {
        catNameValueInt( "id", id );
        catString( ", " );
    }
    catNameValueString( "mac", mac );
    if ( nm != NULL ) {
        catString( ", " );
        catNameValueString( "nm", nm );
    }
    if ( rm >= 0 ) {
        catString( ", " );
        catNameValueInt( "rm", rm );
    }
    if ( ui >= 0 ) {
        catString( ", " );
        catNameValueInt( "ui", ui );
    }
    if ( tmp >= 0 ) {
        catString( ", " );
        catNameValueInt( "tmp", tmp );
    }
    if ( hum >= 0 ) {
        catString( ", " );
        catNameValueInt( "hum", hum );
    }
    if ( prs >= 0 ) {
        catString( ", " );
        catNameValueInt( "prs", prs );
    }
    if ( co2 >= 0 ) {
        catString( ", " );
        catNameValueInt( "co2", co2 );
    }
    if ( bat >= 0 ) {
        catString( ", " );
        catNameValueInt( "bat", bat );
    }
    if ( batl >= 0 ) {
        catString( ", " );
        catNameValueInt( "batl", batl );
    }
    if ( als >= 0 ) {
        catString( ", " );
        catNameValueInt( "als", als );
    }
    if ( xloc != INT_MIN ) {
        catString( ", " );
        catNameValueInt( "xloc", xloc );
    }
    if ( yloc != INT_MIN ) {
        catString( ", " );
        catNameValueInt( "yloc", yloc );
    }
    if ( zloc != INT_MIN ) {
        catString( ", " );
        catNameValueInt( "zloc", zloc );
    }
    if ( joined >= 0 ) {
        catString( ", " );
        catNameValueInt( "jnd", joined );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Pump
// ------------------------------------------------------------------------

char * jsonPump( int id, char * mac, char * nm, int rm, int sen,
                 int sid, char * cmd, int lvl, int joined ) {
    jsonMessage[0] = '\0';
    catName( "pmp" );
    catString( " : { " );
    if ( id >= 0 ) {
        catNameValueInt( "id", id );
        catString( ", " );
    }
    catNameValueString( "mac", mac );
    if ( nm != NULL ) {
        catString( ", " );
        catNameValueString( "nm", nm );
    }
    if ( rm >= 0 ) {
        catString( ", " );
        catNameValueInt( "rm", rm );
    }
    if ( sen >= 0 ) {
        catString( ", " );
        catNameValueInt( "sen", sen );
    }
    if ( sid >= 0 ) {
        catString( ", " );
        catNameValueInt( "sid", sid );
    }
    // if ( !isEmptyString( cmd ) ) {
    if ( cmd != NULL ) {
        catString( ", " );
        catNameValueString( "cmd", cmd );
    }
    if ( lvl >= 0 ) {
        catString( ", " );
        catNameValueInt( "lvl", lvl );
    }
    if ( joined >= 0 ) {
        catString( ", " );
        catNameValueInt( "jnd", joined );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Lamp
// ------------------------------------------------------------------------

static char * jsonLampInternal( int id, char * mac, char * nm, int rm,
                 char * ty, char * cmd, int lvl,
                 int rgb, int kelvin, int xcr, int ycr, int joined ) {
    jsonMessage[0] = '\0';
    catName( "lmp" );
    catString( " : { " );
    if ( id >= 0 ) {
        catNameValueInt( "id", id );
        catString( ", " );
    }
    catNameValueString( "mac", mac );
    if ( nm != NULL ) {
        catString( ", " );
        catNameValueString( "nm", nm );
    }
    if ( rm >= 0 ) {
        catString( ", " );
        catNameValueInt( "rm", rm );
    }
    if ( ty != NULL ) {
        catString( ", " );
        catNameValueString( "ty", ty );
    }
    if ( cmd != NULL ) {
        catString( ", " );
        catNameValueString( "cmd", cmd );
    }
    if ( lvl >= 0 ) {
        catString( ", " );
        catNameValueInt( "lvl", lvl );
    }
    if ( rgb >= 0 ) {
        catString( ", " );
        catNameValueInt( "rgb", rgb );
    }
    if ( kelvin >= 0 ) {
        catString( ", " );
        catNameValueInt( "kelvin", kelvin );
    }
    if ( xcr >= 0 ) {
        catString( ", " );
        catNameValueInt( "xcr", xcr );
    }
    if ( ycr >= 0 ) {
        catString( ", " );
        catNameValueInt( "ycr", ycr );
    }
    if ( joined >= 0 ) {
        catString( ", " );
        catNameValueInt( "jnd", joined );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonLamp( int id, char * mac, char * nm, int rm,
          char * ty, char * cmd, int lvl, int rgb, int kelvin, int joined ) {
    return( jsonLampInternal( id, mac, nm, rm,
          ty, cmd, lvl, rgb, kelvin, -1, -1, joined ) );
}

char * jsonLamp_xyY( int id, char * mac, char * nm, int rm,
           char * ty, char * cmd, int lvl, int xcr, int ycr, int joined ) {
    return( jsonLampInternal( id, mac, nm, rm,
                 ty, cmd, lvl, -1, -1, xcr, ycr, joined ) );
}

// ------------------------------------------------------------------------
// Plug
// ------------------------------------------------------------------------

char * jsonPlug( int id, char * mac, char * nm, int rm,
                 char * cmd, int act, int sum, int h24, int joined, int autoinsert ) {
    jsonMessage[0] = '\0';
    catName( "plg" );
    catString( " : { " );
    if ( id >= 0 ) {
        catNameValueInt( "id", id );
        catString( ", " );
    }
    catNameValueString( "mac", mac );
    if ( nm != NULL ) {
        catString( ", " );
        catNameValueString( "nm", nm );
    }
    if ( rm >= 0 ) {
        catString( ", " );
        catNameValueInt( "rm", rm );
    }
    if ( cmd != NULL ) {
        catString( ", " );
        catNameValueString( "cmd", cmd );
    }
    if ( act >= 0 ) {
        catString( ", " );
        catNameValueInt( "act", act );
    }
    if ( sum >= 0 ) {
        catString( ", " );
        catNameValueInt( "sum", sum );
    }
    if ( h24 >= 0 ) {
        catString( ", " );
        catNameValueInt( "h24", h24 );
    }
    if ( joined >= 0 ) {
        catString( ", " );
        catNameValueInt( "jnd", joined );
    }
    if ( autoinsert >= 0 ) {
        catString( ", " );
        catNameValueInt( "autoinsert", autoinsert );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonPlugCmd( char * mac, char * cmd ) {
    return( jsonPlug( -1, mac, NULL, -1, cmd, -1, -1, -1, -1, -1 ) );
}

// ------------------------------------------------------------------
// ACT
// ------------------------------------------------------------------

char * jsonAct( char * mac, int sid, char * cmd, int lvl ) {
    jsonMessage[0] = '\0';
    catName( "act" );
    catString( " : { " );
    catNameValueString( "mac", mac );
    if ( sid >= 0 ) {
        catString( ", " );
        catNameValueInt( "sid", sid );
    }
    if ( cmd != NULL ) {
        catString( ", " );
        catNameValueString( "cmd", cmd );
    }
    if ( lvl >= 0 ) {
        catString( ", " );
        catNameValueInt( "lvl", lvl );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// CONTROL
// ------------------------------------------------------------------------

char * jsonControl( char * mac, char * cmd, int sid,
                    int lvl, int rgb, int kelvin,
                    int heat, int cool ) {
    jsonMessage[0] = '\0';
    catName( "ctrl" );
    catString( " : { " );
    catNameValueString( "mac", mac );
    if ( cmd != NULL ) {
        catString( ", " );
        catNameValueString( "cmd", cmd );
    }
    if ( lvl >= 0 ) {
        catString( ", " );
        catNameValueInt( "lvl", lvl );
    }
    if ( rgb >= 0 ) {
        catString( ", " );
        catNameValueInt( "rgb", rgb );
    }
    if ( kelvin >= 0 ) {
        catString( ", " );
        catNameValueInt( "kelvin", kelvin );
    }
    if ( heat >= 0 ) {
        catString( ", " );
        catNameValueInt( "heat", heat );
    }
    if ( cool >= 0 ) {
        catString( ", " );
        catNameValueInt( "cool", cool );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Outgoing Lamp Control
// - Requires xyY color space
// ------------------------------------------------------------------------

char * jsonLampToZigbee( char * mac, char * cmd, int lvl, int rgb, int kelvin ) {
    int xcr = -1, ycr = -1;

    jsonMessage[0] = '\0';
    catName( "lmp" );
    catString( " : { " );
    catNameValueString( "mac", mac );
    
    // Command
    if ( cmd != NULL ) {
        catString( ", " );
        catNameValueString( "cmd", cmd );
    }

    // Level
    if ( ( ( kelvin >= 0 ) || ( rgb >= 0 ) ) && ( lvl < 0 ) ) {
        // If the lvl was not specified with a color, then we use 100%
        lvl = 100;
    }
    if ( lvl >= 0 ) {
        catString( ", " );
        catNameValueInt( "lvl", lvl );
    }

    // Color
    if ( ( kelvin == 0 ) && ( rgb == 0 ) ) {
        // Choose white (0xFFFFFF), then we convert that to the xy color space
        rgb2xy( 0xFFFFFF, &xcr, &ycr );
        printf( "x,y = %f,%f\n", (float)xcr / (float)0xFEFF,
                                 (float)ycr / (float)0xFEFF );
        catString( ", " );
        catNameValueInt( "xcr", xcr );
        catString( ", " );
        catNameValueInt( "ycr", ycr );
    } else if ( kelvin >= 0 ) {
        // If kelvin is specified, then we just send that
        catString( ", " );
        catNameValueInt( "kelvin", kelvin );
        //// If kelvin is specified, then we convert that to the xy color space
        //kelvin2xy( kelvin, &xcr, &ycr );
        //printf( "x,y = %f,%f\n", (float)xcr / (float)0xFEFF,
        //                         (float)ycr / (float)0xFEFF );
    } else if ( rgb >= 0 ) {
        // If RGB is specified, then we convert that to the xy color space
        rgb2xy( rgb, &xcr, &ycr );
        printf( "x,y = %f,%f\n", (float)xcr / (float)0xFEFF,
                                 (float)ycr / (float)0xFEFF );
        catString( ", " );
        catNameValueInt( "xcr", xcr );
        catString( ", " );
        catNameValueInt( "ycr", ycr );
    }
    
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Climate Manager Status
// ------------------------------------------------------------------------

char * jsonClimateWater( char * mac, int out, int ret ) {
    jsonMessage[0] = '\0';
    catName( "status" );
    catString( " : { " );
    catNameValueString( "cmh", mac );
    if ( out >= 0 ) {
        catString( ", " );
        catNameValueInt( "out", out );
    }
    if ( ret >= 0 ) {
        catString( ", " );
        catNameValueInt( "ret", ret );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonClimateBurner( char * mac, char * burner, int power ) {
    jsonMessage[0] = '\0';
    catName( "status" );
    catString( " : { " );
    catNameValueString( "cmh", mac );
    if ( burner >= 0 ) {
        catString( ", " );
        catNameValueString( "burner", burner );
    }
    if ( power >= 0 ) {
        catString( ", " );
        catNameValueInt( "power", power );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonClimateCooler( char * mac, char * cooler, int power ) {
    jsonMessage[0] = '\0';
    catName( "status" );
    catString( " : { " );
    catNameValueString( "cmc", mac );
    if ( cooler >= 0 ) {
        catString( ", " );
        catNameValueString( "cooler", cooler );
    }
    if ( power >= 0 ) {
        catString( ", " );
        catNameValueInt( "power", power );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Topo Commands
// ------------------------------------------------------------------------

char * jsonTopoClear( void ) {
    jsonMessage[0] = '\0';
    catName( "tp" );
    catString( " : { " );
    catNameValueString( "cmd", "clearall" );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonTopoClearTopo( void ) {
    jsonMessage[0] = '\0';
    catName( "tp" );
    catString( " : { " );
    catNameValueString( "cmd", "cleartopo" );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonTopoEnd( void ) {
    jsonMessage[0] = '\0';
    catName( "tp" );
    catString( " : { " );
    catNameValueString( "cmd", "endconf" );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonTopoGet( void ) {
    jsonMessage[0] = '\0';
    catName( "tp" );
    catString( " : { " );
    catNameValueString( "cmd", "getconf" );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonTopoUpload( void ) {
    jsonMessage[0] = '\0';
    catName( "tp" );
    catString( " : { " );
    catNameValueString( "cmd", "upload" );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonTopoStatus( int status ) {
    jsonMessage[0] = '\0';
    catName( "tp" );
    catString( " : { " );
    catNameValueInt( "status", status );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Topo Add
// ------------------------------------------------------------------------

static char * jsonTopoAddDevice( char * device, char * mac,
                                 char * name, char * type, int sid ) {
    jsonMessage[0] = '\0';
    catName( "tp+" );
    catString( " : { " );
    catNameValueString( device, mac );
    if ( name != NULL ) {
        catString( ", " );
        catNameValueString( "nm", name );
    }
    if ( type != NULL ) {
        catString( ", " );
        catNameValueString( "ty", type );
    }
    if ( sid > 0 ) {
        catString( ", " );
        catNameValueInt( "sid", sid );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonTopoAddManager( char * mac, char * name, char * type ) {
    return( jsonTopoAddDevice( "man", mac, name, type, 0 ) );
}

char * jsonTopoAddUI( char * mac, char * name ) {
    return( jsonTopoAddDevice( "ui", mac, name, NULL, 0 ) );
}

char * jsonTopoAddSensor( char * mac, char * name ) {
    return( jsonTopoAddDevice( "sen", mac, name, NULL, 0 ) );
}

char * jsonTopoAddPump( char * mac, char * name, int sid ) {
    return( jsonTopoAddDevice( "pmp", mac, name, NULL, sid ) );
}

char * jsonTopoAddPlug( char * mac, char * name ) {
    return( jsonTopoAddDevice( "plg", mac, name, NULL, 0 ) );
}

char * jsonTopoAddLamp( char * mac, char * name, char * type ) {
    return( jsonTopoAddDevice( "lmp", mac, name, type, 0 ) );
}

char * jsonTopoAddNone( void ) {
    jsonMessage[0] = '\0';
    catName( "tp+" );
    catString( " : { " );
    catNameValueString( "cmd", "none" );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Topo Response
// ------------------------------------------------------------------------

char * jsonTopoResponse( int errcode ) {
    jsonMessage[0] = '\0';
    catName( "tprsp" );
    catString( " : { " );
    catNameValueInt( "errcode", errcode );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Tunnel
// ------------------------------------------------------------------------

char * jsonTunnelOpen( char * mac ) {
    jsonMessage[0] = '\0';
    catName( "tunnel" );
    catString( " : { " );
    catNameValueString( "open", mac );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonTunnelClose( void ) {
    jsonMessage[0] = '\0';
    catName( "tunnel" );
    catString( " : { " );
    catNameValueInt( "close", 0 );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Error
// ------------------------------------------------------------------------

char * jsonError( int error ) {
    jsonMessage[0] = '\0';
    catNameValueInt( "error", error );
    catString( "\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// DbGet
// ------------------------------------------------------------------------

char * jsonDbGet( char * mac ) {
    jsonMessage[0] = '\0';
    catName( "dbget" );
    catString( " : { " );
    catNameValueString( "mac", mac );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonDbGetRoom( int room, int ts ) {
    jsonMessage[0] = '\0';
    catName( "dbget" );
    catString( " : { " );
    catNameValueInt( "room", room );
    if ( ts > 0 ) {
        catString( ", " );
        catNameValueInt( "ts", ts );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonDbGetBegin( int ts ) {
    jsonMessage[0] = '\0';
    catName( "dbget" );
    catString( " : { " );
    catNameValueInt( "begin", ts );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonDbGetEnd( int count ) {
    jsonMessage[0] = '\0';
    catName( "dbget" );
    catString( " : { " );
    catNameValueInt( "end", count );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// DbEdit
// ------------------------------------------------------------------------

char * jsonDbEditAdd( char * mac, int dev, char * ty ) {
    jsonMessage[0] = '\0';
    catName( "dbedit" );
    catString( " : { " );
    catNameValueString( "add", mac );
    catString( ", " );
    catNameValueInt( "dev", dev );
    if ( ty != NULL ) {
        catString( ", " );
        catNameValueString( "ty", ty );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonDbEditRem( char * mac ) {
    jsonMessage[0] = '\0';
    catName( "dbedit" );
    catString( " : { " );
    catNameValueString( "rem", mac );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonDbEditClr( char * clr ) {
    jsonMessage[0] = '\0';
    catName( "dbedit" );
    catString( " : { " );
    catNameValueString( "clr", clr );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Group support
// ------------------------------------------------------------------------

char * jsonLampGroup( char * mac, char * grp, int grpid,
                                  char * scn, int scnid ) {
    jsonMessage[0] = '\0';
    catName( "lmp" );
    catString( " : { " );
    catNameValueString( "mac", mac );
    if ( grp != NULL ) {
        catString( ", " );
        catNameValueString( "grp", grp );
    }
    if ( grpid > 0 ) {
        catString( ", " );
        catNameValueInt( "grpid", grpid );
    }
    if ( scn != NULL ) {
        catString( ", " );
        catNameValueString( "scn", scn );
    }
    if ( scnid > 0 ) {
        catString( ", " );
        catNameValueInt( "scnid", scnid );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonGroup( int grpid, char * scn, int scnid, char * cmd,
                  int lvl, int rgb, int kelvin, int xcr, int ycr ) {

    if ( rgb >= 0 ) {
        // If RGB is specified, then we convert that to the xy color space
        rgb2xy( rgb, &xcr, &ycr );

        // If the lvl was not specified then we use 100%
        lvl = ( lvl >= 0 ) ? lvl : 100;
    } else if ( kelvin >= 0 ) {
        // If kelvin is specified, then we convert that to the xy color space
        kelvin2xy( kelvin, &xcr, &ycr );

        // If the lvl was not specified then we use 100%
        lvl = ( lvl >= 0 ) ? lvl : 100;
    } else if ( cmd != NULL ) {
        if ( strcmp( cmd, "on" ) == 0 ) {
            // If the lvl was not specified then we use 100%
            lvl = ( lvl >= 0 ) ? lvl : 100;
        } else {
            lvl = -1;
        }
    }

    jsonMessage[0] = '\0';
    catName( "grp" );
    catString( " : { " );
    catNameValueInt( "grpid", grpid );
    if ( scn != NULL ) {
        catString( ", " );
        catNameValueString( "scn", scn );
    }
    if ( scnid >= 0 ) {
        catString( ", " );
        catNameValueInt( "scnid", scnid );
    }
    if ( cmd != NULL ) {
        catString( ", " );
        catNameValueString( "cmd", cmd );
    }
    if ( lvl >= 0 ) {
        catString( ", " );
        catNameValueInt( "lvl", lvl );
    }
    if ( xcr >= 0 ) {
        catString( ", " );
        catNameValueInt( "xcr", xcr );
    }
    if ( ycr >= 0 ) {
        catString( ", " );
        catNameValueInt( "ycr", ycr );
    }
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------------
// Gateway properties
// ------------------------------------------------------------------------

char * jsonGwProperties( char * name, int ifversion ) {
    jsonMessage[0] = '\0';
    catName( "gateway" );
    catString( " : { " );
    catNameValueString( "name", name );
    catString( ", " );
    catNameValueInt( "ifversion", ifversion );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

// ------------------------------------------------------------------
// Process start/stop
// ------------------------------------------------------------------

char * jsonProcStart( char * proc ) {
    jsonMessage[0] = '\0';
    catName( "proc" );
    catString( " : { " );
    catNameValueString( "start", proc );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonProcStop( char * proc ) {
    jsonMessage[0] = '\0';
    catName( "proc" );
    catString( " : { " );
    catNameValueString( "stop", proc );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

char * jsonProcRestart( char * proc ) {
    jsonMessage[0] = '\0';
    catName( "proc" );
    catString( " : { " );
    catNameValueString( "restart", proc );
    catString( " }\n" );
    DEBUG_PRINTF( "JSON message length = %d\n", (int)strlen( jsonMessage ) );
    return( jsonMessage );
}

