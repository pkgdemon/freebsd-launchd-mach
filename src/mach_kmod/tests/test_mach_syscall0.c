/*
 * test_mach_syscall0 — invoke a 0-argument Mach syscall by number.
 *
 * Usage: MACH_SYSCALL_NUM=<n> ./test_mach_syscall0
 *
 * Reads the syscall number from $MACH_SYSCALL_NUM, calls syscall(n),
 * prints the return value. Used by smoke-sysctls.sh to exercise each
 * wired Mach syscall whose number is discovered at runtime from
 * sysctl mach.syscall.<name>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(void)
{
	const char *env = getenv("MACH_SYSCALL_NUM");
	if (env == NULL) {
		fprintf(stderr, "set MACH_SYSCALL_NUM\n");
		return (1);
	}
	int num = atoi(env);
	long ret = syscall(num);
	printf("syscall(%d) returned %ld (0x%lx)\n", num, ret,
	    (unsigned long)ret);
	return (0);
}
