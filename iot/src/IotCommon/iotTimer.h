// ------------------------------------------------------------------
// IoT Timer - include file
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2015. All rights reserved
// ------------------------------------------------------------------

typedef void (*timerCb)( int );

int timerStart( int msec, timerCb cb, int par );
int timerStop( void );
