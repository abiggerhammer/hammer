# $NetBSD$
VERSION=		1.0.0-rc3
DISTNAME=		hammer-${VERSION}
DISTFILES=		v${VERSION}${EXTRACT_SUFX}
CATEGORIES=		devel
LICENSE=		gnu-gpl-v2
MASTER_SITES=		https://github.com/UpstandingHackers/hammer/archive/
MAINTAINER=		thequux@upstandinghackers.com
HOMEPAGE=		http://github.com/UpstandingHackers/hammer
COMMENT=		Parser combinator library in C

USE_TOOLS+=		gtar
USE_LANGUAGES=		c c++
#TOOLS_GTAR=		${TOOL_GTAR}
FETCH_USING:=		fetch
EXTRACT_USING=		gtar
BUILD_DEPENDS+=         scons-[0-9]*:../../devel/scons

FIND_PREFIX:=           SCONSDIR=scons
.include "../../mk/find-prefix.mk"

SCONS_ARGS=		--variant=opt DESTDIR=${DESTDIR} prefix=${PREFIX}

do-build:
	cd ${WRKSRC} && ${SCONSDIR}/bin/scons ${SCONS_ARGS}

do-install:
	cd ${WRKSRC} && ${SCONSDIR}/bin/scons ${SCONS_ARGS} install



.include "../../mk/bsd.pkg.mk"
