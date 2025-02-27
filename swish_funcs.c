#define _GNU_SOURCE

#include "swish_funcs.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"

#define MAX_ARGS 10

int tokenize(char *s, strvec_t *tokens) {
    // TODO Task 0: Tokenize string s

    const char *delim = " ";
    char *curr = strtok(s, delim);

    while(curr != NULL) {
        strvec_add(tokens, curr);
        curr = strtok(NULL, delim);
    }

    // Assume each token is separated by a single space (" ")
    // Use the strtok() function to accomplish this
    // Add each token to the 'tokens' parameter (a string vector)
    // Return 0 on success, -1 on error
    return 0;
}

int run_command(strvec_t *tokens) {
    // TODO Task 2: Execute the specified program (token 0) with the
    // specified command-line arguments
    // THIS FUNCTION SHOULD BE CALLED FROM A CHILD OF THE MAIN SHELL PROCESS
    // Hint: Build a string array from the 'tokens' vector and pass this into execvp()
    // Another Hint: You have a guarantee of the longest possible needed array, so you
    // won't have to use malloc.

    int argc = tokens->length;
    char *argv[argc+1];

    // redirection file descriptors
    int input_fd = -1;
    int output_fd = -1;
    int skip_indices[4] = {-1,-1,-1,-1};
    int skip_count = 0;

    // locates a redirection file operator and sets index
    for (int i = 0; i < argc; i++) {
        if (i + 1 < argc) {
            if (strcmp(tokens->data[i], ">") == 0) {
                output_fd = open(tokens->data[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                if (output_fd == -1) {
                    perror("Failed to open output file");
                    return -1;
                }
            } else if (strcmp(tokens->data[i], ">>") == 0) {
                output_fd = open(tokens->data[i + 1], O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
                if (output_fd == -1) {
                    perror("Failed to open output file");
                    return -1;
                }
            } else if (strcmp(tokens->data[i], "<") == 0) {
                input_fd = open(tokens->data[i + 1], O_RDONLY);
                if (input_fd == -1) {
                    perror("Failed to open input file");
                    return -1;
                }
            }
            if (output_fd != -1 || input_fd != -1) {
                skip_indices[skip_count++] = i;
                skip_indices[skip_count++] = i + 1;
            }
        }
    }

    // perform file redirection if necessary
    if (input_fd != -1) {
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }
    if (output_fd != -1) {
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }

    // skip redirection tokens
    int j = 0;
    for (int i = 0; i < argc; i++) {
        int skip = 0;
        for (int k = 0; k < skip_count; k++) {
            if (i == skip_indices[k]) {
                skip = 1;
                break;
            }
        }
        if (!skip) {
            argv[j] = tokens->data[i];
            j++;
        }
    }
    argv[j] = NULL;

    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTTOU, &sa, NULL);
    sigaction(SIGTTIN, &sa, NULL);

    // change process group
    pid_t pid = getpid();
    setpgid(pid, pid);

    // executes command
    execvp(argv[0], argv);

    // error check
    perror("exec");
    _exit(1);

    // TODO Task 3: Extend this function to perform output redirection before exec()'ing
    // Check for '<' (redirect input), '>' (redirect output), '>>' (redirect and append output)
    // entries inside of 'tokens' (the strvec_find() function will do this for you)
    // Open the necessary file for reading (<), writing (>), or appending (>>)
    // Use dup2() to redirect stdin (<), stdout (> or >>)
    // DO NOT pass redirection operators and file names to exec()'d program
    // E.g., "ls -l > out.txt" should be exec()'d with strings "ls", "-l", NULL

    // TODO Task 4: You need to do two items of setup before exec()'ing
    // 1. Restore the signal handlers for SIGTTOU and SIGTTIN to their defaults.
    // The code in main() within swish.c sets these handlers to the SIG_IGN value.
    // Adapt this code to use sigaction() to set the handlers to the SIG_DFL value.
    // 2. Change the process group of this process (a child of the main shell).
    // Call getpid() to get its process ID then call setpgid() and use this process
    // ID as the value for the new process group ID

    return 0;
}

int resume_job(strvec_t *tokens, job_list_t *jobs, int is_foreground) {
    // TODO Task 5: Implement the ability to resume stopped jobs in the foreground
    // 1. Look up the relevant job information (in a job_t) from the jobs list
    //    using the index supplied by the user (in tokens index 1)
    //    Feel free to use sscanf() or atoi() to convert this string to an int
    // 2. Call tcsetpgrp(STDIN_FILENO, <job_pid>) where job_pid is the job's process ID
    // 3. Send the process the SIGCONT signal with the kill() system call
    // 4. Use the same waitpid() logic as in main -- don't forget WUNTRACED
    // 5. If the job has terminated (not stopped), remove it from the 'jobs' list
    // 6. Call tcsetpgrp(STDIN_FILENO, <shell_pid>). shell_pid is the *current*
    //    process's pid, since we call this function from the main shell process

    // TODO Task 6: Implement the ability to resume stopped jobs in the background.
    // This really just means omitting some of the steps used to resume a job in the foreground:
    // 1. DO NOT call tcsetpgrp() to manipulate foreground/background terminal process group
    // 2. DO NOT call waitpid() to wait on the job
    // 3. Make sure to modify the 'status' field of the relevant job list entry to BACKGROUND
    //    (as it was STOPPED before this)

    return 0;
}

int await_background_job(strvec_t *tokens, job_list_t *jobs) {
    // TODO Task 6: Wait for a specific job to stop or terminate
    // 1. Look up the relevant job information (in a job_t) from the jobs list
    //    using the index supplied by the user (in tokens index 1)
    // 2. Make sure the job's status is BACKGROUND (no sense waiting for a stopped job)
    // 3. Use waitpid() to wait for the job to terminate, as you have in resume_job() and main().
    // 4. If the process terminates (is not stopped by a signal) remove it from the jobs list

    return 0;
}

int await_all_background_jobs(job_list_t *jobs) {
    // TODO Task 6: Wait for all background jobs to stop or terminate
    // 1. Iterate through the jobs list, ignoring any stopped jobs
    // 2. For a background job, call waitpid() with WUNTRACED.
    // 3. If the job has stopped (check with WIFSTOPPED), change its
    //    status to STOPPED. If the job has terminated, do nothing until the
    //    next step (don't attempt to remove it while iterating through the list).
    // 4. Remove all background jobs (which have all just terminated) from jobs list.
    //    Use the job_list_remove_by_status() function.

    return 0;
}
