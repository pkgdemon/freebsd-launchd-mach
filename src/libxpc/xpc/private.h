/*
 * xpc/private.h — XPC private/SPI surface for launchd-842.
 *
 * On Apple, <xpc/private.h> is the SPI half of libxpc — the bits not
 * in the public <xpc/xpc.h>. launchd-842's core.h includes it.
 *
 * Crucially, almost everything launchd reaches for under the
 * "xpc-private" banner — the xpc_domain_* / xpc_event_* /
 * xpc_process_* / _xpc_domain_* families — is *defined inside
 * launchd's own core.c*, not provided by libxpc. They're launchd's
 * MIG-handler implementations, forward-declared file-locally. So
 * <xpc/private.h> doesn't need to declare them.
 *
 * What core.h actually needs from this include is the base XPC type
 * vocabulary (xpc_object_t etc.) plus the launchd-facing libxpc API
 * (xpc_pipe_*, xpc_dictionary_get_audit_token, ...). Both already
 * live in our ported libxpc headers, so this header just re-exports
 * them. If a real xpc-private SPI turns out to be needed during the
 * launchd-daemon link (Phase I1c), it gets declared here then.
 */
#ifndef _XPC_PRIVATE_H_
#define _XPC_PRIVATE_H_

#include <xpc/xpc.h>		/* xpc_object_t + the base XPC API */
#include <xpc/launchd.h>	/* xpc_pipe_*, xpc_dictionary_get_audit_token, ... */

#endif /* !_XPC_PRIVATE_H_ */
