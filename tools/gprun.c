// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifdef _WIN32
#include <windows.h>
#else
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
} cc_argv = {0};

void* xmalloc(size_t n)
{
    void* p = malloc(n);
    if (p == NULL) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    } else
        return p;
}

void push(char* arg)
{
    if (cc_argv.argc == cc_argv.capacity - 1)
        cc_argv.argv = realloc(cc_argv.argv, (cc_argv.capacity *= 2) * sizeof(char*));
    if (cc_argv.argv == NULL) {
        perror("realloc()");
        exit(EXIT_FAILURE);
    }
    cc_argv.argv[cc_argv.argc++] = arg;
}

int main(int argc, char* argv[])
{
    // Trim whitespace
    if (argv[1] != NULL)
    {
        while (argv[1][0] == ' ')
            argv[1]++;
        char* last = argv[1] + strlen(argv[1]) - 1;
        while (*last == ' ')
            *(last--) = '\0';
    }

    #if _WIN32
        // cl.exe determines executable name from first source file
        char* first_src = NULL;
        char run_out_executable[PATH_MAX] = "./a.exe";
        char* argv1 = argv[1] ? strcpy(xmalloc(strlen(argv[1]) + 1), argv[1]) : NULL;
    #else
        (void)argc;
        char run_out_executable[PATH_MAX] = "./a.out";
        char* argv1 = argv[1];
    #endif
    char* out_executable = &run_out_executable[strlen("./")];
    bool cleanup_required = true; // remove compiled executable

    // Parse argv[1] to get args for compiler
    char compiler[8] = "cc";
    {
        const size_t init_size = 128;
        cc_argv.argv = xmalloc(init_size * sizeof(char*));
        cc_argv.capacity = init_size;

        push(compiler);
        push((char[]){"-Wall"});
        bool optimized = false;

        if (argv1 != NULL) for (char* arg = argv1;;)
        {
            push(arg);

            if (memcmp(arg, "-o", 2) == 0)
            {
                if (strlen(arg) >= PATH_MAX - 1) {
                    fputs("Output file name too long!", stderr);
                    exit(EXIT_FAILURE);
                }
                char* out_exec_name = arg + strlen("-o");
                while (*out_exec_name == ' ')
                    out_exec_name++;

                strcpy(out_executable, out_exec_name);
                char* end = strchr(out_executable, ' ');
                if (end != NULL)
                    *end = '\0';
                cleanup_required = false;
            }

            if (memcmp(arg, "-O", 2) == 0 && '1' <= arg[2] && arg[2] <= '3')
                optimized = true;

            do {
                arg++;
                if (arg[0] == '.' && (arg[1] == 'c' || arg[1] == 'C'))
                {
                    #if _WIN32
                        if (first_src == NULL)
                            first_src = cc_argv.argv[cc_argv.argc - 1];
                    #endif

                    if (memchr("cpx", arg[2], 3) || arg[1] == 'C')
                        strcpy(compiler, "c++");
                }
            } while (*arg != ' ' && *arg != '\0');

            if (*arg == '\0')
                break;
            *arg = '\0';
            arg++;
            while (*arg == ' ')
                arg++;
            if (*arg == '\0')
                break;
        }
        if ( ! optimized) {
            push((char[]){"-ggdb3"});
            push((char[]){"-gdwarf"});
            #if ! _WIN32
                push((char[]){"-fsanitize=address"});
                push((char[]){"-fsanitize=undefined"});
                push((char[]){"-static-libasan"}); // avoid LD_PRELOAD problems
            #endif
            push((char[]){"-lgpcd"});
        } else {
            push((char[]){"-lgpc"});
        }
        push((char[]){"-lm"});
        push((char[]){"-lpthread"});
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

    // Compile whatever was in argv[1]
    #if _WIN32
    {
        STARTUPINFOA start_info = {.cb = sizeof(start_info) };
        PROCESS_INFORMATION process_info = {0};

        char* cc_cmd;
        char* cl_cmd;
        if (argc > 1) {
            size_t len = sizeof(compiler) + sizeof(".exe -Wall -g3") + strlen(argv[1]);
            cc_cmd = xmalloc(len * 2 + sizeof('\0'));
            cl_cmd = cc_cmd + len;
            strcat(strcat(strcpy(cc_cmd, compiler), ".exe -Wall "), argv[1]);
            strcat(strcpy(cl_cmd, "cl.exe "), argv[1]);
        } else {
            cc_cmd = (char[]){"cc.exe"};
            cl_cmd = (char[]){"cl.exe"};
        }

        if ( ! CreateProcessA(NULL,
            cc_cmd, NULL, NULL, false, 0, NULL, NULL,
            &start_info,
            &process_info))
        {
            if ( ! CreateProcessA(NULL,
                cl_cmd, NULL, NULL, false, 0, NULL, NULL,
                &start_info,
                &process_info))
            {
                fprintf(stderr, "Invoking %s.exe or cl.exe failed!", compiler);
                exit(EXIT_FAILURE);
            }
            if (first_src != NULL) {
                strcpy(out_executable, first_src);
                strcpy(strchr(out_executable, '.'), ".exe");
            }
        }
        WaitForSingleObject(process_info.hProcess, INFINITE);

        DWORD compiler_exit_code;
        if (GetExitCodeProcess(process_info.hProcess, &compiler_exit_code) == 0) {
            fputs("GetExitCodeProcess() failed!", stderr);
            exit(EXIT_FAILURE);
        } else if (compiler_exit_code != 0) {
            exit(compiler_exit_code);
        }
        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);
    }
    #else // Unix probably
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
    #endif

    // We got here if compiler succeeded. However, an executable might've not
    // been produced due to user passing something like "--help" as argument.
    FILE* compiled_executable = fopen(out_executable, "r");
    if (compiled_executable == NULL)
        exit(EXIT_SUCCESS);
    else
        fclose(compiled_executable);

    // Run the compiled executable with rest of argv[]
    int exit_status = 0;
    #if _WIN32
    {
        STARTUPINFOA start_info = {.cb = sizeof(start_info) };
        PROCESS_INFORMATION process_info = {0};

        size_t cmd_length = sizeof(run_out_executable);
        for (int i = 2; i < argc; i++)
            cmd_length += strlen(argv[i]) + sizeof(" ");

        char* cmd = xmalloc(cmd_length);
        strcpy(cmd, out_executable);

        for (int i = 2; i < argc; i++)
            strcat(strcat(cmd, " "), argv[i]);

        if ( ! CreateProcessA(NULL,
            cmd, NULL, NULL, false, 0, NULL, NULL,
            &start_info,
            &process_info))
        {
            fputs("Failed running compiled executable!", stderr);
            exit(EXIT_FAILURE);
        }
        WaitForSingleObject(process_info.hProcess, INFINITE);

        DWORD exit_code;
        if (GetExitCodeProcess(process_info.hProcess, &exit_code) == 0) {
            fputs("GetExitCodeProcess() failed!", stderr);
            exit(EXIT_FAILURE);
        }
        exit_status = exit_code;

        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);

        char obj_name_buf[PATH_MAX];
        if (cleanup_required) for (size_t i = 2; i < cc_argv.argc; i++)
        {
            char*  src_name_end = strchr(cc_argv.argv[i], '.');
            size_t src_name_len = src_name_end - cc_argv.argv[i];
            if (src_name_end != NULL)
            {
                memcpy(obj_name_buf, cc_argv.argv[i], src_name_len);
                strcpy(obj_name_buf + src_name_len, ".obj");
                if (remove(obj_name_buf) == -1) // cc used, no obj files
                    break;
            }
        }
    }
    #else // Unix probably
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

    if (cleanup_required && remove(out_executable) == -1)
        perror("remove()");

    return exit_status;
}
