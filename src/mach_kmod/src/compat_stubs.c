/*
 * compat_stubs.c — out-of-tree stub implementations for Mach functions
 * that the MIG-generated *_server.c files reference but ravynOS's
 * compat/mach/ source tree doesn't actually define.
 *
 * Apple's authoritative Mach source has these in osfmk/vm/vm_user.c
 * (and friends). ravynOS ported only osfmk/ipc/ — the VM subsystem
 * stayed inside Apple's tree. So the MIG stubs in our *_server.c files
 * reference functions that nobody in our build implements; kldload
 * fails with `symbol X undefined`.
 *
 * Stubs here return KERN_NOT_SUPPORTED. They satisfy the linker so the
 * module loads; any Mach client that actually invokes these RPCs will
 * see a clean error rather than crashing.
 *
 * The stubs are declared (void) so the compile of this file doesn't
 * depend on the full Mach argument-type universe. At link time only
 * symbol names are matched — the MIG stubs' callers pass arguments
 * per the proper signature, but our (void)-bodied stubs ignore the
 * stack/register state and return immediately. On amd64 the caller
 * manages its own stack frame, so this is safe.
 *
 * Real implementations are out of Phase B scope. The smoke test only
 * needs kldload + kldstat -m mach. Discovered by cross-referencing
 * MIG extern declarations against actually-defined functions in our
 * build tree (only entries in *_server.c that are extern declared but
 * never have a matching definition land here).
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/mach/kern_return.h>
#include <sys/mach/mach_types.h>

#define	MACH_STUB(name)							\
	kern_return_t name(void);					\
	kern_return_t name(void) { return (KERN_NOT_SUPPORTED); }

/* From host_priv_server.c / host_server.c */
MACH_STUB(host_get_UNDServer)
MACH_STUB(host_set_UNDServer)
MACH_STUB(host_statistics64)
MACH_STUB(vm_wire)

/* From mach_vm_server.c — 64-bit-address Mach VM RPCs */
MACH_STUB(mach_make_memory_entry_64)
MACH_STUB(mach_memory_object_memory_entry_64)
MACH_STUB(mach_vm_region_info_64)

/* From task_server.c */
MACH_STUB(task_resume2)
MACH_STUB(task_suspend2)

/* From vm_map_server.c — 32-bit-address Mach VM RPCs */
MACH_STUB(vm_allocate_cpm)
MACH_STUB(vm_behavior_set)
MACH_STUB(vm_copy)
MACH_STUB(vm_inherit)
MACH_STUB(vm_machine_attribute)
MACH_STUB(vm_map_page_query)
MACH_STUB(vm_mapped_pages_info)
MACH_STUB(vm_msync)
MACH_STUB(vm_protect)
MACH_STUB(vm_purgable_control)
MACH_STUB(vm_read)
MACH_STUB(vm_read_overwrite)
MACH_STUB(vm_region)
MACH_STUB(vm_region_64)
MACH_STUB(vm_region_recurse)
MACH_STUB(vm_region_recurse_64)
MACH_STUB(vm_remap)
MACH_STUB(vm_write)
