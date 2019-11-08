/****************************************************************************
 *
 * PROJECT:            	Wireless Sensor Network for Green Building
 *
 * AUTHOR:          	Debby Nirwan
 *
 * DESCRIPTION:        	Buffer Manager
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
#include "bufman.h"


int bufman_init(bufman_t *bufman, char *buffer, int bufsize)
{
	if((bufman == NULL) || (buffer == NULL) || (bufsize <= 0)){
		return -1;
	}
	
	bufman->head = 0;
	bufman->tail = 0;
	bufman->size = bufsize;
	bufman->buf = buffer;
	
	return 0;
}

int bufman_read(bufman_t *bufman, void *buffer, int bufsize)
{
	int i;
	char *p;
	
	if((bufman == NULL) || (buffer == NULL) || (bufsize <= 0)){
		return -1;
	}
	
	p = buffer;
	
	for(i=0; i<bufsize; i++){
		if(bufman->tail != bufman->head){
			*p++ = bufman->buf[bufman->tail];
			bufman->tail++;
			if(bufman->tail == bufman->size){
				bufman->tail = 0;
			}
		}else{
			return i; 
		}
	}
	
	return bufsize;
}

int bufman_write(bufman_t *bufman, void *buffer, int bufsize)
{
	int i;
	char *p;
	
	if((bufman == NULL) || (buffer == NULL) || (bufsize <= 0)){
		return -1;
	}
	
	p = buffer;
	
	for(i=0; i<bufsize; i++){
		if( (bufman->head + 1 == bufman->tail) || ( (bufman->head + 1 == bufman->size) && (bufman->tail == 0) ){
			return i;
		}else{
			bufman->buf[bufman->head] = *p++;
			bufman->head++;
			if(bufman->head == bufman->size){
				bufman->head = 0;
			}
		}
	}
	
	return bufsize;
}
