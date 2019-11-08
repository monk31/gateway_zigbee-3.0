/****************************************************************************
 *
 * PROJECT:            	Wireless Sensor Network for Green Building
 *
 * AUTHOR:          	Debby Nirwan
 *
 * DESCRIPTION:        	DBP M-Search
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2014. All rights reserved
 *
 ***************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <pthread.h>

#include "dbp_search.h"

#define MULTICAST_PORT				2003
#define MULTICAST_STOP_SENDING_TIME	12

int sockd;
pthread_t search_thread, listen_thread;
struct in_addr local_interface;
struct sockaddr_in group_sock;
char message[255];
char dbp_cmd[] = "DBP-SEARCH\r\nDBP-VERSION: 4.0\r\nDBP-CMD: GET_IP";
char check_cmd[] = "DBP-CMD: GET_IP";
char multicast_address[] = "239.255.255.251";
char local_address[NI_MAXHOST];
bool running = true;
bool ip_address_acquired = false;
bool send_message = false;
int time_out_cnt = 0;

static void *dbp_search_main(void *arg);
static void *dbp_search_listener(void *arg);
static int get_ip_address(void);

int dbp_search_init(void)
{
	strcpy(message, "DBP-SEARCH\r\nDBP-VERSION: 4.0\r\nDBP-SERVER: ");
	
	if(pthread_create(&search_thread, NULL, &dbp_search_main, NULL) != 0){
		perror("Failed to dbp search thread");
		return -1;
	}
	
	if(pthread_create(&listen_thread, NULL, &dbp_search_listener, NULL) != 0){
		perror("Failed to dbp search thread");
		return -1;
	}
	return 0;
}

static void *dbp_search_main(void *arg)
{
	char loopch = 0;
	
	/* create datagram socket */
	sockd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockd < 0){
		perror("failed to create datagram socket");
		pthread_exit(NULL);
	}
	
	/* initialize the group sockaddr */
	memset((char *) &group_sock, 0, sizeof(group_sock));
	group_sock.sin_family = AF_INET;
	group_sock.sin_addr.s_addr = inet_addr(multicast_address);
	group_sock.sin_port = htons(MULTICAST_PORT);
	
	/* disable ip multicast loop */
	if(setsockopt(sockd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0){
		perror("failed to disable ip multicast loop");
		close(sockd);
		pthread_exit(NULL);
	}
	
	/* get local ip address from eth0 */
	if(get_ip_address() >= 0){
		strcat(message, local_address);
	}
	
	local_interface.s_addr = inet_addr(local_address);
	if(setsockopt(sockd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&local_interface, sizeof(local_interface)) < 0){
		perror("setting local interface error");
		close(sockd);
		pthread_exit(NULL);
	}
	
	printf("%s: waiting for incoming message before sending multicast\n", __func__);
	
	while(running){
		
		/* sleep 5 seconds */
		sleep(5);
		
		if(send_message == true){
			
			printf("%s: sending multicast message\n", __func__);
			
			if(sendto(sockd, message, sizeof(message), 0, (struct sockaddr*) &group_sock, sizeof(group_sock)) < 0){
				perror("datagram sending failed");
			}
			
			time_out_cnt++;
			
			if(time_out_cnt >= MULTICAST_STOP_SENDING_TIME){
				send_message = false;
				time_out_cnt = 0;
			}
			
		}
		
	}
	
	return NULL;
}

static void *dbp_search_listener(void *arg)
{
	int sd;
	int reuse = 1;
	struct sockaddr_in localSock;
	struct ip_mreq group;
	char buffer[100];
	
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0){
		perror("failed to create datagram socket");
		pthread_exit(NULL);
	}
	
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0){
		perror("failed to set SO_REUSEADDR");
		close(sd);
		pthread_exit(NULL);
	}
	
	memset((char *) &localSock, 0, sizeof(localSock));
	localSock.sin_family = AF_INET;
	localSock.sin_port = htons(MULTICAST_PORT);
	localSock.sin_addr.s_addr = INADDR_ANY;
	
	if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock))){
		perror("binding failed");
		close(sd);
		pthread_exit(NULL);
	}
	
	while(ip_address_acquired == false){
		printf("%s: waiting for ip address\n", __func__);
		sleep(1);
	}
	
	group.imr_multiaddr.s_addr = inet_addr(multicast_address);
	group.imr_interface.s_addr = inet_addr(local_address);
	
	if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0){
		perror("join group failed");
		close(sd);
		pthread_exit(NULL);
	}
	
	memset(buffer, 0, sizeof(buffer));
	
	while(1){
		
		/* receive data from multicast group */
		if(read(sd, buffer, sizeof(buffer)) < 0){
			perror("error receiving data from group");
			continue;
		}
		
		printf("%s: new data has been received %s\n", __func__, buffer);
		
		/* parse the data */
		if(strstr(buffer, check_cmd)){
			send_message = true;
		}
		
		/* init buffer */
		memset(buffer, 0, sizeof(buffer));
	}
	
	return NULL;
}

static int get_ip_address(void)
{
	int fm = AF_INET;
	struct ifaddrs *ifaddr, *ifa;
	int family, s;
	
	if(getifaddrs(&ifaddr) == -1){
		perror("getifaddrs() failed");
		return -1;
	}
	
	for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
		if(ifa->ifa_addr == NULL){
			continue;
		}
		
		/* family */
		family = ifa->ifa_addr->sa_family;
 
		/* for eth0 only */
		if(strcmp(ifa->ifa_name, "eth0") == 0){
			if(family == fm){
				s = getnameinfo(ifa->ifa_addr, (family == AF_INET)?sizeof(struct sockaddr_in):sizeof(struct sockaddr_in6), local_address, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
				if(s != 0){
					freeifaddrs(ifaddr);
					printf("getnameinfo() failed: %s\n", gai_strerror(s));
					return -1;
				}
				printf("%s", local_address);
			}
			printf("\n");
		}
	}
	
	ip_address_acquired = true;
	
	freeifaddrs(ifaddr);
	return 0;
}
