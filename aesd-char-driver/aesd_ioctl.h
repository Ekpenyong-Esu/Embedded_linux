/**
 * @file aesd_ioctl.h
 * @brief IOCTL definitions and structures for AESD character driver
 *
 * This header file defines the IOCTL interface for the AESD character driver,
 * providing userspace applications with the ability to perform seek operations
 * on the circular buffer contents. The interface supports seeking to specific
 * write commands and offsets within those commands.
 *
 * Originally created on: Oct 23, 2019
 * Original author: Dan Walkes
 * Enhanced by: Mahonri Wozny
 * @date Enhanced: November 2024
 * @version 2.0
 *
 * Features:
 * - Cross-platform compatibility (kernel and userspace)
 * - Structured seek operations with command and offset targeting
 * - Standard Linux IOCTL magic number allocation
 * - Bounds checking support for command validation
 *
 * @brief Definitions for the ioctl used on aesd char devices for assignment 9
 */

#ifndef AESD_IOCTL_H
#define AESD_IOCTL_H

// Conditional compilation for kernel vs userspace environments
#ifdef __KERNEL__
#include <asm-generic/ioctl.h>
#include <linux/types.h>
#else
#include <stdint.h>
#include <sys/ioctl.h>
#endif

/**
 * @brief Structure for IOCTL seek operations from userspace to kernel
 *
 * This structure is passed by IOCTL from user space to kernel space,
 * describing the type of seek operation to be performed on the aesdchar
 * driver. It allows precise positioning within the circular buffer by
 * specifying both the target write command and offset within that command.
 *
 * Usage example:
 * struct aesd_seekto seekto = {.write_cmd = 2, .write_cmd_offset = 10};
 * ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto);
 */
struct aesd_seekto
{
    /**
     * @brief Zero-referenced write command index to seek into
     *
     * This specifies which write command (buffer entry) to target.
     * Commands are numbered starting from 0, representing the chronological
     * order of write operations. Must be less than the total number of
     * valid commands in the circular buffer.
     */
    uint32_t write_cmd;

    /**
     * @brief Zero-referenced byte offset within the target write command
     *
     * This specifies the byte position within the selected write command
     * where the file position should be set. Must be less than the size
     * of the target write command buffer entry.
     */
    uint32_t write_cmd_offset;
};

/**
 * @brief Magic number for AESD IOCTL commands
 *
 * This magic number is selected from the unused range in the Linux kernel
 * IOCTL number registry to avoid conflicts with other drivers.
 * Reference: https://github.com/torvalds/linux/blob/master/Documentation/userspace-api/ioctl/ioctl-number.rst
 */
#define AESD_IOC_MAGIC 0x16

/**
 * @brief IOCTL command for seek-to operation
 *
 * This defines the IOCTL command that userspace applications use to
 * perform seek operations on the AESD character device. The command
 * uses _IOWR macro indicating it both reads from and writes to userspace.
 *
 * Command number 1 is used to distinguish from potential future IOCTL
 * operations that might be added to the driver.
 */
#define AESDCHAR_IOCSEEKTO _IOWR(AESD_IOC_MAGIC, 1, struct aesd_seekto)

/**
 * @brief Maximum number of IOCTL commands supported
 *
 * This constant defines the upper bound for IOCTL command numbers
 * supported by the AESD character driver. It is used for bounds
 * checking to ensure only valid IOCTL commands are processed.
 *
 * Currently only one command (AESDCHAR_IOCSEEKTO) is supported,
 * but this allows for future expansion of the IOCTL interface.
 */
#define AESDCHAR_IOC_MAXNR 1

#endif /* AESD_IOCTL_H */
