#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include "xlog/xlog.h"
#include "xswitch/xswitch-private.h"
#include "topo/topo.h"
#include "topo/entity.h"
#include "route.h"

#include "core.h"
#include "packet_parser.h"
#include "spec_parser.h"
#include "trace.h"
#include "trace_tree.h"
#include "map.h"

#include "pop_api.h"

static void *algo_handle;

static struct header *header_spec;

static struct map *env;

/* f */
static struct route *(*f)(struct packet *pkt, struct map *env);
static void *(*init_f)(struct map *env);

/* API for f */
struct packet {
	struct packet_parser *pp;
	struct xswitch *in_sw;
	int in_port;
	bool hack_get_payload;
};

int read_packet_inport(struct packet *pkt)
{
	return pkt->in_port;
}

struct entity *read_packet_inswitch(struct packet *pkt)
{
	return topo_get_switch(pkt->in_sw->dpid);
}

void pull_header(struct packet *pkt)
{
	value_t sel_value;
	struct header *old_spec, *new_spec;
	int next_stack_top;
	/* XXX: hack */
	packet_parser_pull(pkt->pp, &old_spec, &sel_value, &new_spec, &next_stack_top);
	trace_R(header_get_sel(old_spec), sel_value);
	trace_G(old_spec, new_spec, next_stack_top);
	xdebug("current header: %s\n", header_get_name(new_spec));
}

const char *read_header_type(struct packet *pkt)
{
	return packet_parser_read_type(pkt->pp);
}

value_t read_packet(struct packet *pkt, const char *field)
{
	value_t v = packet_parser_read(pkt->pp, field);
	trace_R(field, v);
	return v;
}

bool test_equal(struct packet *pkt, const char *field, value_t value)
{
	value_t v = packet_parser_read(pkt->pp, field);
	bool result = value_equal(v, value);
	trace_T(field, value, result);
	return result;
}

const uint8_t *read_payload(struct packet *pkt, int *length)
{
	pkt->hack_get_payload = true;
	return packet_parser_get_payload(pkt->pp, length);
}

void push_header(struct packet *pkt)
{
	struct header *new_spec;
	int prev_stack_top;
	packet_parser_push(pkt->pp, &new_spec, &prev_stack_top);
	trace_P(new_spec, prev_stack_top);
}

void mod_packet(struct packet *pkt, const char *field, value_t value)
{
	struct header *spec;
	packet_parser_mod(pkt->pp, field, value, &spec);
	trace_M(field, value, spec);
}

void add_header(struct packet *pkt, const char *proto)
{
	int hlen;
	struct header *h = header_lookup(header_spec, proto);
	packet_parser_add_header(pkt->pp, h, &hlen);
	trace_A(hlen);
}

void add_field(struct packet *pkt, int offb, int lenb, value_t value)
{
	packet_parser_add_field(pkt->pp, offb, lenb, value);
	trace_AF(offb, lenb, value);
}

void del_field(struct packet *pkt, int offb, int lenb)
{
	packet_parser_del_field(pkt->pp, offb, lenb);
	trace_DF(offb, lenb);
}

struct entity **get_hosts(int *pnum)
{
	struct entity **hosts = topo_get_hosts(pnum);
	trace_RE("topo_hosts", hosts);
	return hosts;
}

struct entity **get_switches(int *pnum)
{
	struct entity **switches = topo_get_switches(pnum);
	trace_RE("topo_switches", switches);
	return switches;
}

struct entity *get_switch(dpid_t dpid)
{
	struct entity *esw = topo_get_switch(dpid);
	trace_RE("topo_switch", esw);
	return esw;
}

struct entity *get_host_by_haddr(haddr_t addr)
{
	struct entity *eh = topo_get_host_by_haddr(addr);
	trace_RE("topo_host", eh);
	return eh;
}

struct entity *get_host_by_paddr(uint32_t addr)
{
	struct entity *eh = topo_get_host_by_paddr(addr);
	trace_RE("topo_host", eh);
	return eh;
}

enum entity_type get_entity_type(struct entity *e)
{
	return entity_get_type(e);
}

dpid_t get_switch_dpid(struct entity *e)
{
	return entity_get_dpid(e);
}

const struct entity_adj *get_entity_adjs(struct entity *e, int *pnum)
{
	const struct entity_adj *adjs = entity_get_adjs(e, pnum);
	char buf[32];
	snprintf(buf, 32, "entity_adjs%d", *pnum);
	trace_RE(buf, adjs);
	return adjs;
}

struct entity *get_host_adj_switch(struct entity *e, int *sw_port)
{
	assert(entity_get_type(e) == ENTITY_TYPE_HOST);
	int num;
	const struct entity_adj *adjs = get_entity_adjs(e, &num);
	assert(num == 1);
	*sw_port = adjs[0].adj_in_port;
	return adjs[0].adj_entity;
}

struct entity *get_entity(struct entity *e, int port)
{
	int i, n;
	const struct entity_adj *adjs = entity_get_adjs(e, &n);

	for(i = 0; i < n; i++)
		if(adjs[i].out_port == port) {
			if(entity_get_type(adjs[i].adj_entity) == ENTITY_TYPE_HOST)
				trace_RE("topo_host", adjs[i].adj_entity);
			else
				trace_RE("topo_switch", adjs[i].adj_entity);
			return adjs[i].adj_entity;
		}
	abort();
}

void print_entity(struct entity *e)
{
	return entity_print(e);
}

uint64_t get_port_recvpkts(struct entity *e, uint16_t port_id)
{
	return xport_get_recvpkts(xport_lookup(entity_get_xswitch(e), port_id));
}

uint64_t get_port_recvbytes(struct entity *e, uint16_t port_id)
{
	return xport_get_recvbytes(xport_lookup(entity_get_xswitch(e), port_id));
}

uint64_t get_port_recent_recvpkts(struct entity *e, uint16_t port_id)
{
	return xport_get_recent_recvpkts(xport_lookup(entity_get_xswitch(e), port_id));
}

uint64_t get_port_recent_recvbytes(struct entity *e, uint16_t port_id)
{
	return xport_get_recent_recvbytes(xport_lookup(entity_get_xswitch(e), port_id));
}

/* Return false means a wrong port_id or something else.*/
bool get_port_stats(struct entity *e, uint16_t port_id,
		    uint64_t *recvpkts/*OUT*/,
		    uint64_t *recvbytes/*OUT*/,
		    uint64_t *recent_recvpkts/*OUT*/,
		    uint64_t *recent_recvbytes/*OUT*/)
{
	struct xport *xp;
	if ((xp = xport_lookup(entity_get_xswitch(e), port_id)) == NULL)
		return false;
	xport_query(xp, recvpkts, recvbytes, recent_recvpkts, recent_recvbytes);
	return true;
}

/* XXX */
/* XXX: acquire topo_lock first! */
void core_invalidate(bool (*p)(void *p_data, const char *name, const void *arg), void *p_data)
{
	int num_switches;
	struct entity **switches;
	int i;

	switches = topo_get_switches(&num_switches);
	xdebug("core_invalidate\n");
	for(i = 0; i < num_switches; i++) {
		struct xswitch *cur_sw = entity_get_xswitch(switches[i]);
		struct trace_tree *tt = cur_sw->trace_tree;

		xswitch_table_lock(cur_sw);
		trace_tree_invalidate(&tt, cur_sw, cur_sw->table0, p, p_data);
#ifdef ENABLE_WEB
		trace_tree_print_json(cur_sw->trace_tree, cur_sw->dpid);
		trace_tree_print_ft_json(cur_sw->trace_tree, cur_sw->dpid);
#endif
		xswitch_table_unlock(cur_sw);
	}
}

/* call back funtions */
void core_init(const char *algo_file, const char *spec_file)
{
	xinfo("loading algorithm(%s)...\n", algo_file);
	algo_handle = dlopen(algo_file, RTLD_NOW);
	if(!algo_handle)
		xerror("error: %s\n", dlerror());
	assert(algo_handle);
	f = dlsym(algo_handle, "f");
	assert(f);
	init_f = dlsym(algo_handle, "init_f");
	assert(init_f);

	xinfo("loading header spec(%s)...\n", spec_file);
	header_spec = spec_parser_file(spec_file);
	assert(header_spec);
	xinfo("init env...\n");
	env = map(mapf_eq_str, mapf_hash_str, mapf_dup_str, mapf_free_str);
	init_f(env);
}

void core_switch_up(struct xswitch *sw)
{
	/* init trace tree */
	sw->trace_tree = trace_tree();
}

void core_switch_down(struct xswitch *sw)
{
	trace_tree_free(sw->trace_tree);
}

static void mod_in_port(struct trace *trace, int in_port)
{
	int i = trace->num_events - 1;
	assert(i >= 0);
	assert(trace->events[i].type == EV_R);
	assert(strcmp(trace->events[i].u.r.name, "in_port") == 0);
	trace->events[i].u.r.value = value_from_8(in_port);
}


struct namearg
{
	const char *name;
	const void *arg;
};

static bool cmpna_p(void *parg, const char *name, const void *arg)
{
	struct namearg *na = parg;
	if(strcmp(na->name, name) == 0 && (na->arg == arg || NULL == arg))
		return true;
	return false;
}

void core_packet_in(struct xswitch *sw, int in_port, uint8_t *packet, int packet_len)
{
	int i;
	struct route *r;
	struct packet pkt;
	edge_t *edges;
	int num_edges;
	struct trace *trace;
	struct action *ac_edge, *ac_core;
	bool eq_edge_core;

	/* init */
	trace_clear();

	/* run */
	pkt.pp = packet_parser(header_spec, packet, packet_len);
	pkt.in_sw = sw;
	pkt.in_port = in_port;
	pkt.hack_get_payload = false;
	trace_G(NULL, header_spec, 0);

	topo_rdlock();
	r = f(&pkt, env);
	topo_unlock();

	trace_R("in_port", value_from_8(0));

	trace = trace_get();

	/* learn */
	ac_edge = action();
	ac_core = action();
	eq_edge_core = trace_generate_action(trace, ac_core, ac_edge);

	edges = route_get_edges(r, &num_edges);
	struct {
		struct entity *ent;
		struct xswitch *sw;
		struct action *ac;
		int in;
		enum { MODE_EDGE, MODE_CORE } mode;
	} sw_actions[num_edges];
	int n_actions = 0;
	for(i = 0; i < num_edges; i++) {
		struct entity *cur_ent = edges[i].ent1;
		int out_port = edges[i].port1;
		xdebug("handle edge (0x%x, %d, 0x%x, %d):\n",
		       entity_get_dpid(edges[i].ent1),
		       edges[i].port1,
		       entity_get_dpid(edges[i].ent2),
		       edges[i].port2);
		if(cur_ent) {
			struct xswitch *cur_sw = entity_get_xswitch(cur_ent);
			int j, entry;
			bool flag = true;
			for(j = 0; j < n_actions; j++)
				if(cur_ent == sw_actions[j].ent) {
					flag = false;
					entry = j;
					break;
				}
			if(flag) {
				entry = n_actions;
				n_actions++;
				sw_actions[entry].ent = cur_ent;
				sw_actions[entry].sw = cur_sw;
				sw_actions[entry].ac = action();
				if(edges[i].ent2 == NULL) {
					struct msgbuf *mb;
					struct action *a0 = action();
					int plen;
					const uint8_t *p = packet_parser_get_raw(pkt.pp, &plen);
					action_add(a0, AC_COUNTER, out_port);
					action_add(a0, AC_OUTPUT, out_port);
					mb = msg_packet_out(0, p, plen, a0);
					xswitch_send(cur_sw, mb);
					action_free(a0);
					sw_actions[entry].ac = action_copy(ac_edge);
					sw_actions[entry].mode = MODE_EDGE;
				} else {
					sw_actions[entry].ac = action_copy(ac_core);
					sw_actions[entry].mode = MODE_CORE;
				}
			}

			/* XXX */
			assert((eq_edge_core) ||
			       (sw_actions[entry].mode == MODE_EDGE && edges[i].ent2 == NULL) ||
			       (sw_actions[entry].mode == MODE_CORE && edges[i].ent2 != NULL));

			action_add(sw_actions[entry].ac, AC_COUNTER, out_port);
			action_add(sw_actions[entry].ac, AC_OUTPUT, out_port);
			for(j = 0; j < num_edges; j++) {
				if(edges[j].ent2 == cur_ent)
					break;
			}
			assert(j < num_edges);
			sw_actions[entry].in = edges[j].port2;
		}
	}
	for(i = 0; i < n_actions; i++) {
		struct xswitch *cur_sw = sw_actions[i].sw;
		struct entity *cur_ent = sw_actions[i].ent;
		struct action *cur_ac = sw_actions[i].ac;
		mod_in_port(trace, sw_actions[i].in);

		xswitch_table_lock(cur_sw);
		if(trace_tree_augment(&(cur_sw->trace_tree), trace, cur_ac)) {
			xdebug("--- flow table for 0x%x ---\n", entity_get_dpid(cur_ent));
			trace_tree_print(cur_sw->trace_tree);
#ifdef ENABLE_WEB
			trace_tree_print_json(cur_sw->trace_tree, cur_sw->dpid);
#endif
			xdebug("\n");
			trace_tree_emit_rule(cur_sw, cur_sw->trace_tree);
#ifdef ENABLE_WEB
			trace_tree_print_ft_json(cur_sw->trace_tree, cur_sw->dpid);
#endif
		}
		xswitch_table_unlock(cur_sw);

		action_free(cur_ac);
	}

	action_free(ac_edge);
	action_free(ac_core);

	if((!pkt.hack_get_payload) && num_edges == 0) {
		struct action *a = action();
		mod_in_port(trace, in_port);
		action_add(a, AC_DROP, 0);

		xswitch_table_lock(sw);
		if(trace_tree_augment(&(sw->trace_tree), trace, a)) {
			xdebug("--- flow table for cur sw ---\n");
			trace_tree_print(sw->trace_tree);
#ifdef ENABLE_WEB
			trace_tree_print_json(sw->trace_tree, sw->dpid);
#endif
			xdebug("\n");
			trace_tree_emit_rule(sw, sw->trace_tree);
#ifdef ENABLE_WEB
			trace_tree_print_ft_json(sw->trace_tree, sw->dpid);
#endif
		}
		xswitch_table_unlock(sw);

		action_free(a);
	}

	route_free(r);

	/* invalidate */
	for(i = 0; i < trace->num_inv_events; i++) {
		struct namearg na;
		na.name = trace->inv_events[i].name;
		na.arg = trace->inv_events[i].arg;
		xdebug("invalidate \"%s\":\n", na.name);
		topo_rdlock();
		core_invalidate(cmpna_p, &na);
		topo_unlock();
	}

	packet_parser_free(pkt.pp);
}
