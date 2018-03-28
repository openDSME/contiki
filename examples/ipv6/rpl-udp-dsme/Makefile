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
# rate to send udp messages defined in milliseconds
CFLAGS += -DPERIOD=$(RATE)
else
CFLAGS += -DPERIOD=1500
endif
ifdef PAN_COORDINATOR
CFLAGS += -DPAN_COORDINATOR=$(PAN_COORDINATOR)
else
$(error No PAN_COORDINATOR specified)
endif

ifdef DSME_CAP_REDUCTION
CFLAGS += -DDSME_CAP_REDUCTION=$(DSME_CAP_REDUCTION)
else
ifdef DSME
$(error No DSME_CAP_REDUCTION specified)
else
# irrelevant for CSMA, but build error otherwise
CFLAGS += -DDSME_CAP_REDUCTION=0
endif
endif

include $(CONTIKI)/core/net/mac/dsme/Makefile.dsme
