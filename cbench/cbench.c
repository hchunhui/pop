/*
 * Author: Tiwei Bie (btw@mail.ustc.edu.cn)
 *
 * A dumb OpenFlow 1.0 responder for benchmarking the controller I/O engine.
 * Intended to be used with oflops cbench.
 *
 * This is intended to be comparable with pox cbench app
 *     https://github.com/noxrepo/pox/pox/misc/cbench.py
 * and ryu cbench app.
 *     https://github.com/osrg/ryu/ryu/app/cbench.py
 */

#include <stdio.h>
#include <unistd.h>

#include <netinet/in.h>

#include "param.h"

#include "io/msgbuf.h"
#include "io/sw.h"
#include "io/io.h"

#include "openflow/openflow.h"

#ifndef __unused
#define __unused __attribute__((__unused__))
#endif

int realtime = 0;
int verbose  = 0;

void
accept_cb_func(struct sw *sw)
{
	struct msgbuf *msg = msgbuf_new(1024); // XXX
	struct ofp_header *p = (struct ofp_header *)msg->data;

	msg->sw = sw;

	p->version = 0x01;
	p->type = OFPT_FEATURES_REQUEST;
	p->length = htons(sizeof(*p));
	p->xid = 0;

	send_msgbuf(msg);

	printf("Connected.\n");
}

void
close_cb_func(struct sw *sw __unused)
{
	printf("Closed.\n");
}

#ifdef RETURN_PACKET_OUT_PACKET
void
recv_cb_func(struct msgbuf *msg)
{
	struct ofp_header *p = (struct ofp_header *)msg->data;

	if (p->type == OFPT_PACKET_IN) {
		p->type = OFPT_PACKET_OUT;
		send_msgbuf(msg);
	}
}
#else
void
recv_cb_func(struct msgbuf *msg)
{
	struct ofp_header *p = (struct ofp_header *)msg->data;
	struct sw *sw = msg->sw;

	if (p->type == OFPT_PACKET_IN) {
		struct msgbuf *msg = msgbuf_new(sizeof(struct ofp_flow_mod));
		struct ofp_flow_mod *ofm = (struct ofp_flow_mod *)msg->data;

		msg->sw = sw;

		/*
		 * Refer: ryu/ryu/ofproto/ofproto_v1_0_parser.py
		 */
		ofm->header.version = 0x01;
		ofm->header.type = OFPT_FLOW_MOD;
		ofm->header.length = htons(sizeof(*ofm));
		ofm->header.xid = 0; // 每次应该++

		//ofm->match. = xxx; // XXX

		ofm->cookie = 0;
		ofm->command = OFPFC_ADD;
		ofm->idle_timeout = 0;
		ofm->hard_timeout = 0;
		ofm->priority = OFP_DEFAULT_PRIORITY;
		ofm->buffer_id = 0xffffffff;
		ofm->out_port = OFPP_NONE;
		ofm->flags = 0;
		//ofm->actions array is 0 length.

		send_msgbuf(msg);
	}
	msgbuf_delete(msg);
}
#endif

void timeout_cb_func(void)
{
}

int
main(int argc, char **argv)
{
	int ch;

	while ((ch = getopt(argc, argv, "tv?")) != -1) {
		switch (ch) {
		case 't':
			realtime = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		}
	}
	argc -= optind;
	argv += optind;

	async_send = 0;
	init_io_module();
#if 0
	while (1) {
		struct msgbuf *msg;

		msg = recv_msgbuf(0);
		if (msg != NULL) {
			recv_cb_func(msg);
		}
	}
#endif

	fini_io_module();

	return (0);
}
