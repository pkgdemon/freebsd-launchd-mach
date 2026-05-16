/*
 * launchd_helper_server_stub.c — freebsd-launchd-mach (2026-05-15)
 *
 * Weak stub for the MIG-generated `launchd_helper_server` demux from
 * helper.defs (subsystem launchd_helper).
 *
 * Why: libvproc.c (compiled into liblaunch.so) references
 * launchd_helper_server at line 1002:
 *
 *     errno = mach_msg_server_once(launchd_helper_server,
 *                                  vprocmgr_helper_maxmsgsz, port, 0);
 *
 * The launchd daemon's own link includes helperServer.c (MIG output)
 * which defines the strong launchd_helper_server. launchctl's link
 * doesn't include helperServer.c, so when launchctl runs and FreeBSD's
 * rtld eagerly resolves liblaunch.so's references, it can't find
 * launchd_helper_server -> ld-elf.so.1 errors at first dlopen.
 *
 * This weak stub gives rtld a satisfying definition. launchd.exe's
 * strong helperServer.c symbol overrides at link time, so launchd's
 * runtime behavior is unchanged. launchctl uses the weak stub --
 * which is fine because launchctl never invokes mach_msg_server_once
 * on the helper port (that's a daemon-side activity).
 *
 * If a future port needs the real helper-server demux from a process
 * that isn't launchd itself, drop this stub and link helperServer.c
 * directly.
 */

#include <mach/mach.h>
#include <mach/boolean.h>
#include <mach/message.h>

__attribute__((weak))
boolean_t
launchd_helper_server(mach_msg_header_t *InHeadP __attribute__((unused)),
    mach_msg_header_t *OutHeadP __attribute__((unused)))
{
        /* No routine handled. mach_msg_server_once treats this as
         * "message not for me", returns MIG_BAD_ID. The caller
         * (vprocmgr_helper_recv et al) discards and continues. */
        return FALSE;
}
