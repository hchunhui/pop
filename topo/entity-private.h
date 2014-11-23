#ifndef _ENTITY_PRIVATE_H_
#define _ENTITY_PRIVATE_H_

#include "entity.h"

#define MAX_PORT_NUM 16

struct xswitch;

struct host_info
{
	haddr_t haddr;
	uint32_t paddr;
};

struct entity *entity_host(struct host_info addr);
struct entity *entity_switch(struct xswitch *xs);
void entity_free(struct entity *e);
struct xswitch *entity_get_xswitch(struct entity *e);
struct host_info entity_get_addr(struct entity *e);

void entity_add_link(struct entity *e1, int port1, struct entity *e2, int port2);
void entity_adj_down(struct entity *e, int port);

#endif /* _ENTITY_PRIVATE_H_ */