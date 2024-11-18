#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char *argv[]) {

    const char *file_path = argv[1];
    const char *write_string = argv[2];
    // Open connection to syslog
    openlog("writer",  LOG_PID|LOG_CONS, LOG_USER);

    // Check arguments
    if (argc != 3) {
        syslog(LOG_ERR, "Invalid number of arguments. Usage: %s <file> <string>", argv[0]);
        fprintf(stderr, "Usage: %s <string> to <file>\n", argv[0]);
        closelog();
        return 1;
    }

     // Log the write attempt
    syslog(LOG_DEBUG, "Writing %s to %s", write_string, file_path);

    // Open file for writing
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        syslog(LOG_ERR, "Failed to open file: %s", file_path);
        perror("Error opening file");
        closelog();
        return 1;
    }

    // Write the string to the file
    if (fprintf(file, "%s", write_string) < 0) {
        syslog(LOG_ERR, "Failed to write to file: %s", file_path);
        perror("Error writing to file");
        fclose(file);
        closelog();
        return 1;
    }

    // Close the file
    fclose(file);

    // Log success
    syslog(LOG_DEBUG, "Writing '%s' to '%s'", write_string, file_path);
    printf("Successfully wrote to %s\n", file_path);

    // Close the syslog connection
    closelog();

    return 0;
}
