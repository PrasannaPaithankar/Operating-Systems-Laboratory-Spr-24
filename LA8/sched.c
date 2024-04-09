/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory Spr 2023-24
 *  Date:       07/04/2024

 *  File:       sched.c
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

struct
proc2page
{
    sem_t sem;
    sem_t sem2;
    sem_t sem3[1024];
    sem_t sem4;
    sem_t sem5;
    int noOfProcesses;
    int maxNoOfPages;
    int maxFreeFrames;
    pid_t pid[1024];
    int noOfPages[1024];
    int noOfFaults[1024];
};


int
main(int argc, char *argv[])
{
    mqd_t MQ1, MQ2;

    // Open message queues
    MQ1 = mq_open(argv[1], O_RDWR);
    if (MQ1 == -1)
    {
        perror("sched: mq_open");
        exit(1);
    }

    MQ2 = mq_open(argv[2], O_RDWR);
    if (MQ2 == -1)
    {
        perror("sched: mq_open");
        exit(1);
    }

    // Shared memory
    int sm3 = shm_open("SM3", O_RDWR, 0666);
    if (sm3 == -1)
    {
        perror("sched: shm_open");
        exit(1);
    }

    struct proc2page *proc2pageList = mmap(NULL, sizeof(struct proc2page), PROT_READ | PROT_WRITE, MAP_SHARED, sm3, 0);
    if (proc2pageList == MAP_FAILED)
    {
        perror("sched: mmap");
        exit(1);
    }

    while (1)
    {
        pid_t pid;
        char message[100];
        memset(message, 0, 100);

        // Receive PID from message queue
        if (mq_receive(MQ1, message, 100, NULL) == -1)
        {
            perror("sched: mq_receive");
            exit(1);
        }
        
        pid = atoi(message);
        if (pid == getppid())
        {
            break;
        }

        // Send signal to process to continue
        int idx = -1;
        for (int i = 0; i < proc2pageList->noOfProcesses; i++)
        {
            if (proc2pageList->pid[i] == pid)
            {
                idx = i;
                break;
            }
        }

        if (sem_post(&proc2pageList->sem3[idx]) == -1)
        {
            perror("sched: sem_post");
            exit(1);
        }
        
        int flag = 0;
        while (1)
        {
            char message[100];
            memset(message, 0, 100);

            // Receive message from MMU
            if (mq_receive(MQ2, message, 100, NULL) == -1)
            {
                perror("sched: mq_receive");
                exit(1);
            }

            // Check if message is PAGE FAULT HANDLED or TERMINATED
            if (strncmp(message, "PAGE FAULT HANDLED", strlen("PAGE FAULT HANDLED")) == 0)
            {
                break;
            }

            else if (strncmp(message, "TERMINATED", strlen("TERMINATED")) == 0)
            {
                break;
            }
        }

        if (flag)
        {
            break;
        }
    }

    // Garbage collection
    mq_close(MQ1);
    mq_close(MQ2);

    return 0;
}
