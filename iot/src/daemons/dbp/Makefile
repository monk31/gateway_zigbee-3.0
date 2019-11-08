# ------------------------------------------------------------------
# DBP makefile
# ------------------------------------------------------------------
# Author:    nxp80037
# Copyright: NXP B.V. 2014. All rights reserved
# ------------------------------------------------------------------

LDLIBS += -lpthread -lc

TARGET = iot_dbp

INCLUDES = -I../../IotCommon
OBJECTS = dbp_main.o \
	dbp.o \
	dbp_search.o \
	dbp_loc_receiver.o \
	../../IotCommon/atoi.o \
	../../IotCommon/iotError.o \
	../../IotCommon/iotSemaphore.o \
	../../IotCommon/newDb.o \
	../../IotCommon/parsing.o \
	../../IotCommon/json.o \
	../../IotCommon/fileCreate.o \
	../../IotCommon/queue.o \
	../../IotCommon/dump.o \
	../../IotCommon/newLog.o

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -Wall -g -c $< -o $@

all: clean build

build: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(TARGET) $(LDLIBS)
	cp $(TARGET) /usr/local/bin/

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TARGET)
	-rm -f /usr/local/bin/$(TARGET)


