// ------------------------------------------------------------------
// String to-upper
// ------------------------------------------------------------------
// Helper to convert string into upper case
// Note: Input string is altered
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \file
 * \brief Helper to convert string into upper case
 */

#include <ctype.h>
#include <string.h>

/**
 * \brief Converts a string to upper case
 * \returns Converted string (modifies the input string)
 */
char * strtoupper( char * str ) {
    char * c = str;
    while ( ( *c = toupper( *c ) ) ) {
        c++;
    }
    return( str );
}
