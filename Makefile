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

open-zwave-server:	Main.o lib
	$(LD) -o $@ $(LDFLAGS) $< $(LIBS) -lconfig -pthread -ludev

clean:
	rm -f open-zwave-server Main.o

XMLLINT := $(shell whereis -b xmllint | cut -c10-)

ifeq ($(XMLLINT),)
xmltest:	$(XMLLINT)
	$(error xmllint command not found.)
else
xmltest:	$(XMLLINT)
	@$(XMLLINT) --noout --schema ../../../../config/zwcfg.xsd zwcfg_*.xml
	@$(XMLLINT) --noout --schema ../../../../config/zwscene.xsd zwscene.xml
endif
