/*
 * CometOS --- a component-based, extensible, tiny operating system
 *             for wireless networks
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.

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

#include "DSMEMessageBuffer.h"
#include "dsme_atomic.h"

//using namespace cometos;

namespace dsme {

DSMEMessageBuffer::DSMEMessageBuffer() :
        front(buffer), back(buffer + BUFFERSIZE - 1) {
    for (uint16_t i = 0; i < BUFFERSIZE - 1; i++) {
        buffer[i].next = &(buffer[i + 1]);
    }
    buffer[BUFFERSIZE - 1].next = nullptr;
}

DSMEMessageBuffer::~DSMEMessageBuffer() {
}

DSMEMessage* DSMEMessageBuffer::aquire() {
    DSMEMessageBufferElement *element = nullptr;

    if (front == nullptr) {
        return nullptr;
    }

    dsme_atomicBegin();
    {
        element = front;
        front = front->next;

        element->message.prepare();
    }
    dsme_atomicEnd();

    return &(element->message);
}

void DSMEMessageBuffer::release(DSMEMessage* message) {
    if (message == nullptr) {
        return;
    }
    message->clearMessage();
    DSMEMessageBufferElement *element = (DSMEMessageBufferElement*) message;

    dsme_atomicBegin();
    {
        back->next = element;
        back = element;

        if (front == nullptr) {
            front = element;
        }
    }
    dsme_atomicEnd();
    return;
}
}