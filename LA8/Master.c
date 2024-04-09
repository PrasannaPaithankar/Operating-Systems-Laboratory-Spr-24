/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory Spr 2023-24
 *  Date:       07/04/2024

 *  File:       Master.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <semaphore.h>

#define PROB 5

struct
pageTable
{
    int frame;
    int valid;
    int timestamp;
};

struct
freeFrameList
{
    int free;
};

struct
proc2page
{
    sem_t sem;
    sem_t sem2;
    sem_t sem3[1024];
    sem_t sem4;
    int noOfProcesses;
    int maxNoOfPages;
    int maxFreeFrames;
    pid_t pid[1024];
    int noOfPages[1024];
    int noOfFaults[1024];
};


int
main (int argc, char *argv[])
{
    int k, m, f, curridx = 0;

    // Parse command line arguments
    if (argc != 7)
    {
        fprintf(stderr, "Usage: %s -k <number of processes> -m <Virtual Address Space size> -f <Physical Address Space size>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int opt;
    while ((opt = getopt(argc, argv, "k:m:f:")) != -1)
    {
        switch (opt)
        {
            case 'k':
                k = atoi(optarg);
                break;
            case 'm':
                m = atoi(optarg);
                break;
            case 'f':
                f = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -k <number of processes> -m <Virtual Address Space size> -f <Physical Address Space size>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    FILE *fp = fopen("/proc/sys/fs/mqueue/msg_max", "r");
    if (fp == NULL)
    {
        perror("Master: fopen");
        exit(EXIT_FAILURE);
    }

    int maxmsg;
    fscanf(fp, "%d", &maxmsg);
    fclose(fp);

    // Create shared memory Page Table
    int shmidpageTable = shm_open("SM1", O_CREAT | O_RDWR, 0666);
    if (shmidpageTable == -1)
    {
        perror("Master: shm_open");
        exit(EXIT_FAILURE);
    }

    // Resize shared memory object
    if (ftruncate(shmidpageTable, k * m * sizeof(struct pageTable)) == -1)
    {
        perror("Master: ftruncate");
        exit(EXIT_FAILURE);
    }

    // Map shared memory object
    struct pageTable *SM1 = mmap(NULL, k * m * sizeof(struct pageTable), PROT_READ | PROT_WRITE, MAP_SHARED, shmidpageTable, 0);
    if (SM1 == MAP_FAILED)
    {
        perror("Master: mmap");
        exit(EXIT_FAILURE);
    }

    // Create shared memory Free Frame List
    int shmidfreeFrameList = shm_open("SM2", O_CREAT | O_RDWR, 0666);
    if (shmidfreeFrameList == -1)
    {
        perror("Master: shm_open");
        exit(EXIT_FAILURE);
    }

    // Resize shared memory object
    if (ftruncate(shmidfreeFrameList, f * sizeof(struct freeFrameList)) == -1)
    {
        perror("Master: ftruncate");
        exit(EXIT_FAILURE);
    }

    // Map shared memory object
    struct freeFrameList *SM2 = mmap(NULL, f * sizeof(struct freeFrameList), PROT_READ | PROT_WRITE, MAP_SHARED, shmidfreeFrameList, 0);
    if (SM2 == MAP_FAILED)
    {
        perror("Master: mmap");
        exit(EXIT_FAILURE);
    }

    // Create shared memory Process to Page List
    int shmidproc2pageList = shm_open("SM3", O_CREAT | O_RDWR, 0666);
    if (shmidproc2pageList == -1)
    {
        perror("Master: shm_open");
        exit(EXIT_FAILURE);
    }

    // Resize shared memory object
    if (ftruncate(shmidproc2pageList, sizeof(struct proc2page)) == -1)
    {
        perror("Master: ftruncate");
        exit(EXIT_FAILURE);
    }

    // Map shared memory object
    struct proc2page *SM3 = mmap(NULL, sizeof(struct proc2page), PROT_READ | PROT_WRITE, MAP_SHARED, shmidproc2pageList, 0);
    if (SM3 == MAP_FAILED)
    {
        perror("Master: mmap");
        exit(EXIT_FAILURE);
    }

    // Initialize shared memory objects
    for (int i = 0; i < k * m; i++)
    {
        SM1[i].frame = -1;
        SM1[i].valid = 0;
        SM1[i].timestamp = 0;
    }

    for (int i = 0; i < f; i++)
    {
        SM2[i].free = 1;
    }

    SM3->noOfProcesses = k;
    SM3->maxNoOfPages = m;
    SM3->maxFreeFrames = f;

    if (sem_init(&SM3->sem, 1, 0) == -1)
    {
        perror("Master: sem_init");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&SM3->sem2, 1, 0) == -1)
    {
        perror("Master: sem_init");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < k; i++)
    {
        if (sem_init(&SM3->sem3[i], 1, 0) == -1)
        {
            perror("Master: sem_init");
            exit(EXIT_FAILURE);
        }

        SM3->pid[i] = -1;
        SM3->noOfPages[i] = 0;
        SM3->noOfFaults[i] = 0;
    }

    if (sem_init(&SM3->sem4, 1, 0) == -1)
    {
        perror("Master: sem_init");
        exit(EXIT_FAILURE);
    }
    
    // Create message queues
    mqd_t MQ1, MQ2, MQ3;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = maxmsg;
    attr.mq_msgsize = 100;
    attr.mq_curmsgs = 0;

    // Create message queue 1 which is the ready queue
    MQ1 = mq_open("/mq1", O_CREAT | O_RDWR, 0666, &attr);
    if (MQ1 == -1)
    {
        perror("Master: mq_open");
        exit(EXIT_FAILURE);
    }

    // Create message queue 2 to communicate between sched and MMU
    MQ2 = mq_open("/mq2", O_CREAT | O_RDWR, 0666, &attr);
    if (MQ2 == -1)
    {
        perror("Master: mq_open");
        exit(EXIT_FAILURE);
    }

    // Create message queue 3 to communicate between MMU and process
    MQ3 = mq_open("/mq3", O_CREAT | O_RDWR, 0666, &attr);
    if (MQ3 == -1)
    {
        perror("Master: mq_open");
        exit(EXIT_FAILURE);
    }

    pid_t schedPID = fork();
    if (schedPID == -1)
    {
        perror("Master: fork");
        exit(EXIT_FAILURE);
    }

    else if (schedPID == 0)
    {
        // Execute ./sched /mq1 /mq2
        char *args[] = {"./sched", "/mq1", "/mq2", NULL};
        execv(args[0], args);
        perror("Master: execv");
        exit(EXIT_FAILURE);
    }

    pid_t MMUPID = fork();
    if (MMUPID == -1)
    {
        perror("Master: fork");
        exit(EXIT_FAILURE);
    }

    else if (MMUPID == 0)
    {
        // Execute ./MMU /mq2 /mq3 SM1 SM2 in an xterm
        execlp("xterm", "xterm", "-e", "./MMU", "/mq2", "/mq3", "SM1", "SM2", NULL);
        perror("Master: execv");
        exit(EXIT_FAILURE);
    }

    srand(time(0)); // Seed for random number generator

    // Create k processes
    for (int i = 0; i < k; i++)
    {
        SM3->noOfPages[curridx] = rand() % m + 1;   // Number of pages for the current process
        pid_t pid = fork();

        if (pid == -1)
        {
            perror("Master: fork");
            exit(EXIT_FAILURE);
        }
        
        else if (pid == 0)
        {
            int refStringLength = rand() % (8 * SM3->noOfPages[curridx] + 1) + 2 * SM3->noOfPages[curridx]; // Length of reference string for the current process between 2n and 10n
            int *refString = (int *)malloc(refStringLength * sizeof(int)); // Reference string for the current process
            for (int j = 0; j < refStringLength; j++)
            {
                refString[j] = rand() % SM3->noOfPages[curridx];
            }

            // with probability PROB, corrupt the reference string, by putting illegal page number
            for (int j = 0; j < refStringLength; j++)
            {
                if (rand() % 100 < PROB)
                {
                    refString[j] = rand() % m;
                }
            }

            char refStringChar[refStringLength * 10];
            memset(refStringChar, 0, refStringLength * 10);
            sprintf(refStringChar, "%d", refString[0]);
            for (int j = 1; j < refStringLength; j++)
            {
                sprintf(refStringChar, "%s,%d", refStringChar, refString[j]);
            }

            // Execute ./process refString /mq1 /mq3
            char *args[] = {"./process", refStringChar, "/mq1", "/mq3", NULL};
            execv(args[0], args);
            perror("Master: execv");
            exit(EXIT_FAILURE);
        }
        SM3->pid[curridx] = pid;
        curridx++;

        // usleep(250000);
    }

    // Wait for sched to finish
    if (waitpid(MMUPID, NULL, 0) == -1)
    {
        perror("Master: waitpid");
        exit(EXIT_FAILURE);
    }

    // Send PID in ready queue
    char pid[10];
    memset(pid, 0, 10);
    sprintf(pid, "%d", schedPID);
    if (mq_send(MQ1, pid, strlen(pid), 0) == -1)
    {
        perror("Master: mq_send");
        exit(EXIT_FAILURE);
    }

    // Garbage collection
    if (mq_close(MQ1) == -1)
    {
        perror("Master: mq_close");
        exit(EXIT_FAILURE);
    }

    if (mq_close(MQ2) == -1)
    {
        perror("Master: mq_close");
        exit(EXIT_FAILURE);
    }

    if (mq_close(MQ3) == -1)
    {
        perror("Master: mq_close");
        exit(EXIT_FAILURE);
    }

    if (mq_unlink("/mq1") == -1)
    {
        perror("Master: mq_unlink");
        exit(EXIT_FAILURE);
    }

    if (mq_unlink("/mq2") == -1)
    {
        perror("Master: mq_unlink");
        exit(EXIT_FAILURE);
    }

    if (mq_unlink("/mq3") == -1)
    {
        perror("Master: mq_unlink");
        exit(EXIT_FAILURE);
    }

    if (shm_unlink("SM1") == -1)
    {
        perror("Master: shm_unlink");
        exit(EXIT_FAILURE);
    }

    if (shm_unlink("SM2") == -1)
    {
        perror("Master: shm_unlink");
        exit(EXIT_FAILURE);
    }

    return 0;
}
