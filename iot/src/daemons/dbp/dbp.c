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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "dbp.h"
#include "newLog.h"
#include "newDb.h"




#define MAC_STRING_LEN				16
#define MAX_NUMBER_OF_SENSOR_NODES	200
#define DBP_NODE_CMD_SIZE			26
#define DBP_NODE_CHSUM_SIZE			29
#define DBP_NODE_BUFFER_SIZE		(MAX_NUMBER_OF_SENSOR_NODES*DBP_NODE_CMD_SIZE)+(DBP_NODE_CHSUM_SIZE)

#define MAXBUF    10000
typedef struct{
        uint16_t volt; 	/* milli volt */
        uint8_t percent;	/* % */
}battery_t;



const battery_t battery_capacity_table[] = {
                {4300, 100},
                {4100, 100},
                {4027, 95},
                {3967, 85},
                {3871, 70},
                {3826, 60},
                {3796, 50},
                {3772, 40},
                {3745, 30},
                {3708, 20},
                {3666, 12},
                {3637, 9},
                {3577, 7},
                {3494, 5},
                {3437, 4},
                {3368, 3},
                {3283, 2},
                {3164, 1},
                {3000, 0},
                {0, 0}
};


/* default parent's mac address in case of failing retrieving it from database */
char temp_parent[] = "0123456789ABCDEF"; /* Dummy Parent Address */
static char buf[MAXBUF];
bool node_list_flag = false;
char sensor_node_buffer[DBP_NODE_BUFFER_SIZE+1] = {0};

/* external function */
int dbTableGetKeyWhere(char *tablename, char *key, char *value, char *wherekey, char *wherevalue);


/* internal functions */
static bool check_dbp(char *input, char *output);
static int query_handle(DBP_CMD cmd, char *mac, char *output);
static void get_mac_address(char *input, char *mac, int cmd_len);
static uint16_t dbp_swap_uint16(uint16_t val);

static uint8_t battery_get_percent(uint16_t voltage);
static uint64_t get_xor(uint64_t x, uint64_t y);

int database_callback(newdb_dev_t * pdev);

int dbp_parse_data(char *input, uint32_t input_len, char *output, uint32_t output_len, char **extra)
{
        DBP_CMD cmd;
        char mac_addr[MAC_STRING_LEN+1] = {0};
        
        printf("dbp_parse_data");
        
        if( (input == NULL) || (output == NULL) ){
                printf("null pointer");
                return -1;
        }
        
        if( (input_len <= 3) || (output_len <= 0) ){
                printf("error length");
                return -1;
        }
        
        if(!check_dbp(input, NULL)){
                printf("error: not dbp command");
                return -1;
        }
        
        /* command type check */
        if(strstr(input, QUERY_LOC) != NULL){
                if(strstr(input, QUERY_ROUTER_LOC) != NULL){
                        cmd = DBP_CMD_ROUTERS_LOC;
                        get_mac_address(input, mac_addr, 10);
                }else{
                        cmd = DBP_CMD_LOCATION;
                        get_mac_address(input, mac_addr, 9);
                }
        }else if(strstr(input, QUERY_STATE) != NULL){
                cmd = DBP_CMD_STATE_REPORT;
                get_mac_address(input, mac_addr, 10);
        }else if(strstr(input, QUERY_NODE) != NULL){
                cmd = DBP_CMD_SENSOR_NODES;
                memset(mac_addr, 0, sizeof(mac_addr));
                *extra = sensor_node_buffer;
        }else if(strstr(input, QUERY_ROUTER) != NULL){
                cmd = DBP_CMD_ROUTERS;
                memset(mac_addr, 0, sizeof(mac_addr));
        }else if(strstr(input, QUERY_ROUTER_LOC) != NULL){
                cmd = DBP_CMD_ROUTERS_LOC;
                get_mac_address(input, mac_addr, 10);
        }else if(strstr(input, QUERY_PARENT) != NULL){
                cmd = DBP_CMD_ROUTERS_MAC;
                get_mac_address(input, mac_addr, 12);
        }else if(strstr(input, QUERY_KEEPALIVE) != NULL){
                cmd = DBP_CMD_KEEP_ALIVE;
                memset(mac_addr, 0, sizeof(mac_addr));
        }else if(strstr(input, QUERY_CLOSE) != NULL){
                cmd = DBP_CMD_CLOSE_CONNECTION;
                memset(mac_addr, 0, sizeof(mac_addr));
        }else{
                /* unknown command */
                cmd = DBP_CMD_UNKNOWN;
                memset(mac_addr, 0, sizeof(mac_addr));
        }
        
        if(cmd == DBP_CMD_UNKNOWN){
                printf("unknown command");
                return -1;
        }
        
        /* execute query and send back response */
        query_handle(cmd, mac_addr, output);
        
        return 0;
}
// YB adaptation capteur xiaomi, la valeur sur 16 bits a divise par 100 , mais on le fera dans la
// partie du publisher de python 
int dbp_report_temp(int temp, char *mac, char *output)
{
        char parent_mac[MAC_STRING_LEN+1] = {0};
        int16_t temperature = (int16_t) temp;
        
        if(output == NULL) return -1;
        
#if DBP_SUPPORT_PARENT_ADDRESS
        if( dbOpen("dbp/temp", 0) ) {
                if ( !dbTableGetKeyWhere("iot_devs", "par", parent_mac, "mac", mac) ){
                        strcpy(parent_mac, temp_parent);
                }
                dbClose();
        }
#else
        strcpy(parent_mac, temp_parent);
#endif
                
        /* temperature unit is centi celcius (Celcius * 100) */
        if(sprintf(output, "dbp state tmp %08x %s %04x %s\r\n", (int) time(NULL), mac, temperature, parent_mac) < 0){
                printf("sprintf() failed");
                return -1;
        }
        
        return 0;
}

// YB adaptation capteur xiaomi
int dbp_report_rh(uint16_t rh, char *mac, char *output)
{
        char parent_mac[MAC_STRING_LEN+1] = {0};     
        uint16_t humidity = rh;
      
        
        if(output == NULL) return -1;
        
#if DBP_SUPPORT_PARENT_ADDRESS
        if( dbOpen("dbp/rh", 0) ) {
                if ( !dbTableGetKeyWhere("iot_devs", "par", parent_mac, "mac", mac) ){
                        strcpy(parent_mac, temp_parent);
                }
                dbClose();
        }
#else
        strcpy(parent_mac, temp_parent);
#endif
        
     
        
        printf("%s: humidity = %d\n", __func__, humidity);
        
        /* relative humidity unit  */
        if(sprintf(output, "dbp state hum %08x %s %04x %s\r\n", (int) time(NULL), mac, humidity, parent_mac) < 0){
                printf("sprintf() failed");
                return -1;
        }
        
        return 0;
}

int dbp_report_als(uint16_t als, char *mac, char *output)
{
        char parent_mac[MAC_STRING_LEN+1] = {0};
        
        if(output == NULL) return -1;
        
#if DBP_SUPPORT_PARENT_ADDRESS
        if( dbOpen("dbp/als", 0) ) {
                if ( !dbTableGetKeyWhere("iot_devs", "par", parent_mac, "mac", mac) ){
                        strcpy(parent_mac, temp_parent);
                }
                dbClose();
        }
#else
        strcpy(parent_mac, temp_parent);
#endif
        
        /* illuminance unit is LUX */
        if(sprintf(output, "dbp state %08x %s 0108%04x %s\r\n", (int) time(NULL), mac, dbp_swap_uint16(als), parent_mac) < 0){
                printf("sprintf() failed");
                return -1;
        }
        
        return 0;
}
// not used
int dbp_report_bat_lvl(uint16_t bat, char *mac, char *output)
{
        float batt_lvl_per_hundred;
        float max_value = 1023.0;
        uint16_t batt_level;
        uint16_t packed_data;
        char parent_mac[MAC_STRING_LEN+1] = {0};
        
        if(output == NULL){
                printf("%s: error. output is NULL\n", __func__);
                return -1;
        }
        
#if DBP_SUPPORT_PARENT_ADDRESS
        if( dbOpen("dbp/bat", 0) ) {
                if ( !dbTableGetKeyWhere("iot_devs", "par", parent_mac, "mac", mac) ){
                        strcpy(parent_mac, temp_parent);
                }
                dbClose();
        }
#else
        strcpy(parent_mac, temp_parent);
#endif
        
        batt_lvl_per_hundred = (float)bat/100.0;
        batt_level = (uint16_t) (batt_lvl_per_hundred*max_value);
        packed_data = (batt_level & 0x0003) << 14;
        
        /* only 10 bits */
        packed_data |= 0x1000; /* stype = 0x10 */
        packed_data |= ((batt_level >> 2) & 0x00ff);
        
        printf("%s: batt_level = %d\n", __func__, batt_level);
        printf("%s: packed_data = %04x\n", __func__, packed_data);
        
        /* battery capacity unit is within(0-1023) */
        if(sprintf(output, "dbp state %08x %s 01%04x %s\r\n", (int) time(NULL), mac, packed_data, parent_mac) < 0){
                printf("sprintf() failed");
                return -1;
        }
        
        return 0;
}
// YB adaptation pour mesure conso smart plug
int dbp_report_bat(uint16_t bat, char *mac, char *output)
{
        char parent_mac[MAC_STRING_LEN+1] = {0};

        if(output == NULL){
                printf("%s: error. output is NULL\n", __func__);
                return -1;
        }
        
#if DBP_SUPPORT_PARENT_ADDRESS
        if( dbOpen("dbp/bat", 0) ) {
                if ( !dbTableGetKeyWhere("iot_devs", "par", parent_mac, "mac", mac) ){
                        strcpy(parent_mac, temp_parent);
                }
                dbClose();
        }
#else
        strcpy(parent_mac, temp_parent);
#endif
        
        /* on prend la valeur brute et on la retraduit dans mqtt python  */
        if(sprintf(output, "dbp state bat %08x %s %04x %s\r\n", (int) time(NULL), mac, bat, parent_mac) < 0){
                printf("sprintf() failed");
                return -1;
        }
        
        return 0;
}



int dbp_report_location(char *mac, char *output)
{
        if(mac == NULL) return -1;
        if(output == NULL) return -1;
        
        query_handle(DBP_CMD_LOCATION, mac, output);
        
        return 0;
}

int dbp_report_node_deleted(uint64_t mac_addr, char *mac, char *output)
{
        if(output == NULL) return -1;
        
        if(sprintf(output, "dbp del %08x %s\r\n", (int) time(NULL), mac) < 0){
                printf("sprintf() failed");
                return -1;
        }
        
        return 0;
}

int dbp_report_exceeded(uint64_t mac_addr, char *mac, char *output)
{
        if(output == NULL) return -1;
        
        if(sprintf(output, "dbp max %08x %s\r\n", (int) time(NULL), mac) < 0){
                printf("sprintf() failed");
                return -1;
        }
        
        return 0;
}

static bool check_dbp(char *input, char *output)
{
        if( tolower(input[0]) != 'd' ) return false;
        if( tolower(input[1]) != 'b' ) return false;
        if( tolower(input[2]) != 'p' ) return false;
        
        return true;
        
}

static int query_handle(DBP_CMD cmd, char *mac, char *output)
{
        char location[20] = {0};
        char parent[20] = {0};
        int xloc = 0;
        int yloc = 0;
        int zloc = 0;
        int temp = 0;
        int hum = 0;
        int illu = 0;
        float rh_per_hundred;
        float max_value = 1023.0;
        uint16_t humidity_result;
        uint16_t packed_data;
        unsigned int wait_time = 0;
        
        printf("DBP Handled\n");
        
        printf("string length = %d\n", strlen(mac));
        
        mac[MAC_STRING_LEN] = '\0';
        
        if(strlen(mac) != MAC_STRING_LEN){
                if(mac[0] != '\0'){
                        sprintf(output, "dbp error mac\r\n");
                        return 0;
                }
        }
        
        printf("printing mac address\n");
        printf("%s\n", mac);
        
        /* open database for reading */
        if ( newDbOpen() ) {
            
                newdb_dev_t device;
                newDbGetDevice( mac, &device );
        
                switch(cmd){
                case DBP_CMD_LOCATION:
                        
                        /* get loc */
                        xloc = device.xloc;
                        yloc = device.yloc;
                        zloc = device.zloc;
                        
                        if ( device.nm[0] != '\0' ) {
                            strcpy(location, device.nm);
                        } else {
                            strcpy(location, "NXP");
                        }
                        
                        sprintf(output, "dbp loc %08x %s %+011d %+011d %+011d %s\r\n", (int) time(NULL), mac, xloc, yloc, zloc, location);
                        break;
                case DBP_CMD_STATE_REPORT:
                        /* get state */
                        temp = device.tmp;
                        hum = device.hum;
                        illu = device.als;
                        
                        rh_per_hundred = ((float) hum)/100.0;
                        humidity_result = (uint16_t) (rh_per_hundred*max_value);
                        packed_data = (humidity_result & 0x0003) << 14;
                        packed_data |= 0x0f00; /* stype = 0xf */
                        packed_data |= ((humidity_result >> 2) & 0x00ff);

#if DBP_SUPPORT_PARENT_ADDRESS
#else
                        strcpy(parent, temp_parent);
#endif
#if 0
                        sprintf(output, "dbp state %08x %s 0405%04x0F%04x08%04x11%04x %s\r\n", (int) time(NULL), mac, dbp_swap_uint16(temp), 
                                                                                                                                                                packed_data, dbp_swap_uint16(illu), dbp_swap_uint16(batt*100), parent);
#else
                        sprintf(output, "dbp state %08x %s 0305%04x0F%04x08%04x %s\r\n", (int) time(NULL), mac, dbp_swap_uint16(temp), 
                                                                                                                                                                packed_data, dbp_swap_uint16(illu), parent);
#endif
                        break;
                case DBP_CMD_SENSOR_NODES:
                        if(node_list_flag == false){
                                sprintf(output, "dbp node");
                                node_list_flag = true;
                                while(node_list_flag == true){
                                        /* this will block forever if callback isn't called */
                                        usleep(5*1000);
                                        wait_time++;
                                        if(wait_time >= (200*60)){
                                                /* break : 1 minute has passed */
                                                wait_time = 0;
                                                node_list_flag = false;
                                                memset(sensor_node_buffer, 0, sizeof(sensor_node_buffer));
                                                strcat(sensor_node_buffer, "dbp mac unknown\r\n");
                                        }
                                }
                        }
                        break;
                case DBP_CMD_ROUTERS:
                        sprintf(output, "dbp router 'not available'\r\n");
                        break;
                case DBP_CMD_ROUTERS_LOC:
                        sprintf(output, "dbp rloc 'not available'\r\n");
                        break;
                case DBP_CMD_ROUTERS_MAC:
                        sprintf(output, "dbp parent 'not available'\r\n");
                        break;
                case DBP_CMD_KEEP_ALIVE:
                        newLogAdd( NEWLOG_FROM_DBP,"Keep Alive");
                        break;
                case DBP_CMD_CLOSE_CONNECTION:
                        sprintf(output, "dbp close");
                        break;
                default:
                        break;
                }
                
                newDbClose();
        
        }else{
                newLogAdd( NEWLOG_FROM_DBP,"Failed to open database");
        }
        
        return 0;
}

int database_callback( newdb_dev_t * pdev )
{
        int max = 0;
        uint64_t chksum = 0;
        uint64_t temp = 0;
        char *pEnd;
        char chksum_str[30] = {0};
        
        /* init buffer */
        memset(sensor_node_buffer, 0, sizeof(sensor_node_buffer));
        
        temp = strtoll(pdev->mac, &pEnd, 16);
        chksum = get_xor(chksum, temp);
        max++;
        strcat(sensor_node_buffer, "dbp mac ");
        strcat(sensor_node_buffer, pdev->mac);
        strcat(sensor_node_buffer, "\r\n");
        
        /* checksum */
        sprintf(chksum_str, "dbp chksum %llx", chksum);
        strcat(sensor_node_buffer, chksum_str);
        strcat(sensor_node_buffer, "\r\n");
        
        /* tell the client that data is ready */
        node_list_flag = false;
        
        return 1;
}

static uint64_t get_xor(uint64_t x, uint64_t y)
{
        uint64_t a = x & y;
        uint64_t b = ~x & ~y;
        uint64_t z = ~a & ~b;
        
        return z;
}

static void get_mac_address(char *input, char *mac, int cmd_len)
{
        char *pos = NULL;
        
        pos = strstr(input, QUERY_PREFIX);
        
        printf("cmd len = %d\n", cmd_len);
        
        if(pos == NULL){
                printf("pos is NULL\n");
                /* check whether mac address exists. max cmd length = 11. */
                if(strlen(input) < cmd_len+16){
                        strcpy(mac, temp_parent);
                }else{
                        strcpy(mac, input+cmd_len);
                }
        }else{
                printf("pos is not NULL\n");
                /* remove 0x and copy mac address */
                strcpy(mac, pos+2);
        }
        
}

static uint16_t dbp_swap_uint16(uint16_t val)
{
        return (val << 8) | (val >> 8 );
}



#if 0
static uint8_t battery_get_percent(uint16_t voltage)
{
        uint8_t i;
        float temp = 0.0;

        if(voltage >= battery_capacity_table[0].volt){
                return 100;
        }

        for(i=1; i<sizeof(battery_capacity_table)/sizeof(battery_t); i++){
                if( (voltage <= battery_capacity_table[i-1].volt) && (voltage > battery_capacity_table[i].volt) ){
                        temp = (float) (voltage - battery_capacity_table[i].volt) / (float) (battery_capacity_table[i-1].volt - battery_capacity_table[i].volt);
                        temp = ((float) (battery_capacity_table[i-1].percent - battery_capacity_table[i].percent) * temp) + (float) (battery_capacity_table[i].percent);
                }
        }

        return (uint8_t)temp;
}


#endif
