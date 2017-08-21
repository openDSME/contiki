#ifndef __DSME_PLATFORM_H__
#define __DSME_PLATFORM_H__

extern "C" {
#include "lib/assert.h"
//#include "sys/node-id.h"
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

#if 1
#define DSME_LOG(x) {std::stringstream s; s << x << LOG_ENDL; DSME_PRINTF(s.str().c_str());}
#define DSME_CONSOLE(x) DSME_LOG(x) //{std::stringstream s; s << "Node " << palId_id() << ": " << x << LOG_ENDL; std::cout << s.str().c_str() << LOG_ENDL << std::flush;}
#define DSME_LOG_ASSERT(x) DSME_LOG(x) //{std::stringstream s; s << "Node " << palId_id() << ": " << x << LOG_ENDL; std::cout << s.str().c_str() << LOG_ENDL << std::flush;}

//#define LOG_INFO(x) DSME_LOG(x)
#define LOG_INFO(x)
#define LOG_INFO_PURE(x)// {std::stringstream s; s << x; DSME_PRINTF(s.str().c_str());}
#define LOG_INFO_PREFIX
#define LOG_ERROR(x) {DSME_CONSOLE(x);DSME_LOG(x);}
#define HEXOUT std::hex
#define DECOUT std::dec
#define LOG_ENDL std::endl

#else
#define LOG_INFO(x)
#define DSME_CONSOLE(x)
#define DSME_LOG_ASSERT(x)
#define DSME_LOG(x)
#define LOG_ERROR(x)
#define LOG_INFO_PURE(x)
#define LOG_INFO_PREFIX
#define DECOUT
#define HEXOUT
#endif

#define LOG_WARN(x) LOG_INFO(x)
//#define LOG_DEBUG(x) LOG_INFO(x)
#define LOG_DEBUG(x)
#define LOG_DEBUG_PURE(x) LOG_INFO_PURE(x)
#define LOG_DEBUG_PREFIX LOG_INFO_PREFIX

#define DSME_ASSERT(x) if(!(x)) {	DSME_LOG_ASSERT("ASSERT"); assert(x); while(1){}	}
#define DSME_SIM_ASSERT(x) if(!(x)) {	DSME_LOG_ASSERT("SIM ASSERT"); assert(x);	}

#include "DSMEMessage.h"
#include "dsme_settings.h"
#include "dsme_atomic.h"


#endif /* __DSME_PLATFORM_H__ */
