/*
 * bootstrap_server — daemon binary for Phase G2c.
 *
 * Allocates a service port, publishes it host-wide as
 * HOST_BOOTSTRAP_PORT via host_set_special_port, and enters
 * bootstrap_server_run (from libbootstrap) to handle CHECK_IN /
 * LOOK_UP requests over hand-rolled Mach IPC.
 *
 * Lifecycle: synchronous foreground process; SIGTERM sets the stop
 * flag and exits cleanly (un-publishes host slot, drops the port).
 * Not wired into rc.local — the real home for this daemon is a
 * launchd-managed job, which only happens once launchd is PID 1 (a
 * much later phase). For now the smoke harness starts it
 * (run.sh) and kills it on exit.
 *
 * Discovery model from clients: in any fresh process, task's
 * itk_bootstrap is null; task_get_bootstrap_port falls back to
 * realhost.special[HOST_BOOTSTRAP_PORT] (the slot this daemon
 * populated). Clients get a send right to this daemon's service
 * port; bootstrap_check_in / bootstrap_look_up then RPC over it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <mach/mach_traps.h>
#include <mach/mach_port.h>
#include <mach/message.h>
#include <mach/host_special_ports.h>
#include <servers/bootstrap.h>

static volatile int g_stop;

static void
sigterm_handler(int sig __attribute__((unused)))
{

	g_stop = 1;
}

int
main(void)
{
	mach_port_name_t task = mach_task_self();
	mach_port_name_t host = mach_host_self();
	mach_port_name_t port = MACH_PORT_NULL;
	kern_return_t kr;

	signal(SIGTERM, sigterm_handler);
	signal(SIGINT,  sigterm_handler);

	kr = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &port);
	if (kr != KERN_SUCCESS) {
		fprintf(stderr, "bootstrap_server: mach_port_allocate: 0x%x\n",
		    (unsigned)kr);
		return (1);
	}
	kr = mach_port_insert_right(task, port, port,
	    MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		fprintf(stderr, "bootstrap_server: mach_port_insert_right: 0x%x\n",
		    (unsigned)kr);
		return (1);
	}

	/*
	 * Publish host-wide. Any task whose itk_bootstrap is null
	 * (which is every task at the moment — launchd isn't PID 1
	 * yet) will get a send right to this port via
	 * task_get_bootstrap_port's host fallback.
	 */
	kr = host_set_special_port(host, HOST_BOOTSTRAP_PORT, port);
	if (kr != KERN_SUCCESS) {
		fprintf(stderr, "bootstrap_server: host_set_special_port: 0x%x\n",
		    (unsigned)kr);
		return (1);
	}

	fprintf(stderr, "bootstrap_server: ready, port=0x%x\n", port);

	bootstrap_server_run(port, &g_stop);

	/*
	 * Cleanup: un-publish the host slot so the next test in the
	 * same boot doesn't see a stale port. (run.sh kills us before
	 * launching the next phase, but be tidy.)
	 */
	(void)host_set_special_port(host, HOST_BOOTSTRAP_PORT,
	    MACH_PORT_NULL);
	(void)mach_port_deallocate(task, port);

	fprintf(stderr, "bootstrap_server: exiting\n");
	return (0);
}
