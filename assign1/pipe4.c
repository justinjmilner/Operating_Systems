/* gopipe.c
 *
 * CSC 360, Spring 2024
 *
 * Execute up to four instructions, piping the output of each into the
 * input of the next.
 *
 * Please change the following before submission:
 *
 * Author: Din Grogu
 * Login:  babyyoda@uvic.ca 
 */


/* Note: The following are the **ONLY** header files you are
 * permitted to use for this assignment! */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <wait.h>

#define MAX_COMMANDS 4
#define MAX_ARG_LEN 81 
#define MAX_ARGS 8

// Function to parse command line arguments and put them into an array of strings
void parse_command(const char* cmd, char args[MAX_ARGS][MAX_ARG_LEN]) {
    // Make a temp copy of cmd for strtok
    char temp_cmd[MAX_ARG_LEN];
    strncpy(temp_cmd, cmd, MAX_ARG_LEN);

    // Parse up to a maximum of 7 arguments in a command
    int i = 0;
    char* token;
    token = strtok(temp_cmd, " ");
    while (token != NULL && i < MAX_ARGS - 1) {
        strncpy(args[i], token, MAX_ARG_LEN);
        i++;
        token = strtok(NULL, " ");
    }
    // Set final arg to null pointer to signal end of arguments
    args[i][0] = '\0';
}


int main() {
    char input[MAX_ARG_LEN];
    int num_commands = 0;
    char commands[MAX_COMMANDS][MAX_ARG_LEN];
    char args[MAX_COMMANDS][MAX_ARGS][MAX_ARG_LEN];
    int fd[2 * (MAX_COMMANDS - 1)]; // File descriptors for pipes

    // Read commands from user and parse each command into an array of arguments
    for (; num_commands < MAX_COMMANDS;) {
        if (read(STDIN_FILENO, input, MAX_ARG_LEN - 1) <= 0) {
            break;
        }

        // Replace new line character with null terminator
        input[strcspn(input, "\n")] = 0;

        // Stop reading commands if user entered a blank new line
        if (strlen(input) == 0) {
            break;
        }

        // Make a copy of the string and save it in the commands array
        strncpy(commands[num_commands], input, MAX_ARG_LEN);

        // Parse the command into an array of arguments
        parse_command(commands[num_commands], args[num_commands]);
        num_commands++;
    }

    // Create pipes
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(fd + i * 2) < 0) {
            exit(EXIT_FAILURE);
        }
    }

    // Execute each command
    for (int i = 0; i < num_commands; i++) {
        pid_t pid = fork();

        // Child process
        if (pid == 0) {
            // Redirect input form the previous pipe
            if (i != 0) {
                dup2(fd[(i - 1) * 2], STDIN_FILENO);
            } 

            // Redirect output to the next pipe
            if (i != num_commands - 1) {
                dup2(fd[i * 2 + 1], STDOUT_FILENO);
            }
    
            // Close file descriptors
            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(fd[j]);
            }

            // Convert 2D array of args to an array of char* for execvp()
            char* exec_args[MAX_ARGS];
            for (int j = 0; j < MAX_ARGS; j++) {
                exec_args[j] = args[i][j][0] ? args[i][j] : NULL;
            }

            execvp(exec_args[0], exec_args);
            exit(EXIT_FAILURE);
        }
    }

    // Parent process closes file descriptors
    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(fd[i]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < num_commands; i++) {
        wait(NULL);
    } 
    
    return 0;
    
}
