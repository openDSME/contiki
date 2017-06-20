all: lwm2m-example-client
CONTIKI_SOURCEFILES += lwm2m-objects.c

CONTIKI = ../../..

MODULES += core/net/mac/tsch

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

# linker optimizations
SMALL=1

# REST Engine shall use Erbium CoAP implementation
APPS += rest-engine
APPS += er-coap
APPS += oma-lwm2m

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
