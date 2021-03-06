#include "xlog/xlog.h"
#include "pop_api.h"
#include "learning.h"

void init_f(struct map *env)
{
	xinfo("f init\n");
	learning_init(env, "table");
}

struct route *f(struct packet *pkt, struct map *env)
{
	struct entity *me = read_packet_inswitch(pkt);
	int in_port = read_packet_inport(pkt);
	uint64_t dst = value_to_48(read_packet(pkt, "dl_dst"));
	uint64_t src = value_to_48(read_packet(pkt, "dl_src"));

	return learning(env, "table", me, in_port, src, dst);
}
