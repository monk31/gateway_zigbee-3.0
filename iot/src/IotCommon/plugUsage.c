// ------------------------------------------------------------------
// Plug Usage
// ------------------------------------------------------------------
// Calculates plug usage from iot_plughistory database
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \file
 * \brief Calculates plug usage from iot_plughistory database
 */

#include <stdio.h>
#include <stdlib.h>

#include "atoi.h"
#include "newDb.h"
#include "plugUsage.h"

#define PLUG_DEBUG

#ifdef PLUG_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* PLUG_DEBUG */

// ------------------------------------------------------------------
// Cleanup
// - Policy is to keep all minute values for last hour and
//   all hour readings of last 24 hours
// ------------------------------------------------------------------

static int cleanstart;

static int nowmin;
static int lowres;
static int highres;
static int lastMin  = -1;
static int lastHour = -1;
static int keepOneH;
static int keepOneL;
static int minhourday[60+24+1];
static int mhd_sample[60+24+1];

#define MAXCLEANIDLIST 10
static int cleanIdList[MAXCLEANIDLIST];
static int cleanIdNr = 0;

/**
 * \brief Looper callback that determines if an entry is candidate for cleanup
 * \returns 1 when to continue looping, or 0 when we have the maximum cleanup items in one go
 * \returns Fills a global cleanup list with the plughit IDs
 */
static int plugCleanPrepareCb( newdb_plughist_t * phist ) {

    int clean = -1, sample = -1;

    // Determine sample
    int min = phist->lastupdate / 60;   // round to minute

    if ( min < cleanstart ) {
        // Future : keep
    } else if ( min < ( cleanstart + 60 ) ) {
        // Less than 1 hour old: keep highest minute sample
        sample = cleanstart - min;
    } else if ( min < ( cleanstart + ( 60 * 24 ) ) ) {
        // Less than 1 day old: keep highest hour sample
        sample = 60 + ( ( cleanstart - min ) / 60 );
    } else if ( min < ( cleanstart + ( 60 * 24 * 2 ) ) ) {
        // Less than 2 day old: keep highest day sample
        sample = 60 + 24;
    } else {
        // Older than 2 days
        clean = phist->id;
    }
    
    if ( clean < 0 && sample >= 0 ) {
        if ( minhourday[sample] < 0 ) {
            minhourday[sample] = phist->sum;
            mhd_sample[sample] = phist->id;
        } else if ( phist->sum >= minhourday[sample] ) {
            clean = mhd_sample[sample];
            minhourday[sample] = phist->sum;
            mhd_sample[sample] = phist->id;
        } else {
            clean = phist->id;
        }
    }

    if ( clean >= 0 ) {
        if ( cleanIdNr < MAXCLEANIDLIST ) {
            cleanIdList[cleanIdNr++] = clean;
        }
        return( cleanIdNr < MAXCLEANIDLIST );
    }

    return( 1 );
}

/**
 * \brief Performs a plughist cleanup to reduce the number of entries. Loops the database to build
 * a cleanup list and then performs the actual cleanups
 * \param mac Mac of the plug to do the cleanup for
 * \param now Timestamp for now
 * \returns 1 if all cleanups are done, or 0 when there is still work to do
 */
int plugHistoryCleanup( char * mac, int now ) {
    cleanstart  = ( now / 60 );
    lowres   = nowmin - ( 24 * 60 );  // 1 day
    highres  = nowmin - 60;           // 1 hour
    lastMin  = -1;
    lastHour = -1;
    keepOneH = 1;
    keepOneL = 1;
    cleanIdNr = 0;
    
    int i;
    for ( i=0; i<(60+24+1); i++ ) {
        minhourday[i] = -1;
        mhd_sample[i] = -1;
    }
    
    // printf( "\n\n ================= Build cleanuplist\n" );
    
    // Build the list of history ids to cleanup
    newDbLoopPlugHist( mac, plugCleanPrepareCb );
    
    // printf( "Cleanup %d items\n", cleanIdNr );

    // When there are any, now delete them
    if ( cleanIdNr > 0 ) {
        int ok = 1;
        for ( i=0; ok && i<cleanIdNr; i++ ) {
            ok = newDbDeletePlugHist( cleanIdList[i] );
        }
    }
    
    printf( "Plughist cleanup done\n" );
    return( cleanIdNr < MAXCLEANIDLIST );
}

// ------------------------------------------------------------------
// Wh calculation over certain period of time
// ------------------------------------------------------------------

static int findstart;
static int findend;
static int findmax;
static int findmin;

/**
 * \brief Looper callback that finds the minimal and maximal Wh for a certain plug
 * \returns 1 to continue looping
 * \returns Min and max in global variables
 */
static int plugFindUsageCb( newdb_plughist_t * phist ) {

    // Determine sample
    int min = phist->lastupdate / 60;   // round to minute
    
    // See if in range
    if ( min < findstart ) {
        // Future? Ignore
    } else if ( min <= findend ) {
        // Within range: take max and min
        if ( ( findmax < 0 ) || phist->sum > findmax ) {
            findmax = phist->sum;
        }
        if ( ( findmin < 0 ) || phist->sum < findmin ) {
            findmin = phist->sum;
        }
        
    } else {
        /** \todo: PlugHist Usage: handle samples older than range */
    }

    return( 1 );
}

/**
 * \brief Find the Wh usage of a plug for the last period of time. Loops the plughist database
 * to searc for the minimal and maximal Wh for the specified plug and reports the difference
 * \param mac Mac of the plug to investigate
 * \param now Timestamp of now
 * \param period Period to investigate (in minutes)
 * \returns Usage in Wh
 */
static int plugFindUsage( char * mac, int now, int period ) {
    int Wh = 0;
    findstart  = ( now / 60 );        // round to minute
    findend    = findstart - period;  // period in minutes
    findmax    = -1;
    findmin    = -1;

    newDbLoopPlugHist( mac, plugFindUsageCb );
    
    if ( findmax >= 0 && findmin >= 0 ) {
        Wh = findmax - findmin;
    }
    return( Wh );
}

/**
 * \brief Find the Wh usage of a plug for the last hour
 * \param mac Mac of the plug to investigate
 * \param now Timestamp of now
 * \returns Usage in Wh
 */
int plugFindHourUsage( char * mac, int now ) {
    return( plugFindUsage( mac, now, 60 ) );
}

/**
 * \brief Find the Wh usage of a plug for the last day (24 hours)
 * \param mac Mac of the plug to investigate
 * \param now Timestamp of now
 * \returns Usage in Wh
 */
int plugFindDayUsage( char * mac, int now ) {
    return( plugFindUsage( mac, now, 24 * 60 ) );
}

// ------------------------------------------------------------------
// History
// User must provide buffer for "num" readings of "period" length
// ------------------------------------------------------------------

static int * histbuffer;

static int histstart;
static int histperiod;
static int histnum;
static int histmin;
static int histmax;

/**
 * \brief Looper callback to find the maximum usage for each sampling period
 * \returns 1 to keep looping
 * \returns A global array of <num> samples
 */
static int plugHistCb( newdb_plughist_t * phist ) {
    // printf( "Table %s (%d)\n", data, argc );

    // Determine sample
    int min = phist->lastupdate / 60;   // round to minute
    int sample = ( histstart - min ) / histperiod;
    
    // printf( "Plughist[%d] : %d, %d\n", phist->id, sample, phist->sum );

    // See if in range
    if ( sample < 0 ) {
        // Future? Ignore
    } else if ( sample < histnum ) {
        // Within range
        if ( phist->sum > histbuffer[sample] ) {
            // Take the max for this sample
            histbuffer[sample] = phist->sum;
        }
        // Remeber the minimum
        if ( !histmin || phist->sum < histmin ) {
            histmin = phist->sum;
        }
    } else {
        // Older than range: remember the maximum
        if ( !histmax || phist->sum > histmax ) {
            histmax = phist->sum;
        }
    }

    return( 1 );
}

/**
 * \brief Get a history array for a number of samples with a certain period length. Loops the
 * plughist database to fill the usage array. Automatically fills holes where there was no sampling
 * data available.
 * \param mac Mac of the plug to investigate
 * \param now Timestamp of now
 * \param period Sampling period (in minutes)
 * \param num Number of samples to return
 * \param buffer User supplied array to store the (num) results
 * \returns The array result
 */
int * plugGetHistory( char * mac, int now, int period, int num, int * buffer ) {
    histstart  = ( now / 60 );   // round to minute
    histperiod = period;         // period in minutes
    histnum    = num;
    histmin    = 0;
    histmax    = 0;
    histbuffer = buffer;
    int i;
    for ( i=0; i<num; i++ ) histbuffer[i] = 0;
    
    newDbLoopPlugHist( mac, plugHistCb );
    
    /*
            int j;
            printf( "Hour history:\n" );
            for ( i=0; i<6; i++ ) {
                printf( "%2d:", i );
                for ( j=0; j<10; j++ ) {
                    printf( " %4d", histbuffer[(i*10) + j] );
                }
                printf( "\n" );
            }
    */
    
    // Fill gaps
    if ( histbuffer[num-1] == 0 ) {
      // Set start value
      if ( histmax ) histbuffer[num-1] = histmax;
      else  histbuffer[num-1] = histmin;
    }
    for ( i=num-2; i>=0; i-- ) {  // Start with num-2
        if ( histbuffer[i] == 0 ) histbuffer[i] = histbuffer[i+1];
    }
    
    /*
            printf( "Hour history:\n" );
            for ( i=0; i<6; i++ ) {
                printf( "%2d:", i );
                for ( j=0; j<10; j++ ) {
                    printf( " %4d", histbuffer[(i*10) + j] );
                }
                printf( "\n" );
            }
      */      
    return( histbuffer );
}

