#include "DSMEPlatform.h"
#include "dsme_settings.h"
#include "dsme_atomic.h"
#include "dsme_platform.h"

#include "DSMEMessageBuffer.h"

#include "openDSME/dsmeLayer/messages/MACCommand.h"
#include "openDSME/dsmeLayer/DSMELayer.h"
#include "openDSME/dsmeAdaptionLayer/scheduling/PIDScheduling.h"

extern "C" {
#include "sys/pt.h"
#include "limits.h"
#include "sys/ctimer.h"
#include "net/packetbuf.h"
#include "net/mac/frame802154.h"
}

#if !(CONTIKI_TARGET_COOJA || CONTIKI_TARGET_COOJA_IP64)
#include "MacSymbolCounter.h"
#endif

#define RTIMERTICKS_TO_SYMBOLS(T) ((((uint64_t)(T))*(1000000/16))/RTIMER_ARCH_SECOND)
#define SYMBOLS_TO_RTIMERTICKS(SYM) ((((uint64_t)(SYM))*RTIMER_ARCH_SECOND)/(1000000/16))

#ifndef DSME_CHANNELS
#define DSME_CHANNELS "11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26"
#endif

namespace dsme {

uint8_t DSMEPlatform::state = STATE_READY;
DSMEPlatform* DSMEPlatform::instance = nullptr;
Delegate<void(bool)> DSMEPlatform::txEndCallback;
#if CONTIKI_TARGET_COOJA || CONTIKI_TARGET_COOJA_IP64
static struct rtimer rt;
#endif
static struct ctimer handleMessgeTaskCallbackTimer, sendCallbackTimer, CCACallbackTimer;
static bool tx_Success;



static void handlingMessage(void *ptr);
static void sendDone(void *ptr);
static void ccaResult_ready(void *ptr);

/* all zero, this all gets handled by MAC-layer */
mac_ackCfg_t DSMEPlatform::ackCfg { 0, 0 };
mac_backoffCfg_t DSMEPlatform::backoffCfg { 0, 0, 0, 0 };

DSMEPlatform::DSMEPlatform() :
				phy_pib(),
				mac_pib(phy_pib),

				mcps_sap(dsme),
				mlme_sap(dsme),
				dsmeAdaptionLayer(dsme),

				messagesInUse(0),
				initialized(false),
        channel(MIN_CHANNEL),
				currentTXLength(0){
    instance = this;
}

/**
 * Callback of rtimer signaling a timer interrupt.
 */
#if CONTIKI_TARGET_COOJA || CONTIKI_TARGET_COOJA_IP64
static void callInterrupt(struct rtimer *t, void *ptr) {
#else
static void callInterrupt() {
#endif
    dsme::DSMEPlatform::instance->getDSME().getEventDispatcher().timerInterrupt();
}

/**
 * Initialize method for DSMEPlatform. Also initializes DSMELayer and DSMEAdaptionLayer.
 * Has to be called before calling DSMEPlatform::start().
 */
void DSMEPlatform::initialize() {
		this->instance = this;
		this->dsme.setPHY_PIB(&(this->phy_pib));
		this->dsme.setMAC_PIB(&(this->mac_pib));
		this->dsme.setMCPS(&(this->mcps_sap));
		this->dsme.setMLME(&(this->mlme_sap));

    /* Initialize channels */
    char str[] = DSME_CHANNELS;
    constexpr uint8_t MAX_CHANNELS = 16;
    uint8_t channels[MAX_CHANNELS];

    uint8_t num = 0;
    char* token = strtok(str,",");
    while(token != nullptr) {
        DSME_ASSERT(num < MAX_CHANNELS);
        channels[num] = atoi(token);
        num++;
        token = strtok(nullptr,",");
    }

    channelList_t DSSS2450_channels(num);
    for(uint8_t i = 0; i < num; i++) {
        DSSS2450_channels[i] = channels[i];
    }

    phy_pib.setDSSS2450ChannelPage(DSSS2450_channels);

    /* Initialize Address */
    IEEE802154MacAddress address;

    uint16_t id = palId_id();
    translateMacAddress(id, this->mac_pib.macExtendedAddress);

    this->mac_pib.macShortAddress = this->mac_pib.macExtendedAddress.getShortAddress();
    this->mac_pib.macIsPANCoord = (PAN_COORDINATOR == id);
    if(this->mac_pib.macIsPANCoord) {
			DSME_PRINTF("This node is PAN coordinator\n");
			this->mac_pib.macPANId = MAC_DEFAULT_NWK_ID;
		}
    this->mac_pib.macAssociatedPANCoord = this->mac_pib.macIsPANCoord;
    this->mac_pib.macBeaconOrder = 6;
    this->mac_pib.macSuperframeOrder = 3;
    this->mac_pib.macMultiSuperframeOrder = 5;

    this->mac_pib.macMinBE = 3;
    this->mac_pib.macMaxBE = 8;
    this->mac_pib.macMaxCSMABackoffs = 5;
    this->mac_pib.macMaxFrameRetries = 3;

    this->mac_pib.macDSMEGTSExpirationTime = 7;
    this->mac_pib.macResponseWaitTime = 16;

    this->phy_pib.phyCurrentChannel = MAC_DEFAULT_CHANNEL;

    this->dsmeAdaptionLayer.setIndicationCallback(DELEGATE(&DSMEPlatform::handleDataMessageFromMCPSWrapper, *this));
    this->dsmeAdaptionLayer.setConfirmCallback(DELEGATE(&DSMEPlatform::handleConfirmFromMCPSWrapper, *this));

    this->dsme.initialize(this);

#if !(CONTIKI_TARGET_COOJA || CONTIKI_TARGET_COOJA_IP64)
    MacSymbolCounter::getInstance().init(callInterrupt);
#endif

    channelList_t scanChannels;
    scanChannels.add(MAC_DEFAULT_CHANNEL);
    scheduling = new PIDScheduling(this->dsmeAdaptionLayer);
    this->dsmeAdaptionLayer.initialize(scanChannels,DSME_SCAN_DURATION,scheduling);
    this->initialized = true;
}

/**
 * Start method of DSMEPlatform. Calls starting methods of DSMELayer and DSMEAdaptionLayer, and by that, starting the DSME association. Should only be called after DSMEPlatform::initialize().
 */
void DSMEPlatform::start() {
		if(this->initialized) {
				this->dsme.start();
				this->dsmeAdaptionLayer.startAssociation();
		} else {
				DSME_PRINTF("DSME:! DSME has to be initialized before starting!");
				DSME_ASSERT(false);
		}
}

/**
 * Creates an IEEE802154MacAddress out of an uint16_t short address
 */
void DSMEPlatform::translateMacAddress(uint16_t& from, IEEE802154MacAddress& to) {
    if(from == 0xFFFF) {
        to = IEEE802154MacAddress(IEEE802154MacAddress::SHORT_BROADCAST_ADDRESS);
    } else {
        to.setShortAddress(from);
    }
}

void DSMEPlatform::handleDataMessageFromMCPSWrapper(IDSMEMessage* msg) {
		this->handleDataMessageFromMCPS(static_cast<DSMEMessage*>(msg));
}


/**
 * Passes received message to upper layer (NETSTACK_LLSEC) after dsme is done with the message.
 */
void DSMEPlatform::handleDataMessageFromMCPS(DSMEMessage* msg) {
  	packetbuf_copyfrom(msg->getPayload(), msg->getPayloadLength());
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, msg->radio_last_rssi);
    packetbuf_set_attr(RADIO_PARAM_LAST_LINK_QUALITY, msg->messageLQI);
    DSME_ASSERT(msg->channelSent < USHRT_MAX)
    packetbuf_set_attr(PACKETBUF_ATTR_CHANNEL, ((uint16_t) msg->channelSent));

    int frame_parsed = 1;
		frame_parsed = NETSTACK_FRAMER.parse();
		releaseMessage(msg);
		if(frame_parsed < 0) {
			DSME_PRINTF("DSME:! failed to parse %u\n", packetbuf_datalen());
			return;
		}
    NETSTACK_LLSEC.input();
}

void DSMEPlatform::handleConfirmFromMCPSWrapper(IDSMEMessage* msg, DataStatus::Data_Status dataStatus) {
		this->handleConfirmFromMCPS(static_cast<DSMEMessage*>(msg), dataStatus);
}

void DSMEPlatform::handleConfirmFromMCPS(DSMEMessage* msg, DataStatus::Data_Status dataStatus) {
		//TODO Contiki: better mapping of error cases
		if(msg != nullptr) {
				mac_result_t status = MAC_TX_ERR_FATAL;
				switch(dataStatus) {
				case DataStatus::SUCCESS:
						status = MAC_TX_OK;
						break;
				case DataStatus::INVALID_GTS:
						status = MAC_TX_ERR;
						LOG_ERROR("!DSMEPlatform_handleConfirmFromMCPS: GTS was invalid\n");
						break;
				case DataStatus::NO_ACK:
						status = MAC_TX_NOACK;
						LOG_ERROR("!DSMEPlatform_handleConfirmFromMCPS: No ACK received\n");
						break;
				case DataStatus::TRANSACTION_OVERFLOW:
						status = MAC_TX_ERR;
						LOG_ERROR("!DSMEPlatform_handleConfirmFromMCPS: Transaction overflow\n");
						break;
				case DataStatus::TRANSACTION_EXPIRED:
						status = MAC_TX_ERR;
						LOG_ERROR("!DSMEPlatform_handleConfirmFromMCPS: Transaction expired\n");
						break;
				case DataStatus::CHANNEL_ACCESS_FAILURE:
						status = MAC_TX_ERR_FATAL;
						LOG_ERROR("!DSMEPlatform_handleConfirmFromMCPS: Channel access failure\n");
						break;
				default:
						DSME_ASSERT(false);
				}
				uint8_t len = packetbuf_copyfrom(msg->getPayload(), msg->getPayloadLength());
				if(len != msg->getPayloadLength()) {
						DSME_PRINTF("!DSMEPlatform: Payload from msg did not fit into packetbuf\n");
						DSME_ASSERT(false);
				}
				mac_call_sent_callback(msg->getMacCallbackFunction(), msg->getMacCallbackPointer(), status, 1);
		}
    releaseMessage(msg);
}

/**
 * Tests whether the handleMessageTask is occupied.
 */
bool DSMEPlatform::isReceptionFromAckLayerPossible() {
		return (handleMessageTask.isOccupied() == false);
}

void DSMEPlatform::handleReceivedMessageFromAckLayer(IDSMEMessage* message) {
		this->handleReceivedMessageFromAckLayer(static_cast<DSMEMessage*>(message));
}

void DSMEPlatform::handleReceivedMessageFromAckLayer(DSMEMessage* message) {
		DSME_ASSERT(handleMessageTask.isOccupied() == false);
		bool success = handleMessageTask.occupy(message, this->receiveFromAckLayerDelegate);
		DSME_ASSERT(success); // assume that isMessageReceptionFromAckLayerPossible was called before in the same atomic block
		ctimer_set(&handleMessgeTaskCallbackTimer, (clock_time_t) 0, handlingMessage, this);
}

void DSMEPlatform::scheduleStartOfCFP() {
		this->dsme.handleStartOfCFP();

}

void DSMEPlatform::setReceiveDelegate(receive_delegate_t receiveDelegate) {
    this->receiveFromAckLayerDelegate = receiveDelegate;
}

bool DSMEPlatform::isAssociated() {
		return this->mac_pib.macAssociatedPANCoord;
}

mac_result_t DSMEPlatform::getMCPSTtransmitStatus(){
		return this->MCPS_transmit_status;
}

/**
 * Is called if a package from upper layer reaches the DSME MAC layer. Translates the IP-addresses into DSME addresses,
 * parses information from the packetbuffer and copies the payload into an DSMEMessage. DSMEMessage gets further processed
 * in DSMEAdaptionLayer.
 */
void DSMEPlatform::requestFromPacketbufPending(mac_callback_t sent, void *ptr) {
  	const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);

    if(!linkaddr_cmp(dest, &linkaddr_null)) {
			packetbuf_set_attr(PACKETBUF_ATTR_MAC_ACK, 1);
		}
    packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);

#if !NETSTACK_CONF_BRIDGE_MODE
  /*
   * In the Contiki stack, the source address of a frame is set at the RDC
   * layer. Since DSME doesn't use any RDC protocol and bypasses the layer to
   * transmit a frame, it should set the source address by itself.
   */
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
#endif

		if(NETSTACK_FRAMER.create() < 0) {
				DSME_PRINTF("DSME:! can't send packet due to framer error\n");
				int ret = MAC_TX_ERR;
				mac_call_sent_callback(sent, ptr, ret, 1);
		} else {
				dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
				uint16_t rec_id = ((uint8_t) dest->u8[6] << 8 |(uint8_t)dest->u8[7]);
//				DSME_PRINTF("DSMEPlatform: receiver address = %04x\n", rec_id);
				IEEE802154MacAddress receiver = IEEE802154MacAddress(rec_id);
				if(linkaddr_cmp(dest, &linkaddr_null)) {
//						DSME_LOG("DSMEPlatform-reqFromPackBuf: receiver address: "<< HEXOUT << receiver.a1() << ":" << receiver.a2() << ":" << receiver.a3() << ":" << receiver.a4() << DECOUT);
		    		receiver.setShortAddress(IEEE802154MacAddress::SHORT_BROADCAST_ADDRESS);
				}
				DSMEMessage* dsmemsg = this->getEmptyMessage();
				uint8_t len = packetbuf_copyto(dsmemsg->getPayload());
				dsmemsg->setPayloadLength(len);

				dsmemsg->setMacCallbackFunction(sent);
				dsmemsg->setMacCallbackPointer(ptr);

				auto& hdr = dsmemsg->getHeader();
				hdr.setFrameType(IEEE802154eMACHeader::DATA);
				hdr.setSrcAddr(this->mac_pib.macExtendedAddress);

				hdr.setDstAddr(receiver);

				dsmemsg->firstTry = true;
				this->dsmeAdaptionLayer.sendMessage(dsmemsg);
		}
}

std::string DSMEPlatform::printDSMEManagement(uint8_t management, DSMESABSpecification& subBlock, CommandFrameIdentifier cmd) {
#if 0
		std::stringstream ss;
    uint8_t numChannels = this->dsmeAdaptionLayer.getMAC_PIB().helper.getNumChannels();
    uint8_t numGTSlots = this->dsmeAdaptionLayer.getMAC_PIB().helper.getNumGTSlots();
    uint8_t numSuperFramesPerMultiSuperframe = this->dsmeAdaptionLayer.getMAC_PIB().helper.getNumberSuperframesPerMultiSuperframe();

    ss << " ";
    uint8_t type = management & 0x7;
    switch ((ManagementType) type) {
    		case DEALLOCATION:
						ss << "DEALLOCATION";
						break;
				case ALLOCATION:
						ss << "ALLOCATION";
						break;
				case DUPLICATED_ALLOCATION_NOTIFICATION:
						ss << "DUPLICATED-ALLOCATION-NOTIFICATION";
						break;
				case REDUCE:
						ss << "REDUCE";
						break;
				case RESTART:
						ss << "RESTART";
						break;
				case EXPIRATION:
						ss << "EXPIRATION";
						break;
				default:
						ss << (uint16_t)management;
    }

    if(subBlock.getSubBlock().count(true) == 1) {
        for (DSMESABSpecification::SABSubBlock::iterator it = subBlock.getSubBlock().beginSetBits(); it != subBlock.getSubBlock().endSetBits();
                it++) {
            GTS gts = GTS::GTSfromAbsoluteIndex((*it) + subBlock.getSubBlockIndex() * numGTSlots * numChannels, numGTSlots, numChannels,
                    numSuperFramesPerMultiSuperframe);

            ss << " " << gts.slotID << " " << gts.superframeID << " " << (uint16_t)gts.channel;
        }
    }

    return ss.str();
#endif
    return "";
}

//TODO
void DSMEPlatform::printSequenceChartInfo(DSMEMessage* msg, bool outgoing) {
#if 0
		DSME_ASSERT(msg != nullptr);
    IEEE802154eMACHeader &header = msg->getHeader();

    std::stringstream ss;

    if(outgoing) {
				ss << (uint16_t)header.getDestAddr().getShortAddress() << "|";
		} else {
				ss << (uint16_t)header.getSrcAddr().getShortAddress() << "|";
    }

    ss << (uint16_t)header.hasSequenceNumber() << "|";

    ss << (uint16_t)header.getSequenceNumber() << "|";

    switch (header.getFrameType()) {
				case IEEE802154eMACHeader::BEACON:
						ss << "BEACON";
						break;
				case IEEE802154eMACHeader::DATA:
						ss << "DATA";
						break;
				case IEEE802154eMACHeader::ACKNOWLEDGEMENT:
						ss << "ACK";
						break;
				case IEEE802154eMACHeader::COMMAND: {
						uint8_t cmd = msg->getPayload()[0];

						switch((CommandFrameIdentifier)cmd) {
								case ASSOCIATION_REQUEST:
										ss << "ASSOCIATION-REQUEST";
										break;
								case ASSOCIATION_RESPONSE:
										ss << "ASSOCIATION-RESPONSE";
										break;
								case DISASSOCIATION_NOTIFICATION:
										ss << "DISASSOCIATION-NOTIFICATION";
										break;
								case DATA_REQUEST:
										ss << "DATA-REQUEST";
										break;
								case BEACON_REQUEST:
										ss << "BEACON-REQUEST";
										break;
								case DSME_ASSOCIATION_REQUEST:
										ss << "DSME-ASSOCIATION-REQUEST";
										break;
								case DSME_ASSOCIATION_RESPONSE:
										ss << "DSME-ASSOCIATION-RESPONSE";
										break;
								case DSME_BEACON_ALLOCATION_NOTIFICATION:
										ss << "DSME-BEACON-ALLOCATION-NOTIFICATION";
										break;
								case DSME_BEACON_COLLISION_NOTIFICATION:
										ss << "DSME-BEACON-COLLISION-NOTIFICATION";
										break;
								case DSME_GTS_REQUEST:
								case DSME_GTS_REPLY:
								case DSME_GTS_NOTIFY: {
										const uint8_t *cbuffer = DSMEPlatform::bufferDown;
										uint8_t *buffer = DSMEPlatform::bufferDown;
										DSMEMessage* m = this->getEmptyMessage();
										m->makeCopyFrom(msg, cbuffer, buffer);

										m->getHeader().decapsulateFrom(m);

										MACCommand cmdd;
										cmdd.decapsulateFrom(m);
										GTSManagement man;
										man.decapsulateFrom(m);

										switch(cmdd.getCmdId()) {
												case DSME_GTS_REQUEST: {
														ss << "DSME-GTS-REQUEST";
														GTSRequestCmd req;
														req.decapsulateFrom(m);
														ss << printDSMEManagement(msg->getPayload()[1], req.getSABSpec(), cmdd.getCmdId());
														break;
												}
												case DSME_GTS_REPLY: {
														ss << "DSME-GTS-REPLY";
														GTSReplyNotifyCmd reply;
														reply.decapsulateFrom(m);
														ss << printDSMEManagement(msg->getPayload()[1], reply.getSABSpec(), cmdd.getCmdId());
														break;
												}
												case DSME_GTS_NOTIFY: {
														ss << "DSME-GTS-NOTIFY";
														GTSReplyNotifyCmd notify;
														notify.decapsulateFrom(m);
														ss << printDSMEManagement(msg->getPayload()[1], notify.getSABSpec(), cmdd.getCmdId());
														break;
												}
												default:
														break;
										}
										this->releaseMessage(m);
										break;
								}
								default:
										break;
						}
						break;
				}
				default:
						ss << "UNKNOWN";
						break;
    }
    ss << "|" << msg->getTotalSymbols();
    DSME_LOG(ss.str());
    return;
#endif
}

// TODO: make sure ALL callers check for nullptr (BeaconManager / GTSManager)
DSMEMessage* DSMEPlatform::getEmptyMessage()
{
    messagesInUse++;
    DSME_ASSERT(messagesInUse <= MSG_POOL_SIZE); // TODO should return nullptr (and check everywhere!!)

    DSMEMessage* msg = messageBuffer.aquire();
    DSME_ASSERT(msg != nullptr);

    msg->receivedViaMCPS = false;
    signalNewMsg(msg);

    DSME_ASSERT(msg->getPayloadLength() == 0);
    return msg;
}


void DSMEPlatform::releaseMessage(IDSMEMessage* msg) {
		this->releaseMessage(static_cast<DSMEMessage*>(msg));
}

void DSMEPlatform::releaseMessage(DSMEMessage* msg) {
    DSME_ASSERT(msg != nullptr);
    DSME_ASSERT(messagesInUse > 0);
    messagesInUse--;
    signalReleasedMsg(msg);
    messageBuffer.release(msg);
}

void DSMEPlatform::invokeMessageTask() {
		handleMessageTask.invoke();
}

/*
  Get current value of the timer in symbols.
*/
uint32_t DSMEPlatform::getSymbolCounter() {
#if CONTIKI_TARGET_COOJA || CONTIKI_TARGET_COOJA_IP64
    return RTIMERTICKS_TO_SYMBOLS(RTIMER_NOW());
#else
    return MacSymbolCounter::getInstance().getValue();
#endif
}

/*
  Set the Compare value for symbol counter, enable interrupt and enable timer.
*/
void DSMEPlatform::startTimer(uint32_t symbolCounterValue) {
#if CONTIKI_TARGET_COOJA || CONTIKI_TARGET_COOJA_IP64
    rtimer_clock_t time = (rtimer_clock_t) SYMBOLS_TO_RTIMERTICKS(symbolCounterValue) + 1; // +1 to handle rounding effects
    rtimer_set(&rt, time, 1, callInterrupt, NULL);
    rtimer_arch_schedule(time);
#else
    MacSymbolCounter::getInstance().setCompareMatch(symbolCounterValue);
#endif
}

bool DSMEPlatform::setChannelNumber(uint8_t channel) {
    this->channel = channel;
    radio_result_t error = RADIO_RESULT_OK;
    error = NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channel);
    DSME_ASSERT(error == RADIO_RESULT_OK);
    bool success = (error == RADIO_RESULT_OK);
    return success;
}

// Test whether CCA can be performed, if so start ctimer and return whether cca could be performed
bool DSMEPlatform::startCCA() {
		bool radio_idle = false;
#if CONTIKI_TARGET_COOJA || CONTIKI_TARGET_COOJA_IP64
		radio_idle = true;
#else
		//TODO: platform specific test whether radio can perform CCA
		radio_idle = true;
#endif
		ctimer_set(&CCACallbackTimer, (clock_time_t) 0, ccaResult_ready, NULL);

		return radio_idle;

}

/**
 * Called from openDSME. Serializes the DSMEMessage and calls NETSTACK_RADIO to transmit the data. Sets up immediate
 * callback for send_done event.
 */
bool DSMEPlatform::prepareSendingCopy(IDSMEMessage* msg, Delegate<void(bool)> txEndCallback) {
		return this->prepareSendingCopy(static_cast<DSMEMessage*>(msg), txEndCallback);
}

bool DSMEPlatform::prepareSendingCopy(DSMEMessage* msg, Delegate<void(bool)> txEndCallback) {
		printSequenceChartInfo(msg, true);
		tx_Success = false;
//		static uint8_t mac_tx_status;
    if (msg == nullptr) {
        return tx_Success;
    }

    dsme_atomicBegin();
    {
        if (DSMEPlatform::state != STATE_READY) {
            dsme_atomicEnd();
            return tx_Success;
        }
        DSMEPlatform::state = STATE_SEND;
        DSMEPlatform::txEndCallback = txEndCallback;
    }
    dsme_atomicEnd();

    currentTXLength = msg->getHeader().getSerializationLength() + msg->getPayloadLength();

    /* maximum DSME payload size (IEEE 802.15.4e) - FCS length */
    DSME_ASSERT(currentTXLength < DSME_PACKET_MAX_LEN);

    uint8_t *buffer = DSMEPlatform::bufferUp;

    /* serialize header */
    msg->getHeader().serializeTo(buffer);

    /* copy data */
    memcpy(buffer, msg->getPayload(), msg->getPayloadLength());
    tx_Success = (NETSTACK_RADIO.prepare(DSMEPlatform::bufferUp, currentTXLength) == 0);
    return tx_Success;
}

bool DSMEPlatform::sendNow() {
  	//DSME_PRINTF("DSMEPlatform: sending data %i\n",currentTXLength);// now to " << msg->getHeader().getDestAddr().getShortAddress() << " with seq: " << (uint32_t) msg->getHeader().getSequenceNumber() << "!");
		uint8_t status = NETSTACK_RADIO.transmit(currentTXLength);
		tx_Success = (status == RADIO_TX_OK);
		if(!tx_Success) {
			DSME_LOG("DSMEPlatform: tx status is " << (uint32_t) status);
		}
		ctimer_set(&sendCallbackTimer, (clock_time_t) 0, sendDone, NULL);
		return tx_Success;
}

void DSMEPlatform::turnTransceiverOn() {
		NETSTACK_RADIO.on();
}

void DSMEPlatform::turnTransceiverOff() {
		NETSTACK_RADIO.off();
}

void DSMEPlatform::abortPreparedTransmission() {
		DSME_PRINTF("DSMEPlatform: aborted transmission\n");
}

bool DSMEPlatform::sendDelayedAck(IDSMEMessage *ackMsg, IDSMEMessage *receivedMsg, Delegate<void(bool)> txEndCallback) {
		return this->sendDelayedAck(static_cast<DSMEMessage*>(ackMsg), static_cast<DSMEMessage*>(receivedMsg), txEndCallback);
}

bool DSMEPlatform::sendDelayedAck(DSMEMessage *ackMsg, DSMEMessage *receivedMsg, Delegate<void(bool)> txEndCallback) {
    (void) receivedMsg;
    // the time is utilized anyway, so do not delay
    bool result = prepareSendingCopy(ackMsg, txEndCallback);
    if(result) {
    		return sendNow();
    } else {
    		return false;
    }
}

/**
 * This method deserializes a received data package into a DSMEMessage and passes the DSMEMessage to openDSME AckLayer.
 */
void DSMEPlatform::requestPending() {
    if(!initialized) {
        return;
    }
    ///// Get last packet timestamp
#if CONTIKI_TARGET_COOJA || CONTIKI_TARGET_COOJA_IP64
    // RADIO_PARAM_LAST_PACKET_TIMESTAMP is of type rtimer_clock_t and gets
    // converted into Symbols (1 Symbol == 16 us)
    rtimer_clock_t t0;
    radio_result_t test = NETSTACK_RADIO.get_object(RADIO_PARAM_LAST_PACKET_TIMESTAMP, &t0, sizeof(rtimer_clock_t));
    DSME_ASSERT(test == RADIO_RESULT_OK);
     uint32_t sfdTimestamp = RTIMERTICKS_TO_SYMBOLS(t0)+10;
#else
    uint32_t sfdTimestamp = MacSymbolCounter::getInstance().getCapture();
    if(sfdTimestamp == MacSymbolCounter::INVALID_CAPTURE) {
        return;
    }
    sfdTimestamp -= 2; // -2 since the AT86RF231 captures at the end of the PHR (+6us) instead at the end of the SFD as the ATmega256RFR2
#endif

    dsme::DSMEMessage *msg = getEmptyMessage();
    if (msg == nullptr) {
        /* '-> No space for a message could be allocated. */
        return;
    }

    msg->startOfFrameDelimiterSymbolCounter = sfdTimestamp;

    NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_RSSI, &msg->radio_last_rssi);
    NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &msg->channelSent);
    NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_LINK_QUALITY, &msg->messageLQI);

    const uint8_t *buffer = DSMEPlatform::bufferDown;
    packetbuf_copyto((void *)buffer);
    uint16_t len = packetbuf_totlen();


    /* deserialize header */
    bool success = msg->getHeader().deserializeFrom(buffer, len);

		if(!success) {
			 releaseMessage(msg);
			 return;
		}
		//DSME_LOG("DSMEPlatform-reuestPending: src_id: "<< HEXOUT  << msg->getHeader().getSrcAddr().getShortAddress() << DECOUT << " dst_id:"<< HEXOUT << msg->getHeader().getDestAddr().getShortAddress() << DECOUT << " seqid:" << (uint32_t)  msg->getHeader().getSequenceNumber() << "!");
    /* copy data */
    msg->setPayloadLength(len - msg->getHeader().getSerializationLength());

    memcpy(msg->getPayload(), buffer, msg->getPayloadLength());

    dsme.getAckLayer().receive(msg);
}

/**
 * Translates an IEEE802154MacAddress into contiki linkaddr_t
 */
void DSMEPlatform::convertDSMEMacAddressToLinkaddr(IEEE802154MacAddress *from, linkaddr_t *to) {
		to->u8[0] = (uint8_t) (from->a1() >> 8);
		to->u8[1] = (uint8_t) from->a1();
		to->u8[2] = (uint8_t) (from->a2() >> 8);
		to->u8[3] = (uint8_t) from->a2();
		to->u8[4] = (uint8_t) (from->a3() >> 8);
		to->u8[5] = (uint8_t) from->a3();
		to->u8[6] = (uint8_t) (from->a4() >> 8);
		to->u8[7] = (uint8_t) from->a4();
}

static void handlingMessage(void *ptr) {
	dsme::DSMEPlatform::instance->invokeMessageTask();
}

static void sendDone(void *ptr) {
		DSME_ASSERT(dsme::DSMEPlatform::state == dsme::DSMEPlatform::STATE_SEND);
		dsme::DSMEPlatform::txEndCallback(tx_Success);
		dsme::DSMEPlatform::state = dsme::DSMEPlatform::STATE_READY;
}

static void ccaResult_ready(void *ptr) {
		bool clear = false;
		clear = (bool) NETSTACK_RADIO.channel_clear();

    dsme::DSMEPlatform::instance->getDSME().dispatchCCAResult(clear);
    return;
}

}

extern "C" {
    void foo() {
    }
}
