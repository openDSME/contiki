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

#include "DSMEMessage.h"
#include "dsme_platform.h"

namespace dsme {

void DSMEMessage::prependFrom(DSMEMessageElement* me) {
    // TODO better fill buffer from the end (but this is not how it works in CometOS)
		if(!(this->getPayloadLength()+me->getSerializationLength() <= DSME_PACKET_MAX_LEN)) {
			DSME_PRINTF("Payload length: %d,", this->getPayloadLength());
			DSME_PRINTF(" Serialization length: %d,", me->getSerializationLength());
			DSME_PRINTF(" MAX length: %d\n", DSME_PACKET_MAX_LEN);
			DSME_ASSERT(false);
		}
  
    memmove(this->getPayload()+me->getSerializationLength(), this->getPayload(), this->getPayloadLength());
    this->setPayloadLength(this->getPayloadLength()+me->getSerializationLength());
    Serializer s(this->getPayload(), SERIALIZATION);
    me->serialize(s);
    DSME_ASSERT(this->getPayload()+me->getSerializationLength() == s.getData());
}

void DSMEMessage::decapsulateTo(DSMEMessageElement* me) {
    me->copyFrom(this);
    this->setPayloadLength(this->getPayloadLength()-me->getSerializationLength());
    memmove(this->getPayload(), this->getPayload()+me->getSerializationLength(), this->getPayloadLength());
}

void DSMEMessage::copyTo(DSMEMessageElement* me) {
    Serializer s(this->getPayload(), DESERIALIZATION);
    me->serialize(s);
    DSME_ASSERT(this->getPayload()+me->getSerializationLength() == s.getData());
}

}
