/* notify_probes.h — FreeBSD shim. Apple generates this from a .d
 * (DTrace) file via dtrace -h. Stub all NOTIFY_PROBE_* macros to
 * no-ops so notify_client.c compiles without dtrace. */
#ifndef _FREEBSD_SHIM_NOTIFY_PROBES_H_
#define _FREEBSD_SHIM_NOTIFY_PROBES_H_

#define NOTIFY_PROBE_REGISTER_PLAIN(...)		do {} while (0)
#define NOTIFY_PROBE_REGISTER_PLAIN_ENABLED()		0
#define NOTIFY_PROBE_REGISTER_CHECK(...)		do {} while (0)
#define NOTIFY_PROBE_REGISTER_CHECK_ENABLED()		0
#define NOTIFY_PROBE_REGISTER_SIGNAL(...)		do {} while (0)
#define NOTIFY_PROBE_REGISTER_SIGNAL_ENABLED()		0
#define NOTIFY_PROBE_REGISTER_FD(...)			do {} while (0)
#define NOTIFY_PROBE_REGISTER_FD_ENABLED()		0
#define NOTIFY_PROBE_REGISTER_MACH_PORT(...)		do {} while (0)
#define NOTIFY_PROBE_REGISTER_MACH_PORT_ENABLED()	0
#define NOTIFY_PROBE_REGISTER_DISPATCH(...)		do {} while (0)
#define NOTIFY_PROBE_REGISTER_DISPATCH_ENABLED()	0
#define NOTIFY_PROBE_CANCEL(...)			do {} while (0)
#define NOTIFY_PROBE_CANCEL_ENABLED()			0
#define NOTIFY_PROBE_POST(...)				do {} while (0)
#define NOTIFY_PROBE_POST_ENABLED()			0
#define NOTIFY_PROBE_CHECK(...)				do {} while (0)
#define NOTIFY_PROBE_CHECK_ENABLED()			0
#define NOTIFY_PROBE_GET_STATE(...)			do {} while (0)
#define NOTIFY_PROBE_GET_STATE_ENABLED()		0
#define NOTIFY_PROBE_SET_STATE(...)			do {} while (0)
#define NOTIFY_PROBE_SET_STATE_ENABLED()		0
#define NOTIFY_PROBE_NOTIFY(...)			do {} while (0)
#define NOTIFY_PROBE_NOTIFY_ENABLED()			0

/* Generic fallback for any probe we haven't enumerated. */
#define NOTIFY_PROBE_GENERIC(...)			do {} while (0)

#endif
