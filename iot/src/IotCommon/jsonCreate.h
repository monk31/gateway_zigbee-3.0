// ------------------------------------------------------------------
// JSON Message Constructor - include file
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

#define JSON_API_VERSION            3

#define CMD_SYSTEM_SHUTDOWN         1
#define CMD_SYSTEM_SET_HOSTNAME     2
#define CMD_SYSTEM_SET_SSID         3
#define CMD_SYSTEM_SET_CHANNEL      4
#define CMD_SYSTEM_SET_TIMEZONE     5
#define CMD_SYSTEM_SET_ZONENAME     6
#define CMD_SYSTEM_REBOOT           7
#define CMD_SYSTEM_NETWORK_RESTART  8

// ------------------------------------------------------------------
// Status
// ------------------------------------------------------------------

char * jsonCmdGetStatus( void );
char * jsonZcbStatus( int status );

// ------------------------------------------------------------------
// Logging
// ------------------------------------------------------------------

char * jsonZcbLog( char * text );

// ------------------------------------------------------------------
// Restart
// ------------------------------------------------------------------

char * jsonCmdReset( int mode );
char * jsonZcbResetRsp( int ret );
char * jsonCmdResetRsp( int ret );
char * jsonCmdStartNetwork( void );
char * jsonZcbStartRsp( int ret );
char * jsonCmdErase( void );
char * jsonCmdSystem( int system, char * strval );

// ------------------------------------------------------------------
// Device Announce
// ------------------------------------------------------------------

char * jsonZcbAnnounce( char * mac, char * dev, char * ty );

// ------------------------------------------------------------------
// Version
// ------------------------------------------------------------------

char * jsonCmdGetVersion( void );
char * jsonZcbVersion( int major, int minor );

// ------------------------------------------------------------------
// NFC mode
// ------------------------------------------------------------------

char * jsonCmdNfcmode( int nfcMode );

// ------------------------------------------------------------------
// Set Extended PANID
// ------------------------------------------------------------------

char * jsonCmdSetExtPan( char * extPanID );

// ------------------------------------------------------------------
// Set Channel Mask
// ------------------------------------------------------------------

char * jsonCmdSetChannelMask( char * channelMask );
char * jsonZcbChanMaskRsp( char * channelMask );
char * jsonZcbChanRsp( int channel );

// ------------------------------------------------------------------
// Set Device Type
// ------------------------------------------------------------------

char * jsonCmdSetDeviceType( int devType );

// ------------------------------------------------------------------
// Start Network
// ------------------------------------------------------------------

// char * jsonCmdStartNetwork( void );

// ------------------------------------------------------------------
// Permit Join
// ------------------------------------------------------------------

char * jsonCmdGetPermitJoin( void );
char * jsonZcbRspPermitJoin( int status );
char * jsonCmdSetPermitJoin( char * target, int duration );

// ------------------------------------------------------------------
// Authorize
// ------------------------------------------------------------------

char * jsonCmdAuthorizeRequest( char * mac, char * linkkey );

char * jsonZcbAuthorizeResponse( char * mac, char * nwKey, char * mic, char * tcAddress, 
                                 int keySeq, int channel, char * pan, char * extPan  );

char * jsonCmdAuthorizeOobRequest( char * mac, char * key );

char * jsonZcbAuthorizeOobResponse( char * mac, char * nwKey, char * mic, char * hostExtAddress,
                                    int keySeq, int channel, char * pan, char * extPan,
                                    int tcShortAddr, int deviceId );

// ------------------------------------------------------------------
// Single touch commissioning
// ------------------------------------------------------------------

char * jsonTls( char * label, char * val );

char * jsonLinkInfo( int nw_version, int nw_type, int nw_profile,
                     char * mac, char * linkkey );

char * jsonJoinSecure( int channel, int keysequence, char * pan,
                     char * extendedPan, char * nwkey, 
                     char * mic, char * tcAddress );

char * jsonOobCommissioningRequest( int nw_version, int nw_type, int nw_profile,
                     char * mac, char * key );

char * jsonOobCommissioningResponse( int channel, int keysequence, char * pan,
                     char * extendedPan, char * key,
                     char * mic, char * tcExtAdd,
                     int tcShortAdd, int deviceId );

// ------------------------------------------------------------------
// Manager
// ------------------------------------------------------------------

char * jsonManager( int id, char * mac, char * nm, int rm,
                    char * ty, int joined );

// ------------------------------------------------------------------
// UI
// ------------------------------------------------------------------

char * jsonUI( int id, char * mac, char * nm, int rm, int man,
               int heat, int cool, int joined );
char * jsonUiCtrl( char * mac, int heat, int cool );

// ------------------------------------------------------------------
// Sensor
// ------------------------------------------------------------------

char * jsonSensor( int id, char * mac, char * nm, int rm, int ui,
           int tmp, int hum, int prs, int co2, int bat, int batl, int als,
           int xloc, int yloc, int zloc, int joined );

// ------------------------------------------------------------------
// Pump
// ------------------------------------------------------------------

char * jsonPump( int id, char * mac, char * nm, int rm, int sen,
                 int sid, char * cmd, int lvl, int joined );

// ------------------------------------------------------------------
// Lamp
// ------------------------------------------------------------------

char * jsonLamp( int id, char * mac, char * nm, int rm,
         char * ty, char * cmd, int lvl, int rgb, int kelvin, int joined );
char * jsonLamp_xyY( int id, char * mac, char * nm, int rm,
         char * ty, char * cmd, int lvl, int xcr, int ycr, int joined );

// ------------------------------------------------------------------
// Plug
// ------------------------------------------------------------------

char * jsonPlug( int id, char * mac, char * nm, int rm,
                 char * cmd, int act, int sum, int h24, int joined, int autoinsert );
char * jsonPlugCmd( char * mac, char * cmd );

// ------------------------------------------------------------------
// ACT
// ------------------------------------------------------------------

char * jsonAct( char * mar, int sid, char * cmd, int lvl );

// ------------------------------------------------------------------
// CONTROL
// ------------------------------------------------------------------

char * jsonControl( char * mac, char * cmd, int sid,
                    int lvl, int rgb, int kelvin, int heat, int cool );
char * jsonLampToZigbee( char * mac, char * cmd, int lvl, int rgb, int kelvin );

// ------------------------------------------------------------------
// Climate Manager Status
// ------------------------------------------------------------------

char * jsonClimateWater( char * mac, int out, int ret );
char * jsonClimateBurner( char * mac, char * burner, int power );
char * jsonClimateCooler( char * mac, char * cooler, int power );

// ------------------------------------------------------------------
// Topo Commands
// ------------------------------------------------------------------

char * jsonTopoClear( void );
char * jsonTopoClearTopo( void );
char * jsonTopoEnd( void );
char * jsonTopoGet( void );
char * jsonTopoUpload( void );
char * jsonTopoStatus( int status );

// ------------------------------------------------------------------
// Topo Add
// ------------------------------------------------------------------

char * jsonTopoAddManager( char * mac, char * name, char * type );
char * jsonTopoAddUI( char * mac, char * name );
char * jsonTopoAddSensor( char * mac, char * name );
char * jsonTopoAddPump( char * mac, char * name, int sid );
char * jsonTopoAddPlug( char * mac, char * name );
char * jsonTopoAddLamp( char * mac, char * name, char * type );
char * jsonTopoAddNone( void );

// ------------------------------------------------------------------
// Topo Response
// ------------------------------------------------------------------

char * jsonTopoResponse( int mescnt );

// ------------------------------------------------------------------
// Tunnel
// ------------------------------------------------------------------

char * jsonTunnelOpen( char * mac );
char * jsonTunnelClose( void );

// ------------------------------------------------------------------
// Error
// ------------------------------------------------------------------

char * jsonError( int error );

// ------------------------------------------------------------------
// DbGet
// ------------------------------------------------------------------

char * jsonDbGet( char * mac );
char * jsonDbGetRoom( int room, int ts );
char * jsonDbGetBegin( int ts );
char * jsonDbGetEnd( int count );

// ------------------------------------------------------------------
// DbGet
// ------------------------------------------------------------------

char * jsonDbEditAdd( char * mac, int dev, char * ty );
char * jsonDbEditRem( char * mac );
char * jsonDbEditClr( char * clr );
char * jsonDbGetBegin( int room );
char * jsonDbGetEnd( int count );

// ------------------------------------------------------------------------
// Group support
// ------------------------------------------------------------------------

char * jsonLampGroup( char * mac, char * grp, int grpid,
                      char * scn, int scnid );
char * jsonGroup( int grpid, char * scn, int scnid,
            char * cmd, int lvl, int rgb, int kelvin, int xcr, int ycr );

// ------------------------------------------------------------------------
// Gateway properties
// ------------------------------------------------------------------------

char * jsonGwProperties( char * name, int ifversion );

// ------------------------------------------------------------------
// Process start/stop
// ------------------------------------------------------------------

char * jsonProcStart( char * proc );
char * jsonProcStop( char * proc );
char * jsonProcRestart( char * proc );
