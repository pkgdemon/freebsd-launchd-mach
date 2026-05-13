/*
 * libbootstrap.c — userland client + server-loop implementations of
 * the bootstrap-server protocol. Hand-rolled message-ID dispatch
 * (no MIG); see bootstrap_protocol.h for wire format.
 *
 * Phase G1 scope: single-task only. The server-loop's name→port map
 * is process-local (a static array in this file), so client and
 * server must be in the same address space. Phase G2 lifts this to
 * cross-process via complex-message port descriptors + a real daemon.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <mach/mach_traps.h>
#include <mach/mach_port.h>
#include <mach/message.h>
#include <servers/bootstrap.h>

#include "bootstrap_protocol.h"

/*
 * BOOTSTRAP_DEBUG: when set non-zero, libbootstrap traces each
 * mach_msg send/receive on the protocol path so failures can be
 * bisected. Compiled-in for now (Phase G1 single-task validation
 * needs the visibility); strip once the cross-process / daemon
 * path is stable.
 */
#define	BOOTSTRAP_DEBUG	1
#if BOOTSTRAP_DEBUG
#define	bs_dbg(...)	do { fprintf(stderr, "bs: " __VA_ARGS__); fflush(stderr); } while (0)
#else
#define	bs_dbg(...)	((void)0)
#endif

/*
 * Per-call buffer big enough for header + trailer slack the kernel
 * appends on receive. mach_msg_trailer_t is 8 bytes; mach.ko's
 * trailer template is mach_msg_max_trailer_t which can be up to ~64.
 * 128 covers it.
 */
#define	BOOTSTRAP_TRAILER_PAD	128

/* Service map. Phase G1 keeps it as a fixed-size static array in the
 * server's address space. Phase G2 replaces with a real registry. */
#define	BOOTSTRAP_MAX_SERVICES	64

struct bootstrap_service {
	char			name[BOOTSTRAP_MAX_NAME_LEN];
	mach_port_name_t	port;	/* send right held by server */
};

static struct bootstrap_service bootstrap_services[BOOTSTRAP_MAX_SERVICES];
static int bootstrap_service_count;

/*
 * Build and send a request, then receive the reply on a freshly-
 * allocated reply port. Returns the reply's `result` field on success
 * (zero or a kern_return_t failure code), or a Mach msg return code
 * encoded as kern_return_t if the IPC itself failed.
 */
static kern_return_t
bootstrap_call(mach_port_t bp, uint32_t msg_id, const char *name,
    mach_port_name_t *out_port)
{
	struct bootstrap_request_msg req;
	struct {
		struct bootstrap_reply_msg msg;
		uint8_t trailer[BOOTSTRAP_TRAILER_PAD];
	} reply;
	mach_port_name_t reply_port;
	mach_msg_return_t mr;

	reply_port = mach_reply_port();
	if (reply_port == MACH_PORT_NULL)
		return (KERN_RESOURCE_SHORTAGE);

	memset(&req, 0, sizeof(req));
	req.header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
	    MACH_MSG_TYPE_MAKE_SEND_ONCE);
	req.header.msgh_size = sizeof(req);
	req.header.msgh_remote_port = bp;
	req.header.msgh_local_port  = reply_port;
	req.header.msgh_id          = (mach_msg_id_t)msg_id;
	if (name) {
		strncpy(req.service_name, name,
		    BOOTSTRAP_MAX_NAME_LEN - 1);
		req.service_name[BOOTSTRAP_MAX_NAME_LEN - 1] = '\0';
	}

	memset(&reply, 0, sizeof(reply));
	bs_dbg("client: send id=0x%x bp=0x%x reply=0x%x name=%s\n",
	    (unsigned)msg_id, (unsigned)bp, (unsigned)reply_port,
	    name ? name : "(null)");
	mr = mach_msg((mach_msg_header_t *)&req,
	    MACH_SEND_MSG | MACH_RCV_MSG | MACH_SEND_TIMEOUT |
	        MACH_RCV_TIMEOUT,
	    sizeof(req),
	    sizeof(reply),
	    reply_port,
	    5000,	/* 5s — generous, server loop should reply much faster */
	    MACH_PORT_NULL);
	bs_dbg("client: mach_msg returned 0x%x\n", (unsigned)mr);
	if (mr != MACH_MSG_SUCCESS)
		return ((kern_return_t)mr);

	if (out_port)
		*out_port = reply.msg.port;
	return (reply.msg.result);
}

kern_return_t
bootstrap_check_in(mach_port_t bp, const name_t name, mach_port_t *port)
{
	if (!name || !port)
		return (KERN_INVALID_ARGUMENT);
	*port = MACH_PORT_NULL;
	return (bootstrap_call(bp, BOOTSTRAP_MSG_CHECK_IN, name, port));
}

kern_return_t
bootstrap_look_up(mach_port_t bp, const name_t name, mach_port_t *port)
{
	if (!name || !port)
		return (KERN_INVALID_ARGUMENT);
	*port = MACH_PORT_NULL;
	return (bootstrap_call(bp, BOOTSTRAP_MSG_LOOK_UP, name, port));
}

/*
 * Server-side service-map helpers. Phase G1 single-task model: the
 * server holds receive rights on registered service ports, and the
 * `port` field of bootstrap_service is the port NAME in the server's
 * (== caller's) space. Same task, so this name is also valid when
 * later look_up clients receive it.
 */
static struct bootstrap_service *
bootstrap_find(const char *name)
{
	int i;

	for (i = 0; i < bootstrap_service_count; i++) {
		if (strncmp(bootstrap_services[i].name, name,
		    BOOTSTRAP_MAX_NAME_LEN) == 0)
			return (&bootstrap_services[i]);
	}
	return (NULL);
}

static kern_return_t
bootstrap_handle_check_in(const char *name, mach_port_name_t *out_port)
{
	struct bootstrap_service *svc;
	mach_port_name_t port;
	kern_return_t kr;

	if (bootstrap_find(name) != NULL)
		return (KERN_FAILURE);	/* already registered */
	if (bootstrap_service_count >= BOOTSTRAP_MAX_SERVICES)
		return (KERN_RESOURCE_SHORTAGE);

	kr = mach_port_allocate(mach_task_self(),
	    MACH_PORT_RIGHT_RECEIVE, &port);
	if (kr != KERN_SUCCESS)
		return (kr);
	/*
	 * Insert a send right on the same name so the server can hand
	 * a send-right reference back to future look_up callers via
	 * msgh_local_port (or, in single-task mode, by name).
	 */
	kr = mach_port_insert_right(mach_task_self(), port, port,
	    MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		mach_port_deallocate(mach_task_self(), port);
		return (kr);
	}

	svc = &bootstrap_services[bootstrap_service_count++];
	strncpy(svc->name, name, BOOTSTRAP_MAX_NAME_LEN - 1);
	svc->name[BOOTSTRAP_MAX_NAME_LEN - 1] = '\0';
	svc->port = port;

	*out_port = port;
	return (KERN_SUCCESS);
}

static kern_return_t
bootstrap_handle_look_up(const char *name, mach_port_name_t *out_port)
{
	struct bootstrap_service *svc;

	svc = bootstrap_find(name);
	if (svc == NULL)
		return (KERN_FAILURE);	/* no such service */
	*out_port = svc->port;
	return (KERN_SUCCESS);
}

/*
 * Server message loop. Blocks in mach_msg waiting for a request,
 * dispatches on msgh_id, sends a reply on msgh_local_port (the
 * send-once right the client built via MAKE_SEND_ONCE). Returns
 * when *stop becomes non-zero (between iterations — the polling
 * uses a long-but-finite timeout so cancellation is responsive).
 */
void
bootstrap_server_run(mach_port_t service_port, volatile int *stop)
{
	struct {
		struct bootstrap_request_msg msg;
		uint8_t trailer[BOOTSTRAP_TRAILER_PAD];
	} req;
	struct bootstrap_reply_msg reply;
	mach_msg_return_t mr;
	kern_return_t kr;
	mach_port_name_t out_port;
	mach_msg_id_t reply_id;

	bs_dbg("server: enter loop, service_port=0x%x\n",
	    (unsigned)service_port);
	while (stop == NULL || !*stop) {
		memset(&req, 0, sizeof(req));
		mr = mach_msg((mach_msg_header_t *)&req.msg,
		    MACH_RCV_MSG | MACH_RCV_TIMEOUT,
		    0, sizeof(req), service_port, 500, MACH_PORT_NULL);
		if (mr == MACH_RCV_TIMED_OUT)
			continue;
		bs_dbg("server: recv id=0x%x remote=0x%x local=0x%x name=%s\n",
		    (unsigned)req.msg.header.msgh_id,
		    (unsigned)req.msg.header.msgh_remote_port,
		    (unsigned)req.msg.header.msgh_local_port,
		    req.msg.service_name);
		if (mr != MACH_MSG_SUCCESS) {
			bs_dbg("server: mach_msg(RCV) failed 0x%x — exit loop\n",
			    (unsigned)mr);
			return;
		}

		out_port = MACH_PORT_NULL;
		switch (req.msg.header.msgh_id) {
		case BOOTSTRAP_MSG_CHECK_IN:
			kr = bootstrap_handle_check_in(req.msg.service_name,
			    &out_port);
			reply_id = BOOTSTRAP_MSG_CHECK_IN_REPLY;
			break;
		case BOOTSTRAP_MSG_LOOK_UP:
			kr = bootstrap_handle_look_up(req.msg.service_name,
			    &out_port);
			reply_id = BOOTSTRAP_MSG_LOOK_UP_REPLY;
			break;
		default:
			kr = KERN_INVALID_ARGUMENT;
			reply_id = (mach_msg_id_t)
			    (req.msg.header.msgh_id + BOOTSTRAP_MSG_REPLY_OFFSET);
			break;
		}

		memset(&reply, 0, sizeof(reply));
		/*
		 * Reply goes back to the client's reply port, which is in
		 * msgh_local_port of the request (the kernel translated
		 * the client's MAKE_SEND_ONCE into a real send-once-right
		 * to that port). Use MOVE_SEND_ONCE so we consume the
		 * right exactly once on send.
		 */
		reply.header.msgh_bits =
		    MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE, 0);
		reply.header.msgh_size = sizeof(reply);
		reply.header.msgh_remote_port = req.msg.header.msgh_local_port;
		reply.header.msgh_local_port  = MACH_PORT_NULL;
		reply.header.msgh_id          = reply_id;
		reply.result = kr;
		reply.port   = out_port;

		mr = mach_msg((mach_msg_header_t *)&reply,
		    MACH_SEND_MSG | MACH_SEND_TIMEOUT,
		    sizeof(reply), 0, MACH_PORT_NULL, 100, MACH_PORT_NULL);
		bs_dbg("server: reply id=0x%x port=0x%x result=0x%x send_rc=0x%x\n",
		    (unsigned)reply_id, (unsigned)out_port, (unsigned)kr,
		    (unsigned)mr);
	}
}
