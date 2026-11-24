/*	$OpenBSD$ */
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

#include <errno.h>
#include <time.h>
#include "cancel.h"

/*
 * Unlike most system calls, clock_nanosleep() returns the error
 * number instead of setting errno.
 */
int
clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *rqtp,
    struct timespec *rmtp)
{
	int ret, saved_errno;

	saved_errno = errno;
	ENTER_CANCEL_POINT(1);
	ret = HIDDEN(clock_nanosleep)(clock_id, flags, rqtp, rmtp);
	LEAVE_CANCEL_POINT(1);
	if (ret == -1) {
		ret = errno;
		errno = saved_errno;
	}
	return (ret);
}
DEF_CANCEL(clock_nanosleep);
