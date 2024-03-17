// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifdef _WIN32
#include <processthreadsapi.h>
#else // probably Unix variant
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

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
        perror("realloc()");
        exit(EXIT_FAILURE);
    }
    cc_argv.argv[cc_argv.argc++] = arg;
}

int main(int argc, char* argv[])
{
    (void)argc;
    char run_out_executable[PATH_MAX] = "./a.out";
    char* out_executable = &run_out_executable[strlen("./")];
    bool cleanup_required = true; // remove compiled executable

    // Parse argv[1] to get args for compiler
    char compiler[8] = "cc";
    {
        const size_t init_size = 128;
        if ((cc_argv.argv = malloc(init_size * sizeof(char*))) == NULL) {
            perror("malloc()");
            exit(EXIT_FAILURE);
        }
        cc_argv.capacity = init_size;
        push(compiler);
        push((char[]){"-Wall"});

        if (argv[1] != NULL) for (char* arg = argv[1];;)
        {
            while (*arg == ' ')
                arg++;
            if (*arg == '\0')
                break;

            push(arg);

            if (memcmp(arg, "-o", 2) == 0)
            {
                if (strlen(arg) >= PATH_MAX - 1) {
                    fputs("Output file name too long!", stderr);
                    exit(EXIT_FAILURE);
                }
                strcpy(out_executable, &arg[strlen("-o")]);
                char* end = strchr(out_executable, ' ');
                if (end != NULL)
                    *end = '\0';
                cleanup_required = false;
            }

            do {
                arg++;
                if (*arg == '.') // check for C++ file extension
                {
                    bool longer_than_dot_c = arg[2] != ' ' && arg[2] != '\0';
                    if (longer_than_dot_c || arg[1] == 'C')
                        strcpy(compiler, "c++");
                }
            } while (*arg != ' ' && *arg != '\0');

            if (*arg == '\0')
                break;
            *arg = '\0';
            arg++;
        }

        cc_argv.argv[cc_argv.argc] = NULL;
    }

    // User probably gets confused if running a file removes existing executable
    // so let's check for that.
    FILE* existing_executable = fopen(out_executable, "r");
    if (existing_executable != NULL) {
        cleanup_required = false;
        fclose(existing_executable);
    } else {
        errno = 0;
    }

    #if _WIN32

    // Compile whatever was in argv[1]
    {
    }

    #else // Unix implementation

    // Compile whatever was in argv[1]
    {
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork()");
            exit(EXIT_FAILURE);
        }
        else if (child_pid == 0) {
            if (execvp(compiler, cc_argv.argv) == -1) {
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

    // We got here if compiler succeeded. However, an executable might've not
    // been produced due to user passing something like "--help" as argument.
    FILE* compiled_executable = fopen(out_executable, "r");
    if (compiled_executable == NULL)
        exit(EXIT_SUCCESS);
    else
        fclose(compiled_executable);

    // Run the compiled executable with rest of argv[]
    int exit_status = 0;
    {
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork()");
            exit(EXIT_FAILURE);
        }
        else if (child_pid == 0) {
            argv[1] = out_executable;
            if (execv(run_out_executable, &argv[1]) == -1) {
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

    #endif

    if (cleanup_required && remove("a.out") == -1)
        perror("remove()");

    return exit_status;
}
