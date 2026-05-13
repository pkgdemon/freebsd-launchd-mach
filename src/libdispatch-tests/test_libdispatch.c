/*
 * test_libdispatch — basic libdispatch smoke check.
 *
 * Confirms libsystem_dispatch.so (vendored swift-corelibs-libdispatch
 * built in our build.sh chroot pipeline) is loadable, links cleanly,
 * and a serial queue executes a synchronous callback. Uses the
 * function-pointer dispatch API (dispatch_sync_f) instead of the block
 * variant (dispatch_sync) so we don't pull a -fblocks / BlocksRuntime
 * link dep into this test binary.
 *
 * Exit codes:
 *   0 — queue created + sync callback ran + counter mutation visible
 *   1 — dispatch_queue_create returned NULL
 *   2 — sync callback didn't run (counter unchanged)
 */
#include <stdio.h>
#include <dispatch/dispatch.h>

static void
set_counter_to_42(void *ctx)
{
	int *counter = (int *)ctx;
	*counter = 42;
}

int
main(void)
{
	dispatch_queue_t q = dispatch_queue_create("test_libdispatch.serial",
	    DISPATCH_QUEUE_SERIAL);
	if (q == NULL) {
		printf("FAIL: dispatch_queue_create returned NULL\n");
		return 1;
	}

	int counter = 0;
	dispatch_sync_f(q, &counter, set_counter_to_42);

	if (counter != 42) {
		printf("FAIL: counter expected 42 got %d\n", counter);
		return 2;
	}

	dispatch_release(q);
	printf("LIBDISPATCH-OK: dispatch_sync_f executed on serial queue\n");
	return 0;
}
