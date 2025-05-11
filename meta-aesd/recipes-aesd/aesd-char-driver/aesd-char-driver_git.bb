SUMMARY = "AESD char driver"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit module

SRC_URI = "git://github.com/cu-ecen-aeld/assignments-3-and-later-${GITHUB_USERNAME};protocol=ssh;branch=master \
           file://S97aesdchar \
          "
PV = "1.0+git${SRCPV}"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

FILES:${PN} += "${bindir}/aesdchar_load"
FILES:${PN} += "${bindir}/aesdchar_unload"

EXTRA_OEMAKE:append:task-install = " -C ${STAGING_KERNEL_DIR} M=${S}/aesd-char-driver"
EXTRA_OEMAKE += "KERNELDIR=${STAGING_KERNEL_DIR}"

do_install:append() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/aesd-char-driver/aesdchar_load ${D}${bindir}
    install -m 0755 ${S}/aesd-char-driver/aesdchar_unload ${D}${bindir}

    # Install init script
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/S97aesdchar ${D}${sysconfdir}/init.d
}

inherit update-rc.d

INITSCRIPT_NAME = "S97aesdchar"
INITSCRIPT_PARAMS = "start 97 5 . stop 15 0 1 6 ."

RPROVIDES:${PN} += "kernel-module-aesd-char-driver"
