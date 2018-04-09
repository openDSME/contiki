#ifndef __DSME_PLATFORM_H__
#define __DSME_PLATFORM_H__

extern "C" {
#include "lib/assert.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/mac/mac.h"
#include "../../../../openlab/drivers/unique_id.h"
}

#include <sstream>
#include <iostream>

#if CONTIKI_TARGET_IOTLAB_M3
#define palId_id() platform_uid()
#else
#define palId_id() node_id
#endif

#define DSME_LOG(x) {std::stringstream s; s << x << LOG_ENDL; DSME_PRINTF(s.str().c_str());}
#define DSME_LOG_PURE(x) {std::stringstream s; s << x; DSME_PRINTF(s.str().c_str());}

#if defined(DSME_ADAPTION_LAYER) || defined(DSME_ALLOCATION_COUNTER_TABLE) || defined(DSME_BEACON_MANAGER) || defined(DSME_ASSOCIATION_MANAGER)
#define LOG_INFO(x) DSME_LOG(x)
#define LOG_INFO_PURE(x) DSME_LOG_PURE(x)
#else
#define LOG_INFO(x)
#define LOG_INFO_PURE(x)
#endif
#define LOG_INFO_PREFIX

#define LOG_ERROR(x) DSME_LOG(x)
#define HEXOUT std::hex
#define DECOUT std::dec
#define LOG_ENDL std::endl

#define LOG_DEBUG(x)
#define LOG_DEBUG_PURE(x)
#define LOG_DEBUG_PREFIX

#define DSME_ASSERT(x) if(!(x)) { platform_enter_critical(); DSME_LOG("ASSERT"); assert(x); while(1){asm volatile("nop");}	}
//#define DSME_SIM_ASSERT(x) if(!(x)) { DSME_LOG("SIM ASSERT" << __FILE__ << ":" << __LINE__); }
#define DSME_SIM_ASSERT(x) if(!(x)) { DSME_LOG("SIM ASSERT"); }

#define POS(x) ((x) < 0?-(x):(x))
#define FLOAT_OUTPUT(x) ((int16_t)x) << '.' << (uint16_t)POS((x-(int16_t)x)*10)

#include "DSMEMessage.h"
#include "dsme_settings.h"
#include "dsme_atomic.h"


#endif /* __DSME_PLATFORM_H__ */
