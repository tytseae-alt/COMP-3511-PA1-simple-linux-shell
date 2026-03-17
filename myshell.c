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

// TODO: Implement process_cmd
void process_cmd(char *cmdline)
{
    // Uncomment this line to check the content of cmdline
    // printf("cmdline = %s\n", cmdline);
    

    exit(0); // ensure the process cmd is finished
}

// TODO: Implement a signal handler
void signal_callback(int sig){
    printf(TEMPLATE_MYSHELL_TERMINATE, getpid(),sig);
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
    char exit[] = "exit";
    while (1)
    {
        getcwd(path, 256);
        show_prompt(prompt, path);
        if (get_cmd_line(cmdline) == -1)
            continue; /* empty line handling */

        // TODO: implement the exit command
        if(!strcmp(cmdline,exit)){
            printf(TEMPLATE_MYSHELL_END, getpid());
            break;
        }
            
        // TODO: implement the cd command
        // Hint: You can use parse_arguments here
        // The 2nd param will be changed after calling parse_arguments, so we need to backup a copy  
        strcpy(cmdlineCopy, cmdline);


        pid_t pid = fork();
        if (pid == 0)
        {
            // the child process handles the command
            process_cmd(cmdline);
        }
        else
        {
            // the parent process simply wait for the child and do nothing
            wait(0);
        }
    }

    return 0;
}