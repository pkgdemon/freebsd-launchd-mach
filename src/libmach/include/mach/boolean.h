/*
 * mach/boolean.h — Apple-canonical boolean type header.
 *
 * Apple-source code (migcom, MIG-generated stubs, launchd-842) does
 * `#include <mach/boolean.h>` for boolean_t. We already define
 * boolean_t in <mach/message.h> under the _BOOLEAN_T_DEFINED guard;
 * this header is the canonical standalone home so the include
 * resolves on its own.
 */
#ifndef _MACH_BOOLEAN_H_
#define _MACH_BOOLEAN_H_

#ifndef _BOOLEAN_T_DEFINED
#define _BOOLEAN_T_DEFINED
typedef unsigned int boolean_t;
#endif

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#endif /* !_MACH_BOOLEAN_H_ */
