#
# Makefile for OpenzWave Socket Server
# Greg Satz
# Altered by Nick Phillips

# GNU make only

# requires libudev-dev

.SUFFIXES:	.cpp .o .a .s

CC     := $(CROSS_COMPILE)gcc
CXX    := $(CROSS_COMPILE)g++
LD     := $(CROSS_COMPILE)g++
AR     := $(CROSS_COMPILE)ar rc
RANLIB := $(CROSS_COMPILE)ranlib

DEBUG_CFLAGS    := -Wall -Wno-format -g -DDEBUG
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Wno-format -O3

DEBUG_LDFLAGS	:= -g

# Change for DEBUG or RELEASE
CFLAGS	:= -c $(DEBUG_CFLAGS)
LDFLAGS	:= $(DEBUG_LDFLAGS)

INCLUDES	:= -I ../open-zwave/cpp/src -I ../open-zwave/cpp/src/command_classes/ -I ../open-zwave/cpp/src/value_classes/ \
	-I ../open-zwave/cpp/src/platform/ -I ../open-zwave/cpp/h/platform/unix -I ../open-zwave/cpp/tinyxml/ -I ../open-zwave/cpp/hidapi/hidapi/ \
	-I src/
LIBS = $(wildcard ../open-zwave/cpp/lib/linux/*.a)

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

all: open-zwave-server

lib:
	$(MAKE) -C ../open-zwave/cpp/build/linux

ServerSocket.o: src/ServerSocket.cpp src/ServerSocket.h
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ src/ServerSocket.cpp


Socket.o: src/Socket.cpp src/Socket.h src/SocketException.h
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ src/Socket.cpp

md5.o: src/md5.cpp src/md5.h
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ src/md5.cpp

open-zwave-server:	Main.o ServerSocket.o Socket.o md5.o
	$(LD) -o $@ $(LDFLAGS) $< $(LIBS) ServerSocket.o Socket.o md5.o -lconfig -pthread -ludev

clean:
	rm -f open-zwave-server *.o

XMLLINT := $(shell whereis -b xmllint | cut -c10-)

ifeq ($(XMLLINT),)
xmltest:	$(XMLLINT)
	$(error xmllint command not found.)
else
xmltest:	$(XMLLINT)
	@$(XMLLINT) --noout --schema ../../../../config/zwcfg.xsd zwcfg_*.xml
	@$(XMLLINT) --noout --schema ../../../../config/zwscene.xsd zwscene.xml
endif
