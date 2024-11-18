#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define FILE_PERMISSIONS 0644

int main(int argc, char *argv[]) {
    // Open connection to syslog
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

    // Check arguments
    if (argc != 3) {
        syslog(LOG_ERR, "Invalid number of arguments. Usage: %s <file> <string>", argv[0]);
        fprintf(stderr, "Usage: %s <file> <string>\n", argv[0]);
        closelog();
        return 1;
    }

    const char *file_path = argv[1];
    const char *write_string = argv[2];

    // Log the write attempt
    syslog(LOG_DEBUG, "Writing %s to %s", write_string, file_path);

    // Open file for writing
    int file_descriptor = open(file_path, O_WRONLY | O_CREAT | O_APPEND, FILE_PERMISSIONS);
    if (file_descriptor == -1) {
        syslog(LOG_ERR, "Failed to open file %s: %s", file_path, strerror(errno));
        perror("Error opening file");
        closelog();
        return 1;
    }

    // Write the string to the file
    ssize_t bytes_written = write(file_descriptor, write_string, strlen(write_string));
    if (bytes_written == -1) {
        syslog(LOG_ERR, "Failed to write to file %s: %s", file_path, strerror(errno));
        perror("Error writing to file");
        close(file_descriptor);
        closelog();
        return 1;
    }

    // Close the file
    close(file_descriptor);

    // Log success
    syslog(LOG_DEBUG, "Successfully wrote '%s' to '%s'", write_string, file_path);
    printf("Successfully wrote to %s\n", file_path);

    // Close the syslog connection
    closelog();

    return 0;
}
