/*
 * mach/task_policy.h — task-policy RPC vocabulary. launchd-842's
 * core.c uses just the QoS-tier subset (TASK_BASE_QOS_POLICY +
 * struct task_qos_policy) to push latency/throughput hints into newly
 * spawned tasks.
 *
 * FreeBSD has no Mach task-policy mechanism; task_policy_set() here
 * is a libsystem_kernel stub that swallows the call. Declarations
 * and constants follow Apple's <mach/task_policy.h>.
 */
#ifndef _MACH_TASK_POLICY_H_
#define _MACH_TASK_POLICY_H_

#include <mach/mach_types.h>	/* task_t, integer_t */
#include <mach/message.h>	/* mach_msg_type_number_t */

typedef integer_t	*task_policy_t;
typedef int		task_policy_flavor_t;

#define TASK_CATEGORY_POLICY		1
#define TASK_SUPPRESSION_POLICY		3
#define TASK_POLICY_STATE		4
#define TASK_BASE_QOS_POLICY		8
#define TASK_OVERRIDE_QOS_POLICY	9
#define TASK_BASE_LATENCY_QOS_POLICY	10
#define TASK_BASE_THROUGHPUT_QOS_POLICY	11

struct task_qos_policy {
	integer_t	task_latency_qos_tier;
	integer_t	task_throughput_qos_tier;
};
typedef struct task_qos_policy	task_qos_policy_data_t;
typedef struct task_qos_policy *task_qos_policy_t;

#define TASK_QOS_POLICY_COUNT \
	((mach_msg_type_number_t)(sizeof(struct task_qos_policy) / sizeof(integer_t)))

#ifdef __cplusplus
extern "C" {
#endif

kern_return_t task_policy_set(task_t task,
    task_policy_flavor_t flavor, task_policy_t policy_info,
    mach_msg_type_number_t count);

kern_return_t task_policy_get(task_t task,
    task_policy_flavor_t flavor, task_policy_t policy_info,
    mach_msg_type_number_t *count, boolean_t *get_default);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_TASK_POLICY_H_ */
