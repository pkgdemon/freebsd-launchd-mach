/*
 * mach/mig.h — MIG runtime support: visibility macros, server-routine
 * types, and the per-subsystem descriptor struct.
 *
 * Every MIG-generated stub (.c) and header (.h) pulls this in for:
 *   - mig_external / mig_internal — symbol-visibility macros the
 *     generated function definitions are tagged with;
 *   - the __MigTypeCheck / __MigPackStructs feature switches the
 *     generated code tests with #if;
 *   - mig_routine_t and friends — the server demux function-pointer
 *     types, and struct mig_subsystem (the routine table a server
 *     dispatch loop walks).
 *
 * Apple's userland <mach/mig.h> is ~150 lines; this is a faithful
 * minimal subset — enough for the launchd-842 job/internal/helper
 * MIG output to compile. The MIG runtime *functions* (mig_get_reply_
 * port etc.) live in <mach/mig_errors.h>.
 */
#ifndef _MACH_MIG_H_
#define _MACH_MIG_H_

#include <mach/message.h>
#include <mach/kern_return.h>
#include <mach/port.h>
#include <stdint.h>

/* Symbol visibility for generated routines. Apple's build can
 * override mig_external via OTHER_MIGFLAGS (launchd-842 sets it to
 * __private_extern__); absent that, plain `extern` is the default. */
#ifndef mig_external
#define mig_external extern
#endif
#ifndef mig_internal
#define mig_internal static __inline__
#endif

/* Feature switches the generated code tests with #if. Type-checking
 * on (matches Apple's -D__MigTypeCheck=1); no kernel-specific code;
 * structs packed to the wire layout. */
#ifndef __MigTypeCheck
#define __MigTypeCheck		1
#endif
#ifndef __MigKernelSpecificCode
#define __MigKernelSpecificCode	0
#endif
#ifndef __MigPackStructs
#define __MigPackStructs	1
#endif

/* Server-side routine pointer types. A MIG server demux receives a
 * request message + a reply buffer; mig_routine_t is the per-routine
 * handler, mig_server_routine_t the subsystem demux entry point. */
typedef void (*mig_stub_routine_t)(mach_msg_header_t *InHeadP,
    mach_msg_header_t *OutHeadP);
typedef mig_stub_routine_t mig_routine_t;
typedef mig_routine_t (*mig_server_routine_t)(mach_msg_header_t *InHeadP);
typedef kern_return_t (*mig_impl_routine_t)(void);

/*
 * Per-routine descriptor inside a subsystem's routine table.
 * migcom's server.c emits the table as `struct routine_descriptor
 * routine[N]` and initializes each entry with six fields
 * { impl_routine, stub_routine, argc, descr_count, arg_descr,
 * max_reply_msg } — so the struct tag is the unprefixed
 * `routine_descriptor` (matching Apple's <mach/mig.h>), not
 * `mig_routine_descriptor`.
 */
struct routine_descriptor {
	mig_impl_routine_t	impl_routine;	/* the actual handler */
	mig_stub_routine_t	stub_routine;	/* the MIG-generated stub */
	unsigned int		argc;		/* numeric arg count */
	unsigned int		descr_count;	/* reply descriptor count */
	void			*arg_descr;	/* arg type descriptors */
	unsigned int		max_reply_msg;	/* max reply size */
};
typedef struct routine_descriptor *routine_descriptor_t;
typedef struct routine_descriptor mig_routine_descriptor;
typedef mig_routine_descriptor *mig_routine_descriptor_t;

/* A MIG subsystem: a contiguous routine table covering message IDs
 * [start, end). The generated <name>_server() demux walks this. */
typedef struct mig_subsystem {
	mig_server_routine_t		server;
	mach_msg_id_t			start;
	mach_msg_id_t			end;
	mach_msg_size_t			maxsize;
	void				*reserved;
	mig_routine_descriptor		routine[1];	/* variable length */
} *mig_subsystem_t;

#define MIG_ROUTINE_ARG_DESCRIPTOR_NULL	((void *)0)
#define MIG_SUBSYSTEM_NULL		((mig_subsystem_t)0)

#endif /* !_MACH_MIG_H_ */
