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

#include <mach/boolean.h>
#include <mach/kern_return.h>

#ifndef _NATURAL_T_DEFINED
#define _NATURAL_T_DEFINED
typedef unsigned int natural_t;
typedef int          integer_t;
#endif

#endif /* !_MACH_STD_TYPES_H_ */
