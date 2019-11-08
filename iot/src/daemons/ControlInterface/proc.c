// ------------------------------------------------------------------
// Proc
// ------------------------------------------------------------------
// Parses PROC JSON messages from the Control webpage
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup ci
 * \file
 * \brief JSON parser to "proc" messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "parsing.h"
#include "proc.h"

// #define PROC_DEBUG

#ifdef PROC_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* PROC_DEBUG */

// ------------------------------------------------------------------
// Globals
// ------------------------------------------------------------------



// ------------------------------------------------------------------
// Attributes
// ------------------------------------------------------------------

#define NUMINTATTRS  0
static char * intAttrs[NUMINTATTRS] = { };

#define NUMSTRINGATTRS  3
static char * stringAttrs[NUMSTRINGATTRS] = { "start", "stop", "restart" };
static int    stringMaxlens[NUMSTRINGATTRS] = { 16, 16, 16 };

// ------------------------------------------------------------------
// Functions
// ------------------------------------------------------------------

/**
 * \brief Init the JSON Parser for this type of commands
 */

void procInit( void ) {
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        parsingAddIntAttr( intAttrs[i] );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        parsingAddStringAttr( stringAttrs[i], stringMaxlens[i] );
    }
}

#ifdef PROC_DEBUG
static void procDump ( void ) {
    printf( "Proc dump:\n" );
    int i;
    for ( i=0; i<NUMINTATTRS; i++ ) {
        printf( "- %-12s = %d\n", intAttrs[i], parsingGetIntAttr( intAttrs[i] ) );
    }
    for ( i=0; i<NUMSTRINGATTRS; i++ ) {
        printf( "- %-12s = %s\n", stringAttrs[i], parsingGetStringAttr( stringAttrs[i] ) );
    }
}
#endif

// -------------------------------------------------------------
// init.d helper
// -------------------------------------------------------------
#ifndef TARGET_LINUX_PC
static int initd( char * prog, char * ss ) {
    char cmd[80];
#ifdef TARGET_RASPBIAN
    // For this to work, www-data must be made part of the sudoers group: sudo adduser www-data sudo
    // AND the password must be surpressed: sudo visudo
    // Add following line to the end of this file: www-data ALL=(ALL) NOPASSWD: ALL
    sprintf( cmd, "sudo /etc/init.d/iot_%s_initd %s > /dev/null", prog, ss );
#else
//    sprintf( cmd, "/etc/init.d/iot_%s_initd %s > /dev/null", prog, ((start)?"start":"stop") );
    sprintf( cmd, "/etc/init.d/iot_%s_initd %s", prog, ss );
#endif
    return( system( cmd ) );
}
#endif

// ------------------------------------------------------------------
// Handle
// ------------------------------------------------------------------

/**
 * \brief JSON Parser for GW IoT process commands (e.g. start, stop, restart). 
 * Processes are controlled via the init.d interface.
 * \retval 0 When OK
 */

int procHandle( void ) {
#ifdef PROC_DEBUG
    procDump();
#endif

    char * start, * stop, * restart;

    start = parsingGetStringAttr0( "start" );
    stop = parsingGetStringAttr0( "stop" );
    restart = parsingGetStringAttr0( "restart" );

#ifdef TARGET_LINUX_PC
    char cmd[80];

    if ( start != NULL ) {
        DEBUG_PRINTF( "Start %s\n", start );
        sprintf( cmd, "/tmp/iot-test/usr/bin/iot_%s", start);
    } else if ( stop != NULL ) {
        DEBUG_PRINTF( "Stop %s\n", stop );
        sprintf( cmd, "/tmp/iot-test/usr/bin/killbyname iot_%s", stop);
    } else if ( restart != NULL ) {
        printf( "restart not implemented\n");
        sleep( 1 );
    }
    return( system( cmd ) );
#else
    if ( start != NULL ) {
        DEBUG_PRINTF( "Start %s\n", start );
        initd( start, "start" );

    } else if ( stop != NULL ) {
        DEBUG_PRINTF( "Stop %s\n", stop );
        initd( stop, "stop" );

    } else if ( restart != NULL ) {
        DEBUG_PRINTF( "Restart %s\n", restart );
        initd( restart, "stop" );
        sleep( 1 );
        initd( restart, "start" );
    }
    return( 0 );
#endif
}

