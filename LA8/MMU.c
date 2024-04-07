/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory Spr 2023-24
 *  Date:       07/04/2024

 *  File:       MMU.c
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

int globaltime = 0;

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
    sem_t sem3;
    int noOfProcesses;
    int maxNoOfPages;
    int maxFreeFrames;
    pid_t pid[1024];
    int noOfPages[1024];
};

int
pageFaultHandler(int page, int idx, struct proc2page *proc2pageList, struct freeFrameList *freeFrameList, struct pageTable *pageTable);

int
main(int argc, char *argv[])
{
    FILE *fp = fopen("result.txt", "w");    // Open file for writing
    if (fp == NULL)
    {
        perror("MMU: fopen");
        exit(1);
    }
    fprintf(fp, "Page fault sequence\tInvalid page reference\tGlobal ordering\n");
    fprintf(fp, "------------------------------------------------------------\n");
    printf("Page fault sequence\tInvalid page reference\tGlobal ordering\n");
    printf("------------------------------------------------------------\n");

    mqd_t MQ2, MQ3;

    // Open message queues
    MQ2 = mq_open(argv[1], O_RDWR);
    if (MQ2 == -1)
    {
        perror("MMU: mq_open");
        exit(1);
    }

    MQ3 = mq_open(argv[2], O_RDWR);
    if (MQ3 == -1)
    {
        perror("MMU: mq_open");
        exit(1);
    }

    // Open shared memory
    int shmSM1 = shm_open(argv[3], O_RDWR, 0666);
    if (shmSM1 == -1)
    {
        perror("MMU: shm_open");
        exit(1);
    }

    int shmSM2 = shm_open(argv[4], O_RDWR, 0666);
    if (shmSM2 == -1)
    {
        perror("MMU: shm_open");
        exit(1);
    }

    int shmSM3 = shm_open("SM3", O_RDWR, 0666);
    if (shmSM3 == -1)
    {
        perror("MMU: shm_open");
        exit(1);
    }

    struct proc2page *proc2pageList = (struct proc2page *)mmap(NULL, sizeof(struct proc2page), PROT_READ | PROT_WRITE, MAP_SHARED, shmSM3, 0);
    if (proc2pageList == MAP_FAILED)
    {
        perror("MMU: mmap");
        exit(1);
    }

    int k = proc2pageList->noOfProcesses;
    int m = proc2pageList->maxNoOfPages;
    int f = proc2pageList->maxFreeFrames;

    struct pageTable *pageTable = (struct pageTable *)mmap(NULL, k * m * sizeof(struct pageTable), PROT_READ | PROT_WRITE, MAP_SHARED, shmSM1, 0);
    if (pageTable == MAP_FAILED)
    {
        perror("MMU: mmap");
        exit(1);
    }

    struct freeFrameList *freeFrameList = (struct freeFrameList *)mmap(NULL, f * sizeof(struct freeFrameList), PROT_READ | PROT_WRITE, MAP_SHARED, shmSM2, 0);
    if (freeFrameList == MAP_FAILED)
    {
        perror("MMU: mmap");
        exit(1);
    }

    pid_t pid = -1;
    int noOfTerminatedProcesses = 0;

    while (1)
    {
        // Wait for process to send message
        if (sem_wait(&proc2pageList->sem) == -1)
        {
            perror("MMU: sem_wait");
            exit(1);
        }

        int page;
        char buf[100];
        memset(buf, 0, sizeof(buf));
        if (mq_receive(MQ3, buf, 100, 0) == -1)
        {
            perror("MMU: mq_receive");
            exit(1);
        }
        sscanf(buf, "%d:%d", &pid, &page);  // Extract pid and page from message

        // Find index of process in proc2pageList
        int idx = 0, i;
        for (i = 0; i < k; i++)
        {
            if (proc2pageList->pid[i] == pid)
            {
                idx = i;
                break;
            }
        }
        
        // If process not found, print error message and continue
        if (i == proc2pageList->noOfProcesses)
        {
            printf("Invalid Process ID %d\n", pid);
            continue;
        }

        // If page is -9, terminate process
        if (page == -9)
        {
            // Free all frames allocated to process
            for (int i = 0; i < m; i++)
            {
                if (pageTable[(idx * m) + i].valid == 1)
                {
                    freeFrameList[pageTable[(idx * m) + i].frame].free = 1;
                    pageTable[(idx * m) + i].valid = 0;
                }
            }

            if (++noOfTerminatedProcesses == k)
            {
                if (mq_send(MQ2, "END", strlen("END"), 0) == -1)
                {
                    perror("MMU: mq_send");
                    exit(1);
                }

                break;
            }

            // Send message to scheduler
            if (mq_send(MQ2, "TERMINATED", strlen("TERMINATED"), 0) == -1)
            {
                perror("MMU: mq_send");
                exit(1);
            }

            continue;
        }

        // If page reference is invalid, print error message and continue
        if (page >= proc2pageList->noOfPages[idx])
        {
            printf("TRYING TO ACCESS INVALID PAGE REFERENCE\n");
            printf("%d %d\n", page, proc2pageList->noOfPages[idx]);
            fprintf(fp, "\t\t\t( %d, %d )\n", idx, page);
            fflush(fp);
            printf("\t\t\t( %d, %d )\n", idx, page);

            if (mq_send(MQ2, "TERMINATED", strlen("TERMINATED"), 0) == -1)
            {
                perror("MMU: mq_send");
                exit(1);
            }

            if (mq_send(MQ3, "-2", strlen("-2"), 0) == -1)
            {
                perror("MMU: mq_send");
                exit(1);
            }

            if (sem_post(&proc2pageList->sem2) == -1)
            {
                perror("MMU: sem_post");
                exit(1);
            }

            continue;
        }

        fprintf(fp, "\t\t\t\t\t\t( %d, %d )\n", idx, page);
        fflush(fp);
        printf("\t\t\t\t\t\t( %d, %d )\n", idx, page);

        // If page is already in memory, send frame to process
        if (pageTable[(idx * m) + page].valid == 1)
        {
            pageTable[(idx * m) + page].timestamp = globaltime++;

            memset(buf, 0, sizeof(buf));
            sprintf(buf, "%d", pageTable[(idx * m) + page].frame);

            if (mq_send(MQ3, buf, strlen(buf), 0) == -1)
            {
                perror("MMU: mq_send");
                exit(1);
            }

            if (sem_post(&proc2pageList->sem2) == -1)
            {
                perror("MMU: sem_post");
                exit(1);
            }

            continue;
        }

        // If page is not in memory, handle page fault
        int frame = pageFaultHandler(page, idx, proc2pageList, freeFrameList, pageTable);
        fprintf(fp, "( %d, %d )\n", idx, page);
        fflush(fp);
        printf("( %d, %d )\n", idx, page);

        if (mq_send(MQ3, "-1", strlen("-1"), 0) == -1)
        {
            perror("MMU: mq_send");
            exit(1);
        }

        if (mq_send(MQ2, "PAGE FAULT HANDLED", strlen("PAGE FAULT HANDLED"), 0) == -1)
        {
            perror("MMU: mq_send");
            exit(1);
        }

        if (sem_post(&proc2pageList->sem2) == -1)
        {
            perror("MMU: sem_post");
            exit(1);
        }
    }
    printf("------------------------------------------------------------\n");

    // Garbage collection
    fclose(fp);
    mq_close(MQ2);
    mq_close(MQ3);
    
    if (munmap(proc2pageList, sizeof(struct proc2page)) == -1)
    {
        perror("MMU: munmap");
        exit(1);
    }

    if (munmap(pageTable, k * m * sizeof(struct pageTable)) == -1)
    {
        perror("MMU: munmap");
        exit(1);
    }

    if (munmap(freeFrameList, f * sizeof(struct freeFrameList)) == -1)
    {
        perror("MMU: munmap");
        exit(1);
    }

    if (close(shmSM1) == -1)
    {
        perror("MMU: close");
        exit(1);
    }

    if (close(shmSM2) == -1)
    {
        perror("MMU: close");
        exit(1);
    }

    if (close(shmSM3) == -1)
    {
        perror("MMU: close");
        exit(1);
    }

    return 0;
}

int
pageFaultHandler(int page, int idx, struct proc2page *proc2pageList, struct freeFrameList *freeFrameList, struct pageTable *pageTable)
{
    int m = proc2pageList->maxNoOfPages;
    int f = proc2pageList->maxFreeFrames;

    int frame = -1;

    // Find free frame
    for (int i = 0; i < f; i++)
    {
        if (freeFrameList[i].free == 1)
        {
            frame = i;
            freeFrameList[i].free = 0;
            break;
        }
    }

    // If no free frame, use LRU algorithm to find victim frame to replace
    if (frame == -1)
    {
        time_t min = globaltime + 1;
        int minidx = -1;
        for (int i = 0; i < m; i++)
        {
            if ((pageTable[(idx * m) + i].timestamp < min) && (pageTable[(idx * m) + i].valid == 1))
            {
                min = pageTable[(idx * m) + i].timestamp;
                minidx = i;
            }
        }

        frame = pageTable[(idx * m) + minidx].frame;
        pageTable[(idx * m) + minidx].valid = 0;
    }

    // Update page table
    pageTable[(idx * m) + page].frame = frame;
    pageTable[(idx * m) + page].valid = 1;
    pageTable[(idx * m) + page].timestamp = globaltime++;

    return frame;
}
    