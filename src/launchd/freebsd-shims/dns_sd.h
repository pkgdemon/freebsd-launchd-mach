/*
 * freebsd-launchd-mach (2026-05-15)
 *
 * Empty stub for Apple's <dns_sd.h>. launchctl.c includes the header
 * but never calls any DNS-SD / Bonjour API — verified via grep for
 * DNSService*, kDNSService*, TXTRecord*. The include is dead weight
 * from launchctl's pre-port-extraction codebase.
 *
 * If a future Apple-source port (e.g. mDNSResponder, IPConfiguration's
 * service discovery integration) needs real DNS-SD, replace this with
 * either Apple's full dns_sd.h (apple-oss-distributions/mDNSResponder
 * provides it) or a richer shim.
 */
#ifndef _FREEBSD_LAUNCHD_MACH_DNS_SD_H_
#define _FREEBSD_LAUNCHD_MACH_DNS_SD_H_
/* deliberately empty */
#endif
