// ------------------------------------------------------------------
// atoi
// ------------------------------------------------------------------
// Save version to avoid segmentation faults
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \file
 * \brief Save atoi functions to avoid segmentation faults
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "atoi.h"

/**
 * \brief Converts an integer string into an integer
 * \returns The converted text or -1 in case of an error
 */
int Atoi( char * string ) {
    if ( string != NULL && *string != '\0' ) {
        return( atoi( string ) );
    }
    return( -1 );
}

/**
 * \brief Converts an integer string into an integer
 * \returns The converted text or 0 in case of an error
 */
int Atoi0( char * string ) {
    if ( string != NULL && *string != '\0' ) {
        return( atoi( string ) );
    }
    return( 0 );
}

/**
 * \brief Converts an integer string into an integer
 * \returns The converted text or INT_MIN in case of an error
 */
int AtoiMI( char * string ) {
    if ( string != NULL && *string != '\0' ) {
        return( atoi( string ) );
    }
    return( INT_MIN );
}

/**
 * \brief Converts a hexadecimal integer string into an integer
 * \returns The converted text or -1 in case of an error
 */
int AtoiHex( char * string ) {
    if ( string != NULL && *string != '\0' ) {
        int val = (int)strtol( string, NULL, 16 );
        return( val );
    }
    return( -1 );
}

