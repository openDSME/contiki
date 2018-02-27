all: node
CONTIKI=../../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

MODULES += core/net/mac/dsme

CONTIKI_WITH_IPV6 = 1
CFLAGS += -DMAKE_FOR_M3=1

ifdef DSME
CFLAGS += -DDSME
else
ifdef CSMA
CFLAGS += -DCSMA
else
$(error Neither DSME nor CSMA defined)
endif
endif


ifdef RATE
CFLAGS += -DPERIOD=$(RATE)	#rate to send udp messages defined in milliseconds
else
CFLAGS += -DPERIOD=1500
endif
ifdef PAN_COORDINATOR
CFLAGS += -DPAN_COORDINATOR=$(PAN_COORDINATOR)
else
$(error No PAN_COORDINATOR specified)
endif
include $(CONTIKI)/core/net/mac/dsme/Makefile.dsme
