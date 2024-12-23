#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

# Ensure the script exits on any error
set -e
set -u

OUTDIR=${1:-/tmp/aeld} # If OUTDIR is not set, use /tmp/aeldwha
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0)) # The $0 returns the name of the script dirname $0 returns the directory of
# the script wthithout the file name and realpath returns the absolute path of the directory.
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

export PATH=/home/mahonri/Desktop/Embedded_linux/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/bin:$PATH

echo "installing dependencies ..."
sudo apt-get update
sudo apt-get install -y build-essential bc make libc6 libncurses-dev bison flex libssl-dev libelf-dev bc git wget \
        qemu-user-static cpio crossbuild-essential-arm64
echo "dependencies installed"



if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

echo "making output directory"
mkdir -p ${OUTDIR}

echo "Changing to output directory"
cd "$OUTDIR"


echo "checking if the linux-stable directory exists"

if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} TO ${OUTDIR}"

	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi


if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    echo "Building the kernel"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper   # Clean the source tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig  # Set default configuration
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) all  # Build the kernel
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) modules  # Build kernel modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) dtbs  # Build device tree blobs

fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
echo "Creating base directories"
mkdir -p ${OUTDIR}/rootfs/{bin,dev,home,var/{log},tmp,sbin,etc,proc,sys,lib64,usr/{bin,sbin,lib},lib}


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}

    # TODO:  Configure busybox
    echo "Configuring busybox"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} distclean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
# Build and Install BusyBox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc)
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${OUTDIR}/rootfs install

# make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) install
# echo "Copying busybox binary"
#cp ${OUTDIR}/busybox/bin/busybox ${OUTDIR}/rootfs/bin/
cp ${OUTDIR}/busybox/_install/bin/busybox ${OUTDIR}/rootfs/bin/


echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
echo "Adding library dependencies"
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp -a ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/
cp -a ${SYSROOT}/lib64/ld-2.* ${OUTDIR}/rootfs/lib64/
cp -a ${SYSROOT}/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64/
cp -a ${SYSROOT}/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64/
cp -a ${SYSROOT}/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib6

# TODO: Make device nodes
echo "Making device nodes"
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 600 ${OUTDIR}/rootfs/dev/console c 5 1

# TODO: Clean and build the writer utility
echo "Building the writer utility"
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
sudo cp writer ${OUTDIR}/rootfs/home/

# TODO: Copy the finder related scripts and executables to the /home directory
echo "Copying finder related scripts and executables to the /home directory"
cp -r ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/conf/assignment.txt ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/finder_test.sh ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/
chmod +x ${OUTDIR}/rootfs/home/*



# on the target rootfs

# TODO: Chown the root directory
echo "Chown the root directory"
sudo chown -R root:root ${OUTDIR}/rootfs


# TODO: Create initramfs.cpio.gz
echo "Creating initramfs.cpio.gz"
cd ${OUTDIR}/rootfs
find . | cpio -H newc -o --owner root:root | gzip > ${OUTDIR}/initramfs.cpio.gz


echo "Kernel build completed successfully run QEMU using the start-qemu-app.sh script"
