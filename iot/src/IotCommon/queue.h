// ------------------------------------------------------------------
// Queue - include file
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

#define MAXQUEUESIZE     10
#define MAXMESSAGESIZE   300

typedef enum {
    QUEUE_KEY_NONE = 0,
    QUEUE_KEY_CONTROL_INTERFACE,   // 1
    QUEUE_KEY_SECURE_JOINER,       // 2
    QUEUE_KEY_ZCB_IN,              // 3
    QUEUE_KEY_DBP,                 // 4
} queueKey;

int  queueOpen( queueKey key, int forwrite );
int  queueWrite( int queue, char * message );
int  queueRead( int queue, char * message, int size );
int  queueReadWithMsecTimeout( int queue, char * message, int size, int msec );

int  queueWriteOneMessage( queueKey key, char * message );
int  queueGetNumMessages( int queue );

void queueClose( int queue );
