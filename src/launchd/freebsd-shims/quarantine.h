/*
 * quarantine.h — FreeBSD shim for Apple's <quarantine.h>.
 *
 * Apple's quarantine library tracks the "downloaded from the
 * internet" Gatekeeper attribute. launchd-842's libvproc.c calls
 * qtn_proc_to_data() when serializing a posix_spawn quarantine
 * attribute. FreeBSD has no Gatekeeper / quarantine subsystem, so
 * the type is opaque and the one call we hit is stubbed to fail
 * (returns nonzero), which libvproc.c already handles as "no
 * quarantine data to attach". Same posture as libxpc's stubs.c.
 */
#ifndef _QUARANTINE_H_SHIM_
#define _QUARANTINE_H_SHIM_

#include <sys/types.h>
#include <stdio.h>

typedef struct _qtn_proc *qtn_proc_t;
typedef struct _qtn_file *qtn_file_t;

static __inline int
qtn_proc_to_data(qtn_proc_t proc, void *buf, size_t *buflen)
{
	(void)proc; (void)buf;
	if (buflen != NULL)
		*buflen = 0;
	/* nonzero == "no quarantine data" — libvproc.c skips the attr */
	return 1;
}

#endif /* !_QUARANTINE_H_SHIM_ */
