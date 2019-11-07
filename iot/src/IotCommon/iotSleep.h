// ------------------------------------------------------------------
// IotSleep - include file
// ------------------------------------------------------------------
// IoT sleep functions
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

#define IOT_SLEEP(a)   iotSleep(  __func__, a )
#define IOT_MSLEEP(a)  iotMsleep( __func__, a )

int  iotMsleep( const char * who, int msec );
int  iotSleep( const char * who, int sec );
