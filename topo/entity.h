#ifndef _ENTITY_H_
#define _ENTITY_H_

#include "types.h"

struct xswitch;
struct entity;
enum entity_type { ENTITY_TYPE_HOST, ENTITY_TYPE_SWITCH };

#define MAX_PORT_NUM 16
struct entity_adj
{
	int out_port;
	int adj_in_port;
	struct entity *adj_entity;
};

struct host_info
{
	haddr_t haddr;
	uint32_t paddr;
};

struct entity *entity_host(struct host_info addr);
struct entity *entity_switch(struct xswitch *xs);
void entity_free(struct entity *e);
struct host_info entity_get_addr(struct entity *e);
void entity_set_paddr(struct entity *e, uint32_t paddr);

void entity_add_link(struct entity *e1, int port1, struct entity *e2, int port2);
void entity_del_link(struct entity *e1, int port1);

enum entity_type entity_get_type(struct entity *e);
dpid_t entity_get_dpid(struct entity *e);
struct entity *entity_host_get_adj_switch(struct entity *e, int *sw_port);
const struct entity_adj *entity_get_adjs(struct entity *e, int *pnum);
struct xswitch *entity_get_xswitch(struct entity *e);
void entity_print(struct entity *e);

#endif /* _ENTITY_H_ */
