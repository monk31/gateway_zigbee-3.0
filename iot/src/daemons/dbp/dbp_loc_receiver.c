/****************************************************************************
 *
 * PROJECT:            	Wireless Sensor Network for Green Building
 *
 * AUTHOR:          	Debby Nirwan
 *
 * DESCRIPTION:        	DBP Location Receiver
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>

#include "parsing.h"
#include "newLog.h"
#include "newDb.h"
#include "json.h"
#include "jsonCreate.h"
#include "dbp_loc_receiver.h"
#include "dbp.h"
#include "queue.h"

#define LOC_RECEIVER_PORT	2010


pthread_t loc_receiver_thread;
bool dbp_loc_init = false;

void dbp_external_send_data_to_clients(char *data);
static void *dbp_loc_receiver(void *arg);

static void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int dbp_loc_receiver_init(void)
{
	if(pthread_create(&loc_receiver_thread, NULL, &dbp_loc_receiver, NULL) != 0){
		perror("Failed to create dbp location receiver thread");
		return -1;
	}
	
	dbp_loc_init = true;
	
	return 0;
}

static void *dbp_loc_receiver(void *arg)
{
	int sockfd, new_fd;
	int yes = 1;
	socklen_t sin_size;
	struct sockaddr_storage their_addr;
	struct sockaddr_in serv_addr;
	int byte_count;
	char buf[512] = {0};
	bool client_exist = false;
	char s[INET6_ADDRSTRLEN];
	int loop;
	
	printf("%s: running\n", __func__);
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
		perror("server: socket");
		return NULL;
	}
	
	/* Initialize socket structure */
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(LOC_RECEIVER_PORT);
	
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		return NULL;
	}
	
	/* Now bind the host address using bind() call.*/
	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		perror("server: bind");
		close(sockfd);
		return NULL;
	}
	
	if(listen(sockfd, 10) == -1) {
		perror("listen");
		return NULL;
	}
	
	while(1){
		
		printf("%s: accepting...\n", __func__);
		
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
		if(new_fd == -1) {
			perror("accept");
			continue;
		}
		
		client_exist = true;
		
		inet_ntop(their_addr.ss_family, get_in_addr( (struct sockaddr *) &their_addr ), s, sizeof s);
		printf("server: got connection from %s\n", s);
		
		/* receive location info from the phone here */
		while(client_exist == true){
			
			memset(buf, 0, sizeof(buf));
			
			byte_count = recv(new_fd, buf, sizeof buf, 0);
			
			if(byte_count == 0){
				newLogAdd( NEWLOG_FROM_DBP,"DBP Location Receiver connection has been closed");
				client_exist = false;
				break;
			}else if(byte_count < 0){
				newLogAdd( NEWLOG_FROM_DBP,"DBP Location Receiver connection error");
				client_exist = false;
				break;
			}else{
				newLogAdd( NEWLOG_FROM_DBP,"DBP Location Receiver received data");
				
				/* remove 1st 0 */
				for(loop=0; loop<sizeof(buf)-1; loop++){
					buf[loop] = buf[loop+1];
					if(buf[loop+1] == 0){
						break;
					}
				}
				
				/* send to main() to be processed */
				queueWriteOneMessage(QUEUE_KEY_DBP, buf);
				printf("%s: data sent to main()\n", __func__);
				
			}
			
		}
		
	}
	
	return NULL;
}
