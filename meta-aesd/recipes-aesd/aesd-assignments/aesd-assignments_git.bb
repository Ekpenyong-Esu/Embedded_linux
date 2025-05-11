SUMMARY = "Server application and charater driver assignments"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "git://github.com/cu-ecen-aeld/assignments-3-and-later-${GITHUB_USERNAME};protocol=ssh;branch=master \
           file://S98aesdsocket \
          "
PV = "1.0+git${SRCPV}"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

TARGET_CC_ARCH += "${LDFLAGS}"

RDEPENDS:${PN} += "libgcc"
DEPENDS += "aesd-char-driver"

do_configure () {
	:
}

do_compile () {
	oe_runmake -C ${S}/server
}

do_install () {
    install -d ${D}${bindir}
    install -m 0755 ${S}/server/aesdsocket ${D}${bindir}

    # Install init script
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/S98aesdsocket ${D}${sysconfdir}/init.d
}

inherit update-rc.d

INITSCRIPT_NAME = "S98aesdsocket"
INITSCRIPT_PARAMS = "start 98 5 . stop 10 0 1 6 ."
