// ------------------------------------------------------------------
// ZCB main
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup zb
 * \file
 * \section zcblinux ZCB-Linux daemon
 * \brief ZCB-Linux program that communicated with the ZCB-JenOS over a serial port
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <time.h>

#include "queue.h"
#include "socket.h"
#include "parsing.h"
#include "nibbles.h"
#include "iotSleep.h"
#include "plugUsage.h"
#include "jsonCreate.h"
#include "json.h"
#include "newLog.h"
#include "dump.h"

#include "cmd.h"
#include "lmp.h"
#include "grp.h"
#include "plg.h"
#include "ctrl.h"
#include "topo.h"
#include "tunnel.h"

#include "ZigbeeConstant.h"
#include "newDb.h"
#include "zcb.h"

// -------------------------------------------------------------
// Macros
// -------------------------------------------------------------

#define MAIN_DEBUG

#ifdef MAIN_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* MAIN_DEBUG */

#ifdef TARGET_RASPBERRYPI
#define SERIAL_PORT       "/dev/ttyUSB0"
#define SERIAL_BAUDRATE   1000000
#else
#define SERIAL_PORT       "/dev/ttyUSB0"
#define SERIAL_BAUDRATE   1000000
#endif

#define INPUTBUFFERLEN    MAXMESSAGESIZE

#define CONTROL_PORT     "2001"

// -------------------------------------------------------------
// Globals
// -------------------------------------------------------------

int verbosity = 1;               /** Default log level */

/** Main loop running flag */
volatile int   bRunning            = 1;

// -------------------------------------------------------------
// Globals, local
// -------------------------------------------------------------

static char inputBuffer[INPUTBUFFERLEN+2];

// -------------------------------------------------------------
// Quit-Signal handler
// -------------------------------------------------------------

/** The signal handler just clears the running flag and re-enables itself. */
static void vQuitSignalHandler (int sig)
{
    DEBUG_PRINTF( "Got signal %d\n", sig);
    
    /* Signal main loop to exit */
    bRunning = 0;
    
    /* Re-enable signal handler */
    signal (sig, vQuitSignalHandler);
    return;
}

// -------------------------------------------------------------
// Send message to IoT queue
// -------------------------------------------------------------

/**
 * \brief Sends a JSON message to a certain IoT queue.
 * \param message JSON message
 */

void dispatchMessage( queueKey queue, char * message ) {
    printf( "Dispatch to queue %d\n", queue );
    int dispatchQueue;
    if ( ( dispatchQueue = queueOpen( queue, 1 ) ) != -1 ) {
        newLogAdd( NEWLOG_FROM_ZCB_OUT, message );
        queueWrite( dispatchQueue, message );
        queueClose( dispatchQueue );
    } else {
        newLogAdd( NEWLOG_FROM_ZCB_OUT, "Could not open queue" );
    }
}

// -------------------------------------------------------------
// Write control
// -------------------------------------------------------------

/**
 * \brief Send JSON command to the Control Interface's TCP socket (e.g. "2001")
 * \param cmd JSON command
 */

int writeControl( char * cmd ) {

    int handle = socketOpen( "localhost", CONTROL_PORT, 0 );

    if ( handle >= 0 ) {

        newLogAdd( NEWLOG_FROM_ZCB_OUT, cmd );
        socketWriteString( handle, cmd );

        socketClose( handle );

        return( 0 );
    } else {
        newLogAdd( NEWLOG_FROM_ZCB_OUT, "Control socket problem" );
    }
    return( 1 );
}

// -------------------------------------------------------------
// Handle device announce
// -------------------------------------------------------------

/**
 * \brief Adds an announced device into the database, as JOINED.
 */
static void zcbHandleAnnounce( char * mac, char * devstr, char * ty ) {

    int dev = DEVICE_DEV_UNKNOWN;
    if ( strcmp( devstr, "man" ) == 0 ) {
        dev = DEVICE_DEV_MANAGER;
    } else if ( strcmp( devstr, "ui" ) == 0 ) {
        dev = DEVICE_DEV_UI;
    } else if ( strcmp( devstr, "sen" ) == 0 ) {
        dev = DEVICE_DEV_SENSOR;
    } else if ( strcmp( devstr, "pmp" ) == 0 ) {
        dev = DEVICE_DEV_PUMP;
    } else if ( strcmp( devstr, "lmp" ) == 0 ) {
        dev = DEVICE_DEV_LAMP;
    } else if ( strcmp( devstr, "plg" ) == 0 ) {
        dev = DEVICE_DEV_PLUG;
    } else if ( strcmp( devstr, "swi" ) == 0 ) {
        dev = DEVICE_DEV_SWITCH;
    }

    // printf( "Joined %s, %s = %d\n", mac, devstr, dev );
    sprintf( logbuffer, "Announce %s", devstr );
    newLogAdd( NEWLOG_FROM_ZCB_OUT, logbuffer );

    newdb_zcb_t sNode;
    newdb_dev_t device;

    /* Recover Node */
    newDbGetZcb(mac, &sNode);

    if ( newDbGetDevice( mac, &device ) ) {
        if ( device.dev != dev ) {
            if ( ( device.dev == DEVICE_DEV_UI && dev  == DEVICE_DEV_SENSOR ) ||
                    ( dev == DEVICE_DEV_UI && device.dev == DEVICE_DEV_SENSOR ) ) {
                device.dev = DEVICE_DEV_UISENSOR;
            } else if ( device.dev == DEVICE_DEV_UNKNOWN ) {
                device.dev = dev;
            }
        }
        newDbStrNcpy( device.ty, ty, LEN_TY );
        device.flags |= FLAG_DEV_JOINED;
        /* Set OnOff state into device DB */
        strcpy(device.cmd, (sNode.bOnOff ? "On" : "Off"));
        newDbSetDevice( &device );
    } else if ( newDbGetNewDevice( mac, &device ) ) {  // 3-sep-15: joins always insert
        device.dev = dev;
        newDbStrNcpy( device.ty, ty, LEN_TY );
        device.flags = FLAG_DEV_JOINED | FLAG_ADDED_BY_GW;
        /* Set OnOff state into device DB */
        strcpy(device.cmd, (sNode.bOnOff ? "On" : "Off"));
        newDbSetDevice( &device );
    }

    if ( dev == DEVICE_DEV_MANAGER ) {
        // When a new manager joins, then we re-send all topo data
        // We do this via the same socket call as 'control.cgi'
        writeControl( jsonTopoUpload() );
    }
}

// -------------------------------------------------------------
// Handle actuator device
// -------------------------------------------------------------

/**
 * \brief Updates device cmd and level in the database (no autoinsert)
 */
static void zcbHandleActuator( char * mac, int sid, char * cmd, int lvl ) {

    if ( cmd ) sprintf( logbuffer, "Actuator %s %s", mac, cmd );
    else if ( lvl >= 0 ) sprintf( logbuffer, "Actuator %s %d", mac, lvl );
    else sprintf( logbuffer, "Actuator %s", mac );
    newLogAdd( NEWLOG_FROM_ZCB_OUT, logbuffer );

    newdb_dev_t device;
    if ( newDbGetDevice( mac, &device ) ) {
        if ( cmd ) newDbStrNcpy( device.cmd, cmd, LEN_CMD );
        if ( lvl >= 0 ) device.lvl = lvl;
        device.flags |= FLAG_DEV_JOINED;
        newDbSetDevice( &device );
    }
}

// -------------------------------------------------------------
// Handle UI device
// -------------------------------------------------------------

/**
 * \brief Updates device heat/cool setpoints in the database (no autoinsert)
 */
static void zcbHandleUI( char * mac, int heat, int cool ) {
    
    newdb_dev_t device;
    if ( newDbGetDevice( mac, &device ) ) {
        if ( heat != INT_MIN ) {
            sprintf( logbuffer, "UI %s: heat %d", mac, heat );
            newLogAdd( NEWLOG_FROM_ZCB_OUT, logbuffer );
        
            if ( !( device.flags & FLAG_UI_IGNORENEXT_HEAT ) ) {
                device.heat = heat;
            } else {
                device.flags &= ~FLAG_UI_IGNORENEXT_HEAT;
            }
        }
        if ( cool != INT_MIN ) {
            sprintf( logbuffer, "UI %s: cool %d", mac, cool );
            newLogAdd( NEWLOG_FROM_ZCB_OUT, logbuffer );
        
            if ( !( device.flags & FLAG_UI_IGNORENEXT_COOL ) ) {
                device.cool = cool;
            } else {
                device.flags &= ~FLAG_UI_IGNORENEXT_COOL;
            }
        }
        device.flags |= FLAG_DEV_JOINED;
        newDbSetDevice( &device );
    }
}

// -------------------------------------------------------------
// Handle Sensor device
// -------------------------------------------------------------

/**
 * \brief Updates sensor device modalities in the database (no autoinsert). Also forward to DBP.
 */
static void zcbHandleSensor( char * mac, int tmp, int hum, int als, int bat, int batl ) {

    sprintf( logbuffer, "Sensor %s: tmp %d", mac, tmp );

    newdb_dev_t device;
    if ( newDbGetDevice( mac, &device ) ) {
        if ( tmp  >= 0 ) {
            sprintf( logbuffer, "Sensor %s: tmp %d", mac, tmp );
            device.tmp = tmp;
        }
        if ( hum  >= 0 ) {
            sprintf( logbuffer, "Sensor %s: hum %d", mac, hum );
            device.hum = hum;
        }
        if ( als  >= 0 ) {
            sprintf( logbuffer, "Sensor %s: als %d", mac, als );
            device.als = als;
        }
        if ( bat  >= 0 ) {
            sprintf( logbuffer, "Sensor %s: bat %d", mac, bat );
            device.bat = bat;
        }
        if ( batl >= 0 ) {
            sprintf( logbuffer, "Sensor %s: batl %d", mac, batl );
            device.batl = batl;
        }
        device.flags |= FLAG_DEV_JOINED;
        newDbSetDevice( &device );
        
        // Tee to DBP
        char * message = jsonSensor( -1, mac, NULL, -1, -1,
                                    tmp, hum, -1, -1, bat, batl, als,
                                    INT_MIN, INT_MIN, INT_MIN, -1 );
        queueWriteOneMessage( QUEUE_KEY_DBP, message );
    }
 
     newLogAdd( NEWLOG_FROM_ZCB_OUT, logbuffer );
 }

// -------------------------------------------------------------
// Parsing
// -------------------------------------------------------------

static void zcb_onError(int error, char * errtext, char * lastchars) {
    printf("onError( %d, %s ) @ %s\n", error, errtext, lastchars);
}

static void zcb_onObjectStart(char * name) {
    DEBUG_PRINTF("onObjectStart( %s )\n", name);
    parsingReset();
}

static void zcb_onObjectComplete(char * name) {
    DEBUG_PRINTF("onObjectComplete( %s )\n", name);
    if ( strcmp( name, "cmd" ) == 0 ) {
        cmdHandle();
    } else if ( strcmp( name, "lmp" ) == 0 ) {
        lmpHandle();
    } else if ( strcmp( name, "grp" ) == 0 ) {
        grpHandle();
    } else if ( strcmp( name, "plg" ) == 0 ) {
        plgHandle();
    } else if ( strcmp( name, "ctrl" ) == 0 ) {
        ctrlHandle();
    } else if ( strcmp( name, "tp" ) == 0 ) {
        topoHandle();
    } else if ( strcmp( name, "tp+" ) == 0 ) {
        topoHandle();
    } else if ( strcmp( name, "tunnel" ) == 0 ) {
        tunnelHandle();
    }
}

static void zcb_onArrayStart(char * name) {
    // printf("onArrayStart( %s )\n", name);
}

static void zcb_onArrayComplete(char * name) {
    // printf("onArrayComplete( %s )\n", name);
}

static void zcb_onString(char * name, char * value) {
    DEBUG_PRINTF("onString( %s, %s )\n", name, value);
    parsingStringAttr( name, value );
}

static void zcb_onInteger(char * name, int value) {
    DEBUG_PRINTF("onInteger( %s, %d )\n", name, value);
    parsingIntAttr( name, value );
}

// ---------------------------------------------------------------------
// Send update interval message to plug meters
// ---------------------------------------------------------------------

/**
 * \brief Sends a new interval setting to a plug meter
 * \param u64IEEEAddress Mac address of the plug meter
 * \param u16UpdateInterval New interval setting
 */

static void SmartPlugUpdateIntervalMsg( uint64_t u64IEEEAddress, uint16_t u16UpdateInterval ) {
  
    char  mac[LEN_MAC_NIBBLE+2];
    u642nibblestr( u64IEEEAddress, mac );
    uint16_t u16ShortAddress = zcbNodeGetShortAddress( mac );
  
    DEBUG_PRINTF("vSendPlugMeterUpdateIntervalMsg (0x%llx -> 0x%04x): interval=%d seconds\n",
		 u64IEEEAddress, u16ShortAddress, u16UpdateInterval); 
    
    int ZCL_MANUFACTURER_CODE = 0x1037;  // NXP
 
    teZcbStatus eStatus = eZCB_WriteAttributeRequest(
	  u16ShortAddress,                                       // ShortAddress
	  E_ZB_CLUSTERID_SIMPLE_METERING,                        // Cluster ID
	  0,                                                     // Direction
	  1,                                                     // Manufacturer Specific
	  ZCL_MANUFACTURER_CODE,                                 // Manufacturer ID
	  E_ZB_ATTRIBUTEID_SM_ATTR_ID_MAN_SPEC_UPDATE_INTERVAL,  // Attr ID
	  E_ZCL_UINT16,                                          // eType
	  (void *)&u16UpdateInterval );                          // &data
						    
    if (E_ZCB_OK != eStatus)
    {
        DEBUG_PRINTF("eZCB_WriteAttributeRequest returned status %d\n", eStatus);
    }
}

// ---------------------------------------------------------------------
// Send reporting configuration message to lamps
// ---------------------------------------------------------------------

#if 0
/**
 * \brief Sends a reporting-configuration message to a lamp so that it reports onoff/level status changes
 * \param u64IEEEAddress Mac address of the lamp
 */

static void LampSendRepConfMsg( uint64_t u64IEEEAddress ) {
  
    char  mac[LEN_MAC_NIBBLE+2];
    u642nibblestr( u64IEEEAddress, mac );
    uint16_t u16ShortAddress = zcbNodeGetShortAddress( mac );
  
    DEBUG_PRINTF("LampSendRepConfMsg (0x%llx -> 0x%04x)\n",
                 u64IEEEAddress, u16ShortAddress ); 
    
    teZcbStatus eStatus = eZCB_SendConfigureReportingCommand(
          u16ShortAddress,                                       // ShortAddress
          E_ZB_CLUSTERID_ONOFF,                                  // Cluster ID
          E_ZB_ATTRIBUTEID_ONOFF_ONOFF,                          // Attr ID
          E_ZCL_UINT8 );                                         // eType
                                                    
    eStatus |= eZCB_SendConfigureReportingCommand(
          u16ShortAddress,                                       // ShortAddress
          E_ZB_CLUSTERID_LEVEL_CONTROL,                          // Cluster ID
          E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL,                   // Attr ID
          E_ZCL_UINT16 );                                        // eType
                                                    
    if (E_ZCB_OK != eStatus)
    {
        DEBUG_PRINTF("eZCB_SendConfigureReportingCommand returned status %d\n", eStatus);
    }
}
#endif

// ---------------------------------------------------------------------
// Send annouce message to IoT
// ---------------------------------------------------------------------

/**
 * \brief Handle device announces depending on the Simple Descriptor device ID.
 * In case of a plug meter, also the update interval is set to a few seconds.
 * \param u64IEEEAddress Mac address of the device
 * \param u16DeviceID Simple descriptor device ID
 */

void announceDevice( uint64_t u64IEEEAddress, uint16_t u16DeviceID ) {
    DEBUG_PRINTF( "Announce --------- Device 0x%llx, ID = 0x%04x\n", u64IEEEAddress, u16DeviceID );

    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );

    switch ( u16DeviceID ) {
    case SIMPLE_DESCR_CONTROL_BRIDGE:
        break;
        
    case SIMPLE_DESCR_SWITCH_ONOFF:
        zcbHandleAnnounce( mac, "swi", "onoff" );
        break;

    case SIMPLE_DESCR_SWITCH_DIMM:
        zcbHandleAnnounce( mac, "swi", "dim" );
        break;

    case SIMPLE_DESCR_SWITCH_COLL_DIMM:
        zcbHandleAnnounce( mac, "swi", "col" );
        break;

//    case SIMPLE_DESCR_LAMP_ONOFF_ZLL:   // 0x0000
    case SIMPLE_DESCR_LAMP_ONOFF:         // 0x0100
        zcbHandleAnnounce( mac, "lmp", "onoff" );
        eZCB_SendBindCommand( u64IEEEAddress, E_ZB_CLUSTERID_ONOFF );
        break;

//    case SIMPLE_DESCR_LAMP_DIMM_ZLL:    // 0x0100
    case SIMPLE_DESCR_LAMP_DIMM:          // 0x0101
        zcbHandleAnnounce( mac, "lmp", "dim" );
        eZCB_SendBindCommand( u64IEEEAddress, E_ZB_CLUSTERID_ONOFF );
        eZCB_SendBindCommand( u64IEEEAddress, E_ZB_CLUSTERID_LEVEL_CONTROL );
        break;

    case SIMPLE_DESCR_LAMP_COLOUR:       // 0x0102
    case SIMPLE_DESCR_LAMP_COLOUR_DIMM:  // 0x0200
    case SIMPLE_DESCR_LAMP_COLOUR_EXT:   // 0x0210
        zcbHandleAnnounce( mac, "lmp", "col" );
        eZCB_SendBindCommand( u64IEEEAddress, E_ZB_CLUSTERID_ONOFF );
        eZCB_SendBindCommand( u64IEEEAddress, E_ZB_CLUSTERID_LEVEL_CONTROL );
        eZCB_SendBindCommand( u64IEEEAddress, E_ZB_CLUSTERID_COLOR_CONTROL );
        break;

    case SIMPLE_DESCR_LAMP_COLOUR_TEMP:
    case SIMPLE_DESCR_LAMP_CCTW:
        zcbHandleAnnounce( mac, "lmp", "tw" );
        eZCB_SendBindCommand( u64IEEEAddress, E_ZB_CLUSTERID_ONOFF );
        eZCB_SendBindCommand( u64IEEEAddress, E_ZB_CLUSTERID_LEVEL_CONTROL );
        eZCB_SendBindCommand( u64IEEEAddress, E_ZB_CLUSTERID_COLOR_CONTROL );
        break;

    case SIMPLE_DESCR_HVAC_HC_UNIT:
        zcbHandleAnnounce( mac, "man", NULL );
        break;

    case SIMPLE_DESCR_THERMOSTAT:
        zcbHandleAnnounce( mac, "ui", NULL );
        break;

    case SIMPLE_DESCR_SIMPLE_SENSOR:
        zcbHandleAnnounce( mac, "sen", NULL );
        break;

    case SIMPLE_DESCR_HVAC_PUMP:
        zcbHandleAnnounce( mac, "pmp", NULL );
        break;

    case SIMPLE_DESCR_SMART_PLUG:
        zcbHandleAnnounce( mac, "plg", NULL );
        eZCB_SendBindCommand( u64IEEEAddress, E_ZB_CLUSTERID_ONOFF );
        eZCB_SendBindCommand( u64IEEEAddress, E_ZB_CLUSTERID_SIMPLE_METERING );
        SmartPlugUpdateIntervalMsg( u64IEEEAddress, 4 );	// was 2
        break;

    default:
        printf( "Illegal simple descriptor (%d)\n", u16DeviceID );
        break;
    }
}

// ------------------------------------------------------------------
// Send act message to IoT
// ------------------------------------------------------------------

static void sendActuatorOnOff( uint64_t u64IEEEAddress, int sid, int data ) {
    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );
    char * cmd = ( data ) ? "on" : "off";
    // printf( "Act message:  %s", jsonAct( mac, sid, cmd, -1 ) );
    zcbHandleActuator( mac, sid, cmd, -1 );
}

// ------------------------------------------------------------------
// Send level message to IoT
// ------------------------------------------------------------------

static void sendActuatorLevel( uint64_t u64IEEEAddress, int data ) {
    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );
    // In JSON we specify level from 0-100
    // In Zigbee, however, level has to be specified from 0-255
    data = ( data * 100 ) / 255;
    zcbHandleActuator( mac, -1, NULL, data );
}

// ------------------------------------------------------------------
// Send thermostat/sensing messages to IoT
// ------------------------------------------------------------------

static void sendTemperature( uint64_t u64IEEEAddress, int data ) {
    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );
    DEBUG_PRINTF( "Temperature 0x%x\n", data );
    zcbHandleSensor( mac, data, -1, -1, -1, -1 );
}

static void sendHumidity( uint64_t u64IEEEAddress, int data ) {
    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );
    DEBUG_PRINTF( "Humidity 0x%x\n", data );
    zcbHandleSensor( mac, -1, data, -1, -1, -1 );
}

static void sendIllumination( uint64_t u64IEEEAddress, int data ) {
    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );
    DEBUG_PRINTF( "Illumination 0x%x\n", data );
    zcbHandleSensor( mac, -1, -1, data, -1, -1 );
}

#if 0
static void sendBattery( uint64_t u64IEEEAddress, int data ) {
    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );
    DEBUG_PRINTF( "Battery 0x%x\n", data );
    zcbHandleSensor( mac, -1, -1, -1, data, -1 );
}

static void sendBatteryLevel( uint64_t u64IEEEAddress, int data ) {
    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );
    DEBUG_PRINTF( "BatteryLevel 0x%x\n", data );
    zcbHandleSensor( mac, -1, -1, -1, -1, data );
}
#endif

// ------------------------------------------------------------------
// Send setpoint messages to IoT
// ------------------------------------------------------------------

static void sendCoolingSetpoint( uint64_t u64IEEEAddress, int data ) {
    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );
    DEBUG_PRINTF( "Cooling setpoint %x\n", data );
    zcbHandleUI( mac, INT_MIN, data );
}

static void sendHeatingSetpoint( uint64_t u64IEEEAddress, int data ) {
    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );
    DEBUG_PRINTF( "Heating setpoint %x\n", data );
    zcbHandleUI( mac, data, INT_MIN );
}

// ------------------------------------------------------------------
// Send plugmeter messages to IoT
// ------------------------------------------------------------------

static uint64_t eSummationDelivered  = 0;
static uint64_t eInstantaneousDemand = 0;
static uint64_t eMeteringDeviceType  = -1;
static uint64_t eUnitOfMeasure       = -1;
static uint64_t eMultiplier          = 0;
static uint64_t eDivisor             = 0;

static void handlePlugData( uint64_t u64IEEEAddress ) {
    
    static int cleanupCnt = 0;
    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );
    
    /* Check that this is an electricity meter */
    if ((E_CLD_SM_MDT_ELECTRIC == eMeteringDeviceType) &&
         ((E_CLD_SM_UOM_KILO_WATTS == eUnitOfMeasure) ||
          (E_CLD_SM_UOM_KILO_WATTS_BCD == eUnitOfMeasure))) {
        uint64_t eSumDeliv = 0;
        int64_t  eInstDemand = 0;

        // Must have received multipler and divisor first before we start reporting
        if (!eMultiplier) return;
        if (!eDivisor)    return;

        if (E_CLD_SM_UOM_KILO_WATTS_BCD == eUnitOfMeasure) {
            int i;
            for (i=0; i<(48/4); i++) {
                uint8_t digit = (eSummationDelivered >> (i * 4)) & 0xF;
                if (digit > 9) {
                    digit = 0;  // should never happen with BCD!!!
                }
                if (0 == i) {
                    eSumDeliv += digit;
                } else {
                    eSumDeliv += (digit * (i * 10));
                }
            }
        } else {
            eSumDeliv = eSummationDelivered;
        }

        eSumDeliv = (eSumDeliv * eMultiplier * 1000) / eDivisor; // now in Watt/hour
        if ( (int)eSumDeliv < 0 ) eSumDeliv = 0;
        
        eInstDemand = (eInstantaneousDemand * eMultiplier * 1000) / eDivisor; // now in Watt
        if ( (int)eInstDemand < 0 ) eInstDemand = 0;
        
        sprintf( logbuffer, "Plug %d/%d", (int)eInstDemand, (int)eSumDeliv );
        newLogAdd( NEWLOG_FROM_ZCB_OUT, logbuffer );

        // Add to dB (no auto-insert)
        newdb_dev_t device;
        if ( newDbGetDevice( mac, &device ) ) {
            device.act = eInstDemand;
            device.sum = eSumDeliv;
            device.flags |= FLAG_DEV_JOINED;
            newDbSetDevice( &device );
        
            int now = (int)time( NULL );
        
            newdb_plughist_t plughist;
            if ( newDbGetMatchingOrNewPlugHist( mac, now/60, &plughist ) ) {
                plughist.sum = eSumDeliv;
                newDbSetPlugHist( &plughist );
            
                // Clean history entries older than one day
                if ( cleanupCnt-- < 0 ) {
                    if ( plugHistoryCleanup( mac, now ) ) {
                        cleanupCnt = 25;
                    }
                }
            }
        }
    }
}

static uint64_t eAcFrequency         = 0;
static uint64_t eRmsVoltage          = 0;
static uint64_t eRmsCurrent          = 0;
static uint64_t eActivePower         = 0;
static uint64_t ePowerFactor         = 0;
static uint64_t eAcVoltageMultiplier = 0;
static uint64_t eAcVoltageDivisor    = 0;
static uint64_t eAcCurrentMultiplier = 0;
static uint64_t eAcCurrentDivisor    = 0;
static uint64_t eAcPowerMultiplier   = 0;
static uint64_t eAcPowerDivisor      = 0;

static void handlePlugData2( uint64_t u64IEEEAddress ) {
    char  mac[16+2];
    u642nibblestr( u64IEEEAddress, mac );
    /* Check that this is an electricity meter */

    // Must have received multiplers and divisors first before we start reporting
    if (!eAcVoltageMultiplier) return;
    if (!eAcCurrentMultiplier) return;
    if (!eAcPowerMultiplier  ) return;
    if (!eAcVoltageDivisor)    return;
    if (!eAcCurrentDivisor)    return;
    if (!eAcPowerDivisor)      return;

    uint64_t voltage = ( eRmsVoltage  * eAcVoltageMultiplier ) / eAcVoltageDivisor;
    uint64_t current = ( eRmsCurrent  * eAcCurrentMultiplier * 1000 ) / eAcCurrentDivisor;  // mA
    uint64_t power   = ( eActivePower * eAcPowerMultiplier   ) / eAcPowerDivisor;

    printf( "Plug details: %d Hz, %d V, %d mA, %d W\n",
            (int)eAcFrequency, (int)voltage, (int)current, (int)power );
}

// ------------------------------------------------------------------
// Handle incoming attributes
// ------------------------------------------------------------------

/**
 * \brief Handles an incoming attribute message
 * \param u16ShortAddress Short address of the device
 * \param u16ClusterID Cluster ID of the atribute
 * \param u16AttributeID Attribute ID
 * \param u64Data Data containes
 * \param u8Endpoint Originating endpoint
 */
void handleAttribute( uint16_t u16ShortAddress,
                      uint16_t u16ClusterID,
                      uint16_t u16AttributeID,
                      uint64_t u64Data,
                      uint8_t  u8Endpoint ) {

    uint64_t u64IEEEAddress = 0;

    u64IEEEAddress = zcbNodeGetExtendedAddress( u16ShortAddress );

    if ( u64IEEEAddress ) {
        switch ( u16ClusterID ) {

        case E_ZB_CLUSTERID_ONOFF:
            switch ( u16AttributeID ) {
            case E_ZB_ATTRIBUTEID_ONOFF_ONOFF:
                sendActuatorOnOff( u64IEEEAddress, u8Endpoint, (int)u64Data );
                break;

            default:
                printf( "Received attribute 0x%04x in ONOFF cluster\n", u16AttributeID );
                break;
            }
            break;

        case E_ZB_CLUSTERID_LEVEL_CONTROL:
            switch ( u16AttributeID ) {
            case E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL:
                sendActuatorLevel( u64IEEEAddress, (int)u64Data );
                break;

            default:
                printf( "Received attribute 0x%04x in LEVEL CONTROL cluster\n", u16AttributeID );
                break;
            }
            break;

        case E_ZB_CLUSTERID_THERMOSTAT:
            switch ( u16AttributeID ) {
            case E_ZB_ATTRIBUTEID_TSTAT_LOCALTEMPERATURE:
                sendTemperature( u64IEEEAddress, (int)u64Data );
                break;

            case E_ZB_ATTRIBUTEID_TSTAT_OCCUPIEDCOOLSETPOINT:
                sendCoolingSetpoint( u64IEEEAddress, (int)u64Data );
                break;

            case E_ZB_ATTRIBUTEID_TSTAT_OCCUPIEDHEATSETPOINT:
                sendHeatingSetpoint( u64IEEEAddress, (int)u64Data );
                break;

            case E_ZB_ATTRIBUTEID_TSTAT_PICOOLINGDEMAND:
            case E_ZB_ATTRIBUTEID_TSTAT_PIHEATINGDEMAND:
            case E_ZB_ATTRIBUTEID_TSTAT_COLTROLSEQUENCEOFOPERATION:
            case E_ZB_ATTRIBUTEID_TSTAT_SYSTEMMODE:
                // Ignore
                DEBUG_PRINTF( "Attribute 0x%02x ignored\n", 
                        u16AttributeID );
                break;

            default:
                DEBUG_PRINTF( "Received attribute 0x%04x in TSTAT cluster\n", 
                        u16AttributeID );
                break;
            }
        break;

        case E_ZB_CLUSTERID_MEASUREMENTSENSING_ILLUM:
            switch ( u16AttributeID ) {
            case E_ZB_ATTRIBUTEID_MS_ILLUM_MEASURED:
                sendIllumination( u64IEEEAddress, (int)u64Data );
                break;

            default:
                DEBUG_PRINTF( "Received attribute 0x%04x in MS-Illum cluster\n", 
                        u16AttributeID );
                break;
            }
            break;

        case E_ZB_CLUSTERID_MEASUREMENTSENSING_TEMP:
            switch ( u16AttributeID ) {
            case E_ZB_ATTRIBUTEID_MS_TEMP_MEASURED:
                sendTemperature( u64IEEEAddress, (int)u64Data );
                break;

            default:
                DEBUG_PRINTF( "Received attribute 0x%04x in MS-Temperature cluster\n", 
                        u16AttributeID );
                break;
            }
            break;

        case E_ZB_CLUSTERID_MEASUREMENTSENSING_HUM:
            switch ( u16AttributeID ) {
            case E_ZB_ATTRIBUTEID_MS_HUM_MEASURED:
                sendHumidity( u64IEEEAddress, (int)u64Data );
                break;

            default:
                DEBUG_PRINTF( "Received attribute 0x%04x in MS-Humidity cluster\n", 
                        u16AttributeID );
                break;
            }
            break;

        case E_ZB_CLUSTERID_SIMPLE_METERING:
            // Note: this implementation assumes that E_ZB_ATTRIBUTEID_SM_INSTANTANEOUS_DEMAND is always sent
            // by a plug and handled as the last attribute from the incoming Simple Metering message
            switch ( u16AttributeID ) {
            case E_ZB_ATTRIBUTEID_SM_METERING_DEVICE_TYPE:
                DEBUG_PRINTF( "Device type 0x%04x\n", (int)u64Data );
                eMeteringDeviceType = (int)u64Data;
                break;

            case E_ZB_ATTRIBUTEID_SM_UNIT_OF_MEASURE:
                DEBUG_PRINTF( "Unit of measure 0x%04x\n", (int)u64Data );
                eUnitOfMeasure = (int)u64Data;
                break;

            case E_ZB_ATTRIBUTEID_SM_MULTIPLIER:
                DEBUG_PRINTF( "Mutltiplier 0x%04x\n", (int)u64Data );
                eMultiplier = (int)u64Data;
                break;

            case E_ZB_ATTRIBUTEID_SM_DIVISOR:
                DEBUG_PRINTF( "Divisor 0x%04x\n", (int)u64Data );
                eDivisor = (int)u64Data;
                break;

            case E_ZB_ATTRIBUTEID_SM_CURRENT_SUMMATION_DELIVERED:
                DEBUG_PRINTF( "Summation delivered 0x%04x\n", (int)u64Data );
                eSummationDelivered = (int)u64Data;
                break;

            case E_ZB_ATTRIBUTEID_SM_INSTANTANEOUS_DEMAND:
                DEBUG_PRINTF( "Instantaneous demand 0x%04x\n", (int)u64Data );
                eInstantaneousDemand = (int)u64Data;
                
                // Assumption: All relevant attributes have been reported
                handlePlugData(u64IEEEAddress);
                break;

            case E_ZB_ATTRIBUTEID_SM_SUMMATION_FORMATTING:
                DEBUG_PRINTF( "Summation formatting 0x%04x\n", (int)u64Data );
                break;

            case E_ZB_ATTRIBUTEID_SM_STATUS:
                DEBUG_PRINTF( "Status 0x%04x\n", (int)u64Data );
                break;

            default:
                DEBUG_PRINTF( ">>>>>>> Received attribute 0x%04x in SimpleMetering cluster\n", 
                        u16AttributeID );
                break;
            }
            break;
            
            
        case E_ZB_CLUSTERID_ELECTRICAL_MEASUREMENT:
            // Note: this implementation assumes that E_ZB_ATTRIBUTEID_EM_AC_POWER_DIVISOR is always sent
            // by a plug and handled as the last attribute from the incoming Electrical Measurement message
            switch ( u16AttributeID ) {
            case E_ZB_ATTRIBUTEID_EM_MEASUREMENT_TYPE:
                DEBUG_PRINTF( "Measurement type 0x%04x\n", (int)u64Data );
                break;
            case E_ZB_ATTRIBUTEID_EM_AC_FREQUENCY:
                DEBUG_PRINTF( "AC frequency 0x%04x\n", (int)u64Data );
                eAcFrequency = (int)u64Data;
                break;
            case E_ZB_ATTRIBUTEID_EM_RMS_VOLTAGE:
                DEBUG_PRINTF( "RMS voltage 0x%04x\n", (int)u64Data );
                eRmsVoltage = (int)u64Data;
                break;
            case E_ZB_ATTRIBUTEID_EM_RMS_CURRENT:
                DEBUG_PRINTF( "RMS current 0x%04x\n", (int)u64Data );
                eRmsCurrent = (int)u64Data;
                break;
            case E_ZB_ATTRIBUTEID_EM_ACTIVE_POWER:
                DEBUG_PRINTF( "Active power 0x%04x\n", (int)u64Data );
                eActivePower = (int)u64Data;
                break;
            case E_ZB_ATTRIBUTEID_EM_REACTIVE_POWER:
                DEBUG_PRINTF( "Re-active power 0x%04x\n", (int)u64Data );
                break;
            case E_ZB_ATTRIBUTEID_EM_APPARANT_POWER:
                DEBUG_PRINTF( "Apparent power 0x%04x\n", (int)u64Data );
                break;
            case E_ZB_ATTRIBUTEID_EM_POWER_FACTOR:
                DEBUG_PRINTF( "Power factor 0x%04x\n", (int)u64Data );
                ePowerFactor = (int)u64Data;
                break;
            case E_ZB_ATTRIBUTEID_EM_AC_VOLTAGE_MULTIPLIER:
                DEBUG_PRINTF( "AC voltage multiplier 0x%04x\n", (int)u64Data );
                eAcVoltageMultiplier = (int)u64Data;
                break;
            case E_ZB_ATTRIBUTEID_EM_AC_VOLTAGE_DIVISOR:
                DEBUG_PRINTF( "AC voltage divisor 0x%04x\n", (int)u64Data );
                eAcVoltageDivisor = (int)u64Data;
                break;
            case E_ZB_ATTRIBUTEID_EM_AC_CURRENT_MULTIPLIER:
                DEBUG_PRINTF( "AC current multiplier 0x%04x\n", (int)u64Data );
                eAcCurrentMultiplier = (int)u64Data;
                break;
            case E_ZB_ATTRIBUTEID_EM_AC_CURRENT_DIVISOR:
                DEBUG_PRINTF( "AC current divisor 0x%04x\n", (int)u64Data );
                eAcCurrentDivisor = (int)u64Data;
                break;
            case E_ZB_ATTRIBUTEID_EM_AC_POWER_MULTIPLIER:
                DEBUG_PRINTF( "AC power multiplier 0x%04x\n", (int)u64Data );
                eAcPowerMultiplier = (int)u64Data;
                break;
            case E_ZB_ATTRIBUTEID_EM_AC_POWER_DIVISOR:
                DEBUG_PRINTF( "AC power divisor 0x%04x\n", (int)u64Data );
                eAcPowerDivisor = (int)u64Data;

                // Assumption: All relevant attributes have been reported
                handlePlugData2( u64IEEEAddress );
                break;
            }
            break;

        default:
            printf( "Received attribute 0x%04x in cluster 0x%04x\n", 
                    u16AttributeID, u16ClusterID );
            break;
        }
    }
}

// ------------------------------------------------------------------
// Main
// ------------------------------------------------------------------

/**
 * \brief ZCB's main entry point: opens the IoT database,
 * opens the serial communication to the ZCB-JenOS (this starts up two extra threads
 * for serial port handling and message handlers), 
 * opens the incoming ZCB-IN queue,
 * initializes the JSON parsers,
 * and waits for incoming queue messages to parse and handle.
 * \param argc Number of command-line parameters
 * \param argv Parameter list (-h = help, -a = set auto-insert, -c = empty DB and exit immediately)
 */

int main(int argc, char *argv[])
{    
    /* Install signal handlers */
    signal(SIGTERM, vQuitSignalHandler);
    signal(SIGINT, vQuitSignalHandler);
    
    newDbOpen();
     
    if (eZCB_Init(SERIAL_PORT, SERIAL_BAUDRATE) != E_ZCB_OK) {
        goto finish;
    }

    while (bRunning) {
        /* Keep attempting to connect to the control bridge */
        if (eZCB_EstablishComms() == E_ZCB_OK) {
            // Wait for initial messages from control bridge
            if (IOT_SLEEP(2)) {
                goto finish;
            }
            
            break;
        }
    }

    int i;
    int numBytes = 0;

    newLogAdd( NEWLOG_FROM_ZCB_OUT, "ZCB-out started" );
    newLogAdd( NEWLOG_FROM_ZCB_IN, "ZCB-in started" );

    int zcbQueue;
    if ( ( zcbQueue = queueOpen( QUEUE_KEY_ZCB_IN, 0 ) ) != -1 ) {

        jsonSetOnError( zcb_onError );
        jsonSetOnObjectStart( zcb_onObjectStart );
        jsonSetOnObjectComplete( zcb_onObjectComplete );
        jsonSetOnArrayStart( zcb_onArrayStart );
        jsonSetOnArrayComplete( zcb_onArrayComplete );
        jsonSetOnString( zcb_onString );
        jsonSetOnInteger( zcb_onInteger );
        jsonReset();

        DEBUG_PRINTF( "Init parsers ...\n" );

        cmdInit();
        lmpInit();
        grpInit();
        plgInit();
        ctrlInit();
        topoInit();
        tunnelInit();

        printf( "Going to read from data queue endlessly...\n\n" );
        
        int start = 0;

        while ( bRunning ) {
            numBytes = queueReadWithMsecTimeout( zcbQueue,
                              inputBuffer, INPUTBUFFERLEN, 2000 );
            if ( numBytes > 0 ) {
#ifdef MAIN_DEBUG
                dump( inputBuffer, numBytes );
#endif

                newLogAdd( NEWLOG_FROM_ZCB_IN, inputBuffer );

                // Reset parser (each line is one command)
                jsonReset();

                for ( i=0; i<numBytes; i++ ) {
                    jsonEat( inputBuffer[i] );
                }
            } else { 
                // newLogAdd( NEWLOG_FROM_ZCB_IN, "ZCB-R heartbeat" );
#if 1
                int ret = eZCB_NeighbourTableRequest( start );
                if ( ret >= 0 ) start = ret;
#else           
                eGetPermitJoining();
#endif
            }
        }

        // dispatchClose();
        queueClose( zcbQueue );
    } else {
        newLogAdd( NEWLOG_FROM_ZCB_IN, "Could not open ZCB queue" );
    }

    DEBUG_PRINTF( "ZCB: Exit\n");
    
    /* Clean up */
    eZCB_Finish();
    
finish:
    DEBUG_PRINTF( "Exiting");
    newDbClose();
    return 0;
}

// -------------------------------------------------------------
// End of file
// -------------------------------------------------------------

