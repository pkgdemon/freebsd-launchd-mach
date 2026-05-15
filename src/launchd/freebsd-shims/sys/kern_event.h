/*
 * FreeBSD shim for macOS <sys/kern_event.h>.
 *
 * launchd.c's monitor_networking_state() opens a PF_SYSTEM /
 * SYSPROTO_EVENT socket and arms a SIOCSKEVFILT filter to receive
 * Apple's kernel network-state events (interface up/down). FreeBSD
 * has no PF_SYSTEM domain; the native equivalent is a PF_ROUTE
 * listener, which is a separate porting task.
 *
 * For Phase I1c we only need launchd to compile, link, and exec, so
 * this shim supplies the types and constants and lets
 * socket(PF_SYSTEM, ...) fail at runtime. monitor_networking_state()
 * already handles pfs == -1 by returning early — the daemon still
 * runs, just without live network-state notifications
 * (get_network_state() continues to poll getifaddrs()).
 */
#ifndef _FREEBSD_SHIM_SYS_KERN_EVENT_H_
#define _FREEBSD_SHIM_SYS_KERN_EVENT_H_

#include <sys/types.h>
#include <sys/ioccom.h>

#ifndef PF_SYSTEM
#define PF_SYSTEM	32	/* not a real FreeBSD domain — socket() fails EAFNOSUPPORT */
#endif
#ifndef AF_SYSTEM
#define AF_SYSTEM	PF_SYSTEM
#endif
#ifndef SYSPROTO_EVENT
#define SYSPROTO_EVENT	1
#endif

#define KEV_VENDOR_APPLE	1
#define KEV_NETWORK_CLASS	1

struct kev_request {
	u_int32_t	vendor_code;
	u_int32_t	kev_class;
	u_int32_t	kev_subclass;
};

struct kern_event_msg {
	u_int32_t	total_size;
	u_int32_t	vendor_code;
	u_int32_t	kev_class;
	u_int32_t	kev_subclass;
	u_int32_t	id;
	u_int32_t	event_code;
	u_int32_t	event_data[1];
};

#define SIOCSKEVFILT	_IOW('e', 2, struct kev_request)
#define SIOCGKEVID	_IOR('e', 3, u_int32_t)

#endif /* _FREEBSD_SHIM_SYS_KERN_EVENT_H_ */
