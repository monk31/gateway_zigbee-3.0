// ------------------------------------------------------------------
// Control Interface socket program
// ------------------------------------------------------------------
// IoT program that handles control requests from the LAN, e.g.
// DBGET, control and topology
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \file
 * \section controlinterface Control Interface
 * \brief Control Interface socket program that handles control requests from the LAN
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <mqueue.h>
#include <limits.h>
#include <time.h>
#include <pthread.h>

#include "parsing.h"
#include "json.h"
#include "jsonCreate.h"
#include "socket.h"
#include "systemtable.h"
#include "newDb.h"
#include "dump.h"
#include "newLog.h"

#include "topo.h"
#include "dbget.h"
#include "dbEdit.h"
#include "ctrl.h"
#include "sensor.h"
#include "cmd.h"
#include "lmp.h"
#include "grp.h"
#include "proc.h"
#include "iotError.h"
#include "iotSleep.h"
#include "gateway.h"

#define SOCKET_HOST         "localhost"

#define SOCKET_PORT         "2001"

#define INPUTBUFFERLEN      200

/*#define MAIN_DEBUG*/

#ifdef MAIN_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* MAIN_DEBUG */

#define MAXCHILDREN   10

// -------------------------------------------------------------
// Globals
// -------------------------------------------------------------

int globalClientSocketHandle;

// -------------------------------------------------------------
// Globals, local
// -------------------------------------------------------------

static int running = 1;
static int parent  = 1;

static pid_t children[MAXCHILDREN];

static char socketHost[80];
static char socketPort[80];

// -------------------------------------------------------------
// Child handling
// -------------------------------------------------------------

static int addChild( int pid ) {
    int i;
    for ( i=0; i<MAXCHILDREN; i++ ) {
        if ( children[i] == (pid_t)0 ) {
            DEBUG_PRINTF( "Add child %d to %d\n", pid, i );
            children[i] = pid;
            return( i );
        }
    }
    printf( "** ERROR: Add child %d\n", pid );
    return( -1 );
}

static int remChild( int pid ) {
    int i;
    for ( i=0; i<MAXCHILDREN; i++ ) {
        if ( children[i] == pid ) {
            DEBUG_PRINTF( "Rem child %d from %d\n", pid, i );
            children[i] = (pid_t)0;
            return( i );
        }
    }
    printf( "** ERROR: rem child %d\n", pid );
    return( -1 );
}

static int numChildren( void ) {
    int i, cnt = 0;
    for ( i=0; i<MAXCHILDREN; i++ ) {
        if ( children[i] != (pid_t)0 ) cnt++;
    }
    return( cnt );
}

static void killAllChildren( void ) {
    DEBUG_PRINTF( "Kill all children\n" );
    int i;
    for ( i=0; i<MAXCHILDREN; i++ ) {
        kill( children[i], SIGKILL );
    }
}

static int killOldest( void ) {
    int i, oldest = INT_MAX;
    for ( i=0; i<MAXCHILDREN; i++ ) {
        if ( children[i] < oldest ) oldest = children[i];
    }
    if ( oldest != INT_MAX ) {
        DEBUG_PRINTF( "Kill oldest child %d from %d\n",
                      children[oldest], oldest );
        kill( children[oldest], SIGKILL );
    } else {
        printf( "** ERROR: kill oldest child\n" );
    }
    return( oldest );
}

static void initChildren( void ) {
    int i;
    for ( i=0; i<MAXCHILDREN; i++ ) {
        children[i] = (pid_t)0;
    }
}

// -------------------------------------------------------------
// Send socket response
// -------------------------------------------------------------

void sendResponse( char * jsonResponseString ) {
    if ( jsonResponseString != NULL ) {
        DEBUG_PRINTF( "jsonResponse = %s, globalSocket = %d\n",
               jsonResponseString, globalClientSocketHandle );
#ifdef MAIN_DEBUG
        sprintf( logbuffer, "Response - %s", jsonResponseString );
        newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, logbuffer );
#endif
        if ( socketWrite( globalClientSocketHandle,
                     jsonResponseString, strlen(jsonResponseString) ) < 0 ) {
            if ( errno != EPIPE ) {   // Broken pipe
                sprintf( logbuffer, "Socket write failed(%s)",
                         strerror(errno));
                newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, logbuffer );
            }
        }
    }
}

// -------------------------------------------------------------
// Parsing
// -------------------------------------------------------------

static void ci_onError(int error, char * errtext, char * lastchars) {
    printf("onError( %d, %s ) @ %s\n", error, errtext, lastchars);
}

static void ci_onObjectStart(char * name) {
    DEBUG_PRINTF("onObjectStart( %s )\n", name);
    parsingReset();
}

static void ci_onObjectComplete(char * name) {
    DEBUG_PRINTF("onObjectComplete( %s )\n", name);
    char * jsonResponseString = NULL;
    int ret, error = IOT_ERROR_NONE;
    
    sprintf( logbuffer, "Command: %s", name );
    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, logbuffer );

    if ( strcmp( name, "dbget" ) == 0 ) {             // From phone
        if ( !dbgetHandle() ) {
            error = iotError;
        }

    } else if ( strcmp( name, "dbedit" ) == 0 ) {     // From NFC reader
        if ( !dbEditHandle() ) {
            error = iotError;
        }

    } else if ( strcmp( name, "ctrl" ) == 0 ) {       // From phone or website
        if ( ctrlHandle() < 0 ) {
            error = iotError;
        }

    } else if ( strcmp( name, "cmd" ) == 0 ) {        // From control website
        if ( cmdHandle() < 0 ) {
            error = iotError;
        }

    } else if ( strcmp( name, "lmp" ) == 0 ) {        // From phone or website
        if ( lmpHandle() < 0 ) {
            error = iotError;
        }

    } else if ( strcmp( name, "grp" ) == 0 ) {        // From phone or website
        if ( grpHandle() < 0 ) {
            error = iotError;
        }

    } else if ( strcmp( name, "sensor" ) == 0 ) {     // From WSN phone (DBP)
        if ( sensorHandle() < 0 ) {
            error = iotError;
        }

    } else if ( ( strcmp( name, "tp" ) == 0 ) ||
                ( strcmp( name, "tp+" ) == 0 ) ) {        // From phone
        ret = topoHandle();
        if ( ret < 0 ) {
            error = iotError;
        } else if ( ret ) {
            jsonResponseString = jsonTopoStatus( 1 );
        }

    } else if ( strcmp( name, "proc" ) == 0 ) {     // From website
        if ( !procHandle() ) {
            error = iotError;
        }

    }

    if ( ( jsonResponseString == NULL ) && ( error != IOT_ERROR_NONE ) ) {
        jsonResponseString = jsonError( error );
    }

    sendResponse( jsonResponseString );

    // Reset global error after each completed object
    iotError = IOT_ERROR_NONE;
}

static void ci_onArrayStart(char * name) {
    // printf("onArrayStart( %s )\n", name);
}

static void ci_onArrayComplete(char * name) {
    // printf("onArrayComplete( %s )\n", name);
}

static void ci_onString(char * name, char * value) {
    DEBUG_PRINTF("onString( %s, %s )\n", name, value);
    parsingStringAttr( name, value );
}

static void ci_onInteger(char * name, int value) {
    DEBUG_PRINTF("onInteger( %s, %d )\n", name, value);
    parsingIntAttr( name, value );
}

// -------------------------------------------------------------
// Byte convertion before processing
// -------------------------------------------------------------

static char convertPrintable( char c ) {
    if ( c >= ' ' && c <= '~' ) return( c );
    if ( c == '\n' ) return( c );
    if ( c == '\r' ) return( c );
    return( '.' );
}

// ------------------------------------------------------------------------
// Handle client
// ------------------------------------------------------------------------

/**
 * \brief Client child process to parse and handle the JSON commands.
 * \param socketHandle Handle to the client's TCP socket
 */

static void handleClient( int socketHandle ) {

    newDbOpen();
    
    // printf( "Handling client %d\n", socketHandle );
    globalClientSocketHandle = socketHandle;

    // Start with clean sheet
    iotError = IOT_ERROR_NONE;
    int ok = 1;

    char socketInputBuffer[INPUTBUFFERLEN + 2];

    jsonReset();

    while( ok ) {
        int len;
        // len = socketRead( socketHandle, socketInputBuffer, INPUTBUFFERLEN ); 
        len = socketReadWithTimeout( socketHandle, 
                  socketInputBuffer, INPUTBUFFERLEN, 10 ); 
        if ( len <= 0 ) {
            ok = 0;
        } else {
            int i;
            for ( i=0; i<len; i++ ) {
                // Transfer all non-printable bytes into '.'
                socketInputBuffer[i] = convertPrintable( socketInputBuffer[i] );
            }

#ifdef MAIN_DEBUG
            // printf( "Incoming packet '%s', length %d\n",
            //          socketInputBuffer, len );
            dump( socketInputBuffer, len );
#endif
            sprintf( logbuffer, "Socket - %s", socketInputBuffer );
            newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, logbuffer );
            
            for ( i=0; i<len; i++ ) {
                jsonEat( socketInputBuffer[i] );
            }
        }
    }

    newDbClose();
}

// -------------------------------------------------------------
// DB saver
// -------------------------------------------------------------

/**
 * \brief Background thread that saves the IoT Database from shared-memory to file 
 * when updates are available in a rate of once per 10 seconds.
 * \param arg Not used
 * \retval NULL Null pointer
 */

static void * dbSaveThread( void * arg ) {
    int running = *((int *)arg);
    sleep( 5 );
    while ( running ) {
        newDbFileLock();
        newDbSave();
        newDbFileUnlock();
        sleep( 10 );
    }
    return NULL;
}

// -------------------------------------------------------------
// Signal handler to kill all children
// -------------------------------------------------------------

static void vQuitSignalHandler (int sig) {

    int pid, status;
    DEBUG_PRINTF("Got signal %d (%d)\n", sig, parent);

    switch ( sig ) {
    case SIGTERM:
    case SIGINT:
    case SIGKILL:
        DEBUG_PRINTF("Switch off (%d)\n", parent);
        running = 0;
        if ( parent ) {
            killAllChildren();
        }
        exit(0);
        break;
    case SIGCHLD:
        while ( ( pid = waitpid( -1, &status, WNOHANG ) ) > 0 ) {
            remChild( pid );
            DEBUG_PRINTF("Child %d stopped with %d\n", pid, status);
        }
        break;
    }

    signal (sig, vQuitSignalHandler);
}

// ------------------------------------------------------------------------
// Main
// ------------------------------------------------------------------------

/**
 * \brief Control Interface's main entry point: opens the IoT database,
 * sets a start marker in the IoT database (for the SW update mechanism),
 * starts the IoT Database Save thread,
 * initializes the JSON parsers and opens the server socket to wait for clients.
 * Each client gets its own (forked) child process to handle the commands.
 * When there are too many clients, then the oldest one is killed (could be a hangup).
 * \param argc Number of command-line parameters
 * \param argv Parameter list (-h = help, -H <ip> is IP address, -P <port> = TCP port, -c = empty DB and exit immediately)
 */

int main( int argc, char * argv[] ) {
    int skip;
    signed char opt;
    
    initChildren();

    // Install signal handlers
    signal(SIGTERM, vQuitSignalHandler);
    signal(SIGINT,  vQuitSignalHandler);
    signal(SIGCHLD, vQuitSignalHandler);

    printf( "\n\nControl Interface Started\n" );

    strcpy( socketHost, SOCKET_HOST );
    strcpy( socketPort, SOCKET_PORT );

    while ( ( opt = getopt( argc, argv, "hH:P:c" ) ) != -1 ) {
        switch ( opt ) {
        case 'h':
            printf( "Usage: ci [-H host] [-P port] [-c]\n\n");
            exit(0);
        case 'H':
            strcpy( socketHost, optarg );
            break;
        case 'P':
            strcpy( socketPort, optarg );
            break;
        case 'c':
            printf( "Empty DB\n\n" );
            newDbOpen();
            newDbEmptySystem();
            newDbEmptyDevices( MODE_DEV_EMPTY_ALL );
            newDbEmptyPlugHist();
            newDbEmptyZcb();
            newDbClose();
            exit( 0 );
            break;
        }
    }

    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Control Interface Started" );
    
    newDbOpen();
    
    // Install DB save thread
    pthread_t tid;
    int err = pthread_create( &tid, NULL, &dbSaveThread, (void *)&running);
    if (err != 0) {
        printf("Can't create DB thread :[%s]", strerror(err));
    } else {
        DEBUG_PRINTF("DB thread created successfully\n");
    }
    
#if 0
    // Test
    int lup;
    lup = newDbGetLastupdateClimate();
    printf( "lup %d\n", lup );
#endif

    // Add sysstart timestamp in system dB
    newDbSystemSaveIntval( "sysstart", (int )time(NULL) );
    
    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Opening host socket" );

    int serverSocketHandle = socketOpen( socketHost, socketPort, 1 );
    if ( serverSocketHandle >= 0 ) {
        
        jsonSetOnError(ci_onError);
        jsonSetOnObjectStart(ci_onObjectStart);
        jsonSetOnObjectComplete(ci_onObjectComplete);
        jsonSetOnArrayStart(ci_onArrayStart);
        jsonSetOnArrayComplete(ci_onArrayComplete);
        jsonSetOnString(ci_onString);
        jsonSetOnInteger(ci_onInteger);
        jsonReset();

        // printf( "Init parser ...\n" );

        topoInit();
        dbgetInit();
        dbEditInit();
        ctrlInit();
        sensorInit();
        cmdInit();
        lmpInit();
        grpInit();
        procInit();
        
        printf( "Waiting for incoming requests from socket %s/%s ...\n",
            socketHost, socketPort );

        while ( running ) {

            iotError = IOT_ERROR_NONE;

            int clientSocketHandle = socketAccept( serverSocketHandle );
            // printf( "Sockethandle = %d\n", clientSocketHandle );
            if ( clientSocketHandle >= 0 ) {
                if ( running ) {
                    
#if 1
                    pid_t childPID;
                    childPID = fork();

                    if ( childPID < 0 ) {
                        newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Error creating child process\n" );

                    } else if ( childPID == 0 ) {
                        // Child
                        parent = 0;
                        errno  = 0;
                        printf( "+++ Client %d - %d\n", getpid(), errno );
                        socketClose( serverSocketHandle );
                        handleClient( clientSocketHandle );
                        // printf( "============================ %d\n", getpid() );
                        socketClose( clientSocketHandle );
                        char * reason = strerror(errno);
                        if      ( errno == EPIPE ) reason = "client left";
                        else if ( errno == EAGAIN ) reason = "inactive timeout";
                        printf( "--- Client %d - %s\n", getpid(), reason );
                        exit( 0 );

                    } else {
                        // Parent
                        socketClose( clientSocketHandle );
                        if ( addChild( childPID ) >= 0 ) {
                            int num = numChildren();
                            if ( num >= MAXCHILDREN ) {
                                // Max number of children received: wait for
                                // at least one to finish within 10 secs
                                int cnt = 50;
                                do {
                                    IOT_MSLEEP( 200 );
                                } while ( ( cnt-- > 0 ) &&
                                          ( numChildren() >= MAXCHILDREN ) );
                                if ( cnt < 0 ) {
                                    // No child stopped within 10 secs:
                                    // - kill oldest
                                    killOldest();
                                }
                            } else if ( num > ( MAXCHILDREN / 2 ) ) {
                                IOT_MSLEEP( 200 );
                            }
                        } else {
                            // HAZARD - Too many children: kill the new one
                            kill( childPID, SIGKILL );
                        }
                    }
#else
                    handleClient( clientSocketHandle );
                    socketClose( clientSocketHandle );
#endif
                } else {
                    socketClose( clientSocketHandle );
                }
            }

            if ( skip++ > 10 ) {
                skip = 0;
                checkOpenFiles( 5 );   // STDIN,STDOUT,STDERR,ServerSocket,DB
            }
        }

        // printf( "Control Interface exit\n");
        socketClose( serverSocketHandle );

    } else {
        sprintf( logbuffer, "Error opening Control Interface socket %s/%s",
                 socketHost, socketPort );
        newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, logbuffer );
    }
    
    newDbClose();

    newLogAdd( NEWLOG_FROM_CONTROL_INTERFACE, "Exit" );
    
    return( 0 );
}
