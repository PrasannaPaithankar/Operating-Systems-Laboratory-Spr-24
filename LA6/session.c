/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory Spr 2023-24
 *  Date:       10/03/2024

 *  File:       session.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "event.h"

eventQ E, A, PA, SR, RE;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doc_staff = PTHREAD_COND_INITIALIZER;
pthread_cond_t doc_visit = PTHREAD_COND_INITIALIZER;
pthread_cond_t visit_done = PTHREAD_COND_INITIALIZER;

int curr_time = 0;
int e_time = 0;
int when_doc_free = 0;
int doc_left = 0;
int ack = 0;
int ackt = 0;
int ackv = 0;
int ackq = 0;
int people_waiting = 0;

int curr_patient = 1;
int curr_salesrep = 1;
int curr_reporter = 1;

int waiting_patients = 0;
int waiting_salesreps = 0;
int waiting_reporters = 0;

struct params
{
    int id;
    int duration;
};

int con2time(int offset);
void *doctor(void *arg);
void *patient(void *arg);
void *reporter(void *arg);
void *salesrep(void *arg);


int main(int argc, char *argv[])
{
    E = initEQ("arrival.txt");
    PA.n = 0;
    PA.Q = (event *)malloc(128 * sizeof(event));
    SR.n = 0;
    SR.Q = (event *)malloc(128 * sizeof(event));
    RE.n = 0;
    RE.Q = (event *)malloc(128 * sizeof(event));
    
    event e = nextevent(E);
    curr_time = e.time;

    pthread_t doctor_t;
    pthread_create(&doctor_t, NULL, doctor, NULL);

    int patient_count = 0;
    int salesrep_count = 0;
    int reporter_count = 0;

    while (emptyQ(E) == 0 || people_waiting > 0)
    {
        event e = nextevent(E);
        e_time = e.time;

        if (curr_time >= e_time)
        {
            
            E = delevent(E);

            int t = con2time(e_time); 
            if (e.type == 'P')
            {
                patient_count++;
                if (doc_left == 1)
                {
                    printf("\t\t[%2d:%2dam] Patient %d arrives\n", t/100, t%100, patient_count);
                    printf("\t\t[%2d:%2dam] Patient %d leaves (session over)\n", t/100, t%100, patient_count);
                }
                else if (patient_count > 25)
                {
                    printf("\t\t[%2d:%2dam] Patient %d arrives\n", t/100, t%100, patient_count);
                    printf("\t\t[%2d:%2dam] Patient %d leaves (quota full)\n", t/100, t%100, patient_count);
                }
                else
                {
                    printf("\t\t[%2d:%2dam] Patient %d arrives\n", t/100, t%100, patient_count);
                    people_waiting++;
                    waiting_patients++;
                    PA = addevent(PA, e);
                }
            }

            if (e.type == 'S')
            {
                salesrep_count++;
                if (doc_left == 1)
                {
                    printf("\t\t[%2d:%2dam] Sales representative %d arrives\n", t/100, t%100, salesrep_count);
                    printf("\t\t[%2d:%2dam] Sales representative %d leaves (session over)\n", t/100, t%100, salesrep_count);
                }
                else if (salesrep_count > 3)
                {
                    printf("\t\t[%2d:%2dam] Sales representative %d arrives\n", t/100, t%100, salesrep_count);
                    printf("\t\t[%2d:%2dam] Sales representative %d leaves (quota full)\n", t/100, t%100, salesrep_count);
                }
                else
                {
                    printf("\t\t[%2d:%2dam] Sales representative %d arrives\n", t/100, t%100, salesrep_count);
                    people_waiting++;
                    waiting_salesreps++;
                    SR = addevent(SR, e);
                }
            }

            if (e.type == 'R')
            {
                reporter_count++;
                if (doc_left == 1)
                {
                    printf("\t\t[%2d:%2dam] Reporter %d arrives\n", t/100, t%100, reporter_count);
                    printf("\t\t[%2d:%2dam] Reporter %d leaves (session over)\n", t/100, t%100, reporter_count);
                }
                else
                {
                    printf("\t\t[%2d:%2dam] Reporter %d arrives\n", t/100, t%100, reporter_count);
                    people_waiting++;
                    waiting_reporters++;
                    RE = addevent(RE, e);
                }
            }
        }

        if (people_waiting == 0 && patient_count >= 25 && salesrep_count >= 3 && doc_left == 0 && when_doc_free <= curr_time)
        {
            doc_left = 1;
            ackq = 0;
            while (ackq == 0)
            {
                pthread_mutex_lock(&mutex);
                pthread_cond_signal(&doc_staff);
                pthread_mutex_unlock(&mutex);
            }
            pthread_join(doctor_t, NULL);

        }
        if (when_doc_free <= curr_time && doc_left == 0)
        {
            pthread_t visitor;
            if (people_waiting > 0)
            {
                event a;
                if (waiting_reporters > 0)
                {
                    a = nextevent(RE);
                    RE = delevent(RE);
                }
                else if (waiting_patients > 0)
                {
                    a = nextevent(PA);
                    PA = delevent(PA);
                }
                else if (waiting_salesreps > 0)
                {
                    a = nextevent(SR);
                    SR = delevent(SR);
                }

                struct params arg;
                arg.duration = a.duration;
                if (a.type == 'P')
                {
                    arg.id = curr_patient;
                    curr_patient++;
                    pthread_create(&visitor, NULL, patient, (void *)&arg);
                }
                else if (a.type == 'S')
                {
                    arg.id = curr_salesrep;
                    curr_salesrep++;
                    pthread_create(&visitor, NULL, salesrep, (void *)&arg);
                }
                else if (a.type == 'R')
                {
                    arg.id = curr_reporter;
                    curr_reporter++;
                    pthread_create(&visitor, NULL, reporter, (void *)&arg);
                }
                else
                {
                    printf("Error: Invalid event type\n");
                }
            
                when_doc_free = curr_time + a.duration;

                ack = 0;
                while (ack == 0)
                {
                    pthread_mutex_lock(&mutex);
                    pthread_cond_signal(&doc_staff);
                    pthread_mutex_unlock(&mutex);
                }

                ackt = 0;
                while (ackt == 0)
                {
                    pthread_mutex_lock(&mutex);
                    pthread_cond_signal(&doc_visit);
                    pthread_mutex_unlock(&mutex);
                }

                pthread_mutex_lock(&mutex);
                pthread_cond_wait(&visit_done, &mutex);
                ackv = 1;
                pthread_mutex_unlock(&mutex);

                pthread_join(visitor, NULL);
            }
        }

        curr_time++;
    }  
    if (doc_left == 0)
    {
        curr_time = when_doc_free;
        doc_left = 1;
        ackq = 0;
        while (ackq == 0)
        {
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&doc_staff);
            pthread_mutex_unlock(&mutex);
        }
        pthread_join(doctor_t, NULL);
    }
    return 0;
}


void *doctor(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&doc_staff, &mutex);
        if (doc_left == 1)
        {
            int t = con2time(curr_time);
            printf("[%2d:%2dam] Doctor leaves\n", t/100, t%100);
            ackq = 1;
            pthread_mutex_unlock(&mutex);
            break;
        }
        ack = 1;
        int t = con2time(curr_time);
        printf("[%2d:%2dam] Doctor has next visitor\n", t/100, t%100);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(0);
}


void *patient(void *arg)
{
    struct params *u = (struct params *)arg;

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&doc_visit, &mutex);
    int t = con2time(curr_time);
    int t2 = con2time(curr_time + u->duration);
    printf("[%2d:%2dam - %2d:%2dam] Patient %d in doctor's chamber\n", t/100, t%100, t2/100, t2%100, u->id);
    ackt = 1;
    people_waiting--;
    waiting_patients--;
    pthread_mutex_unlock(&mutex);

    ackv = 0;
    while (ackv == 0)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&visit_done);
        pthread_mutex_unlock(&mutex);
    }
    
    pthread_exit(0);
}


void *salesrep(void *arg)
{
    struct params *u = (struct params *)arg;

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&doc_visit, &mutex);
    int t = con2time(curr_time);
    int t2 = con2time(curr_time + u->duration);
    printf("[%2d:%2dam - %2d:%2dam] Sales representative %d in doctor's chamber\n", t/100, t%100, t2/100, t2%100, u->id);
    ackt = 1;
    people_waiting--;
    waiting_salesreps--;
    pthread_mutex_unlock(&mutex);

    ackv = 0;
    while (ackv == 0)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&visit_done);
        pthread_mutex_unlock(&mutex);
    }
    
    pthread_exit(0);
}


void *reporter(void *arg)
{
    struct params *u = (struct params *)arg;

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&doc_visit, &mutex);
    int t = con2time(curr_time);
    int t2 = con2time(curr_time + u->duration);
    printf("[%2d:%2dam - %2d:%2dam] Reporter %d in doctor's chamber\n", t/100, t%100, t2/100, t2%100, u->id);
    ackt = 1;
    people_waiting--;
    waiting_reporters--;
    pthread_mutex_unlock(&mutex);

    ackv = 0;
    while (ackv == 0)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&visit_done);
        pthread_mutex_unlock(&mutex);
    }
    
    pthread_exit(0);
}


int con2time(int offset)
{
    int hour = offset/60;
    int minute = abs(offset)%60;
    int ans;
    if (offset >= 0)
    {
        ans = 900 + (hour*100);
        ans += minute;
    }
    else if (offset < 0)
    {
        ans = 900 + (hour*100);
        ans -= (minute);
        ans -= 40;
    }
    return ans;
}