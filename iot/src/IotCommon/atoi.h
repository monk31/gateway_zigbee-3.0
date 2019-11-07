// ------------------------------------------------------------------
// atoi - include file
// ------------------------------------------------------------------
// Save version of atoi to avoid segmentation faults
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

int Atoi( char * string );      // Returns -1 on NULL-string
int Atoi0( char * string );     // Returns 0 on NULL-string
int AtoiMI( char * string );    // Returns INT_MIN on NULL-string
int AtoiHex( char * string );   // Returns -1 on NULL-string

