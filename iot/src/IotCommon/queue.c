// ------------------------------------------------------------------
// Queue functions
// ------------------------------------------------------------------
// Offers the IoT queue mechanism and all functions to use it
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \file
 * \brief Offers the IoT queue mechanism and all functions to use it
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>

#include "iotError.h"
#include "newLog.h"
#include "dump.h"
#include "fileCreate.h"
#include "queue.h"

#define QUEUE_DEBUG
#define QUEUE_DEBUG_DETAIL
// #define QUEUE_DEBUG_DUMP

#ifdef QUEUE_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* QUEUE_DEBUG */

#ifdef QUEUE_DEBUG_DETAIL
#define DEBUG_DETAIL(...) printf(__VA_ARGS__)
#else
#define DEBUG_DETAIL(...)
#endif /* QUEUE_DEBUG_DETAIL */

#define QUEUE_KEYS_OFFSET 3773   // Magic number

typedef struct msgbuf {
    long mtype;
    char mdata[MAXMESSAGESIZE];
} msgbuf_t;

// -------------------------------------------------------------
// Open
// -------------------------------------------------------------

/**
 * \brief Open a queue
 * \param key Unique ID of the queue
 * \param forwrite User can specify if he also will write to the queue
 * \returns A handle to the queue, or -1 in case of an error (and sets the global iotError)
 */
int queueOpen( queueKey key, int forwrite ) {

    int mq = -1;
    DEBUG_DETAIL( "Opening queue %d and configure (%d) ...\n", key, forwrite );

    key_t kt = key + QUEUE_KEYS_OFFSET;
    
    mq = msgget( kt, 0666 );
    DEBUG_DETAIL( "Queue mq = %d\n", mq );

    if ( mq == -1 ) {
        if ( errno == 2 ) {
            DEBUG_PRINTF( "Queue does not exist yet, create it\n" );
            mq = msgget( kt, 0666 | IPC_CREAT );
            if ( mq == -1 ) {
                printf( "Error opening queue %d (%d - %s)\n",
                    key, errno, strerror( errno ) );
                iotError = IOT_ERROR_QUEUE_CREATE;
            }
        } else {
            printf( "Error opening queue %d (%d - %s)\n",
                key, errno, strerror( errno ) );
            iotError = IOT_ERROR_QUEUE_OPEN;
        }
    } else {
        DEBUG_DETAIL( "Queue %d opened\n", mq );
    }
    return( mq );
}

// -------------------------------------------------------------
// Write
// -------------------------------------------------------------

/**
 * \brief Write a message into a queue
 * \param queue Handle to the queue
 * \param message Message string to write
 * \returns 1 on success, or 0 on error (and sets the global iotError)
 */
int queueWrite( int queue, char * message ) {

    msgbuf_t SEND_BUFFER;

    // int len = strlen( message ) + 1;     // Inclusive '\0'
    int len = strlen( message );

    DEBUG_PRINTF( "QW (%d): %s", len, message );

    if ( len > ( MAXMESSAGESIZE - 2 ) ) {
        iotError = IOT_ERROR_QUEUE_BUFSIZE;
        return( 0 );
    }

    SEND_BUFFER.mtype = 1;
    strcpy( SEND_BUFFER.mdata, message );

#ifdef QUEUE_DEBUG_DUMP
    dump( SEND_BUFFER.mdata, len );
#endif

    if ( msgsnd( queue, &SEND_BUFFER, len, 0 ) < 0 ) {
        printf( "Error writing to queue (%d - %s)\n",
             errno, strerror( errno ) );
        iotError = IOT_ERROR_QUEUE_WRITE;
        return( 0 );
    }
    return( 1 );
}

// -------------------------------------------------------------
// Read
// -------------------------------------------------------------

/**
 * \brief Reads a message from a queue. Note that this is a blocking read.
 * \param queue Handle to the queue
 * \param message User provided string buffer to receive the queue message
 * \param size Size of the user provided string buffer
 * \returns The length of the received message
 */
int queueRead( int queue, char * message, int size ) {

    msgbuf_t RECEIVE_BUFFER;

    memset( message, 0, size );
    int len = msgrcv( queue, &RECEIVE_BUFFER, MAXMESSAGESIZE, 1, MSG_NOERROR );
    if ( len < 0 ) {
        printf( "Error reading from queue (%d - %s)\n",
             errno, strerror( errno ) );
        iotError = IOT_ERROR_QUEUE_READ;
    } else {
        if ( len >= size ) {
            len = size - 1;
        }
        RECEIVE_BUFFER.mdata[len] = '\0';
        strcpy( message, RECEIVE_BUFFER.mdata );
    }

    DEBUG_PRINTF( "QR (%d): %s", len, message );

#ifdef QUEUE_DEBUG_DUMP
    dump( message, len );
#endif
    return( len );
}

// -------------------------------------------------------------
// Read with timeout
// -------------------------------------------------------------

#define QREAD_POLL  50

/**
 * \brief Reads a message from a queue. This call unblocks after <msec> milli-seconds
 * \param queue Handle to the queue
 * \param message User provided string buffer to receive the queue message
 * \param size Size of the user provided string buffer
 * \param msec Unblocking timeout
 * \returns The length of the received message
 */
int queueReadWithMsecTimeout( int queue, char * message, int size, int msec ) {

    msgbuf_t RECEIVE_BUFFER;

    DEBUG_DETAIL( "QRt %d - %d msec...\n", queue, msec );

    int len = 0, cnt = 0;
    while ( len <= 0 && msec > 0 ) {
        len = msgrcv( queue, &RECEIVE_BUFFER, MAXMESSAGESIZE, 1,
                          MSG_NOERROR | IPC_NOWAIT );
        if ( len < 0 ) {
            if ( errno != ENOMSG ) {
                DEBUG_PRINTF( "WARNING reading from queue (%d - %s)\n",
                     errno, strerror( errno ) );
            }

            msec -= QREAD_POLL;
            if ( msec > 0 ) {
                // Retry after QREAD_POLL msec
                len = 0;
                cnt++;
                usleep( QREAD_POLL * 1000 );
            }
        } else {
            if ( len >= size ) {
                len = size - 1;
            }
            memcpy( message, RECEIVE_BUFFER.mdata, len );
            message[len] = '\0';
	}
    }
        
    if ( len < 0 ) {
        if ( errno != ENOMSG ) {
            DEBUG_PRINTF( "ERROR reading from queue (%d - %s)\n",
             errno, strerror( errno ) );
        }
    } else {
        DEBUG_PRINTF( "QRt (%d, %d): %s", len, cnt, message );

#ifdef QUEUE_DEBUG_DUMP
        dump( message, len );
#endif
    }
    return( len );
}

// -------------------------------------------------------------
// Write one message
// - Opens a queue, writes the message and closes queue again
// -------------------------------------------------------------

/**
 * \brief Write one message. Opens a queue, writes the message and closes queue again
 * \param key Unique ID of the queue
 * \param message Message string to write
 * \returns 1 on success, or 0 on error (and sets the global iotError)
 */
int queueWriteOneMessage( queueKey key, char * message ) {
    int queue = -1;
    if ( ( queue = queueOpen( key, 1 ) ) != -1 ) {

        // Write to queue
        int ok = queueWrite( queue, message );

        // Close queue
        queueClose( queue );

        return( ok );
    }
    return( 0 );
}

// -------------------------------------------------------------
// Get the number of messages in a queue (non destructive)
// -------------------------------------------------------------

/**
 * \brief Gets the number of messages in a queue (non destructive)
 * \param queue Handle to the queue
 * \returns Number of messages
 */
int queueGetNumMessages( int queue ) {
    int num = -1;
    struct msqid_ds buf;
    if ( msgctl( queue, IPC_STAT, &buf ) != -1 ) {
        num = buf.msg_qnum;
    } else {
        printf( "Error checking queue %d (%d - %s)\n",
                  queue, errno, strerror( errno ) );
    }
    return( num );
}

// -------------------------------------------------------------
// Close
// -------------------------------------------------------------

/**
 * \brief Closes a queue
 * \param queue Handle to the queue
 */
void queueClose( int queue ) {
    DEBUG_DETAIL( "Close Queue %d\n", queue );
    // Normal (non-POSIX) Linux message queues do not need closing
}
