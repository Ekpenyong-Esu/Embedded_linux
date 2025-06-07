/**
 * @file test_ioctl.c
 * @brief Test program for AESD character driver llseek and ioctl functionality
 *
 * This program tests the newly implemented llseek and ioctl features of the
 * AESD character driver. It verifies:
 * - Writing commands to the driver
 * - llseek operations (SEEK_SET, SEEK_END)
 * - IOCTL AESDCHAR_IOCSEEKTO command with valid parameters
 * - Error handling for invalid IOCTL parameters
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "aesd-char-driver/aesd_ioctl.h"

#define DEVICE_PATH "/dev/aesdchar"

int main()
{
    int fd;
    struct aesd_seekto seekto;
    char buffer[1024];
    ssize_t bytes_read;
    loff_t pos;

    // Open the AESD character device
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open device");
        return 1;
    }

    printf("Device opened successfully\n");

    /**
     * Test 1: Write test commands to the device
     * Each command is terminated with a newline, which signals
     * the driver to store it as a complete command entry
     */
    const char *test_commands[] = {"Hello World\n", "This is command 2\n", "Command 3 here\n", "Final command\n"};

    printf("Writing test commands...\n");
    for (int i = 0; i < 4; i++)
    {
        if (write(fd, test_commands[i], strlen(test_commands[i])) < 0)
        {
            perror("Write failed");
            close(fd);
            return 1;
        }
        printf("Wrote: %s", test_commands[i]);
    }

    /**
     * Test 2: Test llseek with SEEK_SET
     * This tests the ability to seek to an absolute position in the buffer
     */
    printf("\nTesting llseek SEEK_SET to position 5...\n");
    pos = lseek(fd, 5, SEEK_SET);
    if (pos < 0)
    {
        perror("lseek SEEK_SET failed");
    }
    else
    {
        printf("Seeked to position: %ld\n", pos);
    }

    /**
     * Test 3: Test llseek with SEEK_END
     * This tests the ability to seek to the end and get total buffer size
     */
    printf("Testing llseek SEEK_END...\n");
    pos = lseek(fd, 0, SEEK_END);
    if (pos < 0)
    {
        perror("lseek SEEK_END failed");
    }
    else
    {
        printf("Total buffer size: %ld\n", pos);
    }

    /**
     * Test 4: Test IOCTL AESDCHAR_IOCSEEKTO with valid parameters
     * Seek to command 1 (second command), offset 5 (skip "This ")
     * Should position at "is command 2\n"
     */
    printf("\nTesting IOCTL - seeking to command 1, offset 5...\n");
    seekto.write_cmd = 1;        // Command index (0-based)
    seekto.write_cmd_offset = 5; // Byte offset within command

    if (ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto) < 0)
    {
        perror("IOCTL failed");
    }
    else
    {
        printf("IOCTL successful\n");

        // Read from current position to verify seek worked correctly
        bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0)
        {
            buffer[bytes_read] = '\0';
            printf("Read from position: %s\n", buffer);
        }
    }

    /**
     * Test 5: Test IOCTL with invalid parameters
     * This should fail and return -EINVAL
     */
    printf("\nTesting invalid IOCTL parameters...\n");
    seekto.write_cmd = 10; // Invalid command index (only 0-3 exist)
    seekto.write_cmd_offset = 0;

    if (ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto) < 0)
    {
        printf("Expected error for invalid command: %s\n", strerror(errno));
    }

    close(fd);
    printf("\nTest completed\n");
    return 0;
}
