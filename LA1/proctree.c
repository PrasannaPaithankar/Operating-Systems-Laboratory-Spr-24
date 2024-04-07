/*
 *  Author:     Prasanna Paithankar
 *  Roll No.:   21CS30065
 *  Course:     Operating Systems Laboratory (CS39006) Spr 2023-24
 *  Date:       10/01/2024

 *  File:       proctree.c
 *  Purpose:    To solve the problem statement posed by [http://cse.iitkgp.ac.in/~abhij/course/theory/OS/Spring24/lab/LA1.pdf]
 *  Compile:    gcc proctree.c -o proctree
 *  Run:        ./proctree [City Name] <level>
 */

#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<string.h>

#define MAX 1024

int main(int argc, char *argv[])
{
    pid_t pid;                      // To store child's PID after fork()
    
    FILE *fp;                       // File pointer to data file
    char fname[] = "treeinfo.txt";  // Provided data file
    int level;                      
    
    char *base = argv[1];
    int flag = 0;                   // For invalid City Name error handling
    
    if (argc==2)                    // If only ./proctree [City Name] is called then default level to 0
    {
        fp = fopen(fname, "r");
        level = 0;
    }
    else if(argc==3)                // Explicit setting of level
    {
        fp = fopen(fname, "r");
        level = atoi(argv[2]);
    }
    else                            // Error handling
    {
        printf("Run as ./proctree [City Name] <level>\n");
        exit(1);
    }

    char *children[MAX];            // Array to store children names
    int i = 0;                      // Number of children

    // Start file operations
    while(!feof(fp))
    {
        char *line;
        size_t len = 0;
        ssize_t read;
        
        // Read line
        read = getline(&line, &len, fp);
        if (read == -1)
            exit(1);
        
        // Split lines over space character
        char *token = strtok(line, " ");
        
        if (strcmp(token, base)==0) // If we find the city
        {
            flag = 1;               // Set city found
            while(token != NULL)    // Iterate over children (first iteration gives no. of children and last entry is null)
            {
                token = strtok(NULL, " ");
                children[i] = token;
                i++;
            }
            break;
        }
    }

    if (flag == 0)                  // Error handling
    {
        printf("City %s not found\n", base);
        exit(1);
    }

    // Output: [Indent] City Name (pid)
    for (int j = 0; j < level; j++)
    {
        printf("\t");
    }
    printf("%s (%d)\n", argv[1], getpid());

    // Recursive-ish call
    for (int j = 1; j < i-1; j++)
    {
        pid = fork();
        // Child process handling
        if (pid == 0)
        {
            //Convert integer level to string to be passes to execl
            char chldLevel[MAX];
            sprintf(chldLevel, "%d", level+1);

            // Sanity checking strings to avoid the dreaded Segmentation fault (core dumped) :)
            if (children[j][strlen(children[j]) - 1] == '\n')
            {
                children[j][strlen(children[j]) - 1] = '\0';
            }

            // Execute the process proctree with j'th child with higher level (visualize as indent)
            execl("./proctree", "./proctree", children[j], chldLevel, NULL);
        }
        // Parent process handling
        else
        {
            // Wait for the child to exit
            wait(NULL);
        }
    }

    exit(0);
}
