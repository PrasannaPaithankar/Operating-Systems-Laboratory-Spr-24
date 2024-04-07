/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory Spr 2023-24
 *  Date:       07/04/2024

 *  File:       process.c
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
    sem_t sem3;
    int noOfProcesses;
    int maxNoOfPages;
    int maxFreeFrames;
    pid_t pid[1024];
    int noOfPages[1024];
};

void
sigHandler(int sig)
{
    if (sig == SIGCONT)
    {
        // printf("Process %d resumed\n", getpid());
    }
}

int
main(int argc, char *argv[])
{
    signal(SIGCONT, sigHandler);    // Register signal handler

    char *refString = argv[1];
    mqd_t MQ1, MQ3;

    // Open message queues
    MQ1 = mq_open(argv[2], O_RDWR);
    if (MQ1 == -1)
    {
        perror("process: mq_open");
        exit(1);
    }

    MQ3 = mq_open(argv[3], O_RDWR);
    if (MQ3 == -1)
    {
        perror("process: mq_open");
        exit(1);
    }

    // Open shared memory
    int shmSM3 = shm_open("SM3", O_RDWR, 0666);
    if (shmSM3 == -1)
    {
        perror("process: shm_open");
        exit(1);
    }

    struct proc2page *sm3 = (struct proc2page *)mmap(NULL, sizeof(struct proc2page), PROT_READ | PROT_WRITE, MAP_SHARED, shmSM3, 0);
    if (sm3 == MAP_FAILED)
    {
        perror("process: mmap");
        exit(1);
    }

    // Add pid to ready queue
    pid_t pid = getpid();
    char message[100];
    memset(message, 0, 100);
    sprintf(message, "%d", pid);

    if (mq_send(MQ1, message, strlen(message), 0) == -1)
    {
        perror("process: mq_send");
        exit(1);
    }

    printf("Process %d added to ready queue with reference string %s\n", pid, refString);
    fflush(stdout);

    // Wait for scheduler to schedule the process
    // pause();
    if (sem_wait(&sm3->sem3) == -1)
    {
        perror("process: sem_wait");
        exit(1);
    }


    char *token = strtok(refString, ",");
    while (token != NULL)
    {
        int page = atoi(token);
        char pagemsg[100], msg[100];
        memset(pagemsg, 0, 100);
        sprintf(pagemsg, "%d:%d", pid, page);

        // Send page request to MMU
        if (mq_send(MQ3, pagemsg, strlen(pagemsg), 0) == -1)
        {
            perror("process: mq_send");
            exit(1);
        }

        if (sem_post(&sm3->sem) == -1)
        {
            perror("process: sem_post");
            exit(1);
        }

        if (sem_wait(&sm3->sem2) == -1)
        {
            perror("process: sem_post");
            exit(1);
        }

        int frame;
        memset(msg, 0, 100);

        // Receive frame from MMU
        if (mq_receive(MQ3, msg, 100, 0) == -1)
        {
            perror("process: mq_receive");
            exit(1);
        }
        frame = atoi(msg);
        printf("%d: %d->%d\n", pid, page, frame);

        while (frame == -1)   // Page fault
        {
            // Enqueue the ready queue
            if (mq_send(MQ1, message, strlen(message), 0) == -1)
            {
                perror("process: mq_send");
                exit(1);
            }
            
            // pause();
            if (sem_wait(&sm3->sem3) == -1)
            {
                perror("process: sem_wait");
                exit(1);
            }

            if (mq_send(MQ3, pagemsg, strlen(pagemsg), 0) == -1)
            {
                perror("process: mq_send");
                exit(1);
            }

            if (sem_post(&sm3->sem) == -1)
            {
                perror("process: sem_post");
                exit(1);
            }

            if (sem_wait(&sm3->sem2) == -1)
            {
                perror("process: sem_post");
                exit(1);
            }

            memset(msg, 0, 100);
            if (mq_receive(MQ3, msg, 100, 0) == -1)
            {
                perror("process: mq_receive");
                exit(1);
            }

            frame = atoi(msg);
            printf("%d: %d->%d\n", pid, page, frame);
        }
        
        if (frame == -2)    // Process terminated due to invalid page reference
        {
            break;
        }

        token = strtok(NULL, ",");
    }

    sprintf(message, "%d:-9", pid); // Terminate process
    if (mq_send(MQ3, message, strlen(message), 0) == -1)
    {
        perror("process: mq_send");
        exit(1);
    }

    if (sem_post(&sm3->sem) == -1)
    {
        perror("process: sem_post");
        exit(1);
    }

    // Garbage collection
    if (munmap(sm3, sizeof(struct proc2page)) == -1)
    {
        perror("process: munmap");
        exit(1);
    }

    if (close(shmSM3) == -1)
    {
        perror("process: close");
        exit(1);
    }

    mq_close(MQ1);
    mq_close(MQ3);

    return 0;
}