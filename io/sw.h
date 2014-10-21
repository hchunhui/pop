#ifndef _MAPLE_IO_SW_H
#define _MAPLE_IO_SW_H

#include <stdlib.h>
#include <assert.h>

#include "msgqueue.h"

struct xswitch;

struct sw {
	struct msgqueue send_queue;
	int cpuid; /* The id of the cpu on which this
		      switch will be processed */
	struct xswitch *xsw;
};

static inline struct sw *
new_sw(int cpuid)
{
	struct sw *sw = malloc(sizeof(*sw));
	assert(sw != NULL);

	msgqueue_init(&sw->send_queue);
	sw->cpuid = cpuid;

	return (sw);
}

#endif
