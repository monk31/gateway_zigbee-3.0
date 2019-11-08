# ------------------------------------------------------------------
# ZCB makefile
# ------------------------------------------------------------------
# Author:    nlv10677
# Copyright: NXP B.V. 2014. All rights reserved
# ------------------------------------------------------------------

LDLIBS += -lpthread -lm -lc

TARGET = iot_zb

INCLUDES = -I../../IotCommon -I../../IotCommon/cJSON

OBJECTS = zb_main.o \
	cmd.o \
	lmp.o \
	grp.o \
	lmpgrp.o \
	plg.o \
	ctrl.o \
	topo.o \
	tunnel.o \
	Serial.o \
	SerialLink.o \
	Utils.o \
	zcb.o \
	ZigbeeDevices.o \
	../../IotCommon/atoi.o \
	../../IotCommon/newDb.o \
	../../IotCommon/systemtable.o \
	../../IotCommon/iotError.o \
	../../IotCommon/iotSleep.o \
	../../IotCommon/fileCreate.o \
	../../IotCommon/iotSemaphore.o \
	../../IotCommon/socket.o \
	../../IotCommon/queue.o \
	../../IotCommon/parsing.o \
	../../IotCommon/colorConv.o \
	../../IotCommon/RgbSpaceMatrices.o \
	../../IotCommon/blackbody.o \
	../../IotCommon/jsonCreate.o \
	../../IotCommon/plugUsage.o \
	../../IotCommon/json.o \
	../../IotCommon/nibbles.o \
	../../IotCommon/dump.o \
	../../IotCommon/newLog.o \
	../../IotCommon/cJSON/cJSON.o


%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -Wall -std=gnu99 -g -c $< -o $@


all: clean build

build: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(TARGET) $(LDLIBS)
	cp $(TARGET) /usr/local/bin/

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TARGET)
	-rm -f /usr/local/bin/$(TARGET) 

