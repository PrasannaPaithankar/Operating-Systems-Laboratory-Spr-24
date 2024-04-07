/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory Spr 2023-24
 *  Date:       17/03/2024

 *  File:       foothread.c
 */

#include "foothread.h"

tid_t tidarr[FOOTHREAD_THREADS_MAX];                    // Array to store the thread ids
foothread_barrier_t barrierarr[FOOTHREAD_THREADS_MAX];  // Array to store the barriers
int noOfThreads = 0;                                    // Number of threads created
sem_t sem;                                              // Semaphore to protect the global variables


/* Function to create a new thread */
void foothread_create(foothread_t *thread, foothread_attr_t *attr, int (*func)(void *), void *arg)
{   
    if (noOfThreads == 0)   // Initialize the global variables
    {
        for (int i = 0; i < FOOTHREAD_THREADS_MAX; i++)
        {
            tidarr[i] = 0;
            foothread_barrier_init(&barrierarr[i], 2);
        }
        sem_init(&sem, 0, 1);
    }
    
    sem_wait(&sem);
    noOfThreads++;

    if (attr != NULL)
    {
        thread->join_type = attr->join_type;
        thread->stack_size = attr->stack_size;
    }
    else
    {
        thread->join_type = FOOTHREAD_DETACHED;
        thread->stack_size = FOOTHREAD_DEFAULT_STACK_SIZE;
    }

    thread->stack = malloc(thread->stack_size);
    if (thread->stack == NULL)
    {
        perror("malloc");
        return;
    }

    thread->tid = clone(func, thread->stack + thread->stack_size, CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_IO, arg);    // Create a new thread using clone system call
    if (thread->tid == -1)
    {
        perror("clone");
        return;
    }

    if (thread->join_type == FOOTHREAD_JOINABLE)
    {
        tidarr[noOfThreads - 1] = thread->tid;
    }
    sem_post(&sem);

    return;
}


/* Function to set the join type of a thread */
void foothread_attr_setjointype(foothread_attr_t *attr, int join_type)
{
    attr->join_type = join_type;
}


/* Function to set the stack size of a thread */
void foothread_attr_setstacksize(foothread_attr_t *attr, int stack_size)
{
    attr->stack_size = stack_size;
}


/* Function to initialize a mutex */
void foothread_mutex_init(foothread_mutex_t *mutex)
{
    sem_init(&mutex->sem, 0, 1);
    mutex->tid = gettid();
}


/* Function to lock a mutex */
void foothread_mutex_lock(foothread_mutex_t *mutex)
{
    if (sem_wait(&mutex->sem) == -1)
    {
        perror("foothread_mutex_lock");
    }
    else
    {
        mutex->tid = gettid();
    }
}


/* Function to unlock a mutex */
void foothread_mutex_unlock(foothread_mutex_t *mutex)
{
    if (mutex->tid == gettid()) // Check if the mutex is locked by the calling thread
    {
        int semval;
        sem_getvalue(&mutex->sem, &semval);
        if (semval == 0)
        {
            sem_post(&mutex->sem);
        }
        else
        {
            errno = EPERM;
            perror("foothread_mutex_unlock");
        }
    }
    else
    {
        errno = EPERM;
        perror("foothread_mutex_unlock");
        return;
    }
}


/* Function to destroy a mutex */
void foothread_mutex_destroy(foothread_mutex_t *mutex)
{
    sem_destroy(&mutex->sem);
}


/* Function to initialize a barrier */
void foothread_barrier_init(foothread_barrier_t *barrier, int max)
{
    sem_init(&barrier->sem, 0, 0);
    barrier->count = 0;
    barrier->max = max;
}


/* Function to wait on a barrier */
void foothread_barrier_wait(foothread_barrier_t *barrier)
{
    sem_wait(&sem);
    barrier->count++;
    sem_post(&sem);

    if (barrier->count == barrier->max)
    {
        for (int i = 0; i < barrier->max; i++)
        {
            sem_post(&barrier->sem);
        }
        barrier->count = 0;
    }
    else
    { 
        sem_wait(&barrier->sem);
    }
}


/* Function to destroy a barrier */
void foothread_barrier_destroy(foothread_barrier_t *barrier)
{
    sem_destroy(&barrier->sem);
}


/* Function to exit a joinable thread */
void foothread_exit()
{
    if (gettid() == getpid())   // If leader thread, wait for all joinable threads to exit
    {
        for (int i = 0; i < FOOTHREAD_THREADS_MAX; i++)
        {
            if (tidarr[i] != 0)
            {
                foothread_barrier_wait(&barrierarr[i]);
                foothread_barrier_destroy(&barrierarr[i]);
                tidarr[i] = 0;
            }
        }
    }
    else    // If joinable follower thread, wait on the barrier
    {
        for (int i = 0; i < FOOTHREAD_THREADS_MAX; i++)
        {
            if (tidarr[i] == gettid())
            {
                foothread_barrier_wait(&barrierarr[i]);
                break;
            }
        }
    }
}
