/*
 * test_bootstrap — single-task validation of the bootstrap protocol.
 *
 * Phase G1 scope: cross-process is the next commit. For now we
 * exercise the message format, server-loop dispatch, and client
 * round-trip in a single address space.
 *
 * Flow:
 *   1. Allocate a service port; install a MAKE_SEND right onto it
 *      so we can use it as `bp` from client calls.
 *   2. Store it via task_set_bootstrap_port (matches the Apple-
 *      canonical client-discovery path; Phase G2 swaps the in-task
 *      setter for a daemon-side setter on each new task).
 *   3. Spawn a pthread that runs bootstrap_server_run on the service
 *      port until `stop` is set.
 *   4. Main thread:
 *        - retrieve the bootstrap port via task_get_bootstrap_port
 *        - bootstrap_check_in("com.example.test") — gets receive port
 *        - bootstrap_look_up("com.example.test") — should return the
 *          same port name (same task)
 *        - assert match
 *        - signal stop, join server thread
 *
 * Exit codes:
 *   0 — round-trip succeeded; check_in and look_up returned the
 *       same port name.
 *   1 — service port allocation failed.
 *   2 — task_set_bootstrap_port failed.
 *   3 — task_get_bootstrap_port failed / returned wrong port.
 *   4 — pthread_create for server thread failed.
 *   5 — bootstrap_check_in failed.
 *   6 — bootstrap_look_up failed.
 *   7 — port-name mismatch between check_in and look_up.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#include <mach/mach_traps.h>
#include <mach/mach_port.h>
#include <mach/message.h>
#include <mach/task_special_ports.h>
#include <servers/bootstrap.h>

static volatile int server_stop;

static void *
server_thread(void *arg)
{
	mach_port_t service = (mach_port_t)(uintptr_t)arg;
	bootstrap_server_run(service, &server_stop);
	return (NULL);
}

int
main(void)
{
	mach_port_name_t task = mach_task_self();
	mach_port_name_t service_port = MACH_PORT_NULL;
	mach_port_name_t bp = MACH_PORT_NULL;
	mach_port_t      checkin_port = MACH_PORT_NULL;
	mach_port_t      lookup_port  = MACH_PORT_NULL;
	pthread_t        srv_thread;
	kern_return_t kr;
	int err;

	printf("task = 0x%x\n", task);

	/* 1. allocate + insert MAKE_SEND on the service (bootstrap) port. */
	kr = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &service_port);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: mach_port_allocate(service): 0x%x\n", (unsigned)kr);
		return (1);
	}
	kr = mach_port_insert_right(task, service_port, service_port,
	    MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: mach_port_insert_right(service): 0x%x\n", (unsigned)kr);
		return (1);
	}
	printf("service port = 0x%x\n", service_port);

	/* 2. publish as TASK_BOOTSTRAP_PORT. */
	kr = task_set_bootstrap_port(task, service_port);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: task_set_bootstrap_port: 0x%x\n", (unsigned)kr);
		return (2);
	}

	/* 3. spawn the server. */
	err = pthread_create(&srv_thread, NULL, server_thread,
	    (void *)(uintptr_t)service_port);
	if (err != 0) {
		printf("FAIL: pthread_create: errno=%d\n", err);
		return (4);
	}
	/* Give the server thread a beat to enter its receive loop. */
	usleep(50000);

	/* 4. retrieve bp via task_get_bootstrap_port — mirrors the
	 *    Apple-canonical mach_init() discovery path. */
	kr = task_get_bootstrap_port(task, &bp);
	if (kr != KERN_SUCCESS || bp != service_port) {
		printf("FAIL: task_get_bootstrap_port: kr=0x%x bp=0x%x (expected 0x%x)\n",
		    (unsigned)kr, bp, service_port);
		server_stop = 1;
		pthread_join(srv_thread, NULL);
		return (3);
	}

	/* check_in: server allocates a fresh port and tells us its name. */
	kr = bootstrap_check_in(bp, "com.example.test", &checkin_port);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: bootstrap_check_in: 0x%x\n", (unsigned)kr);
		server_stop = 1;
		pthread_join(srv_thread, NULL);
		return (5);
	}
	if (checkin_port == MACH_PORT_NULL) {
		printf("FAIL: bootstrap_check_in returned KERN_SUCCESS but port "
		    "is MACH_PORT_NULL (reply body not received?)\n");
		server_stop = 1;
		pthread_join(srv_thread, NULL);
		return (5);
	}
	printf("bootstrap_check_in OK: port = 0x%x\n", checkin_port);

	/* look_up: should return the SAME port name (single-task, server's
	 * map stored the port name). */
	kr = bootstrap_look_up(bp, "com.example.test", &lookup_port);
	if (kr != KERN_SUCCESS) {
		printf("FAIL: bootstrap_look_up: 0x%x\n", (unsigned)kr);
		server_stop = 1;
		pthread_join(srv_thread, NULL);
		return (6);
	}
	if (lookup_port == MACH_PORT_NULL) {
		printf("FAIL: bootstrap_look_up returned KERN_SUCCESS but port "
		    "is MACH_PORT_NULL\n");
		server_stop = 1;
		pthread_join(srv_thread, NULL);
		return (6);
	}
	printf("bootstrap_look_up   OK: port = 0x%x\n", lookup_port);

	server_stop = 1;
	pthread_join(srv_thread, NULL);

	if (lookup_port != checkin_port) {
		printf("FAIL: check_in returned 0x%x but look_up returned 0x%x\n",
		    checkin_port, lookup_port);
		return (7);
	}

	printf("BOOTSTRAP-OK: check_in/look_up round-trip succeeded "
	    "(port=0x%x)\n", checkin_port);
	return (0);
}
