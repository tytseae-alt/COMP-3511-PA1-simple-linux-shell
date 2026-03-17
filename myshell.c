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
void process_cmd(char *cmdline) //only child calls it, as execvp replace address space 
{ //all happens in child process 
    // Uncomment this line to check the content of cmdline
    //printf("cmdline = %s\n", cmdline);
    //my setup restore cmdline to its original form 

    //so Step 1: get tokenized cmdline into arg 
    char *arg[MAX_ARGUMENTS]; //tokenized arguments in arg[0], arg[1] ... 
    int arg_num; //number of tokens 
    parse_arguments(arg, cmdline, &arg_num,SPACE_CHARS); //store tokens in arg 
    
    // cmdline only has arg[0] now 
    //for loop handle the input and output redirection case 
    for(int i = 0; i < arg_num; i++){ //search for index i for "<" 
        if(!strcmp(arg[i], "<")){  //input case
            int fd = open(arg[i+1], O_RDONLY); // use open to get file descriptor of that particular file 
            //open() note: arg[i+1] is the file name, O_RDONLY is the flag for read only access 
            dup2(fd, 0); // stdin(0) points to the new fd --> redirection 
        }

        if(!strcmp(arg[i], ">")){  //input case
            int fd = open(arg[i+1], O_WRONLY | O_CREAT | O_TRUNC); // use open to get file descriptor of that particular file 
            //open() note: arg[i+1] is the file name, and apparently we need more flag
            dup2(fd, 1); // stdin(1) points to the new fd --> redirection 
        }

    }
    execvp(arg[0], arg); //execute: replace the fork with arg[0] binary, eg wc, ls, etc
    //arg is the pointer to char * (array of strings) stores the tokenized command
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
        // the change: wc -l < myshell.c -> wc as it inserts null terminator whenever there's SPACE_CHARS
        strcpy(cmdlineCopy, cmdline);
        parse_arguments(arg, cmdline, &arg_num, SPACE_CHARS); //divide command according to space, store segments to arg array 
        strcpy(cmdline, cmdlineCopy); //restore cmdline 
        //special case: cd handling 
        if(!strcmp(arg[0],"cd")){ //detect if its cd 
            strcpy(path, arg[1]); //change the path variable to destination
            int flag = chdir(path); //head to path, store status in flag  
            if(flag == -1){ //fail condition 
                printf(TEMPLATE_MYSHELL_CD_ERROR); //print error message 
                continue; //continue the polling
            }
            continue; //continue if everything fine
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