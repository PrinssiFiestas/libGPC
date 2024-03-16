// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// TODO Windows support, C++ support, and -Wall

#ifdef __unix__
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#define PATH_MAX 4096

struct DynamicArgv {
    char** argv;
    size_t argc;
    size_t capacity;
} cc_argv = {};

void push(char* arg)
{
    if (cc_argv.argc == cc_argv.capacity - 1)
        cc_argv.argv = realloc(cc_argv.argv, cc_argv.capacity *= 2);
    if (cc_argv.argv == NULL) {
        perror("Too long arg list!\nrealloc()");
        exit(EXIT_FAILURE);
    }
    cc_argv.argv[cc_argv.argc++] = arg;
}

int main(int argc, char* argv[])
{
    char out_executable[PATH_MAX] = "./a.out";
    bool cleanup_required = true;
    FILE* existing_a_out; // just checking, not actually used
    if ((existing_a_out = fopen("a.out", "r")) != NULL) {
        cleanup_required = false;
        fclose(existing_a_out);
        errno = 0;
    }

    // Parse argv[1] to get args for compiler
    {
        const size_t init_size = 128;
        if ((cc_argv.argv = malloc(init_size * sizeof(char*))) == NULL) {
            perror("malloc()");
            exit(EXIT_FAILURE);
        }
        cc_argv.capacity = init_size;
        push((char[]){"cc"});

        if (argv[1] != NULL) for (char* arg = argv[1];;)
        {
            push(arg);

            if (memcmp(arg, "-o", 2) == 0)
            {
                if (strlen(arg) >= sizeof(out_executable) - 1) {
                    fputs("Input file name too long!", stderr);
                    exit(EXIT_FAILURE);
                }
                strcpy(&out_executable[strlen("./")], &arg[strlen("-o")]);
                char* end = strchr(out_executable, ' ');
                if (end != NULL)
                    *end = '\0';
                cleanup_required = false;
            }

            if ((arg = strchr(arg, ' ')) == NULL)
                break;
            *arg = '\0';
            do {
                arg++;
            } while (*arg == ' ');

            if (*arg == '\0')
                break;
        }

        cc_argv.argv[cc_argv.argc] = NULL;
    }

    #ifdef __unix__

    // Compile whatever was in argv[1]
    {
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork()");
            exit(EXIT_FAILURE);
        }
        else if (child_pid == 0) {
            if (execvp("cc", cc_argv.argv) == -1) {
                perror("execlp()");
                exit(EXIT_FAILURE);
            }
        }

        int wstatus;
        do {
            pid_t w = wait(&wstatus);
            if (w == -1) {
                perror("wait()");
                exit(EXIT_FAILURE);
            } else if (WEXITSTATUS(wstatus) != 0) { // compilation failed
                exit(WEXITSTATUS(wstatus));
            }
        } while ( ! WIFEXITED(wstatus) && ! WIFSIGNALED(wstatus));
    }

    // Run the compiled executable with rest of argv[]
    int exit_status = 0;
    {
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork()");
            exit(EXIT_FAILURE);
        }
        else if (child_pid == 0) {
            argv[1] = &out_executable[strlen("./")];
            if (execv(out_executable, &argv[1]) == -1) {
                perror("execlp()");
                exit(EXIT_FAILURE);
            }
        }

        int wstatus;
        do {
            pid_t w = wait(&wstatus);
            if (w == -1) {
                perror("wait()");
                exit(EXIT_FAILURE);
            }
        } while ( ! WIFEXITED(wstatus) && ! WIFSIGNALED(wstatus));

        exit_status = WEXITSTATUS(wstatus);
    }

    if (cleanup_required && remove("a.out") == -1)
        perror("remove()");

    return exit_status;

    #endif // __unix__
}
