#include "headers.h"
#include <string.h>

void clearResources(int);
void readOptions(int *, int *, int, char **);
struct Process *readProcess(char *);

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // 1. Read the input files.
    FILE *input = fopen(argv[1], "r");
    if (input == NULL)
    {
        printf("Input File not found\n");
        exit(1);
    }
    printf("Reading input file...\n");
    struct Queue *processesQueue = createQueue();
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
        enqueue(processesQueue, process);
    }

    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
    int algorithm = -1;
    int quantum = 2;
    readOptions(&algorithm, &quantum, argc, argv);

    // 3. Initiate and create the scheduler and clock processes.
    int clockPid = fork();
    if (clockPid == -1)
    {
        printf("Error in creating clock process\n");
        exit(1);
    }
    else if (clockPid == 0)
    {
        printf("Creating clock process\n");
        execl("./clk.out", "./clk.out", NULL);
    }
    int schedulerPid = fork();
    if (schedulerPid == -1)
    {
        printf("Error in creating scheduler process\n");
        exit(1);
    }
    else if (schedulerPid == 0)
    {
        execl("./scheduler.out", "./scheduler.out", NULL);
    }
    // To get time use this function.
    initClk();
    int x = getClk();
    sendInt(quantum);
    sendInt(algorithm);

    printf("Current Time is %d\n", x);

    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    while (!isEmpty(processesQueue))
    {
        int currentTime = getClk();
        while (currentTime - x == processesQueue->front->arrivalTime)
        {
            struct Process *process = dequeue(processesQueue);
            sendProcess(process);
        }
    }

    // 7. Clear clock resources
    destroyClk(true);
}

void readOptions(int *algorithm, int *quantum, int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-sch") == 0)
        {
            if (i + 1 > argc)
            {
                printf("Error: No scheduling algorithm specified\n");
                exit(1);
            }
            else
            {
                *algorithm = atoi(argv[i + 1]);
            }
        }
        else if (strcmp(argv[i], "-q") == 0)
        {
            if (i + 1 <= argc)
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

    token = strtok(NULL, "\t");
    int memSize = atoi(token);


    struct Process *process = createProcess(pid, priority, burstTime, arrivalTime,memSize);
    return process;
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
}
