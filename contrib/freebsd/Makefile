# Created by thequux for Upstanding Hackers
# Copyright (c) 2014 Upstanding Hackers
#
# The contents of this file are available under the 2-clause BSD
# license.

PORTNAME = hammer
PORTVERSION = 0.9
CATEGORIES = devel
EXTRACT_SUFX = .tar.bz2

USES = scons

# MASTER_SITE
MAINTAINER = thequux@upstandinghackers.com
COMMENT = Parser combinators. In C.

LICENSE = GPLv2
GIT_REV = master
GIT_URL = git://github.com/UpstandingHackers/hammer.git

SCONS_ARGS = --variant=opt DESTDIR=${STAGEDIR} prefix=${LOCALBASE}
SCONS_TARGET = all
SCONS_INSTALL_TARGET = install

do-fetch:
	${MKDIR} ${WRKDIR}
	git clone -b ${GIT_REV} ${GIT_URL} ${WRKDIR}/${DISTNAME}
	cd ${WRKDIR}; tar cvfy ${DISTDIR}/${DISTNAME}.tar.bz2 ${DISTNAME}
	rm -rf ${WRKDIR}/${DISTNAME}

FETCH_DEPENDS += git:${PORTSDIR}/devel/git

# TODO: strip libhammer.so
.include <bsd.port.mk>
