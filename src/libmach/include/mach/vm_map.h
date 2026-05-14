/*
 * mach/vm_map.h — minimal userland VM-map surface.
 *
 * Apple's <mach/vm_map.h> is the full VM RPC interface (vm_allocate,
 * vm_deallocate, vm_read, vm_map, ...). launchd-842's liblaunch only
 * reaches for vm_deallocate (freeing MIG out-of-line buffers) and the
 * mach_vm_address_t type. We provide just that surface; the rest of
 * Apple's VM RPC is not wired into mach.ko and would need real kernel
 * work to support.
 */
#ifndef _MACH_VM_MAP_H_
#define _MACH_VM_MAP_H_

#include <stdint.h>
#include <mach/mach_traps.h>	/* mach_port_name_t */
#include <mach/kern_return.h>	/* kern_return_t */
#include <mach/std_types.h>	/* natural_t */

#ifdef __cplusplus
extern "C" {
#endif

/* VM address / size types. Apple scales these to the address space;
 * on our 64-bit-only target they are flat uint64_t / unsigned long. */
#ifndef _VM_TYPES_DEFINED
#define _VM_TYPES_DEFINED
typedef unsigned long	vm_address_t;
typedef unsigned long	vm_size_t;
typedef unsigned long	vm_offset_t;
typedef uint64_t	mach_vm_address_t;
typedef uint64_t	mach_vm_size_t;
typedef uint64_t	mach_vm_offset_t;
#endif

typedef mach_port_name_t vm_map_t;

/*
 * vm_deallocate — release a region of a task's VM. liblaunch calls
 * this to free the out-of-line memory MIG hands back from a reply.
 * mach.ko does not implement the VM RPC subsystem yet; the libmach
 * userland stub returns KERN_NOT_SUPPORTED so callers fail-soft
 * rather than link-fail. Promote to a real trap when OOL message
 * memory actually needs reclaiming.
 */
kern_return_t vm_deallocate(vm_map_t target, vm_address_t address,
    vm_size_t size);

kern_return_t vm_allocate(vm_map_t target, vm_address_t *address,
    vm_size_t size, int flags);

#ifdef __cplusplus
}
#endif

#endif /* !_MACH_VM_MAP_H_ */
