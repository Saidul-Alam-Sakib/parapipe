// parapipe.c - Step 1: Parse arguments and split commands
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_COMMANDS 20

void print_usage() {
    fprintf(stderr, "Usage: ./parapipe -n <num_threads> -c \"command -> command -> ...\"\n");
}

// Helper to trim whitespace
char *trim(char *str) {
    while (*str == ' ') str++;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\n')) *end-- = '\0';
    return str;
}

#include <pthread.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_LINE 4096

// Shared data structure for each thread
typedef struct {
    int id;
    int command_count;
    char **commands;
} ThreadData;

// Worker thread function
void *worker_thread(void *arg) {
    ThreadData *data = (ThreadData *)arg;

    // Create pipe chain: one pipe between each pair of commands
    int pipes[data->command_count - 1][2];

    for (int i = 0; i < data->command_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    for (int i = 0; i < data->command_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // CHILD
            // If not first command, set stdin
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            // If not last command, set stdout
            if (i < data->command_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipe ends in child
            for (int j = 0; j < data->command_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Parse command into argv
            char *cmd = strdup(data->commands[i]);
            char *argv[100];
            int k = 0;
            argv[k] = strtok(cmd, " ");
            while (argv[k] != NULL && k < 99) {
                argv[++k] = strtok(NULL, " ");
            }
            argv[k] = NULL;

            execvp(argv[0], argv);
            perror("exec failed");
            exit(1);
        }
    }

    // Close all pipe ends in parent
    for (int i = 0; i < data->command_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes
    for (int i = 0; i < data->command_count; i++) {
        wait(NULL);
    }

    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    int num_threads = 0;
    char *command_string = NULL;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            num_threads = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            command_string = argv[++i];
        } else {
            print_usage();
            return 1;
        }
    }

    if (num_threads <= 0 || command_string == NULL) {
        print_usage();
        return 1;
    }

    // Split command string by "->"
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    char *token = strtok(command_string, "->");
    while (token != NULL && command_count < MAX_COMMANDS) {
        commands[command_count++] = trim(token);
        token = strtok(NULL, "->");
    }

    // Debug output
    printf("Number of threads: %d\n", num_threads);
    printf("Commands:\n");
    for (int i = 0; i < command_count; i++) {
        printf("  [%d] %s\n", i, commands[i]);
    }

    // Prepare and launch threads
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].id = i;
        thread_data[i].command_count = command_count;
        thread_data[i].commands = commands;
        pthread_create(&threads[i], NULL, worker_thread, &thread_data[i]);
    }

    // Join threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }


    // We’ll implement thread/process logic next
}
