/*
 * uuid/uuid.h — libuuid-API shim for FreeBSD.
 *
 * Apple (and Linux/e2fsprogs) ship <uuid/uuid.h> as the *libuuid*
 * API: uuid_t is `unsigned char[16]` and the functions are
 * uuid_generate / uuid_is_null / uuid_compare / uuid_parse /
 * uuid_unparse, etc. This is distinct from FreeBSD's native
 * <uuid.h>, which is the DCE API (uuid_t is `struct uuid`, functions
 * are uuid_create / uuid_to_string / uuid_from_string).
 *
 * launchd-842's liblaunch.c needs the libuuid API (uuid_generate,
 * uuid_is_null); libxpc's xpc_type.c / xpc_internal.h use uuid_t as
 * a 16-byte blob. Neither wants the DCE API. So this shim *is* a
 * minimal libuuid implementation — the functions are tiny byte-array
 * operations (compare = memcmp, clear = memset, ...) and uuid_generate
 * pulls randomness from arc4random_buf(3), which FreeBSD's libc
 * provides.
 *
 * The uuid_t typedef is guarded with _UUID_T_DEFINED — the same guard
 * FreeBSD's <sys/uuid.h> uses — so if a translation unit pulls in
 * both this header and FreeBSD's <uuid.h>, whichever lands first
 * wins and the other's typedef is suppressed. `struct uuid` (the DCE
 * struct) is a different name and is unaffected; code that needs the
 * DCE API can still include <uuid.h> for it.
 */
#ifndef _UUID_UUID_H_SHIM_
#define _UUID_UUID_H_SHIM_

#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>		/* arc4random_buf */
#include <stdio.h>		/* snprintf for uuid_unparse */

#ifndef _UUID_T_DEFINED
#define _UUID_T_DEFINED
typedef unsigned char uuid_t[16];
#endif

/* uuid_string_t — the 36-char canonical form + NUL. */
#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef char uuid_string_t[37];
#endif

static __inline void
uuid_clear(uuid_t uu)
{
	memset(uu, 0, 16);
}

static __inline void
uuid_copy(uuid_t dst, const uuid_t src)
{
	memcpy(dst, src, 16);
}

static __inline int
uuid_compare(const uuid_t a, const uuid_t b)
{
	return memcmp(a, b, 16);
}

static __inline int
uuid_is_null(const uuid_t uu)
{
	int i;
	for (i = 0; i < 16; i++)
		if (uu[i] != 0)
			return 0;
	return 1;
}

/*
 * uuid_generate — RFC 4122 version-4 (random) UUID. arc4random_buf
 * fills all 16 bytes; we then stamp the version (4) and variant (10x)
 * bits. uuid_generate_random is an alias; uuid_generate_time falls
 * back to the same here (no MAC-address / timestamp variant).
 */
static __inline void
uuid_generate_random(uuid_t out)
{
	arc4random_buf(out, 16);
	out[6] = (unsigned char)((out[6] & 0x0f) | 0x40);	/* version 4 */
	out[8] = (unsigned char)((out[8] & 0x3f) | 0x80);	/* variant 10x */
}

static __inline void
uuid_generate(uuid_t out)
{
	uuid_generate_random(out);
}

static __inline void
uuid_generate_time(uuid_t out)
{
	uuid_generate_random(out);
}

static __inline void
uuid_unparse_lower(const uuid_t uu, char *out)
{
	(void)snprintf(out, 37,
	    "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	    uu[0], uu[1], uu[2], uu[3], uu[4], uu[5], uu[6], uu[7],
	    uu[8], uu[9], uu[10], uu[11], uu[12], uu[13], uu[14], uu[15]);
}

static __inline void
uuid_unparse_upper(const uuid_t uu, char *out)
{
	(void)snprintf(out, 37,
	    "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
	    uu[0], uu[1], uu[2], uu[3], uu[4], uu[5], uu[6], uu[7],
	    uu[8], uu[9], uu[10], uu[11], uu[12], uu[13], uu[14], uu[15]);
}

static __inline void
uuid_unparse(const uuid_t uu, char *out)
{
	uuid_unparse_lower(uu, out);
}

/* uuid_parse — "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" -> 16 bytes.
 * Returns 0 on success, -1 on malformed input. */
static __inline int
uuid_parse(const char *in, uuid_t uu)
{
	unsigned int b[16];
	int n, i;

	n = sscanf(in,
	    "%2x%2x%2x%2x-%2x%2x-%2x%2x-%2x%2x-%2x%2x%2x%2x%2x%2x",
	    &b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &b[6], &b[7],
	    &b[8], &b[9], &b[10], &b[11], &b[12], &b[13], &b[14], &b[15]);
	if (n != 16)
		return -1;
	for (i = 0; i < 16; i++)
		uu[i] = (unsigned char)b[i];
	return 0;
}

#endif /* !_UUID_UUID_H_SHIM_ */
