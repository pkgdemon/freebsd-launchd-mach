/*
 * compat_stubs.c — out-of-tree stub implementations for Mach functions
 * that the MIG-generated *_server.c files reference but ravynOS's
 * compat/mach/ source tree doesn't actually define.
 *
 * Apple's authoritative Mach source has these in osfmk/vm/vm_user.c
 * (and friends). ravynOS ported only osfmk/ipc/ — the VM subsystem
 * stayed inside Apple's tree. So the MIG stubs in our host_priv_server.c
 * and host_server.c reference functions that nobody in our build
 * implements; the kld_load() fails with `symbol X undefined`.
 *
 * Stubs here return KERN_NOT_SUPPORTED. They satisfy the linker so the
 * module loads; any Mach client that actually invokes these RPCs will
 * see a clean error rather than crashing.
 *
 * Real implementations are out of Phase B scope. The smoke test only
 * needs kldload + kldstat -m mach.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/mach/kern_return.h>
#include <sys/mach/mach_types.h>

/*
 * Apple Mach RPC: vm_wire(host_priv, task, address, size, desired_access).
 * Wires the given range of a task's virtual memory.
 * Referenced by: host_priv_server.c MIG stub.
 */
kern_return_t
vm_wire(host_priv_t host_priv, vm_map_t task, vm_address_t address,
    vm_size_t size, vm_prot_t desired_access)
{

	(void)host_priv; (void)task; (void)address;
	(void)size; (void)desired_access;
	return (KERN_NOT_SUPPORTED);
}

/* mach_vm_wire is already defined in src/mach_vm.c — no stub needed. */
