all: node
CONTIKI=../../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

MODULES += core/net/mac/dsme

CONTIKI_WITH_IPV6 = 1
CFLAGS += -DMAKE_FOR_M3=1

ifdef RATE
CFLAGS += -DPERIOD=$(RATE)	#rate to send udp messages defined in milliseconds
else
CFLAGS += -DPERIOD=1500
endif
ifdef DSME_PAN_COORDINATOR
CFLAGS += -DDSME_PAN_COORDINATOR=$(DSME_PAN_COORDINATOR)
else
#CFLAGS += -DDSME_PAN_COORDINATOR=1
CFLAGS += -DDSME_PAN_COORDINATOR=0xb674
endif
include $(CONTIKI)/core/net/mac/dsme/Makefile.dsme
