#!/bin/bash
# Script to build Raspberry Pi system
set -e
set -u

OUTDIR=${1:-/tmp/rpi}
RPI_KERNEL_REPO=https://github.com/raspberrypi/linux
KERNEL_VERSION=rpi-6.1.y
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu-

# Update these paths according to your system
export PATH=/opt/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin:$PATH

echo "################### installing dependencies ...  #############################"
sudo apt-get update
sudo apt-get install -y build-essential bc make bison flex libssl-dev libelf-dev git wget \
        qemu-user-static cpio crossbuild-essential-arm64 device-tree-compiler

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

# Add model detection
RPI_MODEL=$2
if [ -z "${RPI_MODEL}" ]; then
    echo "Please specify Pi model (1, 2, 3, 4, or 5) as second argument"
    exit 1
fi

# Modify DTB selection based on model
case ${RPI_MODEL} in
    "1")
        DTB_FILE="bcm2835-rpi-b.dtb"
        FIRMWARE_FILES="bootcode.bin fixup.dat start.elf"
        KERNEL_CONFIG="bcmrpi_defconfig"
        ;;
    "2")
        DTB_FILE="bcm2836-rpi-2-b.dtb"
        FIRMWARE_FILES="bootcode.bin fixup.dat start.elf"
        KERNEL_CONFIG="bcm2709_defconfig"
        ;;
    "3")
        DTB_FILE="bcm2710-rpi-3-b.dtb"
        FIRMWARE_FILES="bootcode.bin fixup.dat start.elf"
        KERNEL_CONFIG="bcm2709_defconfig"
        ;;
    "4")
        DTB_FILE="bcm2711-rpi-4-b.dtb"
        FIRMWARE_FILES="bootcode.bin fixup4.dat start4.elf"
        KERNEL_CONFIG="bcm2711_defconfig"
        ;;
    "5")
        DTB_FILE="bcm2712-rpi-5-b.dtb"
        FIRMWARE_FILES="bootcode.bin fixup5.dat start5.elf"
        KERNEL_CONFIG="bcm2712_defconfig"
        ;;
    *)
        echo "Unsupported Raspberry Pi model"
        exit 1
        ;;
esac

echo "making output directory"
mkdir -p ${OUTDIR}

echo "Changing to output directory"
cd "$OUTDIR"

if [ ! -d "${OUTDIR}/linux" ]; then
    echo "CLONING RASPBERRY PI KERNEL ${KERNEL_VERSION}"
    git clone ${RPI_KERNEL_REPO} --depth 1 --branch ${KERNEL_VERSION} ${OUTDIR}/linux
fi

if [ ! -e ${OUTDIR}/linux/arch/${ARCH}/boot/Image ]; then
    cd ${OUTDIR}/linux
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} ${KERNEL_CONFIG}
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) Image modules dtbs
fi

# Copy kernel and DTBs
cp ${OUTDIR}/linux/arch/${ARCH}/boot/Image ${OUTDIR}/
cp ${OUTDIR}/linux/arch/${ARCH}/boot/dts/broadcom/${DTB_FILE} ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

echo "Creating base directories"
mkdir -p ${OUTDIR}/rootfs/{bin,dev,home/conf,var/log,tmp,sbin,etc,proc,sys,lib64,usr/{bin,sbin,lib},lib}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}

    echo "Configuring busybox"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} distclean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
else
    cd busybox
fi

make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc)
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${OUTDIR}/rootfs install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# Modified library dependencies for aarch64
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp -L ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
cp -L ${SYSROOT}/lib/aarch64-linux-gnu/libc.so.6 ${OUTDIR}/rootfs/lib
cp -L ${SYSROOT}/lib/aarch64-linux-gnu/libm.so.6 ${OUTDIR}/rootfs/lib
cp -L ${SYSROOT}/lib/aarch64-linux-gnu/libresolv.so.2 ${OUTDIR}/rootfs/lib

echo "Making device nodes"
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 600 ${OUTDIR}/rootfs/dev/console c 5 1

echo "Building the writer utility"
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
sudo cp writer ${OUTDIR}/rootfs/home/

echo "Copying finder related scripts and executables to the /home directory"
cp -r ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/conf/
cp -r ${FINDER_APP_DIR}/conf/assignment.txt ${OUTDIR}/rootfs/home/conf/
cp -r ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/
sudo chmod +x ${OUTDIR}/rootfs/home/*

echo "Chown of the root directory to root"
sudo chown -R root:root ${OUTDIR}/rootfs

echo "Creating initramfs.cpio.gz"
cd ${OUTDIR}/rootfs
find . | cpio -H newc -o --owner root:root | gzip > ${OUTDIR}/initramfs.cpio.gz

# Raspberry Pi specific boot files
echo "Downloading Raspberry Pi firmware files"
cd ${OUTDIR}
git clone --depth 1 https://github.com/raspberrypi/firmware.git rpi-firmware
for file in ${FIRMWARE_FILES}; do
    cp rpi-firmware/boot/${file} ${OUTDIR}/rootfs/boot/
done

# Create config.txt with model-specific settings
cat << EOF > ${OUTDIR}/rootfs/boot/config.txt
arm_64bit=1
kernel=Image
device_tree=${DTB_FILE}
enable_uart=1
EOF

# Add model-specific settings
case ${RPI_MODEL} in
    "1")
        cat << EOF >> ${OUTDIR}/rootfs/boot/config.txt
gpu_mem=64
core_freq=250
sdram_freq=400
EOF
        ;;
    "2"|"3")
        cat << EOF >> ${OUTDIR}/rootfs/boot/config.txt
gpu_mem=128
core_freq=400
over_voltage=2
EOF
        ;;
    "4")
        cat << EOF >> ${OUTDIR}/rootfs/boot/config.txt
gpu_mem=128
over_voltage=2
arm_boost=1
EOF
        ;;
    "5")
        cat << EOF >> ${OUTDIR}/rootfs/boot/config.txt
pcie=on
gpu_mem=256
EOF
        ;;
esac

# Create cmdline.txt
echo "console=serial0,115200 console=tty1 root=/dev/mmcblk0p2 rootwait" > ${OUTDIR}/rootfs/boot/cmdline.txt

# SD card partitioning for Raspberry Pi
echo "Formatting SD card for Raspberry Pi"
MEMORY_CARD=$(sudo fdisk -l | grep "mmcblk" | cut -d" " -f1)
if [ -z "${MEMORY_CARD}" ]; then
    echo "Error: No SD card detected"
    exit 1
fi

sudo parted ${MEMORY_CARD} mklabel msdos
sudo parted ${MEMORY_CARD} mkpart primary fat32 1MiB 256MiB
sudo parted ${MEMORY_CARD} mkpart primary ext4 256MiB 100%

# Format partitions
sudo mkfs.vfat -F 32 ${MEMORY_CARD}p1
sudo mkfs.ext4 ${MEMORY_CARD}p2

# Mount and copy files
sudo mount ${MEMORY_CARD}p1 /mnt/boot
sudo mount ${MEMORY_CARD}p2 /mnt/rootfs

# Copy boot files
sudo cp ${OUTDIR}/rootfs/boot/* /mnt/boot/
sudo cp ${OUTDIR}/linux/arch/${ARCH}/boot/Image /mnt/boot/
sudo cp ${OUTDIR}/linux/arch/${ARCH}/boot/dts/broadcom/bcm2711-rpi-4-b.dtb /mnt/boot/

# Copy rootfs
sudo rsync -av ${OUTDIR}/rootfs/ /mnt/rootfs/

# Cleanup
sudo umount /mnt/boot
sudo umount /mnt/rootfs

echo "Raspberry Pi image creation completed and use ./manual-rpi.sh /path/to/output <model> to run the script"
