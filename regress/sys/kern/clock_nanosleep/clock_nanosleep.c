/*	$OpenBSD$	*/
/*
 * Copyright (c) 2026 Brad Smith <brad@comstyle.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static const clockid_t clocks[] = {
	CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_BOOTTIME, CLOCK_UPTIME
};

static void
handler(int sig)
{
}

static void
check(int ret, int want, const char *what)
{
	if (ret != want)
		errx(1, "%s: returned %d (%s), want %d", what, ret,
		    ret > 0 ? strerror(ret) : "", want);
}

static void
test_relative(clockid_t clock)
{
	struct timespec before, after, diff, req = { 0, 100000000 };

	if (clock_gettime(clock, &before) == -1)
		err(1, "clock_gettime");
	check(clock_nanosleep(clock, 0, &req, NULL), 0, "relative");
	if (clock_gettime(clock, &after) == -1)
		err(1, "clock_gettime");
	timespecsub(&after, &before, &diff);
	if (timespeccmp(&diff, &req, <))
		errx(1, "relative: clock %d woke after %lld.%09ld", clock,
		    (long long)diff.tv_sec, diff.tv_nsec);
}

static void
test_absolute(clockid_t clock)
{
	struct timespec now, req, interval = { 0, 100000000 };

	if (clock_gettime(clock, &now) == -1)
		err(1, "clock_gettime");
	timespecadd(&now, &interval, &req);
	check(clock_nanosleep(clock, TIMER_ABSTIME, &req, NULL), 0,
	    "absolute");
	if (clock_gettime(clock, &now) == -1)
		err(1, "clock_gettime");
	if (timespeccmp(&now, &req, <))
		errx(1, "absolute: clock %d woke at %lld.%09ld before "
		    "%lld.%09ld", clock, (long long)now.tv_sec, now.tv_nsec,
		    (long long)req.tv_sec, req.tv_nsec);

	/* A time in the past returns immediately. */
	req.tv_sec = 0;
	req.tv_nsec = 0;
	check(clock_nanosleep(clock, TIMER_ABSTIME, &req, NULL), 0,
	    "absolute past");
	req.tv_sec = -1;
	check(clock_nanosleep(clock, TIMER_ABSTIME, &req, NULL), 0,
	    "absolute negative");
}

static void
test_errors(void)
{
	struct timespec req = { 0, 1000 };
	struct timespec bad = { 0, 1000000000 };
	struct timespec neg = { -1, 0 };
	clockid_t clock;

	errno = 0;
	check(clock_nanosleep(CLOCK_MONOTONIC, 0, &bad, NULL), EINVAL,
	    "bad nsec");
	if (errno != 0)
		errx(1, "errno changed to %d", errno);
	check(clock_nanosleep(CLOCK_MONOTONIC, 0, &neg, NULL), EINVAL,
	    "negative relative");
	check(clock_nanosleep(CLOCK_MONOTONIC, 0x2, &req, NULL), EINVAL,
	    "bad flags");
	check(clock_nanosleep(12345, 0, &req, NULL), EINVAL, "bad clock");
	check(clock_nanosleep(CLOCK_THREAD_CPUTIME_ID, 0, &req, NULL),
	    EINVAL, "own thread cputime");
	check(clock_nanosleep(CLOCK_PROCESS_CPUTIME_ID, 0, &req, NULL),
	    ENOTSUP, "process cputime");
	if (pthread_getcpuclockid(pthread_self(), &clock) != 0)
		errx(1, "pthread_getcpuclockid");
	check(clock_nanosleep(clock, 0, &req, NULL), EINVAL,
	    "own pthread cputime");
	check(clock_nanosleep(CLOCK_MONOTONIC, 0, NULL, NULL), EFAULT,
	    "NULL request");
}

static void
test_eintr(void)
{
	struct sigaction sa;
	struct timespec req = { 10, 0 }, rem = { -1, -1 }, now;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	if (sigaction(SIGALRM, &sa, NULL) == -1)
		err(1, "sigaction");

	alarm(1);
	check(clock_nanosleep(CLOCK_MONOTONIC, 0, &req, &rem), EINTR,
	    "relative EINTR");
	if (rem.tv_sec < 0 || timespeccmp(&rem, &req, >=))
		errx(1, "relative EINTR: bad remainder %lld.%09ld",
		    (long long)rem.tv_sec, rem.tv_nsec);

	/* The remainder is not touched for an absolute sleep. */
	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
		err(1, "clock_gettime");
	timespecadd(&now, &req, &req);
	rem.tv_sec = rem.tv_nsec = -1;
	alarm(1);
	check(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &req, &rem),
	    EINTR, "absolute EINTR");
	if (rem.tv_sec != -1 || rem.tv_nsec != -1)
		errx(1, "absolute EINTR: remainder was written");
}

int
main(void)
{
	size_t i;

	for (i = 0; i < sizeof(clocks) / sizeof(clocks[0]); i++) {
		test_relative(clocks[i]);
		test_absolute(clocks[i]);
	}
	test_errors();
	test_eintr();

	if (sysconf(_SC_CLOCK_SELECTION) <= 0)
		errx(1, "_SC_CLOCK_SELECTION not set");

	return 0;
}
