/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory (CS39006) Spr 2023-24
 *  Date:       31/01/2024

 *  File:       prodcons.c
 *  Purpose:    To solve the problem statement posed by [http://cse.iitkgp.ac.in/~abhij/course/theory/OS/Spring24/lab/LA4.pdf]
 *  Compile:    gcc -o prodcons prodcons.c -DVERBOSE -DSLEEP
 *  Run:        ./prodcons
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(int argc, char *argv[])
{
    int n, t;       // n = no of child processes, t = no of items to be produced
    int shm_id;     // Shared memory id
    int *p;         // Pointer to shared memory
    pid_t pid;      // Process id

    // Create shared memory
    shm_id = shmget(IPC_PRIVATE, 2*sizeof(int), IPC_CREAT | 0777);

    printf("Enter the number of consumers: ");
    scanf("%d", &n);
    printf("Enter the number of items to be produced: ");
    scanf("%d", &t);
    printf("\n");

    // Create n processes
    for (int i = 0; i < n; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            int checksum = 0;
            int item = 0;

            // Attach to shared memory
            p = (int *)shmat(shm_id, NULL, 0);

            printf("\t\t\t\t\t\tConsumer %d is alive\n", i + 1);
            while (1)
            {
                // Wait for p[0] to be non-zero
                while (p[0] == 0)
                {
                    // Busy wait
                }

                if (p[0] == -1)
                {
                    // Detach and remove shared memory
                    shmdt(p);
                    printf("\t\t\t\t\t\tConsumer %d has read %d items: Checksum = %d\n", i + 1, item, checksum);
                    exit(i + 1);
                }
                
                if (p[0] == i + 1)
                {
                    checksum += p[1];
                    item += 1;

                    p[0] = 0;

                    #ifdef VERBOSE
                        printf("\t\t\t\t\t\tConsumer %d reads %d\n", i + 1, p[1]);
                    #endif
                }
            }
        }
        else if (pid < 0)
        {
            perror("fork");
            exit(1);
        }
    }

    // Parent process
    printf("Producer is alive\n");

    // Attach to shared memory
    p = (int *)shmat(shm_id, NULL, 0);

    int *checksums = (int *)malloc(n * sizeof(int));    // Create an array to store the checksums of the child processes
    int *items = (int *)malloc(n * sizeof(int));        // Create an array to store the no of items produced for each child process
    
    // Initialize the arrays
    for (int i = 0; i < n; i++)
    {
        checksums[i] = 0;
        items[i] = 0;
    }

    srand(time(0));     // Seed the random number generator
    p[0] = 0;           // Initialize p[0] to 0

    int c, s;

    for (int i = 0; i < t; i++)
    {
        while (p[0] != 0)
        {
            // Busy wait
        }

        // Set p[0] to any random 1 to n
        c = 1 + rand() % n;
        s = 100 + rand() % (999 - 100 + 1);
        items[c - 1] += 1;
        checksums[c - 1] += s;
        
        p[0] = c;

        #ifdef SLEEP
            usleep(10);
        #endif

        // Generate random 3 digit number and assign to p[1]
        p[1] = s;


        #ifdef VERBOSE
            printf("Producer produces %d for Consumer %d\n", s, c);
        #endif
    }
    usleep(500);    // This is to ensure that th consumer processes has read the last item
    p[0] = -1;


    for (int i = 0; i < n; i++)
    {
        wait(NULL);
    }

    // Detach and remove shared memory
    shmdt(p);
    shmctl(shm_id, IPC_RMID, NULL);

    // Print the checksums and no of items produced by each consumer
    printf("Producer has produced %d items\n", t);
    for (int i = 0; i < n; i++)
    {
        printf("%d items for Consumer %d: Checksum = %d\n", items[i], i + 1, checksums[i]);
    }

    return 0;
}