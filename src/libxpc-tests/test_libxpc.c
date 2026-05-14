/*
 * test_libxpc — Phase H2 smoke check for libxpc.
 *
 * Validates the vendored libxpc (src/libxpc, ported from ravynOS
 * lib/libxpc/) actually links + works at runtime. Stays inside the
 * type-system layer — no connections, no bootstrap, no Mach IPC — so
 * a failure here means the libxpc.so itself is broken, not the lower
 * Mach stack. Connection + bootstrap surface lands in a follow-up
 * test once xpc_connection_create over our libbootstrap is wired.
 *
 * Checks:
 *   1. xpc_dictionary_create with no initial keys returns non-NULL.
 *   2. xpc_dictionary_set_string / _get_string round-trips a value.
 *   3. xpc_dictionary_set_int64 / _get_int64 round-trips an integer.
 *   4. xpc_release on the dictionary doesn't crash.
 *
 * Exit codes:
 *   0 — all checks pass
 *   1 — xpc_dictionary_create returned NULL
 *   2 — string round-trip mismatch
 *   3 — int64 round-trip mismatch
 */
#include <stdio.h>
#include <string.h>
#include <xpc/xpc.h>

int
main(void)
{
	xpc_object_t d = xpc_dictionary_create(NULL, NULL, 0);
	if (d == NULL) {
		printf("FAIL: xpc_dictionary_create returned NULL\n");
		return 1;
	}

	xpc_dictionary_set_string(d, "k_str", "hello-xpc");
	const char *s = xpc_dictionary_get_string(d, "k_str");
	if (s == NULL || strcmp(s, "hello-xpc") != 0) {
		printf("FAIL: string round-trip: got '%s' want 'hello-xpc'\n",
		    s ? s : "(null)");
		xpc_release(d);
		return 2;
	}

	xpc_dictionary_set_int64(d, "k_int", 0x7fffffffabcd1234LL);
	int64_t v = xpc_dictionary_get_int64(d, "k_int");
	if (v != 0x7fffffffabcd1234LL) {
		printf("FAIL: int64 round-trip: got 0x%llx want 0x7fffffffabcd1234\n",
		    (unsigned long long)v);
		xpc_release(d);
		return 3;
	}

	xpc_release(d);
	printf("LIBXPC-OK: dictionary string + int64 round-trip succeeded\n");
	return 0;
}
