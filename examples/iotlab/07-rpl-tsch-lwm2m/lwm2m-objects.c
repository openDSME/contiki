#include "contiki.h"
#include "lib/random.h"
#include <stdlib.h>
#include "lwm2m-object.h"
#include "lwm2m-engine.h"
#include "net/ip/uip-debug.h"
#define DEBUG DEBUG_PRINT

static float
read_temp()
{
  return ((float)rand()/(float)(RAND_MAX)) * 60.0;
}

/*---------------------------------------------------------------------------*/
static int
temp(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outsize)
{
  float value;
  value = read_temp();
  printf("Temperature sensor value: %f\n", value);
  printf("Callback temperature sensor\n");
  return ctx->writer->write_int(ctx, outbuf, outsize, (int32_t)value);
}

/*---------------------------------------------------------------------------*/
LWM2M_RESOURCES(temperature_resources,
                /* Temperature (Current) */
                LWM2M_RESOURCE_CALLBACK(5700, { temp, NULL, NULL }),
                /* Units */
                LWM2M_RESOURCE_STRING(5701, "Cel"),
                );
LWM2M_INSTANCES(temperature_instances,
                LWM2M_INSTANCE(0, temperature_resources));
LWM2M_OBJECT(temperature, 3303, temperature_instances);
/*---------------------------------------------------------------------------*/

void
object_temperature_handle(void)
{
  printf("Notify observer temperature sensor\n");
  lwm2m_object_notify_observers(&temperature, "/0/5700");
}

void
object_temperature_init(void)
{
  int32_t v;

  /* register this device and its handlers - the handlers automatically
     sends in the object to handle */
  lwm2m_engine_register_object(&temperature);
}
