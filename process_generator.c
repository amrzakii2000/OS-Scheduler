#include "headers.h"
#include <string.h>

void clearResources(int);
void readOptions(int *, int *, int, char **);
struct Queue *readFromFile(char *);
struct Process *readProcess(char *);
void alarmHandler(int);

int startTime = 0;
struct Queue *processesQueue;

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    signal(SIGALRM, alarmHandler);
    // Read processes in a queue
    processesQueue = readFromFile(argv[1]);

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
        char algorithmName[5];
        char quantumName[5];

        sprintf(algorithmName, "%d", algorithm);
        sprintf(quantumName, "%d", quantum);
        execl("./scheduler.out", "./scheduler.out", algorithmName, quantumName, NULL);
    }

    initClk();
    startTime = getClk();
    if (processesQueue->front->arrivalTime - startTime > 0)
    {
        alarm(processesQueue->front->arrivalTime - startTime);
        pause();
    }

    while (!isEmpty(processesQueue))
    {
        int currentTime = getClk();
        while (processesQueue->front != NULL && currentTime - startTime >= processesQueue->front->arrivalTime)
        {
            struct Process *process = dequeue(processesQueue);
            sendProcess(process);
        }

        if (isEmpty(processesQueue))
            break;
        
        int waitingTime = processesQueue->front->arrivalTime - getClk();
        if (waitingTime > 0)
        {
            alarm(waitingTime);
            pause();
        }
    }
    kill(schedulerPid, SIGUSR1);
    destroyClk(false);
    pause();
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

struct Queue *readFromFile(char *fileName)
{
    // 1. Read the input files.
    FILE *input = fopen(fileName, "r");
    if (input == NULL)
    {
        printf("Input File not found\n");
        exit(1);
    }

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
    fclose(input);
    return processesQueue;
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

void alarmHandler(int signum) {}

void clearResources(int signum)
{
    destroyClk(true);
    exit(0);
}
