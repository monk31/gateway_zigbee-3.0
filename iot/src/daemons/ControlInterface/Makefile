# ------------------------------------------------------------------
# Topo program makefile
# ------------------------------------------------------------------
# Author:    nlv10677
# Copyright: NXP B.V. 2014. All rights reserved
# ------------------------------------------------------------------

LDLIBS += -lrt -lpthread

TARGET = iot_ci

INCLUDES = -I../../IotCommon
OBJECTS = ci_main.o \
	topo.o \
	topoGen.o \
	dbget.o \
	dbEdit.o \
	ctrl.o \
	sensor.o \
	cmd.o \
	lmp.o \
	grp.o \
	proc.o \
	../../IotCommon/atoi.o \
	../../IotCommon/colorConv.o \
	../../IotCommon/RgbSpaceMatrices.o \
	../../IotCommon/blackbody.o \
	../../IotCommon/iotError.o \
	../../IotCommon/iotSleep.o \
	../../IotCommon/gateway.o \
	../../IotCommon/strtoupper.o \
	../../IotCommon/newDb.o \
	../../IotCommon/plugUsage.o \
	../../IotCommon/systemtable.o \
	../../IotCommon/parsing.o \
	../../IotCommon/json.o \
	../../IotCommon/jsonCreate.o \
	../../IotCommon/fileCreate.o \
	../../IotCommon/iotSemaphore.o \
	../../IotCommon/queue.o \
	../../IotCommon/socket.o \
	../../IotCommon/dump.o \
	../../IotCommon/iotTimer.o \
	../../IotCommon/newLog.o


%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -Wall -g -c $< -o $@

all: clean build

build: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(TARGET) $(LDLIBS) -lc
	cp $(TARGET) /usr/local/bin/

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TARGET)
	-rm -f usr/local/bin/$(TARGET)

