/*
 * openDSME
 *
 * Implementation of the Deterministic & Synchronous Multi-channel Extension (DSME)
 * described in the IEEE 802.15.4-2015 standard
 *
 * Authors: Florian Meier <florian.meier@tuhh.de>
 *          Maximilian Koestler <maximilian.koestler@tuhh.de>
 *          Sandrina Backhauss <sandrina.backhauss@tuhh.de>
 *
 * Based on
 *          DSME Implementation for the INET Framework
 *          Tobias Luebkert <tobias.luebkert@tuhh.de>
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef DSMEMESSAGE_H
#define DSMEMESSAGE_H

#include <stdint.h>

#include "dsme_settings.h"

extern "C" {
#include "net/mac/mac.h"
#include "dev/radio.h"
}

#include "openDSME/mac_services/dataStructures/DSMEMessageElement.h"
#include "openDSME/dsmeLayer/messages/IEEE802154eMACHeader.h"
#include "openDSME/interfaces/IDSMEMessage.h"

namespace dsme {

class DSMEPlatform;

class DSMEMessage : public IDSMEMessage {
public:
//////////////////////////////////////////////////////////////////////////////////
		/* IDSMEMessage */
    void prependFrom(DSMEMessageElement* msg);

    void decapsulateTo(DSMEMessageElement* msg);

    void copyTo(DSMEMessageElement* msg);

    uint8_t getByte(uint8_t pos) {
        return payload[pos];
    }

    bool hasPayload() {
        return (payloadLength > 0);
    }

    // gives the symbol counter at the end of the SFD
    uint32_t getStartOfFrameDelimiterSymbolCounter() override {
        return startOfFrameDelimiterSymbolCounter;
    }

    void setStartOfFrameDelimiterSymbolCounter(uint32_t symbolCounter) override {
        startOfFrameDelimiterSymbolCounter = symbolCounter;
    }

    uint16_t getTotalSymbols() {
				uint16_t bytes = macHdr.getSerializationLength()
																	 + payloadLength
																	 + 2 // FCS
																	 + 4 // Preamble
																	 + 1 // SFD
																	 + 1; // PHY Header
				return bytes*2; // 4 bit per symbol
		}

    IEEE802154eMACHeader& getHeader() {
        return macHdr;
    }

    //TODO
    uint8_t getLQI() override {
    		return messageLQI;
    }

		bool getReceivedViaMCPS() override {
				return receivedViaMCPS;
		}

		void setReceivedViaMCPS(bool receivedViaMCPS) override {
				this->receivedViaMCPS = receivedViaMCPS;
		}

		bool getCurrentlySending() override {
				return currentlySending;
		}

		void setCurrentlySending(bool currentlySending) override {
				this->currentlySending = currentlySending;
		}

    void increaseRetryCounter() override {
    		this->retryCounter++;
    }

    uint8_t getRetryCounter() override {
    		return this->retryCounter;
    }

//////////////////////////////////////////////////////////////////////////////////
    void clearMessage() {
    		this->setPayloadLength(0);
    }

    uint8_t getPayloadLength() {
				return payloadLength;
		}

    radio_value_t getRSSI() {
    		return this->radio_last_rssi;
    }

    radio_value_t getChannelSent() {
    		return this->channelSent;
    }

		mac_callback_t getMacCallbackFunction() const {
				return macCallbackFunction;
		}

		void setMacCallbackFunction(mac_callback_t macCallbackFunction) {
				this->macCallbackFunction = macCallbackFunction;
		}

		void* getMacCallbackPointer() const {
				return macCallbackPointer;
		}

		void setMacCallbackPointer(void* macCallbackPointer) {
				this->macCallbackPointer = macCallbackPointer;
		}

		void makeCopyFrom(DSMEMessage* msg, const uint8_t * cbuf, uint8_t * buf) {
				this->firstTry = msg->firstTry;
				this->payloadLength = msg->getPayloadLength();
				this->radio_last_rssi = msg->getRSSI();
				this->channelSent = msg->getChannelSent();
				this->messageLQI = msg->getLQI();
				this->receivedViaMCPS = msg->getReceivedViaMCPS();
				this->currentlySending = msg->getCurrentlySending();
				this->retryCounter = msg->getRetryCounter();
				this->startOfFrameDelimiterSymbolCounter = msg->getStartOfFrameDelimiterSymbolCounter();
				memcpy(this->getPayload(), msg->getPayload(), msg->getPayloadLength());

				msg->getHeader().serializeTo(buf);
				//const uint8_t * buf = buffer;
				this->getHeader().deserializeFrom(cbuf, 2);
		}

    bool firstTry;

private:
    DSMEMessage() :
        		payloadLength(0),
						radio_last_rssi(0),
						channelSent(0),
						messageLQI(0),
    				receivedViaMCPS(false),
    				currentlySending(false),
						retryCounter(0)
        {
        }

		~DSMEMessage() {
		}

    void prepare() {
        currentlySending = false;
        firstTry = true;
        receivedViaMCPS = false;

        macHdr.reset();
    }

    uint8_t* getPayload() {
        return payload;
    }
  
    void setPayloadLength(uint8_t length) {
        payloadLength = length;
    }
  
    IEEE802154eMACHeader macHdr;
    uint8_t payload[DSME_PACKET_MAX_LEN];
    uint8_t payloadLength;
    radio_value_t radio_last_rssi;
    radio_value_t channelSent;
    radio_value_t messageLQI;

    mac_callback_t macCallbackFunction; /* callback for this packet */
		void *macCallbackPointer; /* MAC callback parameter */

    uint32_t startOfFrameDelimiterSymbolCounter;
    bool receivedViaMCPS; // TODO better handling?
    bool currentlySending;
    uint8_t retryCounter;

    friend class DSMEPlatform;
    friend class DSMEMessageElement;
    friend class DSMEPlatformBase;
    friend class DSMEMessageBuffer;
};

}

#endif
