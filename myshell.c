/*
    COMP3511 Spring 2026
    PA1: Simplified Linux Shell (MyShell)

    Your name:
    Your ITSC email:tytseae@connect.ust.hk

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

*/

/*
    Header files for MyShell
    Necessary header files are included.
    Do not include extra header files
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h>    // For open/read/write/close syscalls
#include <signal.h>   // For signal handling
#include <errno.h>

#define MYSHELL_MESSAGE "Myshell for COMP3511 PA1 (Spring 2026)"

// Define template strings so that they can be easily used in printf
//
// Usage: assume pid is the process ID
//
//  printf(TEMPLATE_MYSHELL_START, pid);
//
#define TEMPLATE_MYSHELL_START "Myshell (pid=%d) starts\n"
#define TEMPLATE_MYSHELL_END "Myshell (pid=%d) ends\n"
#define TEMPLATE_MYSHELL_TERMINATE "Myshell (pid=%d) terminates by Ctrl-C with signal: %d\n"
#define TEMPLATE_MYSHELL_CD_ERROR "Myshell cd command error\n"

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LENGTH 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters:
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements,
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
// We also need to add an extra NULL item to be used in execvp
// Thus: 8 + 1 = 9
//
// Example:
//   echo a1 a2 a3 a4 a5 a6 a7
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the standard file descriptor IDs here
#define STDIN_FILENO 0  // Standard input
#define STDOUT_FILENO 1 // Standard output

// This function will be invoked by main()
// TODO: Replace the shell prompt with your own ITSC account name
void show_prompt(char *prompt, char *path)
{

    // TODO: replace the shell prompt with your ITSC account name
    // For example, if you ITSC account is cspeter@connect.ust.hk
    // You should replace ITSC with cspeter
    printf("%s %s> ", prompt, path);
}

// This function will be invoked by main()
// This function is given
int get_cmd_line(char *cmdline) //
{
    int i;
    int n;
    if (!fgets(cmdline, MAX_CMDLINE_LENGTH, stdin))
        return -1;
    // Ignore the newline character
    n = strlen(cmdline);
    if (n == 0)
        return -1;
    cmdline[--n] = '\0';
    i = 0;
    while (i < n && cmdline[i] == ' ')
    {
        ++i;
    }
    if (i == n)
    {
        // Empty command
        return -1;
    }
    return 0;
}

// parse_arguments function is given
// This function helps you parse the command line
//
// Suppose the following variables are defined:
//
// char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
// int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
// char cmdline[MAX_CMDLINE_LENGTH]; // The input command line
//
// Sample usage:
//
//  parse_arguments(pipe_segments, cmdline, &num_pipe_segments, "|");
//
void parse_arguments(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

// Helper: trim leading and trailing spaces in-place
static void trim_inplace(char *s)
{
    if (!s) return;
    // trim leading
    char *start = s;
    while (*start && (*start == ' ' || *start == '\t')) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    // trim trailing
    int len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t')) s[--len] = '\0';
}

// TODO: Implement process_cmd
void process_cmd(char *cmdline) // only child calls it, as execvp replace address space
{                               // all happens in child process
    // Uncomment this line to check the content of cmdline
    // printf("cmdline = %s\n", cmdline);
    // my setup restore cmdline to its original form

    // so Step 1: get tokenized cmdline into arg
    char *arg[MAX_ARGUMENTS_PER_SEGMENT];                             // tokenized arguments in arg[0], arg[1] ...
    int arg_num;                                          // number of tokens
    parse_arguments(arg, cmdline, &arg_num, SPACE_CHARS); // store tokens in arg

    // safety: do not exceed buffer
    if (arg_num >= MAX_ARGUMENTS_PER_SEGMENT) {
        fprintf(stderr, "Too many arguments (max %d)\n", MAX_ARGUMENTS_PER_SEGMENT - 1);
        exit(1);
    }

    // cmdline only has arg[0] now
    // for loop handle the input redirection case
    for (int i = 0; i < arg_num; i++)
    { // search for index i for "<"
        if (!strcmp(arg[i], "<"))
        {                                        // input case
            if (i + 1 >= arg_num) {
                fprintf(stderr, "No input file specified for '<'\n");
                exit(1);
            }
            int fd = open(arg[i + 1], O_RDONLY); // use open to get file descriptor of that particular file
            if (fd == -1) { perror("open"); exit(1); }
            // open() note: arg[i+1] is the file name, O_RDONLY is the flag for read only access
            if (dup2(fd, STDIN_FILENO) == -1) { perror("dup2"); close(fd); exit(1); } // stdin(0) points to the new fd --> redirection
            close(fd);   // closing fd, why we do this? lab example?

            // now we delete the < filename before we pass it to the binary
            for (int j = i; j < arg_num - 2; j++)
            { // the "<"
                arg[j] = arg[j + 2];
            }
            arg_num -= 2;
            break; // end for as we handle at most 1 of this
        }
    }
    for (int i = 0; i < arg_num; i++)
    { // handle output case
        if (!strcmp(arg[i], ">"))
        {                                                                  // output case
            if (i + 1 >= arg_num) {
                fprintf(stderr, "No output file specified for '>'\n");
                exit(1);
            }
            int fd = open(arg[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666); // FIXED: added ,0666 (required by O_CREAT)
            if (fd == -1) { perror("open"); exit(1); }
            // open() note: arg[i+1] is the file name, and apparently we need more flag
            if (dup2(fd, STDOUT_FILENO) == -1) { perror("dup2"); close(fd); exit(1); } // stdin(1) points to the new fd --> redirection
            close(fd);   ////closing fd, why we do this? lab example?

            // now we delete the > filename before we pass it to the binary
            for (int j = i; j < arg_num - 2; j++)
            { // the ">"
                arg[j] = arg[j + 2];
            }
            arg_num -= 2;

            break; // end for as we only need to do one of it at a time
        }
    }
    arg[arg_num] = NULL; // required by execvp

    if (arg_num == 0 || arg[0] == NULL || strlen(arg[0]) == 0) {
        exit(0);
    }

    execvp(arg[0], arg); // execute: replace the fork with arg[0] binary, eg wc, ls, etc
    // arg is the pointer to char * (array of strings) stores the tokenized command
    perror("execvp");
    exit(127); // ensure the process cmd is finished
}

// TODO: Implement a signal handler
void signal_callback(int sig)
{
    printf(TEMPLATE_MYSHELL_TERMINATE, getpid(), sig);
    exit(1);
}

/* The main function implementation */
int main()
{
    printf("%s\n\n", MYSHELL_MESSAGE);

    // TODO: Associate the interrupt signal to the handler
    signal(SIGINT, signal_callback);
    // TODO: replace the shell prompt with your ITSC account name
    // For example, if you ITSC account is cspeter@connect.ust.hk
    // You should replace ITSC with cspeter
    char *prompt = "tytseae";
    char cmdline[MAX_CMDLINE_LENGTH];
    char cmdlineCopy[MAX_CMDLINE_LENGTH];
    printf(TEMPLATE_MYSHELL_START, getpid());

    char *arg[MAX_ARGUMENTS];
    int arg_num;
    char path[256];

    // The main event loop
    char exit_char[] = "exit";
    while (1)
    {
        getcwd(path, 256);
        show_prompt(prompt, path);
        if (get_cmd_line(cmdline) == -1)
            continue; /* empty line handling */

        // TODO: implement the exit command
        if (!strcmp(cmdline, exit_char))
        {
            printf(TEMPLATE_MYSHELL_END, getpid());
            break;
        }

        // TODO: implement the cd command
        // Hint: You can use parse_arguments here
        // The 2nd param will be changed after calling parse_arguments, so we need to backup a copy
        // the change: wc -l < myshell.c -> wc as it inserts null terminator whenever there's SPACE_CHARS
        strncpy(cmdlineCopy, cmdline, MAX_CMDLINE_LENGTH);
        cmdlineCopy[MAX_CMDLINE_LENGTH - 1] = '\0';
        parse_arguments(arg, cmdlineCopy, &arg_num, SPACE_CHARS); // divide command according to space, store segments to arg array
        // when did I add this???//strcpy(cmdline, cmdlineCopy); //restore cmdline
        // special case: cd handling
        if (arg_num > 0 && !strcmp(arg[0], "cd"))
        {                           // detect if its cd
            if (arg_num < 2) {
                char *home = getenv("HOME");
                if (!home) {
                    printf(TEMPLATE_MYSHELL_CD_ERROR);
                    continue;
                }
                if (chdir(home) == -1) {
                    printf(TEMPLATE_MYSHELL_CD_ERROR);
                    continue;
                }
            } else {
                if (chdir(arg[1]) == -1)
                {                                      // fail condition
                    printf(TEMPLATE_MYSHELL_CD_ERROR); // print error message
                    continue;                          // continue the polling
                }
            }
            continue; // continue if everything fine
        }
        /*
                else if(!strcmp(arg[0], "wc") && !strcmp(arg[1], "-l")){ //implement of the wc -l < file command
                    if(!strcmp(arg[2], "<")){ //input redirection case: file content input to feed wc command --> count number of lines
                        int fd = open(arg[3], O_RDONLY); //get file id
                        char buffer[4096]; //buffer for reading file text
                        int bytes;
                        int lines = 0;
                        close(0); //closing stdin, file descriptor 0
                        dup(fd); //replace stdin with file, automatically look for smallest available descriptor
                        while((bytes = read(fd, buffer, 4096))>0){ //read return byte it read, keeps returning the accumulate amount it read until all, then return zero
                            for(int i = 0; i < bytes; i++){ //search for next line character and increase counter
                                if(buffer[i] == '\n'){
                                    lines++;
                                }
                            }
                        }
                        printf("%d\n", lines); //display amount of (nextline character) lines
                    }
                }
                else {
                    strcpy(cmdline, cmdlineCopy); //for some reason cmdline change?
                    //restore it if it is not cd, i.e some other command using "|" as seperation
                    continue; //continue the loop so it doesn't stuck here
                    //but what if we need the latter codes????~
                }
        */
        // pipe handling
        /*
        eg: ls | wc -l
        for (n level pipe)
            int pipe_arr[2]; // arr[0] is input end, arr[1] is output end
            pipe(pipe_arr);
            fork();
            if(pid == 0) // inside child
                dup2(arr[0], STDOUT) //stdout of child(ls) input to pipe
                execlp(ls command portion)
            else //inside parent
                dup2(arr[1], STDIN) //input of parent is output of child
                //input of wc is ls

        */

        char *pipe_segments[MAX_PIPE_SEGMENTS]; // array of strings to store pipe command segments
        int num_pipe_segments;
        strncpy(cmdlineCopy, cmdline, MAX_CMDLINE_LENGTH);
        cmdlineCopy[MAX_CMDLINE_LENGTH - 1] = '\0';
        parse_arguments(pipe_segments, cmdlineCopy, &num_pipe_segments, PIPE_CHAR); // parse our cmdline according to pipe char |
        // Trim whitespace around each segment
        for (int i = 0; i < num_pipe_segments; i++) {
            trim_inplace(pipe_segments[i]);
        }

        if (num_pipe_segments == 1)
        {                       // single command, no pipe case --> original handling
            pid_t pid = fork(); // fork a child
            if (pid == -1) {
                perror("fork");
                continue;
            }
            if (pid == 0)
                process_cmd(pipe_segments[0]); // child will execvp and process cmdline
            else
                wait(0); // parent will wait for child to terminate
        }

        else
        { // multi-pipe
            if (num_pipe_segments > MAX_PIPE_SEGMENTS) {
                fprintf(stderr, "Too many pipe segments (max %d)\n", MAX_PIPE_SEGMENTS);
                continue;
            }

            int pipes[MAX_PIPE_SEGMENTS - 1][2];

            // Create pipes
            for (int i = 0; i < num_pipe_segments - 1; i++)
            {
                if (pipe(pipes[i]) == -1)
                {
                    perror("pipe");
                    // close any previously opened pipes
                    for (int k = 0; k < i; k++) {
                        close(pipes[k][0]);
                        close(pipes[k][1]);
                    }
                    goto wait_children;
                }
            }

            for (int i = 0; i < num_pipe_segments; i++)
            {
                pid_t pid = fork();
                if (pid == -1)
                {
                    perror("fork");
                    // close all pipes
                    for (int j = 0; j < num_pipe_segments - 1; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    break;
                }
                if (pid == 0)
                { // CHILD
                    // Close ALL unused pipes first (prevents any FD leak)
                    // Child only needs pipes[i-1][0] (if i>0) and pipes[i][1] (if i < last)
                    if (i > 0)
                    {
                        if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                            perror("dup2 stdin");
                            exit(1);
                        }
                    }
                    if (i < num_pipe_segments - 1)
                    {
                        if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                            perror("dup2 stdout");
                            exit(1);
                        }
                    }

                    for (int j = 0; j < num_pipe_segments - 1; j++)
                    {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }

                    process_cmd(pipe_segments[i]);
                    exit(127); // IMPORTANT: execvp failed
                }

                // PARENT: close ends we no longer need
                if (i > 0)
                {
                    close(pipes[i - 1][0]); // previous read end
                }
                if (i < num_pipe_segments - 1)
                {
                    close(pipes[i][1]); // current write end
                }
            }

            // NO final cleanup loop needed — everything is already closed
            for (int j = 0; j < num_pipe_segments - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

        wait_children:
            for (int i = 0; i < num_pipe_segments; i++)
            {
                wait(NULL);
            }
        }
    }
    return 0;
}
