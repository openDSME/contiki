#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/rpl/rpl.h"
#include "net/netstack.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/rpl/rpl-private.h"

#include "net/mac/dsme/dsme.h"

#include "lib/assert.h"

#if CONTIKI_TARGET_IOTLAB_M3
#include "../../../../openlab/drivers/unique_id.h"
#define IS_SERVER (DSME_PAN_COORDINATOR == platform_uid())
#elif CONTIKI_TARGET_COOJA
#include "node-id.h"
#define IS_SERVER (DSME_PAN_COORDINATOR == node_id)
#else
#error "Node ID not specified"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

#include "dev/serial-line.h"

#ifndef PERIOD
#define PERIOD 30
#endif

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL		(PERIOD * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
#define MAX_PAYLOAD_LEN		30

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define UDP_EXAMPLE_ID  190

static struct uip_udp_conn *connection;
static uip_ipaddr_t server_ipaddr;
static int seq_id;

/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  char *appdata;

  if(uip_newdata()) {
    appdata = (char *)uip_appdata;
    appdata[uip_datalen()] = 0;
    PRINTF("DATA recv '%s' from ", appdata);
    PRINTF("%d",
           UIP_IP_BUF->srcipaddr.u8[sizeof(UIP_IP_BUF->srcipaddr.u8) - 1]);
    PRINTF("\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
  char buf[MAX_PAYLOAD_LEN];

  seq_id++;
  PRINTF("DATA send to %d 'Hello %d'\n",
         server_ipaddr.u8[sizeof(server_ipaddr.u8) - 1], seq_id);
  sprintf(buf, "Hello %d from the client", seq_id);
  uip_udp_packet_sendto(connection, buf, strlen(buf),
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
}
/*---------------------------------------------------------------------------*/

PROCESS(main_process, "DSME Node");

AUTOSTART_PROCESSES(&main_process);

PROCESS_THREAD(main_process, ev, data)
{
  static struct etimer periodic;
  static struct ctimer backoff_timer;
  struct uip_ds6_addr *root_if;

  PROCESS_BEGIN();

  uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);

  PRINTF("max_nbr:%d max_routes:%d\n",
           NBR_TABLE_CONF_MAX_NEIGHBORS, UIP_CONF_MAX_ROUTES);

  if(IS_SERVER) {
    PRINTF("UDP server started\n");

    uip_ds6_addr_add(&server_ipaddr, 0, ADDR_MANUAL);
    root_if = uip_ds6_addr_lookup(&server_ipaddr);
    if(root_if != NULL) {
      rpl_dag_t *dag;
      dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)&server_ipaddr);
      uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
      rpl_set_prefix(dag, &server_ipaddr, 64);
      PRINTF("created a new RPL dag\n");
    } else {
      PRINTF("failed to create a new RPL DAG\n");
    }

    connection = udp_new(NULL, UIP_HTONS(UDP_CLIENT_PORT), NULL);
	if(connection == NULL) {
		PRINTF("No UDP connection available, exiting the process!\n");
		PROCESS_EXIT();
	}
	udp_bind(connection, UIP_HTONS(UDP_SERVER_PORT));

	PRINTF("Created a server connection with remote address ");
	PRINT6ADDR(&connection->ripaddr);
	PRINTF(" local/remote port %u/%u\n", UIP_HTONS(connection->lport),
				 UIP_HTONS(connection->rport));
  }
  else {
    PRINTF("UDP client started\n");

    /* new connection with remote host */
    connection = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL); 
    if(connection == NULL) {
      PRINTF("No UDP connection available, exiting the process!\n");
      PROCESS_EXIT();
    }
    udp_bind(connection, UIP_HTONS(UDP_CLIENT_PORT)); 

    PRINTF("Created a connection with the server ");
    PRINT6ADDR(&connection->ripaddr);
    PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(connection->lport), UIP_HTONS(connection->rport));
  }

  if(!IS_SERVER) {
    etimer_set(&periodic, SEND_INTERVAL);
  }
  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }

    if(!IS_SERVER && etimer_expired(&periodic)) {
      etimer_reset(&periodic);
      ctimer_set(&backoff_timer, SEND_TIME, send_packet, NULL);
    }
  }

  PROCESS_END();
}
