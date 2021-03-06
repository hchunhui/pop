#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "xlog/xlog.h"
#include "entity.h"
#include "xswitch/xswitch.h"

#include "core/core.h"

struct entity
{
	enum entity_type type;
	union {
		struct xswitch *xs;
		struct host_info addr;
	} u;

	// int num_ports;
	int num_adjs;
	struct entity_adj adjs[MAX_PORT_NUM];
};

static bool host_p(void *phost, const char *name, const void *arg)
{
	if(strcmp(name, "topo_host") == 0 &&
	   (arg == phost || arg == NULL))
	   return true;
	return false;
}


static bool entity_adjs_p(void *adjs, const char *name, const void *arg)
{
	if(strncmp(name, "entity_adjs", 11) == 0 &&
	   arg == adjs)
		return true;
	return false;
}

void entity_print(struct entity *e)
{
	int i;
	if (e->type == ENTITY_TYPE_HOST) {
		xinfo("HOST:\nEth Addr: ");
		for (i=0; i<6; i++)
			xinfo("%02x ",e->u.addr.haddr.octet[i]);
		xinfo("\nIPv4: %08x\n", e->u.addr.paddr);
		xinfo("num_adjs: %d\n", e->num_adjs);
		for (i=0; i<e->num_adjs; i++) {
			xinfo("  %3d: %d, %d, %d\n", i,
			      e->adjs[i].out_port, e->adjs[i].adj_in_port,
			      entity_get_dpid(e->adjs[i].adj_entity));
		}
	} else if (e->type == ENTITY_TYPE_SWITCH) {
		xinfo("SWITCH:\nDpid: %d\n", entity_get_dpid(e));
		xinfo("num_adjs: %d\n", e->num_adjs);
		for (i=0; i<e->num_adjs; i++) {
			xinfo("  %3d: %d, %d, %d\n", i,
			      e->adjs[i].out_port, e->adjs[i].adj_in_port,
			      (e->adjs[i].adj_entity)->type);
		}
	}
}

struct entity *entity_host(struct host_info addr)
{
	struct entity *e = malloc(sizeof(struct entity));
	e->type = ENTITY_TYPE_HOST;
	e->u.addr = addr;
	e->num_adjs = 0;
	return e;
}

struct entity *entity_switch(struct xswitch *xs)
{
	struct entity *e = malloc(sizeof(struct entity));
	e->type = ENTITY_TYPE_SWITCH;
	e->u.xs = xs;
	e->num_adjs = 0;
	return e;
}

void entity_free(struct entity *e)
{
	int i, j;
	for(i = 0; i < e->num_adjs; i++) {
		struct entity *peer = e->adjs[i].adj_entity;
		int port = e->adjs[i].adj_in_port;
		for(j = 0; j < peer->num_adjs; j++) {
			if(peer->adjs[j].out_port == port &&
			   peer->adjs[j].adj_entity == e) {
				peer->num_adjs--;
				peer->adjs[j] = peer->adjs[peer->num_adjs];
				break;
			}
		}
	}
	free(e);
}

enum entity_type entity_get_type(struct entity *e)
{
	return e->type;
}

struct xswitch *entity_get_xswitch(struct entity *e)
{
	assert(e->type == ENTITY_TYPE_SWITCH);
	return e->u.xs;
}

dpid_t entity_get_dpid(struct entity *e)
{
	if(e) {
		assert(e->type == ENTITY_TYPE_SWITCH);
		return xswitch_get_dpid(e->u.xs);
	} else {
		return 0;
	}
}

struct host_info entity_get_addr(struct entity *e)
{
	assert(e->type == ENTITY_TYPE_HOST);
	return e->u.addr;
}

void entity_set_paddr(struct entity *e, uint32_t paddr)
{
	assert(e->type == ENTITY_TYPE_HOST);
	e->u.addr.paddr = paddr;
	core_invalidate(host_p, e);
}

struct entity *entity_host_get_adj_switch(struct entity *e, int *sw_port)
{
        if (e->type != ENTITY_TYPE_HOST)
                return NULL;
        *sw_port = e->adjs[0].adj_in_port;
        return e->adjs[0].adj_entity;
}

const struct entity_adj *entity_get_adjs(struct entity *e, int *pnum)
{
	*pnum = e->num_adjs;
	return e->adjs;
}

void entity_add_link(struct entity *e1, int port1, struct entity *e2, int port2)
{
	int k, i, j;
	for (k = 0; k < e1->num_adjs; k++) {
		if (e1->adjs[k].out_port == port1 
		    && e1->adjs[k].adj_in_port == port2
		    && e1->adjs[k].adj_entity == e2)
			return;
	}
	i = e1->num_adjs;
	j = e2->num_adjs;
	assert(i < MAX_PORT_NUM);
	assert(j < MAX_PORT_NUM);
	e1->adjs[i].out_port = port1;
	e1->adjs[i].adj_in_port = port2;
	e1->adjs[i].adj_entity = e2;
	e2->adjs[j].out_port = port2;
	e2->adjs[j].adj_in_port = port1;
	e2->adjs[j].adj_entity = e1;
	e1->num_adjs++;
	e2->num_adjs++;
#ifdef STRICT_INVALIDATE
	core_invalidate(entity_adjs_p, e1->adjs);
	core_invalidate(entity_adjs_p, e2->adjs);
#else
	if(e1->type != ENTITY_TYPE_HOST && e2->type != ENTITY_TYPE_HOST) {
		core_invalidate(entity_adjs_p, e1->adjs);
		core_invalidate(entity_adjs_p, e2->adjs);
	}
#endif
}

void entity_del_link(struct entity *e1, int port1)
{
	int i, j;
	bool flag = e1->type == ENTITY_TYPE_SWITCH ? true : false;
	for (i = 0; i < e1->num_adjs;) {
		if (e1->adjs[i].out_port == port1) {
			struct entity *e2 = e1->adjs[i].adj_entity;
			int port2 = e1->adjs[i].adj_in_port;
			if(e2->type != ENTITY_TYPE_HOST)
				flag = false;
			for(j = 0; j < e2->num_adjs;) {
				if(e2->adjs[j].out_port == port2) {
					e2->num_adjs--;
					e2->adjs[j] = e2->adjs[e2->num_adjs];
				} else {
					j++;
				}
			}
			core_invalidate(entity_adjs_p, e2->adjs);
			e1->num_adjs--;
			e1->adjs[i] = e1->adjs[e1->num_adjs];
		} else {
			i++;
		}
	}
#ifdef STRICT_INVALIDATE
	core_invalidate(entity_adjs_p, e1->adjs);
#else
	if(flag == false)
		core_invalidate(entity_adjs_p, e1->adjs);
#endif
}
