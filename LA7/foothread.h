/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory Spr 2023-24
 *  Date:       17/03/2024

 *  File:       foothread.h
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sched.h>

#define FOOTHREAD_THREADS_MAX 100
#define FOOTHREAD_DEFAULT_STACK_SIZE 2097152
#define FOOTHREAD_JOINABLE 0
#define FOOTHREAD_DETACHED 1
#define FOOTHREAD_ATTR_INITIALIZER {FOOTHREAD_DETACHED, FOOTHREAD_DEFAULT_STACK_SIZE}

typedef pid_t tid_t;

typedef struct
{
    int join_type;
    size_t stack_size;
    tid_t tid;
    void *stack;
} foothread_t;

typedef struct 
{
    sem_t sem;
    tid_t tid;
} foothread_mutex_t;

typedef struct
{
    sem_t sem;
    int count;
    int max;
} foothread_barrier_t;

typedef struct
{
    int join_type;
    size_t stack_size;
} foothread_attr_t;

void foothread_create(foothread_t *, foothread_attr_t *, int (*)(void *), void *);

void foothread_attr_setjointype(foothread_attr_t *, int);

void foothread_attr_setstacksize(foothread_attr_t *, int);

void foothread_exit();

void foothread_mutex_init(foothread_mutex_t *);

void foothread_mutex_lock(foothread_mutex_t *);

void foothread_mutex_unlock(foothread_mutex_t *);

void foothread_mutex_destroy(foothread_mutex_t *);

void foothread_barrier_init(foothread_barrier_t *, int);

void foothread_barrier_wait(foothread_barrier_t *);

void foothread_barrier_destroy(foothread_barrier_t *);