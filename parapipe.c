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

    // We’ll implement thread/process logic next
    return 0;
}
