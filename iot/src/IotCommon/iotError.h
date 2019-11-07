// ------------------------------------------------------------------
// IOT Error include file
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

#define IOT_ERROR_NONE                    0

// #define IOT_ERROR_NO_SECRET               1
// #define IOT_ERROR_WRONG_SECRET            2
// #define IOT_ERROR_NOT_AUTHORIZED          3
#define IOT_ERROR_AUTHORIZE_TIMEOUT       4
#define IOT_ERROR_AUTHORIZE_RESPONSE      5
#define IOT_ERROR_NO_TLS_RECEIVED         6

#define IOT_ERROR_NO_MAC                  11
#define IOT_ERROR_NON_EXISTING_MAC        12
#define IOT_ERROR_NON_TUNNEL_MAC          13
#define IOT_ERROR_ALREADY_EXISTING_MAC    14
#define IOT_ERROR_TUNNEL_SEND             15

#define IOT_ERROR_DEVICE_HAS_NO_GET       20
#define IOT_ERROR_DEVICE_HAS_NO_CONTROLS  21
#define IOT_ERROR_DEVICE_MISSING          22

#define IOT_ERROR_TOPO_ILLEGAL_CMD        30
#define IOT_ERROR_TOPO_MISSING_NAME       31
#define IOT_ERROR_TOPO_TIMEOUT            32
#define IOT_ERROR_TOPO_NOT_ACCEPTED       33
#define IOT_ERROR_TOPO_OVERRUN            34

#define IOT_ERROR_QUEUE_CREATE            60
#define IOT_ERROR_QUEUE_OPEN              61
#define IOT_ERROR_QUEUE_READ              62
#define IOT_ERROR_QUEUE_WRITE             63
// #define IOT_ERROR_QUEUE_NAME              64
// #define IOT_ERROR_QUEUE_NOT_FOUND         65
#define IOT_ERROR_QUEUE_BUFSIZE           66
#define IOT_ERROR_QUEUE_OVERRUN           67

#define IOT_ERROR_SOCKET_OPEN             70
#define IOT_ERROR_SOCKET_READ             71
#define IOT_ERROR_SOCKET_WRITE            72

#define IOT_ERROR_DB_OPEN                 80
#define IOT_ERROR_DB_CREATE               81

extern int iotError;

char * iotErrorStringRaw( int err );
char * iotErrorString( void );
