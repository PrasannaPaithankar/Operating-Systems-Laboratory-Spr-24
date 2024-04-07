/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory (CS39006) Spr 2023-24
 *  Date:       07/02/2024

 *  File:       worker.c
 *  Purpose:    To solve the problem statement posed by [http://cse.iitkgp.ac.in/~abhij/course/theory/OS/Spring24/lab/LA5.pdf]
 *  Compile:    gcc -o worker worker.c
 *  Run:        ./worker
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
    int n = atoi(argv[1]);
    int i = atoi(argv[2]);
    struct sembuf pop, vop;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    //  Attach to shared memory A, T and idx
    int shmid = shmget(ftok("graph.txt", 65), n * n * sizeof(int), 0777 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("shmget");
        exit(1);
    }
    int *A = (int *)shmat(shmid, 0, 0);

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

    //  Attach to semaphores
    int mtx = semget(ftok("boss.c", 68), 1, 0777 | IPC_CREAT);
    if (mtx == -1)
    {
        perror("semget");
        exit(1);
    }

    int sync = semget(ftok("boss.c", 69), n, 0777 | IPC_CREAT);
    if (sync == -1)
    {
        perror("semget");
        exit(1);
    }

    int nfty = semget(ftok("boss.c", 70), 1, 0777 | IPC_CREAT);
    if (nfty == -1)
    {
        perror("semget");
        exit(1);
    }

    //  waits for the sync[i] signals from all incoming links (j, i) to vertex i to be 0
    pop.sem_num = i;
    for (int j = 0; j < n; j++)
    {
        if (A[j * n + i] == 1)
        {
            P(sync);
        }
    }

    //  Append its ID i at the index idx of T, and to update (increment) idx
    pop.sem_num = 0;
    vop.sem_num = 0;
    P(mtx);
    T[*idx] = i;
    (*idx)++;
    V(mtx);

    //  Signal the sync signals to all outgoing links (i, j) (if any exists)
    for (int j = 0; j < n; j++)
    {
        if (A[i * n + j] == 1)
        {
            vop.sem_num = j;
            V(sync);
        }
    }

    //  Signal the nfty semaphore to let boss know worker is done
    vop.sem_num = 0;
    V(nfty);

    // Detach from shared memory
    shmdt(A);
    shmdt(T);
    shmdt(idx);

    return 0;
}
