/*
 * mach/host_info.h — host_info() / host_statistics() flavor codes and
 * the result structures. Apple's macOS ships a large matrix of host
 * info flavors; freebsd-launchd-mach only needs what launchd-842
 * consults, which is HOST_VM_INFO (runtime.c's memory-pressure
 * sampler reads struct vm_statistics via host_statistics()).
 *
 * The struct vm_statistics layout matches Apple's classic
 * <mach/vm_statistics.h> definition. host_statistics() in
 * libsystem_kernel fills it from FreeBSD's VM sysctls; the field
 * count (HOST_VM_INFO_COUNT) is what both sides agree on.
 */
#ifndef _MACH_HOST_INFO_H_
#define _MACH_HOST_INFO_H_

#include <mach/mach_types.h>	/* host_t, natural_t, integer_t */
#include <mach/message.h>	/* mach_msg_type_number_t */

typedef integer_t	*host_info_t;
typedef integer_t	*host_info64_t;
typedef natural_t	host_flavor_t;

/* host_info() / host_statistics() flavors (Apple-canonical values). */
#define HOST_BASIC_INFO		1
#define HOST_VM_INFO		2
#define HOST_SCHED_INFO		3
#define HOST_LOAD_INFO		1	/* legacy alias */
#define HOST_CPU_LOAD_INFO	3	/* legacy alias */

struct vm_statistics {
	natural_t	free_count;
	natural_t	active_count;
	natural_t	inactive_count;
	natural_t	wire_count;
	natural_t	zero_fill_count;
	natural_t	reactivations;
	natural_t	pageins;
	natural_t	pageouts;
	natural_t	faults;
	natural_t	cow_faults;
	natural_t	lookups;
	natural_t	hits;
	natural_t	purgeable_count;
	natural_t	purges;
	natural_t	speculative_count;
};
typedef struct vm_statistics	*vm_statistics_t;
typedef struct vm_statistics	vm_statistics_data_t;

#define HOST_VM_INFO_COUNT \
	((mach_msg_type_number_t)(sizeof(vm_statistics_data_t) / sizeof(integer_t)))

#endif /* !_MACH_HOST_INFO_H_ */
