/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory Spr 2023-24
 *  Date:       17/03/2024

 *  File:       computesum.c
 */

#include "foothread.h"

int sum[100];                           // Partial sum at each node
int n;                                  // Number of nodes in the tree
int P[100];                             // Parent of each node
int root;                               // Root of the tree
int input[100];                         // Input at each leaf node
int isLeaf[100];                        // 1 if the node is a leaf node, 0 otherwise

foothread_mutex_t printmutex;           // Mutex for printing
foothread_barrier_t barrier[100];       // Barrier for each node


/* Function to compute partial sum at the given node (arg <- node number) */
void *computesum(void *arg)
{
    int i = (int)(intptr_t)arg;   // Node number
    int j;

    if (isLeaf[i])
    {
        sum[i] = input[i];
        foothread_barrier_wait(&barrier[P[i]]); // Wait on the parent barrier
    }

    else
    {
        foothread_barrier_wait(&barrier[i]);    // Wait on own barrier
        for (j = 0; j < n; j++)
        {
            if (P[j] == i && j != i)
            {
                sum[i] += sum[j];
            }
        }

        foothread_mutex_lock(&printmutex);
        if (i != root)
            printf("Internal node %2d gets the partial sum %2d from its children\n", i, sum[i]);
        else
            printf("Sum at root (node %2d) = %2d\n", i, sum[i]);
        fflush(stdout);
        foothread_mutex_unlock(&printmutex);

        foothread_barrier_wait(&barrier[P[i]]); // Wait on the parent barrier
    }

    foothread_exit();
    return NULL;
}
    

int main(int argc, char *argv[])
{
    FILE *fp;
    int i, j;
    int noOfChildren[100]; // Number of children of each node

    memset(sum, 0, sizeof(sum));
    memset(P, 0, sizeof(P));
    memset(input, 0, sizeof(input));
    memset(noOfChildren, 0, sizeof(noOfChildren));
    for (i = 0; i < 100; i++)
    {
        isLeaf[i] = 1;
    }

    // Tree setup
    fp = fopen("tree.txt", "r");

    fscanf(fp, "%d", &n);

    for (int k = 0; k < n; k++)
    {
        fscanf(fp, "%d %d", &i, &j);
        P[i] = j;
        isLeaf[j] = 0;
        noOfChildren[j]++;
        if (i == j)
        {
            noOfChildren[j]--;
            root = i;
        }
    }
    fclose(fp);

    foothread_mutex_init(&printmutex);
    for (i = 0; i < n; i++)
    {
        if (isLeaf[i] == 1)
        {
            printf("Leaf node %2d :: Enter a positive integer: ", i);
            scanf("%d", &input[i]);
        }

        foothread_barrier_init(&barrier[i], noOfChildren[i] + 1);   // Initialize barrier for each node
    }

    // Multi-threading
    foothread_t threads[n];
    foothread_attr_t attr = FOOTHREAD_ATTR_INITIALIZER;
    foothread_attr_setjointype(&attr, FOOTHREAD_JOINABLE);

    for (i = 0; i < n; i++)
    {
        foothread_create(&threads[i], &attr, (void *)computesum, (void *)(intptr_t)i);
    }

    // Wait for all joinable threads to finish
    foothread_exit();

    // Clean up
    for (i = 0; i < n; i++)
    {
        foothread_barrier_destroy(&barrier[i]);
    }

    #ifdef DEBUG
        int finalSum = 0;
        for (i = 0; i < n; i++)
        {
            finalSum += input[i];
        }
        if (finalSum == sum[root] && errno == 0)
        {
            printf("Test passed\n");
        }
        else
        {
            printf("Test failed\n");
        }
    #endif

    return 0;
}
