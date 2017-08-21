
extern "C" {
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"

#include "net/mac/dsme/dsme.h"

#include "net/packetbuf.h"
#include "net/mac/mac-sequence.h"
#include "net/mac/framer-802154.h"
#include "cpu/arm/common/sys/mtarch.h"
}

#include "dsme_platform.h"
#include "dsme_settings.h"
#include "DSMEPlatform.h"
#include "DSMEMessage.h"

#include "openDSME/mac_services/dataStructures/IEEE802154MacAddress.h"


#if COOJA_MTARCH_STACKSIZE < 4096 or MTARCH_CONF_STACKSIZE < 1024
#error "Stack probably too small"
#endif

dsme::DSMEPlatform m_dsme;

/*
PROCESS(dsme_pending_packet_input, "DSME: pending packet input process");

PROCESS_THREAD(dsme_pending_packet_input, ev, data)
{
  PROCESS_BEGIN();
  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
  	NETSTACK_RADIO.off();
  	m_dsme.requestPending();
  	NETSTACK_RADIO.on();
  }
  PROCESS_END();
}
*/


/*
 * This function will be called from higher level
 */
static void
send_packet(mac_callback_t sent, void *ptr)
{
	m_dsme.requestFromPacketbufPending(sent, ptr);
	return;
}


/*
 * Packet received from lower level
 */
static void
packet_input(void)
{
  /* packetbuf attributes already set by handleDataMessageFromMCPS */
//	process_poll(&dsme_pending_packet_input);
  	m_dsme.requestPending();
}

static int
turn_on(void)
{
  return 0;
}

static int
turn_off(int keep_radio_on)
{
  if(keep_radio_on) {
    NETSTACK_RADIO.on();
  } else {
    NETSTACK_RADIO.off();
  }
  return 1;
}

static unsigned short
channel_check_interval(void)
{
  return 0;
}

static void
dsme_init(void)
{
	DSME_PRINTF("dsme pointer %p\n", &m_dsme);
  radio_value_t radio_rx_mode;
  radio_value_t radio_tx_mode;
  rtimer_clock_t t;
  /* Radio Rx mode */
  if(NETSTACK_RADIO.get_value(RADIO_PARAM_RX_MODE, &radio_rx_mode) != RADIO_RESULT_OK) {
  	DSME_CONSOLE("DSME:! radio does not support getting RADIO_PARAM_RX_MODE. Abort init.\n");
    return;
  }
  /* Disable poll mode */
  radio_rx_mode &= ~RADIO_RX_MODE_POLL_MODE;
  if(NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, radio_rx_mode) != RADIO_RESULT_OK) {
  	DSME_CONSOLE("DSME:! radio does not support setting required RADIO_PARAM_RX_MODE. Abort init.\n");
    return;
  }

  /* Radio Tx mode */
  if(NETSTACK_RADIO.get_value(RADIO_PARAM_TX_MODE, &radio_tx_mode) != RADIO_RESULT_OK) {
  	DSME_CONSOLE("DSME:! radio does not support getting RADIO_PARAM_TX_MODE. Abort init.\n");
    return;
  }
  /* Unset CCA */
  //radio_tx_mode |= RADIO_TX_MODE_SEND_ON_CCA;
  if(NETSTACK_RADIO.set_value(RADIO_PARAM_TX_MODE, radio_tx_mode) != RADIO_RESULT_OK) {
    DSME_PRINTF("DSME:! radio does not support setting required RADIO_PARAM_TX_MODE. Abort init.\n");
    return;
  }
  /* Test setting channel */
  if(NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, dsme::MAC_DEFAULT_CHANNEL) != RADIO_RESULT_OK) {
  	DSME_CONSOLE("DSME:! radio does not support setting channel. Abort init.\n");
    return;
  }
  /* Test getting timestamp */
  if(NETSTACK_RADIO.get_object(RADIO_PARAM_LAST_PACKET_TIMESTAMP, &t, sizeof(rtimer_clock_t)) != RADIO_RESULT_OK) {
  	DSME_CONSOLE("DSME:! radio does not support getting last packet timestamp. Abort init.\n");
    return;
  }

  m_dsme.initialize();
  m_dsme.start();
  DSME_PRINTF("DSME: Initialized\n");
  dsme::DSMEMessage* dsmemsg = m_dsme.getEmptyMessage(); // TODO why?
  //process_start(&dsme_pending_packet_input, NULL);

}

uint32_t dsmeAssociated() {
		if(m_dsme.isAssociated()) {
				return 1;
		} else {
				return 0;
		}
}

const struct mac_driver dsmemac_driver = {
  (char*) "DSME",
  dsme_init,
  send_packet,
  packet_input,
  turn_on,
  turn_off,
  channel_check_interval,
};
