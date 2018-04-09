
#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__


#ifndef WITH_NON_STORING
#define WITH_NON_STORING 0 /* Set this to run with non-storing mode */
#endif /* WITH_NON_STORING */

#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#undef UIP_CONF_MAX_ROUTES

#ifdef COOJA_CONF_TRANSMIT_ON_CCA
#undef COOJA_CONF_TRANSMIT_ON_CCA
#endif
#define COOJA_CONF_TRANSMIT_ON_CCA 0

#ifdef TEST_MORE_ROUTES
/* configure number of neighbors and routes */
#define NBR_TABLE_CONF_MAX_NEIGHBORS     10
#define UIP_CONF_MAX_ROUTES   30
#else
/* configure number of neighbors and routes */
#define NBR_TABLE_CONF_MAX_NEIGHBORS     10
#define UIP_CONF_MAX_ROUTES   10
#endif /* TEST_MORE_ROUTES */

/* Netstack layers */
#undef NETSTACK_CONF_MAC
#undef NETSTACK_CONF_RDC
#ifdef DSME
#define NETSTACK_CONF_MAC     dsmemac_driver
#define NETSTACK_CONF_RDC     dsme_rdc_driver
#else
#ifdef CSMA
#define NETSTACK_CONF_MAC	csma_driver
#define NETSTACK_CONF_RDC     nullrdc_driver
#else
#error "MAC not defined!"
#endif
#endif

#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER  framer_802154

/* IEEE802.15.4 frame version */
#undef FRAME802154_CONF_VERSION
#define FRAME802154_CONF_VERSION FRAME802154_IEEE802154E_2012

/* IEEE802.15.4 PANID */
#undef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID 0xabcd

/* Define as minutes */
#define RPL_CONF_DEFAULT_LIFETIME_UNIT   60

/* 10 minutes lifetime of routes */
#define RPL_CONF_DEFAULT_LIFETIME        10

#define RPL_CONF_DEFAULT_ROUTE_INFINITE_LIFETIME 1

#if WITH_NON_STORING
#undef RPL_NS_CONF_LINK_NUM
#define RPL_NS_CONF_LINK_NUM 40 /* Number of links maintained at the root. Can be set to 0 at non-root nodes. */
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 0 /* No need for routes */
#undef RPL_CONF_MOP
#define RPL_CONF_MOP RPL_MOP_NON_STORING /* Mode of operation*/
#endif /* WITH_NON_STORING */

#define RPL_MRHOF_CONF_SQUARED_ETX 1
#define RPL_CONF_WITH_MC 1
#define RPL_CONF_DAG_MC RPL_DAG_MC_ETX

/* DAO ACKs lead to repeated allocation and deallocation of slots. It is also not really useful for our application.
 * Alternatively, we could also mark DAO ACKs as CAP traffic.
 * The same holds for PROBING
 */
#define RPL_CONF_WITH_DAO_ACK 0
#define RPL_CONF_WITH_PROBING 0

#if CONTIKI_TARGET_COOJA
#define COOJA_CONF_SIMULATE_TURNAROUND 0
#endif /* CONTIKI_TARGET_COOJA */

#undef COOJA_MTARCH_STACKSIZE
#define COOJA_MTARCH_STACKSIZE 4096
#undef MTARCH_CONF_STACKSIZE
#define MTARCH_CONF_STACKSIZE 1024

/* Needed for IoT-LAB M3 nodes */
#undef RF2XX_SOFT_PREPARE
#define RF2XX_SOFT_PREPARE 0
#undef RF2XX_WITH_TSCH
#define RF2XX_WITH_TSCH 1
#define RF2XX_TX_POWER  PHY_POWER_3dBm
#define RF2XX_RX_RSSI_THRESHOLD  RF2XX_PHY_RX_THRESHOLD__m101dBm
#define TSCH_CONF_JOIN_HOPPING_SEQUENCE TSCH_HOPPING_SEQUENCE_1_1
#define RPL_CONF_DIO_INTERVAL_MIN 3
#define RPL_CONF_DIO_INTERVAL_MAX 8

#endif /* __PROJECT_CONF_H__ */
