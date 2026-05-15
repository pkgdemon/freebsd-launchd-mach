/*
 * mach/std_types.h — Apple-canonical "standard Mach types" header.
 *
 * Apple-source code does `#include <mach/std_types.h>` for the
 * machine-word-sized integer aliases (natural_t / integer_t) plus
 * boolean_t / kern_return_t / mach_port_t. migcom uses natural_t
 * (sizeof, word-alignment math); MIG-generated stubs use the full
 * set. We pull the boolean / kern_return typedefs from their
 * canonical headers and add the integer aliases here.
 *
 * natural_t / integer_t are 32-bit by Mach convention regardless of
 * the host word size — they describe the *Mach message wire*, not
 * the C ABI. Matches Apple's <mach/i386/vm_types.h>.
 */
#ifndef _MACH_STD_TYPES_H_
#define _MACH_STD_TYPES_H_

#include <stdint.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>

#ifndef _NATURAL_T_DEFINED
#define _NATURAL_T_DEFINED
typedef unsigned int natural_t;
typedef int          integer_t;
#endif

/*
 * pointer_t — Apple's <mach/std_types.h> defines it as a typedef for
 * vm_offset_t (a word-sized "byte pointer over the wire" used by MIG
 * for variable-length OOL data). MIG-generated mach_excUser.c
 * references it. void* matches the same size and gives the right
 * generic-pointer semantics for our uses.
 */
#ifndef _POINTER_T_DEFINED
#define _POINTER_T_DEFINED
typedef void *pointer_t;
#endif

/*
 * Exception-type vocabulary. mach_exc.defs declares these as MIG
 * types; the MIG-generated mach_exc.h / mach_excUser.c references
 * them but doesn't pull in <mach/exception.h>. Define here behind
 * shared guards so <mach/exception.h> can re-state them without
 * conflict.
 */
#ifndef _EXCEPTION_TYPE_T_DEFINED
#define _EXCEPTION_TYPE_T_DEFINED
typedef int exception_type_t;
#endif
#ifndef _MACH_EXCEPTION_DATA_T_DEFINED
#define _MACH_EXCEPTION_DATA_T_DEFINED
typedef int64_t				mach_exception_data_type_t;
typedef mach_exception_data_type_t     *mach_exception_data_t;
typedef mach_exception_data_type_t      mach_exception_code_t;
typedef mach_exception_data_type_t      mach_exception_subcode_t;
#endif

#endif /* !_MACH_STD_TYPES_H_ */
