/****************************************************************************
 *
 * PROJECT:            	Wireless Sensor Network for Green Building
 *
 * AUTHOR:          	Debby Nirwan
 *
 * DESCRIPTION:        	DBP Protocol
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
#ifndef __DBP_PARSER_H__
#define __DBP_PARSER_H__

#include <stdint.h>
#include <stdbool.h>

#define DBP_SUPPORT_PARENT_ADDRESS	0

#define QUERY_LOC			"loc:"
#define QUERY_STATE			"stat:"
#define QUERY_NODE			"mac:"
#define QUERY_ROUTER		"router:"
#define QUERY_ROUTER_LOC	"rloc:"
#define QUERY_PARENT		"parent:"
#define QUERY_KEEPALIVE		"keepalive:"
#define QUERY_CLOSE			"close:"
#define QUERY_PREFIX		"0x"

typedef enum{
	DBP_CMD_LOCATION = 0,
	DBP_CMD_STATE_REPORT,
	DBP_CMD_SENSOR_NODES,
	DBP_CMD_ROUTERS,
	DBP_CMD_ROUTERS_LOC,
	DBP_CMD_ROUTERS_MAC,
	DBP_CMD_KEEP_ALIVE,
	DBP_CMD_CLOSE_CONNECTION,
	DBP_CMD_UNKNOWN
}DBP_CMD;

int dbp_parse_data(char *input, uint32_t input_len, char *output, uint32_t output_len, char **extra);
int dbp_report_temp(int temp, char *mac, char *output);
int dbp_report_rh(uint16_t rh, char *mac, char *output);
int dbp_report_als(uint16_t als, char *mac, char *output);
int dbp_report_bat(uint16_t bat, char *mac, char *output);
int dbp_report_bat_lvl(uint16_t bat, char *mac, char *output);
int dbp_report_location(char *mac, char *output);
int dbp_report_node_deleted(uint64_t mac_addr, char *mac, char *output);
int dbp_report_exceeded(uint64_t mac_addr, char *mac, char *output);
int dbp_report_discovery_device(char *output);
#endif
