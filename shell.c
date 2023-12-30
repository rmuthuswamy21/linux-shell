#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 255

// Helper function declarations
void execute_tokens(char **tokens);
void execute_cd(char **tokens);
void execute_source(char **tokens);
void execute_prev();
void execute_command(char **tokens);
void execute_script(const char *filename);

// New tokenize function to return them as a list of char*
char** tokenize(char *input) {
    bool inQuotes = false;
    char *token = malloc(MAX_INPUT_SIZE * sizeof(char));
    char **tokens = malloc(MAX_INPUT_SIZE * sizeof(char*));
    int inputIndex = 0;
    int tokensIndex = 0;

    for (int i = 0; i < strlen(input); i++) {
        if (!inQuotes) {
            if (input[i] == ' ') {
                if (inputIndex > 0) {
                    token[inputIndex] = '\0';
                    tokens[tokensIndex] = strdup(token);
                    inputIndex = 0;
                    tokensIndex++;
                }
            } else if (input[i] == '"') {
                inQuotes = true;
            } else {
                token[inputIndex] = input[i];
                inputIndex++;
            }
        } else {
            if (input[i] == '"') {
                inQuotes = false;
            } else {
                token[inputIndex] = input[i];
                inputIndex++;
            }
        }
    }

    token[inputIndex] = '\0';
    tokens[tokensIndex] = strdup(token);
    tokensIndex++;
    tokens[tokensIndex] = NULL;
    free(token);
    return tokens;
}

// Executes the tokens
void execute_tokens(char **tokens) {
    pid_t pid = fork();

    if (pid == 0) {
        // In child process
        int output_fd;

        // Check if > and if there is anything after
        for (int i = 0; tokens[i] != NULL; i++) {
            if (strcmp(tokens[i], ">") == 0 && tokens[i + 1] != NULL) {
                // Open file
                output_fd = open(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC);

                if (output_fd == -1) {
                    perror("Error opening file");
                    exit(EXIT_FAILURE);
                }

                // Redirect stdout to the file
                if (dup2(output_fd, STDOUT_FILENO) == -1) {
                    perror("Error redirecting stdout");
                    // Close end
                    close(output_fd);
                    exit(EXIT_FAILURE);
                }

                // Remove '>' and the file name from the tokens array
                for (int j = i; tokens[j] != NULL; j++) {
                    tokens[j] = tokens[j + 2];
                }
            }
        }

        if (execvp(tokens[0], tokens) == -1) {
            perror("Error");
        }

        exit(EXIT_FAILURE); // Exec failed, exit child process
    } else if (pid < 0) {
        perror("Error");
    } else { // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
}


// Executes the cd command
void execute_cd(char **tokens) {
    if (tokens[1] != NULL) {
        if (chdir(tokens[1]) != 0) {
            perror("cd");
        }
    } else {
        // No additional arguments provided, go up to parent directory
        if (chdir("..") != 0) {
            perror("cd");
        }
    }
}

// Executes source
void execute_source(char **tokens) {
    if (tokens[1] != NULL) {
        execute_script(tokens[1]);
    } else {
        printf("source: missing argument\n");
    }
}

// Keeps track of previous commands
char previous_command[MAX_INPUT_SIZE];

// Executes previous command
void execute_prev() {
    if (previous_command[0] != '\0') {
        char **prev_tokens = tokenize(previous_command);
        execute_command(prev_tokens);

        // Free the memory allocated by tokenize
        for (int i = 0; prev_tokens[i] != NULL; i++) {
            free(prev_tokens[i]);
        }
        free(prev_tokens);
    } else {
        printf("prev: no previous command\n");
    }
}

// Executes the help command
void execute_help() {
    printf("Built-in commands:\n");
    printf("  cd <directory>    Change the current working directory\n");
    printf("  source <filename> Execute commands from a script file\n");
    printf("  prev              Repeat the previous command\n");
    printf("  help              Display information about built-in commands\n");
    printf("  exit              Exit the shell\n");
}

// Execute the process to use pipe with two args
void execute_pipe(char **tokens1, char **tokens2) {
    int pipe_fd[2];
    pid_t child1, child2;

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return;
    }

    child1 = fork();

    if (child1 == 0) {
        // Child process 1
        
        // Close unused read end
        close(pipe_fd[0]);
        
        // Redirect stdout to pipe write end
        dup2(pipe_fd[1], STDOUT_FILENO);
        
        // Close write end
        close(pipe_fd[1]);

        execute_command(tokens1);
        exit(EXIT_SUCCESS);
    } else if (child1 < 0) {
        perror("fork");
        return;
    } else {
        // Child process 2
    
        child2 = fork();

        if (child2 == 0) {
            close(pipe_fd[1]);
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[0]);

            execute_command(tokens2);
            exit(EXIT_SUCCESS);
        } else if (child2 < 0) {
            perror("fork");
            return;
        } else { // Parent process
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            waitpid(child1, NULL, 0);
            waitpid(child2, NULL, 0);
        }
    }
}

// Execute the given tokens
void execute_command(char **tokens) {
    if (strcmp(tokens[0], "source") == 0) {
        execute_source(tokens);
    } else if (strcmp(tokens[0], "cd") == 0) {
        execute_cd(tokens);
    } else if (strcmp(tokens[0], "prev") == 0) {
        execute_prev();
    } else if (strcmp(tokens[0], "help") == 0) {
        execute_help();
    } else {
        execute_tokens(tokens);
    }
}

// Execute a script
void execute_script(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    // Represents a line in the script
    char line[MAX_INPUT_SIZE];

    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }

        char **tokens = tokenize(line);

        if (tokens != NULL) {
            execute_command(tokens);

            // Save the command to previous_command
            if (strcmp(line, "prev") != 0) {
                strncpy(previous_command, line, MAX_INPUT_SIZE);
            }

            // Free the memory allocated by tokenize
            for (int i = 0; tokens[i] != NULL; i++) {
                free(tokens[i]);
            }
            free(tokens);
        }
    }

    fclose(file);
}

// Main function
int main(int argc, char **argv) {
    char input[MAX_INPUT_SIZE];
    char *sequences = malloc(MAX_INPUT_SIZE * sizeof(char));

    printf("Welcome to mini-shell.\n");
    while (1) {
        printf("shell $ ");
        if (fgets(input, sizeof(input), stdin) == NULL || strcmp(input, "exit\n") == 0) {
            printf("Bye bye.\n");
            break;
        }

        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        sequences = strtok(input, ";");

                while (sequences != NULL) {
                    char **tokens = tokenize(sequences);

                    // Check for pipes
                    char **tokens1 = NULL;
                    char **tokens2 = NULL;
                    int pipe_index = -1;

                    for (int i = 0; tokens[i] != NULL; i++) {
                        if (strcmp(tokens[i], "|") == 0) {
                            tokens[i] = NULL;
                            tokens1 = tokens;
                            tokens2 = tokens + i + 1;
                            pipe_index = i;
                            break;
                        }
                    }

                    if (tokens1 && tokens2) {
                        execute_pipe(tokens1, tokens2);
                    } else {
                        execute_command(tokens);
                    }

                    // Save the command to previous_command
                    if (strcmp(sequences, "prev") != 0) {
                        strncpy(previous_command, sequences, MAX_INPUT_SIZE);
                    }

                    // Free the memory allocated by tokenize
                    for (int i = 0; tokens[i] != NULL; i++) {
                        free(tokens[i]);
                    }
                    free(tokens);

                    sequences = strtok(NULL, ";"); // Get the next sequence
                }
            }

    free(sequences);
    return 0;
}
