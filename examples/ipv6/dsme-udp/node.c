#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-route.h"

#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#ifdef MAKE_FOR_M3
#include "../../../../openlab/drivers/unique_id.h"
#define nodeID platform_uid()
#else
#include "node-id.h"
#define nodeID node_id
#endif

#include "net/mac/dsme/dsme.h"

#include "dev/serial-line.h"

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#include <stdbool.h>

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define UDP_EXAMPLE_ID  190

#ifndef PERIOD
#define PERIOD 10
#endif

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL			(PERIOD * CLOCK_SECOND)
#define SEND_TIME					(random_rand() % (SEND_INTERVAL))
#define MAX_PAYLOAD_LEN		40

static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;
static uint32_t seq_id, messages;
static uint8_t valid;
static char *str;

/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
  char buf[MAX_PAYLOAD_LEN];
  seq_id++;
  PRINTF("DATA send to %d 'Hello %lu', validity %u\n",
         server_ipaddr.u8[sizeof(server_ipaddr.u8) - 1], seq_id, valid);
  sprintf(buf, "Hello %lu from the client, validity %u", seq_id, valid);
  if(valid) {
  	messages++;
  }
  uip_udp_packet_sendto(client_conn, buf, strlen(buf),
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
	uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

/* The choice of server address determines its 6LoPAN header compression.
 * (Our address will be compressed Mode 3 since it is derived from our link-local address)
 * Obviously the choice made here must also be selected in udp-server.c.
 *
 * For correct Wireshark decoding using a sniffer, add the /64 prefix to the 6LowPAN protocol preferences,
 * e.g. set Context 0 to fd00::.  At present Wireshark copies Context/128 and then overwrites it.
 * (Setting Context 0 to fd00::1111:2222:3333:4444 will report a 16 bit compressed address of fd00::1111:22ff:fe33:xxxx)
 *
 * Note the IPCMV6 checksum verification depends on the correct uncompressed addresses.
 */

#if 0
/* Mode 1 - 64 bits inline */
   uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 1);
#elif 1
/* Mode 2 - 16 bits inline */
  uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
#else
/* Mode 3 - derived from server link-local (MAC) address */
  uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0x0250, 0xc2ff, 0xfea8, 0xcd1a); //redbee-econotag
#endif
}

PROCESS(coordinator_process, "DSME Node");

AUTOSTART_PROCESSES(&coordinator_process);


PROCESS_THREAD(coordinator_process, ev, data)
{
  static struct etimer et;
  static struct ctimer backoff_timer;
  PROCESS_BEGIN();

  valid = 0;
  messages = 0;
  static bool warmupDone = false;

  set_global_address();

  PRINTF("UDP client process started nbr:%d routes:%d\n",
		 NBR_TABLE_CONF_MAX_NEIGHBORS, UIP_CONF_MAX_ROUTES);

  print_local_addresses();

  client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL);
	if(client_conn == NULL) {
		PRINTF("No UDP connection available, exiting the process!\n");
		PROCESS_EXIT();
	}
	udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT));

	PRINTF("Created a connection with the server ");
	PRINT6ADDR(&client_conn->ripaddr);
	PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));



	/* initialize serial line */
	serial_line_init();

  /* Print out routing tables every minute */
  etimer_set(&et, SEND_INTERVAL);//CLOCK_SECOND * 60);
  while(1) {
			PROCESS_YIELD();

			if(etimer_expired(&et)) {
					PRINTF("Node %d still alive, asso = %lu\n", nodeID, dsmeAssociated());
					if(dsmeAssociated() && warmupDone) {
						valid = 1;
					}
					ctimer_set(&backoff_timer, SEND_TIME, send_packet, NULL);
					etimer_reset(&et);
			}
			if(ev == serial_line_event_message) {
				if(data != NULL) {
					str = data;
					PRINTF("Serial Input received: %s\n", str);
					if(str[0] == 'e') {
						PRINTF("received 'ending' msg\n");
						valid = 0;
						warmupDone = false;
						//messages = seq_id;
					} else if(str[0] == 'd') {
						PRINTF("DONE received -- terminating\n");
						PRINTF("Client mote with ID %d sent %lu messages\n", nodeID, messages);
						etimer_stop(&et);
						etimer_reset(&et);
						break;
					} else if(str[0] == 'w') {
						PRINTF("serialInput: warmup\n");
						warmupDone = true;
					}
				} else{
					PRINTF("Serial Input received but DATA == NULL\n");
				}
	    }

  }

  PROCESS_END();
}
