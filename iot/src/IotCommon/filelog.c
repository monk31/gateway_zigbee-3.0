

#include <stdio.h>

int filelog( char * filename, char * text ) {    
    char timestr[40];
    time_t clk;
    clk = time( NULL );
    sprintf( timestr, "%s", ctime( &clk ) );
    timestr[ strlen( timestr ) - 1 ] = '\0';
    
    FILE * f;
    if ( ( f = fopen( filename, "a" ) ) != NULL ) {
        fprintf( f, "%s - %d : %s\n", timestr, getpid(), text );
        fclose( f );
        return 1;
    }
    return 0;
}
