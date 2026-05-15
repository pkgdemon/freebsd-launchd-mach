/*
 * mach/mig_errors.h — MIG runtime error codes + reply-error struct.
 *
 * Every MIG-generated stub (.c) and header (.h) pulls this in: the
 * MIG_* codes are what a stub returns on a marshalling failure, and
 * mig_reply_error_t is the wire shape of a generic error reply.
 *
 * Adapted from ravynOS sys/sys/mach/mig_errors.h — kernel
 * <sys/mach/...> includes rewritten to our flat <mach/> layout, the
 * kernel-only <sys/mach/rpc.h> dropped (the rpc.h types aren't used
 * by userland MIG output), <sys/mach/vm_types.h> replaced with
 * <mach/vm_map.h> for vm_size_t.
 *
 * NB: mig_reply_error_t here carries the real `NDR_record_t NDR`
 * field — earlier a placeholder `int NDR` version lived in
 * <mach/message.h>; that's been removed in favor of this canonical
 * definition. The MIG runtime functions (mig_get_reply_port,
 * mig_user_allocate, ...) are declared here but not yet implemented;
 * the generated stubs reference them and the symbols resolve when a
 * MIG runtime lands.
 */
#ifndef _MACH_MIG_ERRORS_H_
#define _MACH_MIG_ERRORS_H_

#include <mach/kern_return.h>
#include <mach/message.h>
#include <mach/ndr.h>
#include <mach/vm_map.h>	/* vm_size_t */

#define MIG_TYPE_ERROR		-300	/* client type check failure */
#define MIG_REPLY_MISMATCH	-301	/* wrong reply message ID */
#define MIG_REMOTE_ERROR	-302	/* server detected error */
#define MIG_BAD_ID		-303	/* bad request message ID */
#define MIG_BAD_ARGUMENTS	-304	/* server type check failure */
#define MIG_NO_REPLY		-305	/* no reply should be sent */
#define MIG_EXCEPTION		-306	/* server raised exception */
#define MIG_ARRAY_TOO_LARGE	-307	/* array not large enough */
#define MIG_SERVER_DIED		-308	/* server died */
#define MIG_TRAILER_ERROR	-309	/* trailer has an unknown format */

#pragma pack(4)
typedef struct {
	mach_msg_header_t	Head;
	NDR_record_t		NDR;
	kern_return_t		RetCode;
} mig_reply_error_t;
#pragma pack()

typedef struct mig_symtab {
	char	*ms_routine_name;
	int	ms_routine_number;
	void	(*ms_routine)(void);	/* unknown signature — best we can do */
} mig_symtab_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Client-side reply-port management (MIG runtime — not yet wired). */
extern mach_port_t mig_get_reply_port(void);
extern void mig_dealloc_reply_port(mach_port_t reply_port);
extern void mig_put_reply_port(mach_port_t reply_port);

/* Out-of-stack MIG buffer management (MIG runtime — not yet wired). */
extern char *mig_user_allocate(vm_size_t size);
extern void mig_user_deallocate(char *data, vm_size_t size);

/*
 * mig_allocate / mig_deallocate — Apple-shape OOL-data hooks the
 * generated stubs call to obtain/release a variable-length buffer.
 * Distinct from mig_user_allocate; matches Apple's <mach/mig.h>.
 */
extern kern_return_t mig_allocate(vm_address_t *addr, vm_size_t size);
extern kern_return_t mig_deallocate(vm_address_t addr, vm_size_t size);

/* Bounded string copy used by generated string-argument stubs. */
extern int mig_strncpy(char *dest, const char *src, int len);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_MIG_ERRORS_H_ */
