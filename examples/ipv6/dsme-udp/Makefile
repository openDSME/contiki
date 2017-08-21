all: node coordinator 
CONTIKI=../../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

MODULES += core/net/mac/dsme

CONTIKI_WITH_IPV6 = 1
CFLAGS += -DMAKE_FOR_M3=1

ifdef RATE
CFLAGS += -DPERIOD=$(RATE)	#rate to send udp messages defined in seconds
else
CFLAGS += -DPERIOD=10
endif
include $(CONTIKI)/core/net/mac/dsme/Makefile.dsme
