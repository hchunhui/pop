#ifndef _XSWITCH_PRIVATE_H_
#define _XSWITCH_PRIVATE_H_
#include "xswitch.h"

/* xport */
struct xport {
	// XXX int
	uint16_t port_id;
	uint64_t recvpkts;
	uint64_t recvbytes;
	uint64_t recent_recvpkts;
	uint64_t recent_recvbytes;

	/* hash table
	 * hash function: port_id % XPORT_HASH_SIZE
	 */
	struct xport *next;
};

/* xswitch */
struct trace_tree;
struct xswitch
{
	enum {
		XS_HELLO,
		XS_FEATURES_REPLY,
		XS_RUNNING,
	} state;
	dpid_t dpid;
	int n_ports;
	int n_ready_ports;

	struct xport *xports[XPORT_HASH_SIZE];
	//TODO port number

	int next_table_id;
	struct flow_table *table0;
	struct trace_tree *trace_tree;
	pthread_mutex_t table_lock;

	void *conn;
};

void xswitch_up(struct xswitch *sw);
void xswitch_down(struct xswitch *sw);
void xswitch_packet_in(struct xswitch *sw, int in_port, uint8_t *packet, int packet_len);
void xswitch_port_status(struct xswitch *sw, int port, enum port_status status);

/* msg */
void msg_process(struct xswitch *sw, const struct msgbuf *msg);
bool msg_process_hello(const struct msgbuf *msg);
bool msg_process_features_reply(const struct msgbuf *msg, dpid_t *dpid, int *n_ports);


/* flow table */
#define MATCH_FIELD_NAME_LEN 32
#define FLOW_TABLE_NUM_FIELDS 8

struct match_field
{
	char name[MATCH_FIELD_NAME_LEN];
	enum match_field_type type;
	int offset;
	int length;
};

struct flow_table
{
	int tid;
	enum flow_table_type type;
	int size;
	int fields_num;
	struct match_field fields[FLOW_TABLE_NUM_FIELDS];
	unsigned long index_map[0];
};


/* match */
struct match
{
	int fields_num;
	struct {
		char name[MATCH_FIELD_NAME_LEN];
		value_t value;
		value_t mask;
	} m[FLOW_TABLE_NUM_FIELDS];
};


/* action */
#define ACTION_NUM_ACTIONS 18
struct action
{
	int num_actions;
	struct action_entry {
		enum action_type type;
		union {
			int arg;
			struct {
				int tid;
				int offset;
			} goto_table;
			struct {
				enum action_oper_type op_type;
				enum match_field_type dst_type;
				int dst_offset;
				int dst_length;
				enum match_field_type src_type;
				int src_offset;
				int src_length;
			} op_r;
			struct {
				enum action_oper_type op_type;
				enum match_field_type dst_type;
				int dst_offset;
				int dst_length;
				uint32_t src_value;
			} op_i;
			struct {
				int dst_offset;
				int dst_length;
				value_t val;
			} write_metadata;
			struct {
				enum move_direction dir;
				enum match_field_type type;
				int offset;
				int length;
			} move_packet;
			struct {
				enum move_direction dir;
				int value;
			} move_packet_imm;
			struct {
				int dst_offset;
				int dst_length;
				value_t val;
			} set_field;
			struct {
				int dst_offset;
				int dst_length;
				value_t val;
			} add_field;
			struct {
				int dst_offset;
				int dst_length;
			} del_field;
			struct {
				int sum_offset;
				int sum_length;
				int cal_offset;
				int cal_length;
			} checksum;
		} u;
	} a[ACTION_NUM_ACTIONS];
};

#endif /* _XSWITCH_PRIVATE_H_ */
