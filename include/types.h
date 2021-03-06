#ifndef _TYPES_H_
#define _TYPES_H_

/* bool */
#include <stdbool.h>

/* intX_t, uintX_t */
#include <inttypes.h>

/* value_t */
#include "value.h"

typedef uint32_t dpid_t;
typedef uint16_t port_t;

struct map;
struct route;
struct packet;
struct entity;

#endif /* _TYPES_H_ */
