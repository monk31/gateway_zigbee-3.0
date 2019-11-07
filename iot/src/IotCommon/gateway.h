// ------------------------------------------------------------------
// Gateway include file
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

typedef struct _wireless {
    int channel;    
    char ssid[40];
} wireless_t;


typedef struct _interface {
    int proto;
    char ip[40];
    char netmask[40];
} interface_t;

typedef struct _network {
    interface_t interface[2];
} network_t;

char * myIP( void );
char * myHostname( void );
int    checkOpenFiles( int mentionAbove );

int    gwGetWireless( wireless_t * wt );
int    gwGetNetwork( network_t * nt );
int    gwGetTimezone( char * tz, char * zn );
char * gwSerializeSystem( int maxlen, char * buf );

int    gwConfigEditOption( char * filename, char * section, char * option, char * strval );


