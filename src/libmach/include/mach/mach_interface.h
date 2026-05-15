/*
 * mach/mach_interface.h — on Apple this is the umbrella for the
 * MIG-generated kernel RPC interfaces (mach_port, mach_host,
 * host_priv, ...). freebsd-launchd-mach exposes that surface as
 * direct libsystem_kernel calls rather than MIG stubs, so this header
 * is just a convenience umbrella over the pieces launchd-842 expects
 * a single <mach/mach_interface.h> include to provide.
 */
#ifndef _MACH_MACH_INTERFACE_H_
#define _MACH_MACH_INTERFACE_H_

#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mach_port.h>
#include <mach/host_special_ports.h>
#include <mach/task_special_ports.h>
#include <mach/mach_host.h>

#endif /* !_MACH_MACH_INTERFACE_H_ */
