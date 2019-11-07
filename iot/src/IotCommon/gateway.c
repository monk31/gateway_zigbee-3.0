// ------------------------------------------------------------------
// Gateway
// ------------------------------------------------------------------
// Miscellaneous functions about this specific gateway
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \mainpage IoT Gateway
 * \section daemons IoT Daemons
 *
 * 1. \ref controlinterface 
 * 
 * 2. \ref securejoiner 
 * 
 * 3. \ref nfcdaemon 
 * 
 * 4. \ref zcblinux 
 * 
 * 5. \ref gwdiscovery 
 * 
 * \section iottesting IoT Testing
 *
 * 1. \ref controltest
 * 
 * 2. \ref joinertest
 * 
 * 3. \ref gatewaytest
 * 
 * 4. \ref zcbtest
 * 
 * 5. \ref plugtest
 * 
 * 6. \ref logtest
 */

/** \file
 * \brief Miscellaneous functions about this specific gateway
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <time.h>

#include "atoi.h"
#include "newLog.h"
#include "iotSemaphore.h"
#include "gateway.h"

#define GW_DEBUG

#ifdef GW_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* GW_DEBUG */

#define GW_SEMKEY  151002

static char myIpBuf[INET_ADDRSTRLEN];
static char hostname[40];

// ------------------------------------------------------------------
// IP Address
// ------------------------------------------------------------------

/**
 * \brief Get IP address of GW
 * \returns The (IP4) IP address
 */
char * myIP( void ) {
    struct ifaddrs * ifAddrStruct = NULL;
    struct ifaddrs * ifa = NULL;
    void * tmpAddrPtr = NULL;
    char * result = NULL;


    if (getifaddrs( &ifAddrStruct ) != -1)
    {
    	for ( ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next ) {
            if (ifa->ifa_addr == NULL)
                continue;
    		if ( ifa->ifa_addr->sa_family == AF_INET ) {
    			// IPV4
    			tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
    			if ( strstr( ifa->ifa_name, "eth" ) != NULL ) {
    				inet_ntop( AF_INET, tmpAddrPtr, myIpBuf, INET_ADDRSTRLEN );
    			}
    		} else {
    			// IPV6
    		}
    	}

    	// The data returned by getifaddrs() is dynamically allocated and should
    	// be freed using freeifaddrs() when no longer needed.
    	freeifaddrs(ifAddrStruct);
    }

    // printf( "IP: %s\n", myIpBuf );

    return( myIpBuf );
}

// ------------------------------------------------------------------
// Host name
// ------------------------------------------------------------------

/**
 * \brief Get the hostname of the GW
 * \returns The hostname
 */
char * myHostname( void ) {
    gethostname( hostname, 40 );
    return( hostname );
}

// ------------------------------------------------------------------
// Error checking for too many open files
// ------------------------------------------------------------------

/**
 * \brief Checks how many file descriptors your program has open. Is used to detect errors.
 * \returns The number of open file descriptors
 */
int checkOpenFiles( int mentionAbove ) {
    int i, num = getdtablesize();
    int numOpen = 0;

    // printf( "cof: num = %d\n", num );

    for ( i=0; i<num; i++ ) {

        int fd_flags = fcntl( i, F_GETFD );
        if ( fd_flags != -1 ) {

            int fl_flags = fcntl( i, F_GETFL );
            if ( fl_flags != -1 ) {
            
                char path[256];
                sprintf( path, "/proc/self/fd/%d", i );

                char buf[256];
                memset( buf, 0, 256 );

                ssize_t s = readlink( path, buf, 256 );
                if ( s != -1 ) {
                    numOpen++;

                    if ( i >= mentionAbove ) {

                        printf( "checkOpenFiles - UNEXPECTED FILE %s: ", buf );

                        if ( fl_flags & O_APPEND   ) printf( "append " );
                        if ( fl_flags & O_NONBLOCK ) printf( "nonblock " );
                        if ( fl_flags & O_RDONLY   ) printf( "read-only " );
                        if ( fl_flags & O_RDWR     ) printf( "read-write " );
                        if ( fl_flags & O_WRONLY   ) printf( "write-only " );
                        if ( fl_flags & O_DSYNC    ) printf( "dsync " );
                        if ( fl_flags & O_RSYNC    ) printf( "rsync " );
                        if ( fl_flags & O_SYNC     ) printf( "sync " );

                        struct flock fl;
                        fl.l_type = F_WRLCK;
                        fl.l_whence = 0;
                        fl.l_start = 0;
                        fl.l_len = 0;
                        fcntl( i, F_GETLK, &fl );
                        if ( fl.l_type != F_UNLCK ) {
                            if ( fl.l_type == F_WRLCK ) {
                                printf( "write-locked" );
                            } else {
                                printf( "read-locked" );
                            }
                            printf( "(%d) ", fl.l_pid );
                        }

                        printf( "\n" );
                    }
                }
            }
        }
    }

    // printf( "cof: open = %d\n", numOpen );
    return( numOpen );
}

// -------------------------------------------------------------
// Wireless
// -------------------------------------------------------------

static char * skipWhite( char * start ) {
    while ( *start == ' ' || *start == '\t' ) start++;
    return start;
}

int gwGetWireless( wireless_t * wt ) {
    FILE * fp;
    char line[80];
    char * s;
    
    wt->ssid[0] = '\0';
    wt->channel = 0;

    if ( ( fp = fopen( "/etc/config/wireless", "r" ) ) != NULL ) {
        while ( fgets( line, 80, fp ) != NULL ) {
            // Remove \n at end
            int len = strlen( line );
            if ( len > 1 ) {
                if ( line[len-1] == '\n' ) line[len-1] = '\0';
                len--;
            }
            
            // printf( "Line: %s", line );
            if ( ( s = strstr( line, "ssid" ) ) != NULL ) {
                strcpy( wt->ssid, skipWhite( s+4 ) );
            } else if ( ( s = strstr( line, "channel" ) ) != NULL ) {
                wt->channel = Atoi0( skipWhite( s+7 ) );
            }
        }
        fclose( fp );
        return 1;
    }
    return 0;
}

int gwGetNetwork( network_t * nt ) {
    FILE * fp;
    char line[80];
    char * s;
    
    nt->interface[0].proto = -1;
    nt->interface[1].proto = -1;

    if ( ( fp = fopen( "/etc/config/network", "r" ) ) != NULL ) {
        int iface = -1;
        while ( fgets( line, 80, fp ) != NULL ) {
            // Remove \n at end
            int len = strlen( line );
            if ( len > 1 ) {
                if ( line[len-1] == '\n' ) line[len-1] = '\0';
                len--;
            }
            
            // Remove quotes
            char * pline = line + len;
            while ( pline > line ) {
                pline--;
                if ( *pline == '\'' || *pline == '\"' ) *pline = ' ';
            }
            
            // Now parse
            if ( ( s = strstr( line, "interface" ) ) != NULL ) {
                if ( strstr( s, "wlan" ) != NULL ) {
                    iface = 1;
                } else if ( strstr( s, "lan" ) != NULL ) {
                    iface = 0;
                } else {
                    iface = -1;
                }
            } else if ( iface >= 0 ) {
                if ( ( s = strstr( line, "proto" ) ) != NULL ) {
                    if ( strstr( s, "static" ) != NULL ) {
                        nt->interface[iface].proto = 0;
                    } else if ( strstr( line, "dhcp" ) != NULL ) {
                        nt->interface[iface].proto = 1;
                    } else {
                        nt->interface[iface].proto = -1;
                    }
                } else if ( ( s = strstr( line, "ipaddr" ) ) != NULL ) {
                    strcpy( nt->interface[iface].ip, skipWhite( s+6 ) );
                } else if ( ( s = strstr( line, "netmask" ) ) != NULL ) {
                    strcpy( nt->interface[iface].netmask, skipWhite( s+7 ) );
                }
            }
        }
        fclose( fp );
        return 1;
    }
    return 0;
}

int gwGetTimezone( char * tz, char * zn ) {
    FILE * fp;
    char line[80];
    char * s;
    
    tz[0] = '\0';
    zn[0] = '\0';

    if ( ( fp = fopen( "/etc/config/system", "r" ) ) != NULL ) {
        while ( fgets( line, 80, fp ) != NULL ) {
            // Remove \n at end
            int len = strlen( line );
            if ( len > 1 ) {
                if ( line[len-1] == '\n' ) line[len-1] = '\0';
                len--;
            }
            
            // printf( "Line: %s", line );
            if ( ( s = strstr( line, "timezone" ) ) != NULL ) {
                strcpy( tz, skipWhite( s+8 ) );
            }
            if ( ( s = strstr( line, "zonename" ) ) != NULL ) {
                strcpy( zn, skipWhite( s+8 ) );
            }
        }
        fclose( fp );
        return 1;
    }
    return 0;
}

// -------------------------------------------------------------
// Serialize System
// -------------------------------------------------------------

char * gwSerializeSystem( int maxlen, char * buf ) {
    buf[0] = '\0';
    char * pbuf;
    
    struct utsname uts;
    uname( &uts );
    
    struct sysinfo info;
    sysinfo(&info);
        
    wireless_t wireless;
    gwGetWireless(&wireless);

    network_t network;
    gwGetNetwork( &network );
    
    char tz[40];
    char zn[40];
    gwGetTimezone( tz, zn );

    pbuf = &buf[strlen(buf)];
    sprintf( pbuf, "Key|Value" );
    pbuf = &buf[strlen(buf)];
    sprintf( pbuf, ";Kernel version|%s %s", uts.sysname, uts.release );
    pbuf = &buf[strlen(buf)];
    sprintf( pbuf, ";Kernel build|%s", uts.version );
    pbuf = &buf[strlen(buf)];
    sprintf( pbuf, ";Up-time|%lddays %ldh %ldm %lds", info.uptime/(24*3600), (info.uptime/3600)%24, (info.uptime/60)%60, info.uptime%60 );
    pbuf = &buf[strlen(buf)];
    sprintf( pbuf, ";Total RAM|%ld", info.totalram );
    pbuf = &buf[strlen(buf)];
    sprintf( pbuf, ";Free RAM|%ld", info.freeram );
    pbuf = &buf[strlen(buf)];
    sprintf( pbuf, ";Hostname|%s", uts.nodename );
    pbuf = &buf[strlen(buf)];
    sprintf( pbuf, ";Timezone|%s", tz );
    pbuf = &buf[strlen(buf)];
    sprintf( pbuf, ";Zonename|%s", zn );   // Send zonename after timezone (see system.js)
    pbuf = &buf[strlen(buf)];
    
    
    struct ifaddrs * ifaddr = NULL;
    struct ifaddrs * ifa = NULL;
    void * tmpAddrPtr = NULL;
    char myIpBuf[INET_ADDRSTRLEN];

    getifaddrs( &ifaddr );

    for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next ) {
        if (ifa->ifa_addr == NULL)
            continue;  

        if ( ifa->ifa_addr->sa_family == AF_INET ) {
            // IPV4
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            if ( strcmp( ifa->ifa_name, "lo" ) != 0 ) {
                inet_ntop( AF_INET, tmpAddrPtr, myIpBuf, INET_ADDRSTRLEN );
                sprintf( pbuf, ";IP address (%s)|%s", ifa->ifa_name, myIpBuf );
                pbuf = &buf[strlen(buf)];
            }
        } else {
            // IPV6
        }
    }

    // The data returned by getifaddrs() is dynamically allocated and should
    // be freed using freeifaddrs() when no longer needed.
    freeifaddrs(ifaddr);
    
#ifdef TARGET_RASPBERRYPI
    sprintf( pbuf, ";SSID|%s", wireless.ssid );
    pbuf = &buf[strlen(buf)];
    sprintf( pbuf, ";Channel|%d", wireless.channel );
    pbuf = &buf[strlen(buf)];
#endif
    
    return buf;
}

// ------------------------------------------------------------------
// Config file editor
// ------------------------------------------------------------------

/**
 * \brief Edits and option in the given section of the given config file. Put in critical section.
 * \returns 1 on success, 0 on error, -1 when section was not found
 */
int gwConfigEditOption( char * filename, char * section, char * option, char * strval ) {
    char tmpfilename[40];
    FILE * fp_in, * fp_out;
    char line[202];
    int ret = 0, found = 0, insection = 0;
    char * s;
    
    semPautounlock( GW_SEMKEY, 10 );
    DEBUG_PRINTF( "GW locked\n" );
    
    DEBUG_PRINTF( "Finding option %s in section %s and replace value with %s\n", option, section, strval );
    sprintf( tmpfilename, "%s_tmp", filename );
    
    if ( ( fp_in = fopen( filename, "r" ) ) != NULL ) {
        if ( ( fp_out = fopen( tmpfilename, "w+" ) ) != NULL ) {
            ret = 1;
            while ( ret && fgets( line, 200, fp_in ) != NULL ) {
                DEBUG_PRINTF( "Line: %s", line );
                
                if ( ( s = strstr( line, "config" ) ) != NULL ) {
                    // Check occurance of section name
                    if ( strstr( s, section ) != NULL ) {
                        insection = 1;
                        DEBUG_PRINTF( "In section %s\n", section );
                    } else if ( insection ) {
                        insection = 0;
                        DEBUG_PRINTF( "Out of section again\n" );
                    }
                } else  if ( ( s = strstr( line, "option" ) ) != NULL ) {
                    if ( insection ) {
                        if ( ( s = strstr( s, option ) ) != NULL ) {
                            DEBUG_PRINTF( "Option %s found: replace with %s\n", option, strval );
                            found = 1;
                            s += strlen( option );
                            while ( *s == ' ' || *s == '\t' ) s++;
                            sprintf( s, "%s\n", strval );
                        }
                    }
                } else {
                    // If empty line, then leave section
                    s = line;
                    while ( *s == ' ' || *s == '\t' || *s == '\n' ) s++;
                    if ( strlen( s ) == 0 ) {
                        DEBUG_PRINTF( "Empty line (%d/%d)\n", insection, found );
                        // Empty line: when in requested section and option not found, then add it now
                        if ( insection ) {
                            if ( !found ) {
                                found = 1;
                                DEBUG_PRINTF( "Adding option %s with value %s\n", option, strval );
                                fprintf( fp_out, "\toption %s %s\n", option, strval );
                            }
                            insection = 0;
                            DEBUG_PRINTF( "Out of section again\n" );
                        }
                    }
                }
                
                if ( fputs( line, fp_out ) == EOF ) {
                    ret = 0;
                }
            }
            fclose( fp_out );
        } else {
            DEBUG_PRINTF( "Error %d opening temporary config file (%s): %s\n", errno, tmpfilename, strerror( errno ) );
        }
        fclose( fp_in );
        
        if ( ret ) {
            // Succeeded
            ret = 0;
            if ( found ) {
                // Now replace original file
                sprintf( line, "%s_%d", filename, (int)time(NULL) );
                if ( rename( filename, line ) == 0 ) {
                    ret = ( rename( tmpfilename, filename ) == 0 );
                }
            } else {
                // Option not found: no need to replace original file. Throw away the temporary file.
                if ( ( ret = ( remove( tmpfilename ) == 0 ) ) ) ret = -1;
            }
        }
    } else {
        DEBUG_PRINTF( "Error %d opening config file (%s): %s\n", errno, filename, strerror( errno ) );
    }
    
    DEBUG_PRINTF( "GW unlocked\n" );
    semV( GW_SEMKEY );
    
    return( ret );
}


