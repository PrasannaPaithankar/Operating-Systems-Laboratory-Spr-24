/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory (CS39006) Spr 2023-24
 *  Date:       17/01/2024

 *  File:       mgr.c
 *  Purpose:    To solve the problem statement posed by [http://cse.iitkgp.ac.in/~abhij/course/theory/OS/Spring24/lab/LA2.pdf]
 *  Compile:    gcc mgr.c -o mrg
 *  Run:        ./mgr
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

/*
 *  Status Codes:
 *  ************
 *  FINISHED    -- 1
 *  TERMINATED  -- 2
 *  SUSPENDED   -- 3
 *  KILLED      -- 4
 */

// Global variables
pid_t pid;
int PT[11][4];              // Process Table with columns <pid, pgid, status code, (int) character arg to the job>
int PT_entries = 0;         // No. of entries

// Signal Handlers
void sigHandlerC(int sig)   // ^C Signal Handler
{
    kill(pid, SIGINT);
for (int i = 0; i < PT_entries+1; i++)
    {
        if(PT[i][0] == pid)
        {
            PT[i][2] = 2;   // Update status
        }
    }
}

void sigHandlerZ(int sig)   // ^Z Signal Handler
{
    kill(pid, SIGTSTP);
    for (int i = 0; i < PT_entries+1; i++)
    {
        if(PT[i][0] == pid)
        {
            PT[i][2] = 3;   // Update status
        }
    }
}

int main(int argc, char *argv[])
{  
    // Set up signal handlers
    signal(SIGINT, sigHandlerC);
    signal(SIGTSTP, sigHandlerZ);

    // Fill Process Table for the mgr process
    PT[0][0] = getpid();
    PT[0][1] = getpgid(getpid());
    PT[0][2] = 0;
    PT[0][3] = 0;

    // Start the REPL environment
    printf("CS39002 Operating Systems Laboratory\nAssignment 2\nAuthor: Prasanna Paithankar (21CS30065)");
    while(1)
    {
        printf("\nmgr> ");
        char cmd;
        scanf(" %c", &cmd);  
        fflush(stdin);

        if(cmd == 'c')
        {
            printf("\nSuspended jobs: ");
            for(int i = 0; i < PT_entries+1; i++)
            {
                if(PT[i][2] == 3)
                {
                    printf("%d ", i);
                }
            }
            printf("(Pick one): ");
            int job;
            scanf("%d", &job);
            if(PT[job][2]==3 && job <= PT_entries)
            {
                PT[job][2] = 1;
                pid = PT[job][0];
                kill(PT[job][0], SIGCONT);
            }
        }
        
        else if(cmd == 'h')
        {
            printf("Command\t : Action\n");
            printf("c\t : Continue a suspended job\n");
            printf("h\t : Print this help message\n");
            printf("k\t : Kill a suspended job\n");
            printf("p\t : Print the process table\n");
            printf("q\t : Quit\n");
            printf("r\t : Run a new job\n");
        }
        
        else if(cmd == 'k')
        {
            printf("\nSuspended jobs: ");
            for(int i = 0; i < PT_entries+1; i++)
            {
                if(PT[i][2] == 3)
                {
                    printf("%d ", i);
                }
            }
            printf("(Pick one): ");
            int job2;
            scanf("%d", &job2);
            if(PT[job2][2]==3 && job2 <= PT_entries)
            {
                PT[job2][2] = 4;
                kill(PT[job2][0], SIGKILL);
            }
        }
        
        else if(cmd == 'p')
        {
            printf("NO.\t\tPID\t\tPGID\t\tSTATUS\t\tNAME\n");
            printf("0\t\t%d\t\t%d\t\tSELF\t\tmgr\n", PT[0][0], PT[0][1]);
            for(int i = 1; i < PT_entries+1; i++)
            {
                printf("%d\t\t%d\t\t%d\t\t", i, PT[i][0], PT[i][1]);
                if(PT[i][2] == 1)
                {
                    printf("FINISHED\t");
                }
                else if(PT[i][2] == 2)
                {
                    printf("TERMINATED\t");
                }
                else if(PT[i][2] == 3)
                {
                    printf("SUSPENDED\t");
                }
                else if(PT[i][2] == 4)
                {
                    printf("KILLED\t\t");
                }
                printf("job %c\n", PT[i][3]);
            }
        }

        else if(cmd == 'q')
        {
            printf("Exiting...\n");
            exit(0);
        }
        
        else if(cmd == 'r')
        {
            PT_entries++;
            if (PT_entries > 10)
            {
                printf("Process table is full. Quitting...\n");
                exit(0);
            }

            srand((unsigned int)time(NULL));
            char letter = 'A' + (rand() % 26);

            pid = fork();

            // Change the process group id of the child process so as to only act on the child process
            setpgid(pid, 0);

            // Fill Process Table for the job
            PT[PT_entries][0] = pid;
            PT[PT_entries][1] = pid;
            PT[PT_entries][2] = 1;
            PT[PT_entries][3] = letter;

            if(pid < 0)
            {
                printf("Fork error\n");
                exit(1);
            }
            else if(pid == 0)
            {
                char *args[] = {"./job", &letter, NULL};
                execvp(args[0], args);
                exit(0);
            }
            else
            {
                waitpid(pid, NULL, WUNTRACED);  // also return if the child has stopped, refer man waitpid
            }
        }
        
        else
        {
            printf("Invalid command\n");
        }   
    }

    return 0;
}
