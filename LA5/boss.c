/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory (CS39006) Spr 2023-24
 *  Date:       07/02/2024

 *  File:       boss.c
 *  Purpose:    To solve the problem statement posed by [http://cse.iitkgp.ac.in/~abhij/course/theory/OS/Spring24/lab/LA5.pdf]
 *  Compile:    gcc -o boss boss.c
 *  Run:        ./boss
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

int main(int argc, char *argv[])
{
    //  Read graph.txt
    FILE *f = fopen("graph.txt", "r");
    int n;
    fscanf(f, "%d", &n);
    int shmid = shmget(ftok("graph.txt", 65), n * n * sizeof(int), 0777 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("shmget");
        exit(1);
    }
    int *A = (int *)shmat(shmid, 0, 0);
    for (int i = 0; i < n * n; i++)
        fscanf(f, "%d", &A[i]);
    fclose(f);

    //  Create shared array T of n integers and another int idx
    int shmid2 = shmget(ftok("graph.txt", 66), n * sizeof(int), 0777 | IPC_CREAT);
    if (shmid2 == -1)
    {
        perror("shmget");
        exit(1);
    }
    int *T = (int *)shmat(shmid2, 0, 0);
    int shmid3 = shmget(ftok("graph.txt", 67), sizeof(int), 0777 | IPC_CREAT);
    if (shmid3 == -1)
    {
        perror("shmget");
        exit(1);
    }
    int *idx = (int *)shmat(shmid3, 0, 0);

    //  Initialize T and idx
    *idx = 0;
    for (int i = 0; i < n; i++)
        T[i] = -1;

    // semaphore mtx for mutual exclusion of worker processes trying to update T and idx, use ftok
    int mtx = semget(ftok("boss.c", 68), 1, 0777 | IPC_CREAT);
    semctl(mtx, 0, SETVAL, 1);

    // Create array of semaphores 
    int sync = semget(ftok("boss.c", 69), n, 0777 | IPC_CREAT);
    for (int i = 0; i < n; i++)
        semctl(sync, i, SETVAL, 0);

    // Create a semphore nfty to let boss know all workers are done
    int nfty = semget(ftok("boss.c", 70), 1, 0777 | IPC_CREAT);
    semctl(nfty, 0, SETVAL, 0);

    struct sembuf pop, vop;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    printf("+++ Boss: Setup done...\n");

    //  Wait for all workers to finish
    for (int i = 0; i < n; i++)
    {
        P(nfty);
    }

    // Check if T is a valid topological sorting
    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            if (A[T[j] * n + T[i]] == 1)
            {
                printf("+++ Boss: Error: T is not a valid topological sorting\n");
                return 0;
            }
        }
    }

    // Print T
    printf("+++ Topological sorting of the vertices\n");
    for (int i = 0; i < n; i++)
    {
        printf("%d ", T[i]);
    }
    printf("\n");
    printf("+++ Boss: Well done, my team...\n");

    //  Detach from shared memory
    shmdt(A);
    shmdt(T);
    shmdt(idx);

    //  Delete shared memory
    shmctl(shmid, IPC_RMID, 0);
    shmctl(shmid2, IPC_RMID, 0);
    shmctl(shmid3, IPC_RMID, 0);

    //  Delete semaphores
    semctl(mtx, 0, IPC_RMID, 0);
    semctl(sync, 0, IPC_RMID, 0);
    semctl(nfty, 0, IPC_RMID, 0);

    return 0;
}
