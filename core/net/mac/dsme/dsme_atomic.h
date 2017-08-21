#ifndef __DSME_ATOMIC_H__
#define __DSME_ATOMIC_H__

#include "dsme_settings.h"

#if CONTIKI_TARGET_COOJA || CONTIKI_TARGET_COOJA_IP64
// recursive locks necessary!
inline void dsme_atomicBegin() {
//  DSME_PRINTF("dsme_atomicB\n");
//  atomic_begin_pt(&blocker);
}

inline void dsme_atomicEnd() {
//  DSME_PRINTF("dsme_atomicE\n");
//  atomic_end_pt(&blocker);
}
#elif MAKE_FOR_M3
// recursive locks necessary!
inline void dsme_atomicBegin() {
	__asm volatile ("cpsid i" : : : "memory");
}

inline void dsme_atomicEnd() {
	__asm volatile ("cpsie i" : : : "memory");
}
#endif

#endif /* __DSME_ATOMIC_H__ */
