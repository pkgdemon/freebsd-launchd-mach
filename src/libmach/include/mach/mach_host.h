/*
 * mach/mach_host.h — host-port query RPCs. On Apple these are
 * MIG-generated stubs to the host port; on freebsd-launchd-mach
 * host_info() / host_statistics() are thin libsystem_kernel wrappers
 * over FreeBSD sysctls. launchd-842's runtime.c uses host_statistics()
 * for its HOST_VM_INFO memory sampler.
 */
#ifndef _MACH_MACH_HOST_H_
#define _MACH_MACH_HOST_H_

#include <mach/mach_types.h>	/* host_t, kern_return_t */
#include <mach/host_info.h>	/* host_flavor_t, host_info_t */
#include <mach/message.h>	/* mach_msg_type_number_t */

#ifdef __cplusplus
extern "C" {
#endif

kern_return_t host_info(host_t host, host_flavor_t flavor,
    host_info_t host_info_out, mach_msg_type_number_t *host_info_outCnt);

kern_return_t host_statistics(host_t host, host_flavor_t flavor,
    host_info_t host_info_out, mach_msg_type_number_t *host_info_outCnt);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_MACH_HOST_H_ */
