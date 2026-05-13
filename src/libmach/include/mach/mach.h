/*
 * mach/mach.h — Apple-canonical umbrella header for the Mach userland
 * API. Apple's macOS exposes a single `#include <mach/mach.h>` that
 * pulls in the entire Mach surface (mach_msg, port management, special
 * ports, host APIs, bootstrap, etc.). Apple-source consumers
 * (libxpc, launchd, configd, ...) include this one header.
 *
 * On freebsd-launchd-mach we don't have Apple's full Mach userland;
 * we provide a coordinated subset via libsystem_kernel. This header
 * is the umbrella that maps the Apple-canonical include to that subset
 * — so Apple-source code compiles unchanged.
 */
#ifndef _MACH_MACH_H_
#define _MACH_MACH_H_

#include <mach/mach_traps.h>		/* mach_reply_port, mach_task_self, ... */
#include <mach/mach_port.h>		/* mach_port_allocate, kern_return_t */
#include <mach/message.h>		/* mach_msg, mach_msg_header_t, ... */
#include <mach/task_special_ports.h>	/* task_get_bootstrap_port, ... */
#include <mach/host_special_ports.h>	/* host_set_special_port, HOST_BOOTSTRAP_PORT */

#endif /* !_MACH_MACH_H_ */
