#ifndef HANDLEMESSAGETASK_H
#define HANDLEMESSAGETASK_H

#include "dsme_atomic.h"
#include "openDSME/helper/DSMEDelegate.h"
#include "DSMEMessage.h"
#include "openDSME/interfaces/IDSMEPlatform.h"

extern "C" {
#include "pt.h"
#include "pt-sem.h"
}

namespace dsme {

typedef IDSMEPlatform::receive_delegate_t receive_delegate_t;

class HandleMessageTask {
public:
		HandleMessageTask() :
					message(nullptr), occupied(false) {
		}

    bool occupy(DSMEMessage* message, receive_delegate_t receiveDelegate) {
        dsme_atomicBegin();
        if(this->occupied) {
            dsme_atomicEnd();
            return false;
        }
        else {
            this->occupied = true;
            this->message = message;
            this->receiveDelegate = receiveDelegate;
            dsme_atomicEnd();
            return true;
        }
    }

    bool isOccupied() {
        return occupied;
    }

    virtual void invoke() {
        DSMEMessage* copy = nullptr;
        dsme_atomicBegin();
        {
            copy = message;
            if(!occupied) {
                dsme_atomicEnd();
                return;
            }
        }
        dsme_atomicEnd();

        receiveDelegate(copy);

        dsme_atomicBegin();
        {
            message = nullptr;
            occupied = false;
        }
        dsme_atomicEnd();
    }
private:
    DSMEMessage* message;
    receive_delegate_t receiveDelegate;
    bool occupied;
//    static struct pt_sem mutex;
};
}

#endif
