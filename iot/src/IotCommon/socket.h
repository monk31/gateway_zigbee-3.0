// ------------------------------------------------------------------
// Socket - include file
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

int  socketOpen( char * host, char * port, int server );
int  socketAccept( int handle );
int  socketRead( int handle, char * buffer, int size );
int  socketReadWithTimeout( int handle, char * buffer, int size, int sec );
int  socketWrite( int handle, char * buffer, int len );
int  socketWriteString( int handle, char * string );
void socketClose( int handle );


