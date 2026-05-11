# Out-of-tree build of mach.ko.
#
# Source lineage:
#   - sources under src/ derive from ravynOS legacy FreeBSD branch
#     sys/compat/mach/ (OSF / CMU / BSD 2-Clause), itself a NextBSD-era
#     port of XNU's osfmk/ipc/
#   - kernel-internal Mach headers under include/sys/mach/ come from
#     ravynOS sys/sys/mach/
#   - Apple-ism userland-shape headers under include/apple/ come from
#     ravynOS include/apple/
#
# This Makefile follows FreeBSD's standard out-of-tree kmod pattern. It
# does NOT require a built kernel by default — the opt_*.h headers
# shipped alongside this Makefile match a GENERIC kernel configuration.
#
# Users on custom kernel configs (anything that differs from GENERIC on
# CAPABILITIES, PPS_SYNC, COMPAT_MACH) should rebuild this module
# against their kernel build directory:
#
#     make KERNBUILDDIR=/usr/obj/usr/src/sys/MYKERNEL
#
# bsd.kmod.mk honors KERNBUILDDIR natively and will pick up the actual
# opt_*.h files generated from that kernel's config, overriding our
# shipped defaults. No source changes; no #ifdef wallpaper.
#
# SYSDIR points at the kernel source tree (default /usr/src/sys). The
# tree must be available (install src.txz on FreeBSD installs that don't
# have it). End users consuming the pre-built mach.ko tarball don't
# need /usr/src/sys/ — only people building from source do.

SYSDIR?=	/usr/src/sys

.PATH:	${.CURDIR}/src
.PATH:	${.CURDIR}/src/ipc
.PATH:	${.CURDIR}/src/kern

KMOD=	mach

DEBUG_FLAGS+= -g -O0

.if .defined(X_COMPILER_TYPE) && ${X_COMPILER_TYPE} == gcc
WERROR=
.endif

# Option headers. By default the ones shipped in this directory are used
# (matching GENERIC). If KERNBUILDDIR is set, bsd.kmod.mk uses the
# kernel build's actual opt_*.h instead. See README for details.
SRCS=	bus_if.h device_if.h vnode_if.h
SRCS+=	opt_compat_mach.h opt_ntp.h opt_capsicum.h

# MIG-generated server stubs (pre-checked in; no mig invocation at build).
SRCS+=	host_priv_server.c mach_host_server.c mach_port_server.c mach_vm_server.c
SRCS+=	task_server.c vm_map_server.c clock_server.c

# compat/mach
SRCS+=	proc_info.c mach_clock.c mach_convert.c mach_host.c mach_host_priv.c
SRCS+=	mach_misc.c mach_module.c mach_processor.c mach_semaphore.c
SRCS+=	mach_task.c mach_thread.c mach_traps.c mach_vm.c

# compat/mach/ipc
SRCS+=	ipc_entry.c ipc_hash.c ipc_init.c ipc_kmsg.c ipc_kobject.c ipc_mqueue.c ipc_notify.c
SRCS+=	ipc_object.c ipc_port.c ipc_pset.c ipc_right.c ipc_space.c ipc_table.c ipc_thread.c
SRCS+=	mach_debug.c mach_msg.c mach_port.c

# compat/mach/kern
SRCS+=	ipc_tt.c ipc_host.c task.c thread_pool.c

# Include paths:
#   include/apple/  — Apple-ism headers (proc_info.h, kern_event.h, ...)
#   include/       — top-level so '#include <sys/mach/...>' resolves to
#                   our shipped kernel-internal Mach headers
#   ${.CURDIR}     — opt_*.h are at the top level of mach_kmod/
CFLAGS+=	-I${.CURDIR}/include/apple
CFLAGS+=	-I${.CURDIR}/include
CFLAGS+=	-I${.CURDIR}
CFLAGS+=	-DCOMPAT_MACH

# Force-include the out-of-tree compat shim. Provides aliases for
# struct proc/thread fields ravynOS added (p_machdata -> p_emuldata,
# td_machdata -> td_emuldata) and stub declarations for Apple-shape
# syscall handlers. See include/compat_shim.h for rationale.
CFLAGS+=	-include ${.CURDIR}/include/compat_shim.h

.include <bsd.kmod.mk>
