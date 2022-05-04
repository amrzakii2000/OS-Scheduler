#include "headers.h"
#include <string.h>

void clearResources(int);
void readOptions(int*, int*, int, char**);
struct Process *readProcess(char *);

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    FILE *input = fopen(argv[1], "r");
    if (input == NULL)
    {
        printf("Input File not found\n");
        exit(1);
    }

    // read lines of input file
    char *line = NULL;
    size_t len = 0;
    int lineCount = 0;

    while (getline(&line, &len, input) != -1)
    {
        lineCount++;
        if (lineCount == 1)
            continue;

        // read process
        struct Process *process = readProcess(line);
    }

    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
    int algorithm = -1;
    int quantum = 2;
    readOptions(&algorithm, &quantum, argc, argv);
    printf("Algorithm: %d\n", algorithm);
    printf("Quantum: %d\n", quantum);
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock.
    initClk();
    // To get time use this function.
    int x = getClk();
    printf("Current Time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

void readOptions(int* algorithm, int* quantum, int argc, char** argv)
{
    for(int i=1; i< argc; i++)
    {
        if(strcmp(argv[i], "-sch") == 0)
        {
            if(i+1 > argc)
            {
                printf("Error: No scheduling algorithm specified\n");
                exit(1);
            }
            else
            {
                *algorithm = atoi(argv[i + 1]);
            }
        }
        else if(strcmp(argv[i], "-q") == 0)
        {
            if(i+1 <= argc)
            {
                *quantum = atoi(argv[i + 1]);
            }
        }
    }
}
struct Process *readProcess(char *line)
{
    char *token = strtok(line, "\t");
    int pid = atoi(token);

    token = strtok(NULL, "\t");
    int arrivalTime = atoi(token);

    token = strtok(NULL, "\t");
    int burstTime = atoi(token);

    token = strtok(NULL, "\t");
    int priority = atoi(token);

    struct Process *process = createProcess(pid, priority, burstTime, arrivalTime);
    return process;
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
}
