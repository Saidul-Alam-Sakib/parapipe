#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_COMMANDS 10
#define MAX_LINE 4096

// Trim whitespace
char *trim(char *str) {
    while (*str == ' ') str++;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\n')) *end-- = '\0';
    return str;
}

typedef struct {
    int id;
    int command_count;
    char **commands;
} ThreadData;

void *worker_thread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int i;

    // SAFELY DECLARE PIPE ARRAY
    int **pipes = malloc((data->command_count - 1) * sizeof(int *));
    for (i = 0; i < data->command_count - 1; i++) {
        pipes[i] = malloc(2 * sizeof(int));
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    for (i = 0; i < data->command_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            if (i > 0) dup2(pipes[i - 1][0], STDIN_FILENO);
            if (i < data->command_count - 1) dup2(pipes[i][1], STDOUT_FILENO);

            for (int j = 0; j < data->command_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            char *cmd_copy = strdup(data->commands[i]);
            char *argv[100];
            int k = 0;
            argv[k] = strtok(cmd_copy, " ");
            while (argv[k] != NULL && k < 99) {
                argv[++k] = strtok(NULL, " ");
            }
            argv[k] = NULL;

            execvp(argv[0], argv);
            perror("exec failed");
            exit(1);
        }
    }

    for (i = 1; i < data->command_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    int write_fd = pipes[0][1];
    int read_fd = pipes[data->command_count - 2][0];

    fcntl(read_fd, F_SETFL, O_NONBLOCK);

    // Simulated input
    char *lines[] = {
        "abc def 123\n",
        "hello abc\n",
        "xyz abc 123\n",
        "nothing here\n"
    };

    for (i = 0; i < 4; i++) {
        write(write_fd, lines[i], strlen(lines[i]));
    }
    close(write_fd);

    sleep(1);

    char outbuf[MAX_LINE];
    ssize_t bytes_read;
    while ((bytes_read = read(read_fd, outbuf, sizeof(outbuf) - 1)) > 0) {
        outbuf[bytes_read] = '\0';
        printf("[Thread %d Output] %s", data->id, outbuf);
    }

    close(read_fd);

    for (i = 0; i < data->command_count; i++) {
        wait(NULL);
    }

    // CLEAN UP
    for (i = 0; i < data->command_count - 1; i++) {
        free(pipes[i]);
    }
    free(pipes);

    pthread_exit(NULL);
}

int main() {
    int num_threads = 2;
    char *command_string = strdup("grep abc -> grep 123");

    char *commands[MAX_COMMANDS];
    int command_count = 0;
    char *token = strtok(command_string, "->");
    while (token != NULL && command_count < MAX_COMMANDS) {
        commands[command_count++] = trim(token);
        token = strtok(NULL, "->");
    }

    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].id = i;
        thread_data[i].command_count = command_count;
        thread_data[i].commands = commands;
        pthread_create(&threads[i], NULL, worker_thread, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(command_string);
    return 0;
}
