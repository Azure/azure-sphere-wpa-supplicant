#
# This recipe builds googletest.
#

SUMMARY = "googletest"
DESCRIPTION = "Google Test"
LICENSE = "BSD-3-Clause"

SRC_URI = "git://github.com/google/googletest;protocol=https;tag=release-${PV}"
LIC_FILES_CHKSUM = "file://googletest/LICENSE;md5=cbbd27594afd089daa160d3a16dd515a"

ALLOW_EMPTY_${PN} = "1"
ALLOW_EMPTY_${PN}-dbg = "1"

RDEPENDS_${PN}-dev += "${PN}-staticdev"

BBCLASSEXTEND = "native nativesdk"

S = "${WORKDIR}/git"

inherit pkgconfig cmake

# compiling with -fPIC allows to link with native applications that are
# PIE-hardened, such as Ubuntu 16.10
OECMAKE_C_FLAGS += " -fPIC"
OECMAKE_CXX_FLAGS += " -fPIC"
