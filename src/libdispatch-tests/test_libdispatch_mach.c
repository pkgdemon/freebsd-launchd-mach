/*
 * test_libdispatch_mach — verify Mach source-type symbols exist.
 *
 * STUB-level test (this commit). Confirms:
 *   1. libsystem_dispatch.so exports the DISPATCH_SOURCE_TYPE_MACH_RECV
 *      and _SEND symbols (link-time check — if they were missing this
 *      binary wouldn't link).
 *   2. dispatch_source_create with MACH_RECV returns NULL gracefully
 *      (our stub dst_create returns DISPATCH_UNOTE_NULL, source.c
 *      handles that and returns NULL to the caller). No crash.
 *
 * Real round-trip Mach IPC verification (allocate port, send self
 * message, handler fires) lands in a follow-up commit alongside the
 * real polling-thread receive implementation.
 *
 * Exit codes:
 *   0 — symbols exist + dispatch_source_create returned NULL safely
 *   1 — dispatch_source_create returned non-NULL with stub backend
 *       (unexpected — would mean someone wired up a real impl; update
 *       this test)
 */
#include <stdio.h>
#include <dispatch/dispatch.h>

int
main(void)
{
	/* Reference the symbols by taking their addresses — link-time check
	 * that libsystem_dispatch.so exports them. If missing, we'd get a
	 * link error before this binary builds. */
	const void *recv_type = DISPATCH_SOURCE_TYPE_MACH_RECV;
	const void *send_type = DISPATCH_SOURCE_TYPE_MACH_SEND;

	if (recv_type == NULL) {
		printf("FAIL: DISPATCH_SOURCE_TYPE_MACH_RECV symbol resolves to NULL\n");
		return 1;
	}
	if (send_type == NULL) {
		printf("FAIL: DISPATCH_SOURCE_TYPE_MACH_SEND symbol resolves to NULL\n");
		return 1;
	}
	printf("symbols present: MACH_RECV=%p MACH_SEND=%p\n", recv_type, send_type);

	/* dispatch_source_create with MACH_RECV. With our stub dst_create
	 * returning DISPATCH_UNOTE_NULL, source.c should hand back NULL. */
	dispatch_source_t src = dispatch_source_create(
	    DISPATCH_SOURCE_TYPE_MACH_RECV,
	    1,  /* arbitrary port handle (stub ignores) */
	    0,
	    dispatch_get_main_queue());

	if (src != NULL) {
		printf("FAIL: dispatch_source_create returned %p with stub backend\n",
		    src);
		printf("(stub dst_create should return DISPATCH_UNOTE_NULL — if you\n");
		printf(" just wired up a real Mach backend, update this test to\n");
		printf(" exercise the real round-trip path)\n");
		return 1;
	}

	printf("LIBDISPATCH-MACH-OK: stub Mach backend present, source create returned NULL safely\n");
	return 0;
}
