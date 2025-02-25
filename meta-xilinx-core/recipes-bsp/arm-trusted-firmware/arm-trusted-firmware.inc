DESCRIPTION = "ARM Trusted Firmware"

LICENSE = "BSD"
LIC_FILES_CHKSUM ?= "file://license.rst;md5=1dd070c98a281d18d9eefd938729b031"

PROVIDES = "virtual/arm-trusted-firmware"

inherit deploy

DEPENDS += "u-boot-mkimage-native"

S = "${WORKDIR}/git"
B = "${WORKDIR}/build"

SYSROOT_DIRS += "/boot"

XILINX_RELEASE_VERSION ?= ""
ATF_VERSION ?= "2.2"
ATF_VERSION_EXTENSION ?= "-xilinx-${XILINX_RELEASE_VERSION}"
PV = "${ATF_VERSION}${ATF_VERSION_EXTENSION}+git${SRCPV}"

BRANCH ?= ""
REPO ?= "git://github.com/Xilinx/arm-trusted-firmware.git;protocol=https"
BRANCHARG = "${@['nobranch=1', 'branch=${BRANCH}'][d.getVar('BRANCH', True) != '']}"
SRC_URI = "${REPO};${BRANCHARG}"

inherit image-artifact-names

ATF_BASE_NAME ?= "${PN}-${PKGE}-${PKGV}-${PKGR}${IMAGE_VERSION_SUFFIX}"

COMPATIBLE_MACHINE ?= "^$"
COMPATIBLE_MACHINE:zynqmp = ".*"
COMPATIBLE_MACHINE:versal = ".*"

PACKAGE_ARCH = "${MACHINE_ARCH}"

PLATFORM:zynqmp = "zynqmp"
PLATFORM:versal = "versal"

# requires CROSS_COMPILE set by hand as there is no configure script
export CROSS_COMPILE="${TARGET_PREFIX}"

# Let the Makefile handle setting up the CFLAGS and LDFLAGS as it is a standalone application
CFLAGS[unexport] = "1"
LDFLAGS[unexport] = "1"
AS[unexport] = "1"
LD[unexport] = "1"

ATF_CONSOLE ?= ""
ATF_CONSOLE:zynqmp = "cadence"
ATF_CONSOLE:versal ?= "pl011"

DEBUG_ATF ?= ""
DEBUG_ATF:versal ?= "1"

EXTRA_OEMAKE:append:zynqmp = "${@' ZYNQMP_CONSOLE=${ATF_CONSOLE}' if d.getVar('ATF_CONSOLE', True) != '' else ''}"
EXTRA_OEMAKE:append:versal = "${@' VERSAL_CONSOLE=${ATF_CONSOLE}' if d.getVar('ATF_CONSOLE', True) != '' else ''}"
EXTRA_OEMAKE:append = " ${@bb.utils.contains('DEBUG_ATF', '1', ' DEBUG=${DEBUG_ATF}', '', d)}"

OUTPUT_DIR = "${@bb.utils.contains('DEBUG_ATF', '1', '${B}/${PLATFORM}/debug', '${B}/${PLATFORM}/release', d)}"

ATF_MEM_BASE ?= ""
ATF_MEM_SIZE ?= ""

EXTRA_OEMAKE:append:zynqmp = "${@' ZYNQMP_ATF_MEM_BASE=${ATF_MEM_BASE}' if d.getVar('ATF_MEM_BASE', True) != '' else ''}"
EXTRA_OEMAKE:append:zynqmp = "${@' ZYNQMP_ATF_MEM_SIZE=${ATF_MEM_SIZE}' if d.getVar('ATF_MEM_SIZE', True) != '' else ''}"

EXTRA_OEMAKE:append:versal = "${@' VERSAL_ATF_MEM_BASE=${ATF_MEM_BASE}' if d.getVar('ATF_MEM_BASE', True) != '' else ''}"
EXTRA_OEMAKE:append:versal = "${@' VERSAL_ATF_MEM_SIZE=${ATF_MEM_SIZE}' if d.getVar('ATF_MEM_SIZE', True) != '' else ''}"

ATF_PROVENCORE = "SPD=pncd ZYNQMP_BL32_MEM_BASE=0x70000000 ZYNQMP_BL32_MEM_SIZE=0x10000000 PRELOADED_BL33_BASE=0x80000000"
EXTRA_OEMAKE:append = "${@bb.utils.contains('MACHINE_FEATURES', 'provencore', ' ${ATF_PROVENCORE}', '', d)}"

do_configure() {
	oe_runmake clean -C ${S} BUILD_BASE=${B} PLAT=${PLATFORM}
}

do_compile() {
	oe_runmake -C ${S} BUILD_BASE=${B} PLAT=${PLATFORM} RESET_TO_BL31=1 bl31
}

do_install() {
	install -d ${D}/boot
	install -Dm 0644 ${OUTPUT_DIR}/bl31/bl31.elf ${D}/boot/${PN}.elf
}

do_deploy() {
	install -d ${DEPLOYDIR}
	install -m 0644 ${OUTPUT_DIR}/bl31/bl31.elf ${DEPLOYDIR}/${ATF_BASE_NAME}.elf
	ln -sf ${ATF_BASE_NAME}.elf ${DEPLOYDIR}/${PN}.elf
	install -m 0644 ${OUTPUT_DIR}/bl31.bin ${DEPLOYDIR}/${ATF_BASE_NAME}.bin
	ln -sf ${ATF_BASE_NAME}.bin ${DEPLOYDIR}/${PN}.bin

	# Get the entry point address from the elf.
	BL31_BASE_ADDR=$(${READELF} -h ${OUTPUT_DIR}/bl31/bl31.elf | egrep -m 1 -i "entry point.*?0x" | sed -r 's/.*?(0x.*?)/\1/g')
	mkimage -A arm64 -O arm-trusted-firmware -T kernel -C none \
		-a $BL31_BASE_ADDR -e $BL31_BASE_ADDR \
		-d ${OUTPUT_DIR}/bl31.bin ${DEPLOYDIR}/${ATF_BASE_NAME}.ub
	ln -sf ${ATF_BASE_NAME}.ub ${DEPLOYDIR}/${PN}.ub
	ln -sf ${ATF_BASE_NAME}.ub ${DEPLOYDIR}/atf-uboot.ub
}
addtask deploy before do_build after do_compile
FILES:${PN} += "/boot/${PN}.elf"
