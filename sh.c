#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "./jobs.h"

// global variables
job_list_t *job_list;
int jid;

/*
 * Checking for whether to execute file redirection, then executing
 * file redirection appropriately, includes error checking
 * input: char buffer[]
 * output: ssize_t representing read value
 */
ssize_t read_input(char buffer[]) {
    ssize_t x = read(0, buffer, 1024);

    if (x == -1) {
        fprintf(stderr, "Read failed");
    }
    // buffer is null terminated so that it works with strtok
    buffer[x] = '\0';

    return x;
}

/*
 * This function takes in a command array, buffer holding the read input, and a
 * redirection_file_array. The function parses the buffer array without altering
 * the buffer and fills in the command array and the redirection_file_array
 * appropriately so it can be used later in the main function. input: char *
 * cmd_array[], char buffer[], char *redirection_file_array[]) output: integer,
 * 0 on success, 1 on failure.
 *
 * THIS IS A LONG COMPLICATED FUNCTION, PLEASE READ MY README FOR A HIGHER LEVEL
 * EXPLANATION OF THIS FUNCTION FIRST
 */
int parsing(char *cmd_array[], char buffer[], char *redirection_file_array[],
            int *counter) {
    // setting needed local vairables
    char *tok = strtok(buffer, " \n\t");
    int input_flag = 0;
    int output_flag = 0;
    int append_flag = 0;
    int include_flag = 1;
    // int counter = 0;
    //  int num_input_files = 0;
    //  int num_output_files = 0;

    // Using strtok to parse, therefore I continue to parse until token is null
    while (tok != NULL) {
        // I check for input/output/append flags because token is assigned at
        // the end of the function,
        // so if the token would be the filename if the flag is set to true. See
        // line 114 - 133 to see where and how I check for these flags and set
        // them to true

        if (input_flag) {
            // checking for whether there are multiple output files
            if (strncmp(redirection_file_array[0], "1", 1) != 0) {
                fprintf(stderr, "syntax error: multiple input files\n");
                return 1;
            }

            // setting redirection array to keep track of filename
            redirection_file_array[0] = tok;

            // checking for missing output file

            // Set flag to 0 to not include token (which presents filename) in
            // cmd_array
            include_flag = 0;
            // reset flag
            input_flag = 0;
        }

        if (output_flag) {
            // checking for whether there are multiple output files error
            if (strncmp(redirection_file_array[1], "1", 1) != 0) {
                fprintf(stderr, "syntax error: multiple output files\n");
                return 1;
            }

            // setting redirection array to keep track of filename
            redirection_file_array[1] = tok;

            // Set flag to 0 to not include token (which presents filename) in
            // cmd_array
            include_flag = 0;
            // reset flag
            output_flag = 0;
        }

        if (append_flag) {
            // checking for whether there are multiple output files
            if (strncmp(redirection_file_array[2], "1", 1) != 0) {
                fprintf(stderr, "syntax error: multiple output files\n");
                return 1;
            }

            // setting redirection array to keep track of filename
            redirection_file_array[2] = tok;

            // Set flag to 0 to not include token (which presents filename) in
            // cmd_array
            include_flag = 0;
            // reset flag
            append_flag = 0;
        }

        // Checking for redirection symbols

        if (strncmp(tok, "<", 2) == 0) {
            // Set flag to 0 to not include token (which presents redirection
            // symbol) in cmd_array
            include_flag = 0;
            // set flag to true so function knows that following token should be
            // treated as a filename
            input_flag = 1;

        } else if (strncmp(tok, ">", 2) == 0) {
            // Set flag to 0 to not include token (which presents redirection
            // symbol) in cmd_array
            include_flag = 0;
            // set flag to true so function knows that following token should be
            // treated as a filename
            output_flag = 1;

        } else if (strncmp(tok, ">>", 2) == 0) {
            // Set flag to 0 to not include token (which presents redirection
            // symbol) in cmd_array
            include_flag = 0;
            // set flag to true so function knows that following token should be
            // treated as a filename
            append_flag = 1;

        } else {
            // dealing with command array and setting it to arguments

            if (include_flag) {  // then token is not filename or redirection
                                 // symbol
                // set cmd_array index to token and increment counter
                cmd_array[*counter] = tok;
                (*counter)++;
            } else {
                // reset include flag
                include_flag = 1;
            }
        }

        // set token to find next string in buffer using strtok
        tok = strtok(NULL, " \n\t");

        // Checking for no input file error
        if ((input_flag) && tok == NULL) {
            fprintf(stderr, "syntax error: no input file\n");

            return 1;
        }

        // Checking for no output file error
        if ((output_flag || append_flag) && tok == NULL) {
            fprintf(stderr, "syntax error: no output file\n");

            return 1;
        }

        // Checking for input file is a redicretion symbol error
        if ((input_flag) &&
            ((strncmp(tok, ">", 1) == 0) || (strncmp(tok, "<", 1) == 0) ||
             (strncmp(tok, ">>", 2) == 0))) {
            fprintf(stderr,
                    "syntax error: input file is a redicretion symbol\n");

            return 1;
        }

        // Checking for output file is a redicretion symbol error
        if ((output_flag || append_flag) &&
            ((strncmp(tok, ">", 1) == 0) || (strncmp(tok, "<", 1) == 0) ||
             (strncmp(tok, ">>", 2) == 0))) {
            fprintf(stderr,
                    "syntax error: output file is a redicretion symbol\n");

            return 1;
        }
    }

    return 0;
}

/*
 * Executing and error checking
 * change directory function
 * input: char * cmd_array[]
 * output: void
 */
void cd(char *cmd_array[]) {
    int ret = chdir(cmd_array[1]);
    if (cmd_array[1] == NULL) {
        fprintf(stderr, "cd: syntax error\n");
    } else if (ret == -1) {
        perror("cd");
    }
}

/*
 * Executing and error checking link function
 * input: char * cmd_array[]
 * output: void
 */
void ln(char *cmd_array[]) {
    int ret = link(cmd_array[1], cmd_array[2]);
    if (cmd_array[1] == NULL) {
        fprintf(stderr, "ln: syntax error\n");
    } else if (ret == -1) {
        perror("ln");
    }
}

/*
 * Executing and error checking remove
 * directory function
 * input: char * cmd_array[]
 * output: void
 */
void rm(char *cmd_array[]) {
    int ret = unlink(cmd_array[1]);

    if (cmd_array[1] == NULL) {
        fprintf(stderr, ": syntax error\n");
    } else if (ret == -1) {
        perror("rm");
    }
}

/*
 * Checking for built in functions in command array,
 * then executing built in functions appropriately
 * input: char * cmd_array[]
 * output: integer, returns 0 on success, returns 1 on failure
 */
int built_in_methods(char *cmd_array[]) {
    if (cmd_array[0] != NULL) {
        if (strncmp(cmd_array[0], "cd", 2) == 0) {
            cd(cmd_array);
            return 1;
        }

        if (strncmp(cmd_array[0], "exit", 4) == 0) {
            exit(0);
        }

        if (strncmp(cmd_array[0], "ln", 2) == 0) {
            ln(cmd_array);
            return 1;
        }

        if (strncmp(cmd_array[0], "rm", 2) == 0) {
            rm(cmd_array);
            return 1;
        }
    }
    return 0;
}

/*
 * Checking for whether to execute file redirection, then executing
 * file redirection appropriately, includes error checking
 * input: char * cmd_array[]
 * output: void
 */
int redirection(char *redirection_file_array[]) {
    // When dealing with output <
    if ((strncmp(redirection_file_array[0], "1", 1)) != 0) {
        if (close(STDIN_FILENO) == -1) {
            fprintf(stderr, "close: No such file or directory\n");
            return 1;
        }
        // opening file saved in redirection array
        if (open(redirection_file_array[0], O_RDWR, 0600) == -1) {
            fprintf(stderr, "open: No such file or directory\n");
            return 1;
        }
    }

    // When dealing with output >
    if ((strncmp(redirection_file_array[1], "1", 1)) != 0) {
        // closing output to open output file from command (with error checking)
        if (close(STDOUT_FILENO) == -1) {
            fprintf(stderr, "close: No such file or directory\n");
            return 1;
        }
        // opening file saved in redirection array
        if (open(redirection_file_array[1], O_RDWR | O_CREAT | O_TRUNC, 0600) ==
            -1) {
            fprintf(stderr, "open: No such file or directory\n");
            return 1;
        }
    }

    // When dealing with output >>
    if ((strncmp(redirection_file_array[2], "1", 1)) != 0) {
        if (close(STDOUT_FILENO) == -1) {
            fprintf(stderr, "close: No such file or directory\n");
            return 1;
        }
        // opening file saved in redirection array
        if (open(redirection_file_array[2], O_RDWR | O_APPEND | O_CREAT,
                 0600) == -1) {
            fprintf(stderr, "open: No such file or directory\n");
            return 1;
        }
    }
    return 0;
}

/*
 * Checking for jobs command, prints out job list.
 * Input: cmd_array, counter which represnets number of strings in cmd_array
 * Output: int 0 on success, 1 on error
 */
int check_jobs(char *cmd_array[], int *counter) {
    if (counter != 0 && strcmp(cmd_array[0], "jobs") == 0 &&
        cmd_array[1] == NULL) {
        jobs(job_list);
        return 1;
    }
    return 0;
}

/*
 * Ignores signals which is called by shell so that shell does not terminate
 */
void ignoring_signals() {
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}
/*
 * Sets signal to default so that processes in the window are rightfully
 * affected by keyboard generated signals
 */
void default_signals() {
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}

/*
 * Checking for bg command with error checking. Sends continue signal to command
 * and moves process into background Input: command array Output: int 0 on
 * success, 1 on error
 */
int bg(char *cmd_array[]) {
    // error checking
    if (strncmp(cmd_array[1], "%", 1) != 0) {
        fprintf(stderr, "bg: syntax error");
        return 1;
    }

    // cmd_array[1] points to jid of process
    cmd_array[1]++;

    // getting pid and jid
    int bg_jid = atoi(cmd_array[1]);
    int bg_pid = get_job_pid(job_list, bg_jid);

    // error checking
    if (bg_pid == -1) {
        fprintf(stderr, "job not found\n");
        return 1;
    }

    // sending SIGCONT and updating status in job list accordingly
    if (kill(-1 * bg_pid, SIGCONT) == -1) {
        fprintf(stderr, "kill failed");
    }
    update_job_jid(job_list, bg_jid, RUNNING);

    return 0;
}

/*
 * Checking for fg command with error checking. Sends continue signal to command
 * and moves process to foreground. Check status for this process and prints
 * messages accordingly Input: command array Output: int 0 on success, 1 on
 * error
 */
int fg(char *cmd_array[]) {
    // error checking
    if (strncmp(cmd_array[1], "%", 1) != 0) {
        fprintf(stderr, "fg: syntax error");
        return 1;
    }

    // cmd_array[1] points to jid of process
    cmd_array[1]++;

    // getting pid and jid
    int fg_jid = atoi(cmd_array[1]);
    int fg_pid = get_job_pid(job_list, fg_jid);

    // error checking
    if (fg_pid == -1) {
        fprintf(stderr, "job not found\n");
        return 1;
    }

    // sending SIGCONT and updating status in job list accordingly

    if (kill(-1 * fg_pid, SIGCONT) == -1) {
        fprintf(stderr, "kill failed");
    }
    tcsetpgrp(STDIN_FILENO, fg_pid);

    // check status using waitpid
    int status;
    if (waitpid(fg_pid, &status, WUNTRACED) > 0) {
        // stopped/suspended signals
        if (WIFSTOPPED(status)) {
            printf("[%d] (%d) suspended by signal %d\n", fg_jid, fg_pid,
                   WSTOPSIG(status));
            update_job_jid(job_list, fg_jid, STOPPED);
            jid++;
        }

        // terminated by signal
        if (WIFSIGNALED(status)) {
            // terminated by signal
            printf("[%d] (%d) terminated by signal %d\n", fg_jid, fg_pid,
                   WTERMSIG(status));
            remove_job_pid(job_list, fg_pid);
        }

        // exited normally
        if (WIFEXITED(status)) {
            remove_job_pid(job_list, fg_pid);
        }
    } else if (waitpid(fg_pid, &status, WUNTRACED) == -1) {
        fprintf(stderr, "Waitpid failed");
    }

    return 0;
}

/*
 * Reaps foreground process
 */
void reaping_foreground(char *cmd_array[], pid_t *fork_pid) {
    int status;
    if (waitpid((*fork_pid), &status, WUNTRACED) > 0) {
        // Stopped/suspended by signal
        if (WIFSTOPPED(status)) {
            add_job(job_list, jid, (*fork_pid), STOPPED, cmd_array[0]);
            printf("[%d] (%d) suspended by signal %d\n", jid, (*fork_pid),
                   WSTOPSIG(status));
            jid++;
        }

        // terminated by signal
        if (WIFSIGNALED(status)) {
            // terminated by signal
            printf("[%d] (%d) terminated by signal %d\n", jid, (*fork_pid),
                   WTERMSIG(status));
            remove_job_pid(job_list, (*fork_pid));
        }
    } else if (waitpid((*fork_pid), &status, WUNTRACED) == -1) {
        fprintf(stderr, "waitpid failed");
    }
}

/*
 * Reaps all processes in job list (background and stopped foreground processes)
 */
void reaping_background() {
    int curr_pid = get_next_pid(job_list);
    int wstatus;

    while (curr_pid != -1) {  // until reaching end of job list
        int curr_jid = get_job_jid(job_list, curr_pid);

        if (waitpid(curr_pid, &wstatus, WNOHANG | WUNTRACED | WCONTINUED) > 0) {
            // process terminated normally
            if (WIFEXITED(wstatus)) {
                printf("[%d] (%d) terminated with exit status %d\n", curr_jid,
                       curr_pid, WEXITSTATUS(wstatus));
                remove_job_pid(job_list, curr_pid);
            }

            // process terminated by signal
            if (WIFSIGNALED(wstatus)) {
                printf("[%d] (%d) terminated by signal %d\n", curr_jid,
                       curr_pid, WTERMSIG(wstatus));
                remove_job_pid(job_list, curr_pid);
            }

            // process continued by signal
            if (WIFCONTINUED(wstatus)) {
                printf("[%d] (%d) resumed\n", curr_jid, curr_pid);
                update_job_pid(job_list, curr_pid, RUNNING);
            }

            // process stopped by signal
            if (WIFSTOPPED(wstatus)) {
                printf("[%d] (%d) suspended by signal %d\n", curr_jid, curr_pid,
                       WSTOPSIG(wstatus));
                update_job_pid(job_list, curr_pid, STOPPED);
            }
        } else if (waitpid(curr_pid, &wstatus,
                           WNOHANG | WUNTRACED | WCONTINUED) == -1) {
            fprintf(stderr, "waitpid failed");
        }
        // ressign curr_pid to iterate through list of jobs
        curr_pid = get_next_pid(job_list);
    }
}

/*
 * Main function
 */
int main() {
    job_list = init_job_list();
    jid = 1;
    while (1) {
        // read eval while loop begins here

        // can check on background jobs here using waitpid

// dealing with prompt MACRO
#ifdef PROMPT

        if (printf("meowwoof> ") < 0) {
            fprintf(stderr, "Prompt printing error");
            return 1;

        } else {
            fflush(stdout);
        }
#endif
        // part 1 ignoring signals
        // if there is no foreground job, avoid shells exiting
        // use signal(SIGINT, SIGIGN), signal(SIGTSTP, SIGIGN), signal(SIGQUIT,
        // SIGIGN), signal(SIGTTOU, SIGIGN) FOR ERROR CHECKING, if signal fails
        // => EXIT

        char buffer[1024];

        // reading commands!
        ssize_t read_value = read_input(buffer);

        ignoring_signals();
        // to avoid timeout errors
        if (read_value == 0) {
            cleanup_job_list(job_list);
            exit(0);
        }

        // only execute these executions if the read value isn't the "End of
        // File"
        if (read_value != EOF) {
            char *cmd_array[512];

            // keeps track of number of strings in input, set in parsing, used
            // in checking for commands handled by my shell e.g. "fg", "bg",
            // "cd" etc
            int counter = 0;
            int *counter_ptr = &counter;

            // initialising cmd_array to avoid weird horrible horrible errors
            // (and allocate memory!)
            for (int i = 0; i < 512; i++) {
                cmd_array[i] = NULL;
            }

            // redirection array keeps track of filenames for input and/or
            // output index 0 => input, index 1 => output, index 2 => ouput
            // (with the >>)
            char *redirection_file_array[3] = {"1", "1", "1"};

            // line parsing
            int parse =
                parsing(cmd_array, buffer, redirection_file_array, counter_ptr);

            // boolean to keep track of user input where forking and execution
            // is not needed
            int skip_exec = 0;

            // checking for if parsing is successful
            if (parse == 0) {
                // check for built in functions to execute
                if (built_in_methods(cmd_array) == 1) {
                    skip_exec = 1;
                }
            }

            // If input is blank, skip forking and execution
            if (counter == 0) {
                skip_exec = 1;
            }

            // Kepping track of whether command requests a job to be run in the
            // background with andpersand
            int background_boolean = 0;
            if (counter != 0 && strncmp(cmd_array[counter - 1], "&", 1) == 0 &&
                cmd_array[counter] == NULL) {
                background_boolean = 1;
            }

            // If jobs is called correctly (not null input, command after jobs
            // is null, strings match), execute jobs()
            if (counter != 0 && strcmp(cmd_array[0], "jobs") == 0 &&
                cmd_array[1] == NULL) {
                jobs(job_list);
                // avoid forking and excution for this user input
                skip_exec = 1;
            }

            // If bg is called, foreground process is brought into the
            // background
            if (counter != 0 && strcmp(cmd_array[0], "bg") == 0 &&
                cmd_array[1] != NULL) {
                if (bg(cmd_array) == 1) {
                    continue;
                }
                // avoid forking and excution for this user input
                skip_exec = 1;
            }

            // If fg is called, background processes are sent to the background
            if (counter != 0 && strcmp(cmd_array[0], "fg") == 0 &&
                cmd_array[1] != NULL) {
                if (fg(cmd_array) == 1) {
                    continue;
                }
                // avoid forking and excution for this user input
                skip_exec = 1;
            }

            if (!skip_exec) {  // if command inputted does not require forking
                               // and execution, (e.g. "cd", "fg" etc)
                // getting pid of child process for reaping later in code
                pid_t fork_pid = fork();
                pid_t *fork_pid_ptr = &fork_pid;

                // forking
                if (!fork_pid) {
                    // setting PID of child prcoess to be different from the
                    // parent process
                    setpgid(0, 0);
                    pid_t pid = getpid();

                    if (background_boolean) {  // setting andpersand to null if
                                               // inputted so process can be
                                               // executed
                        cmd_array[counter - 1] = NULL;

                    } else {
                        // setting child processes to be in the windo
                        if (tcsetpgrp(STDIN_FILENO, pid) == -1) {
                            fprintf(stderr, "tcsetpgrp failed");
                        }
                    }

                    // setting signals to default
                    default_signals();

                    // checking for redirection
                    if (redirection(redirection_file_array) !=
                        1) {  // returns 1 if error occurred withinn the
                              // function so that
                        // execv isn't executed

                        // setting up variables to be passed in as parameters to
                        // execv
                        const char *filename = cmd_array[0];
                        cmd_array[0] = (strrchr(cmd_array[0], '/'));
                        cmd_array[0]++;

                        // execv
                        if (execv(filename, cmd_array) == -1) {
                            perror("execv");
                        }

                        // will never reach here but just in case, cleaning job
                        // list before exiting loop
                        cleanup_job_list(job_list);
                        exit(1);
                    }
                }

                // if andpersand is inputted correctly, add background job to
                // job list
                if (background_boolean) {
                    add_job(job_list, jid, fork_pid, RUNNING, cmd_array[0]);
                    printf("[%d] (%d)\n", jid, fork_pid);
                    jid++;
                }

                // reaping foreground processes but only if the command inputted
                // did not have an andpersand and was not blank
                if (counter != 0 &&
                    strncmp(cmd_array[counter - 1], "&", 1) != 0) {
                    reaping_foreground(cmd_array, fork_pid_ptr);
                }
            }

            // reaping for backrgound processes
            reaping_background();

            // setting parent process group back to the window
            pid_t parent = getpgrp();
            if (tcsetpgrp(STDIN_FILENO, parent) == -1) {
                fprintf(stderr, "tcsetpgrp failed");
            }

        } else if (read_value == -1) {
            // this means error has occurred
            perror("Read error has occurred");

        } else {
            exit(0);
            cleanup_job_list(job_list);
        }
    }

    // readl eval while loop ends here
    // cleaning job list just before leaving REPL
    cleanup_job_list(job_list);

    return 0;
}
