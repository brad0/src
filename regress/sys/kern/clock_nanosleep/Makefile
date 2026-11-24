#	$OpenBSD$

PROG=	clock_nanosleep
WARNINGS=	yes
LDADD+=		-lpthread
DPADD+=		${LIBPTHREAD}

REGRESS_TARGETS=	run-regress-${PROG}

run-regress-${PROG}: ${PROG}
	./${PROG}

.include <bsd.regress.mk>
