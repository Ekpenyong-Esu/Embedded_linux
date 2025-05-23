#include "systemcalls.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
     // Check if the command is valid
    if (cmd == NULL) {
        return false; // Invalid command
    }

    // Execute the command using the system() function as a shell command
    int result = system(cmd);

    // Check if the command was executed successfully
    if (result != 0)
    {
        return false;
    }
    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    // va_list is a type to hold information about variable arguments. unspecified arguments
    va_list args; // list of arguments

    // va_start initializes the va_list to retrieve the variable arguments
    va_start(args, count);

    // create an array of strings to hold the command and arguments
    char * command[count+1];

    // populate the command array with the arguments from the va_list
    int num_of_argue = 0;
    for(num_of_argue=0; num_of_argue<count; num_of_argue++)
    {
        command[num_of_argue] = va_arg(args, char *);
    }
    command[count] = NULL;

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    pid_t pid = fork();

    // Check if the fork was successful
    if (pid == -1){
        // fork failed
        perror("fork failed");
        return false;
    }
    if (pid == 0) {

        // Child process
        execv(command[0], command);

        // If execv returns, there was an error
        perror("execv");

        exit(EXIT_FAILURE);
    }
    else {
        // Parent process
        int status = 0;
        waitpid(pid, &status, 0);

        va_end(args);

        // Check if child exited normally
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;

    va_start(args, count);

    char * command[count+1];
    int num_of_argue = 0;
    for(num_of_argue=0; num_of_argue<count; num_of_argue++)
    {
        command[num_of_argue] = va_arg(args, char *);
    }
    command[count] = NULL;

/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    pid_t pid = fork();
    if (pid == -1) {
        // Fork failed
        perror("fork");
        return false;
    }
    if (pid == 0) {
        // Child process
        int file_desc = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (file_desc == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        if (dup2(file_desc, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(file_desc);
            exit(EXIT_FAILURE);
        }
        close(file_desc);
        execv(command[0], command);
        // If execv returns, there was an error
        perror("execv");
        exit(EXIT_FAILURE);
    }
    else {
        // Parent process
        int status = 0;
        if (waitpid(pid, &status,0) == -1) {
            perror("wait");
            return false;
        }
        va_end(args);
        // Check if child exited normally
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}
