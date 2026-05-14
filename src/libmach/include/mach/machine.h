/*
 * mach/machine.h — minimal CPU type/subtype declarations.
 *
 * Apple's <mach/machine.h> is the full cpu_type_t / cpu_subtype_t
 * taxonomy (every CPU_TYPE_* / CPU_SUBTYPE_* the loader knows about).
 * launchd-842's launch_priv.h only needs the cpu_type_t typedef —
 * the spawn_binpref field in launch_data_t's posix_spawn-attr struct
 * is a `const cpu_type_t *` array. We provide just the integer
 * typedefs; the full CPU_TYPE_* enum can land if a real consumer
 * (e.g. a fat-binary code path) ever needs it — our ELF-only port
 * has no fat binaries, so binpref is effectively unused.
 */
#ifndef _MACH_MACHINE_H_
#define _MACH_MACHINE_H_

#include <stdint.h>

typedef int		cpu_type_t;
typedef int		cpu_subtype_t;
typedef int		cpu_threadtype_t;

#define CPU_TYPE_ANY		((cpu_type_t)-1)
#define CPU_SUBTYPE_MULTIPLE	((cpu_subtype_t)-1)

#endif /* !_MACH_MACHINE_H_ */
