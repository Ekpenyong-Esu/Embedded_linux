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
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-

UBOOT_REPO=git://git.denx.de/u-boot.git
UBOOT_VERSION=v2023.04

export PATH=/home/mahonri/Desktop/Embedded_linux/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-linux-gnueabihf/bin:$PATH
# export ARCH
# export CROSS_COMPILE
# export PATH=/home/mahonri/Desktop/Embedded_linux/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-linux-gnueabihf:$PATH

echo "################### installing dependencies ...  #############################"
sudo apt-get update
sudo apt-get install -y build-essential bc make libc6 libncurses-dev bison flex libssl-dev libelf-dev bc git wget \
        qemu-user-static cpio crossbuild-essential-armhf
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


echo "checking if the linux-stable directory does not exists"

if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} TO ${OUTDIR}"

	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi


echo "Checking if the Image file does not exist"
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/zImage ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    echo "Building the kernel"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper   # Clean the source tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig  # Set default configuration
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) zImage  # Build the kernel
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) modules  # Build kernel modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) dtbs  # Build device tree blobs

fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/zImage ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
echo "Creating base directories"
mkdir -p ${OUTDIR}/rootfs/{bin,dev,home/conf,var/log,tmp,sbin,etc,proc,sys,lib64,usr/{bin,sbin,lib},lib}


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



echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
echo "Adding library dependencies"
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp -L ${SYSROOT}/lib/ld-linux-armhf.so.3 ${OUTDIR}/rootfs/lib
cp -L ${SYSROOT}/lib/arm-linux-gnueabihf/libm.so.6 ${OUTDIR}/rootfs/lib
cp -L ${SYSROOT}/lib/arm-linux-gnueabihf/libresolv.so.2 ${OUTDIR}/rootfs/lib
cp -L ${SYSROOT}/lib/arm-linux-gnueabihf/libc.so.6 ${OUTDIR}/rootfs/lib

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
cp -r ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/conf/
cp -r ${FINDER_APP_DIR}/conf/assignment.txt ${OUTDIR}/rootfs/home/conf/
cp -r ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/
sudo chmod +x ${OUTDIR}/rootfs/home/*



# on the target rootfs

# TODO: Chown the root directory
echo "Chown of the root directory to root"
sudo chown -R root:root ${OUTDIR}/rootfs


# TODO: Create initramfs.cpio.gz
echo "Creating initramfs.cpio.gz"
cd ${OUTDIR}/rootfs
find . | cpio -H newc -o --owner root:root | gzip > ${OUTDIR}/initramfs.cpio.gz

echo "Checking if the U-Boot directory does not exist"
if [ ! -d "${OUTDIR}/u-boot" ]; then
    echo "Cloning U-Boot repository"
    git clone ${UBOOT_REPO} --depth 1 --branch ${UBOOT_VERSION} ${OUTDIR}/u-boot
fi

echo "Building U-Boot"
cd ${OUTDIR}/u-boot
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} distclean
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} am335x_evm_defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc)

# Create /boot directory in root filesystem if it doesn't exist
mkdir -p ${OUTDIR}/rootfs/boot

# Copy U-Boot files to the root filesystem if necessary
echo "Copying U-Boot files to the root filesystem"
sudo cp ${OUTDIR}/u-boot/MLO ${OUTDIR}/rootfs/boot/
sudo cp ${OUTDIR}/u-boot/u-boot.img ${OUTDIR}/rootfs/boot/

# TODO: Format the memory card and create partitions
echo "Formatting the memory card and creating partitions"
MEMORY_CARD=$(sudo fdisk -l | grep "mmcblk" | cut -d" " -f1)
if [ -z "${MEMORY_CARD}" ]; then
    echo "Error: No SD card detected"
    exit 1
fi

sudo dd if=/dev/zero of=${MEMORY_CARD} bs=1M count=10
sudo sfdisk ${MEMORY_CARD} << EOF
,1G,c,*
,,L
EOF

# Create filesystems
sudo mkfs.vfat ${MEMORY_CARD}1
sudo mkfs.ext4 ${MEMORY_CARD}2

# Mount the partitions
mkdir -p /mnt/boot
mkdir -p /mnt/rootfs
sudo mount -o sync ${MEMORY_CARD}1 /mnt/boot
sudo mount -o sync ${MEMORY_CARD}2 /mnt/rootfs

# TODO: Move necessary files to the respective partitions
echo "Moving necessary files to the respective partitions"
sudo cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/zImage /mnt/boot/
sudo cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/dts/*.dtb /mnt/boot/
sudo tar -xpf ${OUTDIR}/rootfs.tar -C /mnt/rootfs

# Copy bootloader files if necessary
echo "Copying bootloader files"
sudo cp ${OUTDIR}/u-boot/MLO /mnt/boot/
sudo cp ${OUTDIR}/u-boot/u-boot.img /mnt/boot/

# Create bootcmd in /boot/uEnv.txt
cat << EOF > /mnt/boot/uEnv.txt
bootargs=console=ttyO0,115200n8 root=/dev/mmcblk0p2 rw rootfstype=ext4 rootwait
uenvcmd=fatload mmc 0:1 0x80200000 zImage; fatload mmc 0:1 0x80F00000 am335x-boneblack.dtb; bootz 0x80200000 - 0x80F00000
EOF

# Unmount the partitions
sudo umount /mnt/boot
sudo umount /mnt/rootfs

echo "Kernel build completed successfully run QEMU using the start-qemu-app.sh script"

echo "Memory card setup completed successfully"

echo "Kernel build completed successfully run QEMU using the start-qemu-app.sh script"
