#ifndef __DSME_H__
#define __DSME_H__

#include "contiki.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mac_driver dsmemac_driver;

uint32_t dsmeAssociated();

#ifdef __cplusplus
}
#endif
  
#endif /* __DSMEMAC_H__ */
