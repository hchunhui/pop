#ifndef _POP_API_H_
#define _POP_API_H_
#include "types.h"

int read_packet_inport(struct packet *pkt);
struct entity *read_packet_inswitch(struct packet *pkt);
void pull_header(struct packet *pkt);
const char *read_header_type(struct packet *pkt);
value_t read_packet(struct packet *pkt, const char *field);
bool test_equal(struct packet *pkt, const char *field, value_t value);
const uint8_t *read_payload(struct packet *pkt, int *length);
void mod_packet(struct packet *pkt, const char *field, value_t value);
void push_header(struct packet *pkt);

void add_header(struct packet *pkt, const char *proto);

void add_field(struct packet *pkt, int offb, int lenb, value_t value);
void del_field(struct packet *pkt, int offb, int lenb);

struct entity **get_hosts(int *pnum);
struct entity **get_switches(int *pnum);
struct entity *get_switch(dpid_t dpid);
struct entity *get_host_by_haddr(haddr_t addr);
struct entity *get_host_by_paddr(uint32_t addr);

#ifndef _ENTITY_H_
enum entity_type { ENTITY_TYPE_HOST, ENTITY_TYPE_SWITCH };
struct entity_adj
{
	int out_port;
	int adj_in_port;
	struct entity *adj_entity;
};
#endif

struct entity *get_entity(struct entity *e, int port);
enum entity_type get_entity_type(struct entity *e);
dpid_t get_switch_dpid(struct entity *e);
struct entity *get_host_adj_switch(struct entity *e, int *sw_port);
const struct entity_adj *get_entity_adjs(struct entity *e, int *pnum);
void print_entity(struct entity *e);

uint64_t get_port_recvpkts(struct entity *e, uint16_t port_id);
uint64_t get_port_recvbytes(struct entity *e, uint16_t port_id);
/* recent: in the last 5 seconds, the pkts(or bytes) received by the port.*/
uint64_t get_port_recent_recvpkts(struct entity *e, uint16_t port_id);
uint64_t get_port_recent_recvbytes(struct entity *e, uint16_t port_id);
/* Return false means a wrong port_id or something else.*/
bool get_port_stats(struct entity *e, uint16_t port_id,
		    uint64_t *recvpkts/*OUT*/,
		    uint64_t *recvbytes/*OUT*/,
		    uint64_t *recent_recvpkts/*OUT*/,
		    uint64_t *recent_recvbytes/*OUT*/);

#endif /* _POP_API_H_ */
