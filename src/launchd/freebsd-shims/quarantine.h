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

/* Upper bound on a serialized quarantine blob. libvproc.c stack-
 * allocates a char[QTN_SERIALIZED_DATA_MAX] before calling
 * qtn_proc_to_data(). Apple's value; our qtn_proc_to_data stub
 * returns "no data" so the buffer is never actually filled, but the
 * array still needs a valid compile-time size. */
#ifndef QTN_SERIALIZED_DATA_MAX
#define QTN_SERIALIZED_DATA_MAX	4096
#endif

typedef struct _qtn_proc *qtn_proc_t;
typedef struct _qtn_file *qtn_file_t;

/*
 * qtn_proc_alloc() — Apple's allocator for a per-process quarantine
 * handle (later filled via qtn_proc_init_with_*() and serialized via
 * qtn_proc_to_data()). FreeBSD has no Gatekeeper; the stub returns
 * NULL so launchd's `if (job_assumes(j, qp = qtn_proc_alloc()))`
 * check takes the "no quarantine" branch.
 */
static __inline qtn_proc_t
qtn_proc_alloc(void)
{
	return (qtn_proc_t)0;
}

static __inline void
qtn_proc_free(qtn_proc_t proc)
{
	(void)proc;
}

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
