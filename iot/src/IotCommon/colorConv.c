// ------------------------------------------------------------------
// Color space conversion for Zigbee lamps
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------
// Source:
//    www.brucelindbloom.com/index.html?Equations.html
//    http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_xyY.html
//    http://en.wikipedia.org/wiki/SRGB
//    http://www.vendian.org/mncharity/dir3/blackbody/UnstableURLs/bbr_color.html
//    http://en.wikipedia.org/wiki/File:PlanckianLocus.png
//    http://www.cs.rit.edu/~ncs/color/t_convert.html#RGB to XYZ & XYZ to RGB
//    http://upload.wikimedia.org/wikipedia/commons/6/60/Cie_Chart_with_sRGB_gamut_by_spigget.png
// ------------------------------------------------------------------

/** \file
 * \brief Color space conversion for Zigbee lamps
 */

#include <stdio.h>

#include "RgbSpaceMatrices.h"

#define MATRIX  14         // sRGB D65

#define MAX_XY  0xFEFF
#define MAX_Y   100


/**
 * \brief Converts an rgb value into x/y space
 * \param rgb RGB color value between (and including) 0 and 0xFFFFFF
 * \param x Pointer to receive X coordinate
 * \param y Pointer to receive Y coordinate
 * \returns x,y scaled between 0-0xFEFF or 0,0 in case of an error
 */
void rgb2xy( int rgb, int * x, int * y ) {
    float r, g, b, fx, fy, fz;

    // Unpack and scale to 0...1
    r = (float)( ( rgb >> 16 ) & 0xFF ) / (float)0xFF;
    g = (float)( ( rgb >> 8  ) & 0xFF ) / (float)0xFF;
    b = (float)( ( rgb       ) & 0xFF ) / (float)0xFF;

    fx = ( RgbSpaceMatrices[ (MATRIX*18) + 0 ] * r ) +
         ( RgbSpaceMatrices[ (MATRIX*18) + 1 ] * g ) +
         ( RgbSpaceMatrices[ (MATRIX*18) + 2 ] * b );
    fy = ( RgbSpaceMatrices[ (MATRIX*18) + 3 ] * r ) +
         ( RgbSpaceMatrices[ (MATRIX*18) + 4 ] * g ) +
         ( RgbSpaceMatrices[ (MATRIX*18) + 5 ] * b );
    fz = ( RgbSpaceMatrices[ (MATRIX*18) + 6 ] * r ) +
         ( RgbSpaceMatrices[ (MATRIX*18) + 7 ] * g ) +
         ( RgbSpaceMatrices[ (MATRIX*18) + 8 ] * b );

    float sum = ( fx + fy + fz );

    if ( sum != 0 ) {
        fx = fx / sum;
        fy = fy / sum;

        // Clipping
        if ( fx < 0 ) fx=0; if ( fx > 1.0 ) fx=1.0;
        if ( fy < 0 ) fy=0; if ( fy > 1.0 ) fy=1.0;

        // Scale to 0...MAX
        *x = (int)( fx * (float)MAX_XY );
        *y = (int)( fy * (float)MAX_XY );
    } else {
        *x = 0;
        *y = 0;
    }
};


/**
 * \brief Converts x/y space color into rgb
 * \param x X coordinate
 * \param y Y coordinate
 * \param rgb Pointer to receive RGB value
 * \returns rgb color
 */
void xy2rgb( int x, int y, int * rgb ) {
    float fx, fy, fz = 0, fY;

    // Scale back to 0...1
    fx = (float)x / (float)MAX_XY;
    fy = (float)y / (float)MAX_XY;
    fY = 1.0;

    if ( y != 0 ) {
        fz = ( 1 - fx - fy ) * fY / fy ;
        fx = fx * fY / fy;
    }

    float fr = RgbSpaceMatrices[ (MATRIX*18) +  9 ] * fx +
               RgbSpaceMatrices[ (MATRIX*18) + 10 ] * fY +
               RgbSpaceMatrices[ (MATRIX*18) + 11 ] * fz;
    float fg = RgbSpaceMatrices[ (MATRIX*18) + 12 ] * fx +
               RgbSpaceMatrices[ (MATRIX*18) + 13 ] * fY +
               RgbSpaceMatrices[ (MATRIX*18) + 14 ] * fz;
    float fb = RgbSpaceMatrices[ (MATRIX*18) + 15 ] * fx +
               RgbSpaceMatrices[ (MATRIX*18) + 16 ] * fY +
               RgbSpaceMatrices[ (MATRIX*18) + 17 ] * fz;

    // Clipping
    if ( fr < 0 ) fr=0; if ( fr > 1 ) fr=1;
    if ( fg < 0 ) fg=0; if ( fg > 1 ) fg=1;
    if ( fb < 0 ) fb=0; if ( fb > 1 ) fb=1;

    // Scale to 0...255 and pack
    int r = (int)( fr * 0xFE );
    int g = (int)( fg * 0xFF );
    int b = (int)( fb * 0xFF );
    *rgb = ( r << 16 ) + ( g << 8 ) + b;
};

// ---------------------------------------------------------------------
// Kelvin interface
// ---------------------------------------------------------------------

#include "blackbody.h"

/**
 * \brief Finds closest entry in blackbody curve table
 * \param kelvin Kelvin color value
 * \returns index to closest blackbody curve color
 */
static int findbestkelvin( int kelvin ) {
    // printf( "sizeof( bb_curve ) = %d\n", (int)sizeof( bb_curve ) );
    int len = sizeof( bb_curve ) / sizeof( bb_point );
    // printf( "#bb_points = %d\n", len );

    bb_point * p;
    int i, dkelvin, dbest = 10000, index = 0;
    for ( i=0; i<len; i++ ) {
        p = &bb_curve[i];
        dkelvin = kelvin - p->kelvin; if ( dkelvin < 0 ) dkelvin = -dkelvin;
        if ( dkelvin < dbest ) {
           dbest = dkelvin;
           index = i;
        }
    }
    return( index );
}

/**
 * \brief Converts Kelvin color into x/y color space
 * \param kelvin Kelvin color value
 * \param x Pointer to receive X coordinate
 * \param y Pointer to receive Y coordinate
 */
void kelvin2xy( int kelvin, int * x, int * y ) {
    int index = findbestkelvin( kelvin );
    *x = (int)( bb_curve[index].x * MAX_XY );
    *y = (int)( bb_curve[index].y * MAX_XY );
}
