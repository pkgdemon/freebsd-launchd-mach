/*
 * hwregd.c — freebsd-launchd-mach hardware registry daemon.
 *
 * Phase 0 / iter 1: minimum viable. Open /dev/devctl, read kernel
 * device events line-by-line, log each one to stderr. No parsing,
 * no kldload, no Mach IPC yet -- those land in subsequent iters
 * once we know the daemon survives boot under PID-1 launchd.
 *
 * Eventual scope (Phase 0): lift sbin/devmatch/devmatch.c's
 * search_hints() parser; kldload(2) on every ?nomatch event;
 * publish +attach/-detach/!notify system=IFNET events over a simple
 * Mach-RPC pub/sub. Then Phase 1 builds the full IORegistry tree
 * + IOKit-shape query API on top.
 *
 * Plan: https://pkgdemon.github.io/freebsd-hardware-registry-iokit-plan.html
 *
 * Event line format (from sys/kern/kern_devctl.c devctl_notify +
 * devaddq, observed live via `cat /dev/devctl` on stock FreeBSD):
 *
 *   +em0 at bus=0x0 slot=0x19 function=0x0 on pci0
 *   -em0 at ... on pci0
 *   ?at bus=0x... on pci0
 *   !system=IFNET subsystem=em0 type=LINK_UP
 *   !system=ACPI subsystem=Lid type=close
 *
 * First byte is the event class (+attach, -detach, ?nomatch,
 * !notify); rest is space-separated key=value pairs.
 */

#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DEVCTL_PATH		"/dev/devctl"
#define READ_BUF_SIZE		(64 * 1024)

static volatile sig_atomic_t got_term = 0;

static void
on_signal(int sig)
{
	got_term = sig;
}

static void
xlog(const char *fmt, ...)
{
	struct timespec ts;
	char tbuf[32];

	(void)clock_gettime(CLOCK_REALTIME, &ts);
	{
		struct tm tm;
		(void)gmtime_r(&ts.tv_sec, &tm);
		(void)strftime(tbuf, sizeof(tbuf),
		    "%Y-%m-%dT%H:%M:%SZ", &tm);
	}
	(void)fprintf(stderr, "hwregd %s ", tbuf);
	{
		va_list ap;
		va_start(ap, fmt);
		(void)vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	(void)fputc('\n', stderr);
	(void)fflush(stderr);
}

static int
open_devctl(void)
{
	int fd = open(DEVCTL_PATH, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		xlog("open(%s) failed: %s", DEVCTL_PATH, strerror(errno));
	}
	return fd;
}

/*
 * Read whatever's pending on /dev/devctl into buf, split into
 * '\n'-terminated lines, log each one. Returns number of bytes
 * read on success, -1 on read failure (caller should retry or
 * give up), 0 on EOF (shouldn't happen for /dev/devctl but treat
 * as fatal).
 */
static ssize_t
drain_devctl(int fd, char *buf, size_t bufsz)
{
	ssize_t n = read(fd, buf, bufsz - 1);
	if (n < 0) {
		if (errno == EINTR || errno == EAGAIN) {
			return 0;	/* retry */
		}
		xlog("read(%s) failed: %s", DEVCTL_PATH, strerror(errno));
		return -1;
	}
	if (n == 0) {
		xlog("read(%s) returned EOF", DEVCTL_PATH);
		return 0;
	}
	buf[n] = '\0';

	/* Split on '\n'. Each devctl line is one event. */
	char *p = buf;
	char *end = buf + n;
	while (p < end) {
		char *nl = memchr(p, '\n', (size_t)(end - p));
		size_t linelen = nl ? (size_t)(nl - p) : (size_t)(end - p);
		if (linelen > 0) {
			/* iter-1: just log it. Future iters parse + act. */
			xlog("event[%c]: %.*s",
			    p[0] ? p[0] : '?', (int)linelen, p);
		}
		if (!nl) {
			break;
		}
		p = nl + 1;
	}
	return n;
}

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	xlog("starting (Phase 0 iter 1: read-only /dev/devctl logger)");

	/* Clean shutdown on SIGTERM / SIGINT. launchd sends SIGTERM
	 * during shutdown; the default action is exit() which is fine,
	 * but installing handlers gives us a chance to log the reason
	 * and flush stderr. */
	{
		struct sigaction sa = { .sa_handler = on_signal };
		sigemptyset(&sa.sa_mask);
		(void)sigaction(SIGTERM, &sa, NULL);
		(void)sigaction(SIGINT, &sa, NULL);
	}

	int fd = open_devctl();
	if (fd < 0) {
		return 1;
	}
	xlog("opened %s fd=%d", DEVCTL_PATH, fd);

	static char buf[READ_BUF_SIZE];

	while (!got_term) {
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
		int r = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (r < 0) {
			if (errno == EINTR) {
				continue;
			}
			xlog("select failed: %s", strerror(errno));
			break;
		}
		if (r == 0) {
			/* idle tick; just loop. Future iters might use
			 * this to poll for sysctl-derived state changes
			 * (thermal, link-state) that aren't pushed via
			 * devctl. */
			continue;
		}
		if (FD_ISSET(fd, &rfds)) {
			ssize_t n = drain_devctl(fd, buf, sizeof(buf));
			if (n < 0) {
				break;
			}
		}
	}

	xlog("shutting down (signal=%d)", (int)got_term);
	(void)close(fd);
	return 0;
}
