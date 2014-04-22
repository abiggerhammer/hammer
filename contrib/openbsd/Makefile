# $OpenBSD: Makefile.template,v 1.67 2013/07/10 15:22:23 bcallah Exp $
COMMENT =		parser combinators in C

HAMMER_VERSION=1.0.0-rc3
DISTNAME =		v${HAMMER_VERSION}
PKGNAME =		hammer-${HAMMER_VERSION}

# for any port that creates shared libraries.
# both libtool and cmake handle this automatically.
# for libtool-based ports, have a look at WRKBUILD/shared_libs.log
#SHARED_LIBS =		???

CATEGORIES =		devel
MODULES=		devel/scons

HOMEPAGE =		https://github.com/UpstandingHackers/hammer

MAINTAINER =		TQ Hirsch <thequux@upstandinghackers.com>

# License GPLv2
PERMIT_PACKAGE_CDROM =	Yes
PERMIT_PACKAGE_FTP =	Yes
PERMIT_DISTFILES_FTP =	Yes

# "make port-lib-depends-check" can help
#WANTLIB =		???

# where the source files and patches can be fetched
#
MASTER_SITES =		https://github.com/UpstandingHackers/hammer/archive/
DIST_SUBDIR =		hammer-${HAMMER_VERSION}

WRKDIST=		${WRKDIR}/${PKGNAME}

#MODULES =		???
TEST_DEPENDS = 		devel/glib2
#BUILD_DEPENDS =		scons
#RUN_DEPENDS =		???
#LIB_DEPENDS =		???
#TEST_DEPENDS =		???

#MAKE_FLAGS =		???
#DESTDIRNAME = 		DESTIR_IGNOREME
#FAKE_FLAGS =		???
#TEST_FLAGS =		???

# build/configuration variables
#
#MODSCONS_ENV+=		DESTDIR=${DESTDIR} prefix=${PREFIX}
MODSCONS_ENV+=		prefix=${TRUEPREFIX}
MODSCONS_OPTS+=		--variant=opt

#ALL_TARGET =		???
#INSTALL_TARGET =	???
#TEST_TARGET =		???

MODSCONS_TEST_TARGET = \
	${SETENV} ${MAKE_ENV} ${MODSCONS_BIN} -C ${WRKSRC} \
		${MODSCONS_ENV} ${MODSCONS_FLAGS} ${TEST_TARGET}

do-test:
	@$(MODSCONS_TEST_TARGET)


.include <bsd.port.mk>
