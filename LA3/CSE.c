/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory (CS39006) Spr 2023-24
 *  Date:       24/01/2024

 *  File:       CSE.c
 *  Purpose:    To solve the problem statement posed by [http://cse.iitkgp.ac.in/~abhij/course/theory/OS/Spring24/lab/LA3.pdf]
 *  Compile:    gcc -o CSE CSE.c 
 *  Run:        ./CSE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

#define MAXLINE 1024

void sigHandler(int sig)   // ^C, ^Z Signal Handler
{
    write(STDERR_FILENO, "Type exit in the input terminal\n", 32);
}

int main(int argc, char *argv[])
{

    int pipefd[2];      // Create a pipe for default role
    int pipefdrev[2];   // Create a pipe for swapped role
    pid_t child1, child2;   // Create a pipe for communication
    int role = 0;           // 0 for default role, 1 for swapped role

    // Signal Handlers
    signal(SIGINT, sigHandler);
    signal(SIGTSTP, sigHandler);

    // If no arguments are passed, run in supervisor mode
    if (argc == 1)
    {
        
        if (pipe(pipefd) == -1)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        if (pipe(pipefdrev) == -1)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        printf("+++ CSE in supervisor mode: Started\n");
        printf("+++ CSE in supervisor mode: pipefd = [%d, %d]\n", pipefd[0], pipefd[1]);
        
        printf("+++ CSE in supervisor mode: Forking first child in command-input mode\n");        
        child1 = fork();
        if (child1 == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (child1 == 0)
        {
            // Child process 1: Execute gnome-terminal with a command
            close(pipefd[0]);                   // Close reading end of the pipe
            close(pipefdrev[1]);                // Close writing end of the pipe
            dup2(pipefd[1], STDOUT_FILENO);     // Redirect stdout to the writing end of the pipe
            dup2(pipefdrev[0], STDIN_FILENO);   // Redirect stdin to the reading end of the pipe

            char arg1[10];
            char arg2[10];
            char arg3[10];
            char arg4[10];
            sprintf(arg1, "%d", pipefd[0]);
            sprintf(arg2, "%d", pipefd[1]);
            sprintf(arg3, "%d", pipefdrev[0]);
            sprintf(arg4, "%d", pipefdrev[1]);
            execl("/usr/bin/xterm", "xterm", "-T", "Terminal 1", "-e", "./CSE", "C", arg1, arg2, arg3, arg4, NULL);

            // This part will only be reached if execl fails
            perror("execl");
            exit(EXIT_FAILURE);
        }

        // Spawn second terminal
        printf("+++ CSE in supervisor mode: Forking second child in execute mode\n");
        child2 = fork();
        if (child2 == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (child2 == 0)
        {
            // Child process 2: Execute gnome-terminal with a command
            close(pipefd[1]);                   // Close writing end of the pipe
            close(pipefdrev[0]);                // Close reading end of the pipe
            dup2(pipefd[0], STDIN_FILENO);      // Redirect stdin to the reading end of the pipe
            dup2(pipefdrev[1], STDOUT_FILENO);  // Redirect stdout to the writing end of the pipe

            char arg1[10];
            char arg2[10];
            char arg3[10];
            char arg4[10];
            sprintf(arg1, "%d", pipefd[0]);
            sprintf(arg2, "%d", pipefd[1]);
            sprintf(arg3, "%d", pipefdrev[0]);
            sprintf(arg4, "%d", pipefdrev[1]);
            execl("/usr/bin/xterm", "xterm", "-T", "Terminal 2", "-e", "./CSE", "E", arg1, arg2, arg3, arg4, NULL);

            // This part will only be reached if execl fails
            perror("execl");
            exit(EXIT_FAILURE);
        }

        // Close the pipe in the parent process
        close(pipefd[0]);
        close(pipefd[1]);
        close(pipefdrev[0]);
        close(pipefdrev[1]);

        // Wait for both child processes to exit
        waitpid(child1, NULL, 0);
        printf("+++ CSE in supervisor mode: First child terminated\n");
        waitpid(child2, NULL, 0);
        printf("+++ CSE in supervisor mode: Second child terminated\n");
    }

    else
    {   
        while(1)
        {
            // Terminal 1 in command-input mode
            if (((strcmp(argv[1], "C") == 0)&&(role == 0)))
            {
                while(1)
                {
                    fprintf(stderr, "Enter command> ");
                    char command[MAXLINE];
                    for (int i = 0; i < MAXLINE; i++)
                    {
                        command[i] = '\0';
                    }
                    fgets(command, MAXLINE, stdin);
                    if (command[strlen(command) - 1] == '\n')
                    {
                        command[strlen(command) - 1] = '\0';
                    }

                    fflush(stdin);
                    write(atoi(argv[3]), command, strlen(command));
                    if (strncmp(command, "exit", 4) == 0)
                    {
                        exit(EXIT_SUCCESS);
                    }
                    if (strncmp(command, "swaprole", 8) == 0)
                    {
                        role = 1 - role;
                        break;
                    }
                }
            }

            // Terminal 2 in execute mode
            else if (((strcmp(argv[1], "E") == 0)&&(role == 0)))
            {
                int c;
                while(1)
                {
                    fprintf(stdout, "Waiting for command> ");
                    fflush(stdout);
                    char command[MAXLINE];
                    for (int i = 0; i < MAXLINE; i++)
                    {
                        command[i] = '\0';
                    }
                    read(atoi(argv[2]), command, MAXLINE);
                    // end command with terminator
                    printf("%s\n", command);
                    if (strncmp(command, "exit", 4) == 0)
                    {
                        exit(EXIT_SUCCESS);
                    }
                    if (strncmp(command, "swaprole", 8) == 0)
                    {
                        role = 1 - role;
                        break;
                    }
                    
                    // Create a child to execute the command
                    pid_t gchild = fork();
                    if (gchild == -1)
                    {
                        perror("fork");
                        exit(EXIT_FAILURE);
                    }
                    if (gchild == 0)
                    {
                        // Child process: Execute command
                        execl("/bin/sh", "sh", "-c", command, NULL);
                        // This part will only be reached if execl fails
                        perror("execl");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        // Parent process: Wait for child to exit
                        waitpid(gchild, NULL, 0);
                    }

                }
                
            }

            // Terminal 1 in execute mode
            else if (((strcmp(argv[1], "C") == 0)&&(role == 1)))
            {
                int c;
                while (1)
                {
                    fprintf(stdout, "Waiting for command> ");
                    fflush(stdout);
                    char command[MAXLINE];
                    for (int i = 0; i < MAXLINE; i++)
                    {
                        command[i] = '\0';
                    }
                    read(atoi(argv[4]), command, MAXLINE);
                    printf("%s\n", command);

                    if (strncmp(command, "exit", 4) == 0)
                    {
                        exit(EXIT_SUCCESS);
                    }
                    if (strncmp(command, "swaprole", 8) == 0)
                    {
                        role = 1 - role;
                        break;
                    }
                    // Create a child to execute the command
                    pid_t gchild = fork();
                    if (gchild == -1)
                    {
                        perror("fork");
                        exit(EXIT_FAILURE);
                    }
                    if (gchild == 0)
                    {
                        // Child process: Execute command
                        execl("/bin/sh", "sh", "-c", command, NULL);
                        // This part will only be reached if execl fails
                        perror("execl");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        // Parent process: Wait for child to exit
                        waitpid(gchild, NULL, 0);
                    }
                }
            }

            // Terminal 2 in command-input mode
            else if (((strcmp(argv[1], "E") == 0)&&(role == 1)))
            {
                while(1)
                {
                    fprintf(stderr, "Enter command> ");
                    char command[MAXLINE];
                    for (int i = 0; i < MAXLINE; i++)
                    {
                        command[i] = '\0';
                    }
                    fgets(command, MAXLINE, stdin);
                    if (command[strlen(command) - 1] == '\n')
                    {
                        command[strlen(command) - 1] = '\0';
                    }

                    fflush(stdin);
                    write(atoi(argv[5]), command, strlen(command));
                    if (strncmp(command, "exit", 4) == 0)
                    {
                        exit(EXIT_SUCCESS);
                    }
                    if (strncmp(command, "swaprole", 8) == 0)
                    {
                        role = 1 - role;
                        break;
                    }
                }
            }
        }
    }

    return 0;
}