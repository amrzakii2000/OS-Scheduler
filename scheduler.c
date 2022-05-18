#include "headers.h"
#include <string.h>

#define PRIORITY_LEVELS 10
#define SHORTEST_JOB_FIRST 1
#define HIGHEST_PRIORITY_FIRST 2
#define ROUND_ROBIN 3
#define MULTILEVEL_FEEDBACK_QUEUE 4
#define MAXMEMSIZE 1024

void clearResources(int);
void SJF();
void HPF();
void RR(int);
void MLFQ(int);

void handler(int);
void alarmHandler(int);
void childHandler(int);
void recieveProcess();
void recieveProcessHPF();
void stoppingHandler();
void InitializeMultiLevelQueue();
void recieveMultiLevelProcesses();
void countIdleClockCycles();
void countClockCycles();
bool checkMemory(struct Process *p);

int AlgoType;
int quantum;
int processesCount;
struct Queue *processesQueue;
bool recivedAllProcesses = false;
bool currentRunning = false;
int pGeneratorToSchedulerQueue;
struct Process *processSend;
FILE *schedulerLog;
FILE *schedulerPerf;
int rec_process = 1;
struct Process *p = NULL;
struct processMsgBuff message;
struct Queue **multiLevelQueue;
int runningProcessPid;
int runningProcessRemainingTime;
int totalWaitingTime = 0;
float totalWeightedTurnAroundTime = 0;
int currentClock = 0;
int totalClockCycles = 0;
int idleClockCycles = 0;
struct Queue *diskQueue;
FILE *memoryLog;
bool memory[MAXMEMSIZE] = {false};
struct memQueue *memoryQueue;

int main(int argc, char *argv[])
{
    // clear resources on interrupt
    signal(SIGINT, clearResources);

    // Set recieved all processes flag to true
    signal(SIGUSR1, handler);

    // Handle the signal sent by finished process
    signal(SIGUSR2, childHandler);

    initClk();

    // read command line arguments
    AlgoType = atoi(argv[1]);
    quantum = atoi(argv[2]);
    processesCount = atoi(argv[3]);

    schedulerLog = fopen("scheduler.log", "w");
    memoryLog = fopen("memory.log", "w");

    switch (AlgoType)
    {
    case 1:
        SJF();
        break;
    case 2:
        HPF();
        break;
    case 3:
        RR(quantum);
        break;
    case 4:
        MLFQ(quantum);
        break;
    default:
        break;
    }

    fprintf(schedulerLog, "At time %d all processes finished\n", getClk());
    fclose(schedulerLog);
    fclose(memoryLog);

    schedulerPerf = fopen("scheduler.perf", "w");
    fprintf(schedulerPerf, "CPU Utilization: %.2f %%\n", ((float)(totalClockCycles - idleClockCycles) / totalClockCycles) * 100);
    fprintf(schedulerPerf, "Average WT: %.2f\n", (float)totalWaitingTime / processesCount);
    fprintf(schedulerPerf, "Average WTA: %.2f\n", totalWeightedTurnAroundTime / processesCount);
    fclose(schedulerPerf);

    // Raise interrupt signal to clear all resources
    raise(SIGINT);

    return 0;
}

// shortest job first algorithm
void SJF()
{
    fprintf(schedulerLog, "----------          SJF algorithm started          ---------\n");
    fprintf(schedulerLog, "At time x process y state arr w total z remain y wait k\n\n");
    fprintf(memoryLog, "#At time x allocated y bytes for process z from i to j\n\n");

    processesQueue = createQueue();
    memoryQueue = createMemQueue();

    diskQueue = createQueue();

    // Intialize IPC
    pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);

    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
    }
    while (1)
    {
        // Recieve processes sent by process generator
        recieveProcess();

        // Get a process ready for running if no other process is running and the queue is not empty
        if (!currentRunning && !isEmpty(processesQueue))
        {
            processSend = dequeue(processesQueue);
            processSend->state = STARTED;
            processSend->waitTime += getClk() - processSend->stoppingTime;
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
            processSend->remainingTime = 0;

            // Fork the running process
            runningProcessPid = fork();
            if (runningProcessPid == 0)
            {
                char runTimeValue[5];
                sprintf(runTimeValue, "%d", processSend->runTime);
                execl("./process.out", "./process.out", runTimeValue, runTimeValue, NULL);
            }
            else if (runningProcessPid != -1)
            {
                currentRunning = true;
            }
        }
        // Break from the schduler when all processes are finished
        if (recivedAllProcesses && isEmpty(processesQueue) && !currentRunning)
        {
            break;
        }

        // Count total cycles and idle clock cycles
        countClockCycles();
    }
}

void HPF()
{
    fprintf(schedulerLog, "----------          HPF algorithm started          ---------\n");
    fprintf(schedulerLog, "At time x process y state arr w total z remain y wait k\n\n");
    fprintf(memoryLog, "#At time x allocated y bytes for process z from i to j\n\n");

    processesQueue = createQueue();
    memoryQueue = createMemQueue();

    diskQueue = createQueue();

    pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);

    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
    }
    while (1)
    {
        // Function used to recieve process for HPF algorithm
        recieveProcessHPF();
        if (!currentRunning && !isEmpty(processesQueue))
        {
            currentRunning = true;
            processSend = dequeue(processesQueue);
            processSend->startTime = getClk();
            processSend->waitTime += getClk() - processSend->stoppingTime;
            processSend->state = STARTED;

            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);

            // Used to store older pre-empted process remaining time
            runningProcessRemainingTime = processSend->remainingTime;
            processSend->remainingTime = 0;

            // If the process has not been forked before
            if (processSend->pid == -1)
            {
                runningProcessPid = fork();
                if (runningProcessPid == 0)
                {
                    char runTimeValue[5];
                    sprintf(runTimeValue, "%d", processSend->runTime);
                    execl("./process.out", "./process.out", runTimeValue, runTimeValue, NULL);
                }
                else if (runningProcessPid != -1)
                {
                    processSend->pid = runningProcessPid;
                }
            }
            else
            {
                // If the process has been forked before and it is its turn to run, we send continue signal
                processSend->state = RESUMED;
                kill(processSend->pid, SIGCONT);
                runningProcessPid = processSend->pid;
            }
        }
        if (recivedAllProcesses && isEmpty(processesQueue) && !currentRunning)
        {
            break;
        }
        countClockCycles();
    }
}

void RR(int quantum)
{
    fprintf(schedulerLog, "----------          RR algorithm started          ---------\n");
    fprintf(schedulerLog, "At time x process y state arr w total z remain y wait k\n\n");
    fprintf(memoryLog, "#At time x allocated y bytes for process z from i to j\n\n");
    processesQueue = createQueue();
    memoryQueue = createMemQueue();

    diskQueue = createQueue();

    pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    // signal(SIGCHLD, childHandler);

    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
    }
    while (1)
    {
        // recieve processes sent
        recieveProcess();

        // Handle the stopping of the running process if it finished its quantum
        stoppingHandler();
        if (!currentRunning && !isEmpty(processesQueue))
        {
            currentRunning = true;
            processSend = dequeue(processesQueue);
            processSend->state = STARTED;
            processSend->startTime = getClk();
            processSend->waitTime += getClk() - processSend->stoppingTime;

            runningProcessRemainingTime = processSend->remainingTime;

            // Calculate process remaining time
            if (processSend->remainingTime >= quantum)
                processSend->remainingTime -= quantum;
            else
                processSend->remainingTime = 0;

            if (processSend->pid == -1)
            {
                runningProcessPid = fork();
                if (runningProcessPid == 0)
                {
                    char remainingTimeValue[5];
                    char quantumTimeValue[5];

                    sprintf(remainingTimeValue, "%d", processSend->runTime);
                    sprintf(quantumTimeValue, "%d", quantum);
                    execl("./process.out", "./process.out", remainingTimeValue, quantumTimeValue, NULL);
                }
                else if (runningProcessPid != -1)
                {
                    processSend->pid = runningProcessPid;
                }
            }
            else
            {
                // Continue the process if it is forked before
                processSend->state = RESUMED;
                kill(processSend->pid, SIGCONT);
                runningProcessPid = processSend->pid;
            }
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, runningProcessRemainingTime, processSend->waitTime);
        }
        if (recivedAllProcesses && isEmpty(processesQueue) && !currentRunning)
        {
            break;
        }
        countClockCycles();
    }
}

void MLFQ(int quantum)
{
    fprintf(schedulerLog, "----------          MLFQ algorithm started          ---------\n");
    fprintf(schedulerLog, "At time x process y state arr w total z remain y wait k\n\n");
    fprintf(memoryLog, "#At time x allocated y bytes for process z from i to j\n\n");
    pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);

    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
    }

    // Memory allocation for multilievel feedback queue
    InitializeMultiLevelQueue();
    memoryQueue = createMemQueue();
    diskQueue = createQueue();

    while (1)
    {
        // Recive arriving process in its corressponding level
        recieveMultiLevelProcesses();
        // Handle process stopping
        stoppingHandler();
        processesQueue = getPriorityQueue(multiLevelQueue, PRIORITY_LEVELS);

        if (processesQueue != NULL && !currentRunning && !isEmpty(processesQueue))
        {
            currentRunning = true;
            processSend = dequeue(processesQueue);
            processSend->state = STARTED;
            processSend->startTime = getClk();
            processSend->waitTime += getClk() - processSend->stoppingTime;

            runningProcessRemainingTime = processSend->remainingTime;
            if (processSend->remainingTime >= quantum)
                processSend->remainingTime -= quantum;
            else
                processSend->remainingTime = 0;

            if (processSend->pid == -1)
            {
                runningProcessPid = fork();
                if (runningProcessPid == 0)
                {
                    char remainingTimeValue[5];
                    char quantumTimeValue[5];

                    sprintf(remainingTimeValue, "%d", processSend->runTime);
                    sprintf(quantumTimeValue, "%d", quantum);
                    execl("./process.out", "./process.out", remainingTimeValue, quantumTimeValue, NULL);
                }
                else if (runningProcessPid != -1)
                {
                    processSend->pid = runningProcessPid;
                }
            }
            else
            {
                processSend->state = RESUMED;
                kill(processSend->pid, SIGCONT);
                runningProcessPid = processSend->pid;
            }
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, runningProcessRemainingTime, processSend->waitTime);
        }
        if (recivedAllProcesses && (processesQueue == NULL || isEmpty(processesQueue)) && !currentRunning)
        {
            break;
        }
        countClockCycles();
    }
}

void alarmHandler(int signum)
{
}

void handler(int signum)
{
    recivedAllProcesses = true;
}

void childHandler(int signum)
{
    if (processSend->remainingTime == 0)
    {
        currentRunning = false;
        processSend->state = FINISHED;
        processSend->finishTime = getClk();
        float turnAround = processSend->finishTime - processSend->arrivalTime;
        float weightedTurnAround = turnAround / processSend->runTime;
        totalWaitingTime += processSend->waitTime;
        totalWeightedTurnAroundTime += weightedTurnAround;
        fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d TA %.2f WTA %.2f\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime, turnAround, weightedTurnAround);
        fprintf(memoryLog, "#At time %d freed %d bytes from process %d from %d to %d\n", getClk(), processSend->memSize, processSend->id, processSend->memStart, processSend->memEnd);
        for (int i = processSend->memStart; i < processSend->memEnd + 1; i++)
        {
            memory[i] = false;
        }
        free(processSend);
    }
}

void clearResources(int signum)
{
    if (AlgoType == MULTILEVEL_FEEDBACK_QUEUE)
    {
        for (int i = 0; i <= PRIORITY_LEVELS; i++)
            free(multiLevelQueue[i]);

        free(multiLevelQueue);
    }
    free(processesQueue);
    free(diskQueue);
    msgctl(pGeneratorToSchedulerQueue, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
    exit(0);
}

void recieveProcess()
{
    rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 0, IPC_NOWAIT);
    if (rec_process == -1)
    {
        rec_process = 1;
    }
    else
    {
        p = createProcess(message.process.id, message.process.priority, message.process.runTime, message.process.arrivalTime, message.process.memSize);
        if (AlgoType == SHORTEST_JOB_FIRST)
        {
            bool freeSpace = false;
            // if (!isEmpty(diskQueue))
            // {
            //     printf("Disk queue is not empty\n");
            //     printf("Disk queue size: %d\n", getQueueSize(diskQueue));
            //     for (int i = 0; i < getQueueSize(diskQueue); i++)
            //     {
            //         struct Process *temp = dequeue(diskQueue);
            //         freeSpace = checkMemory(temp);
            //         if (freeSpace)
            //         {
            //             insertByRuntime(processesQueue, temp);
            //             fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), temp->memSize, temp->id, temp->memStart, temp->memEnd);
            //             fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), temp->id, getProcessStateText(temp->state), temp->arrivalTime, temp->runTime, temp->remainingTime, temp->waitTime);
            //         }
            //         else
            //         {
            //             enqueue(diskQueue, temp);
            //         }
            //     }
            // }
            freeSpace = checkMemory(p);
            if (freeSpace)
            {
                insertByRuntime(processesQueue, p);
                fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);
                fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), p->memSize, p->id, p->memStart, p->memEnd);
            }
            else
            {
                printf("No free space\n");
                enqueue(diskQueue, p);
            }
        }

        else if (AlgoType == ROUND_ROBIN)
        {
            bool freeSpace = false;
            // if (!isEmpty(diskQueue))
            // {
            //     for (int i = 0; i < getQueueSize(diskQueue); i++)
            //     {
            //         struct Process *temp = dequeue(diskQueue);
            //         freeSpace = checkMemory(temp);
            //         if (freeSpace)
            //         {
            //             fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), temp->memSize, temp->id, temp->memStart, temp->memEnd);
            //             fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);
            //             enqueue(processesQueue, temp);
            //         }
            //         else
            //         {
            //             enqueue(diskQueue, temp);
            //         }
            //     }
            // }
            freeSpace = checkMemory(p);
            if (freeSpace)
            {
                enqueue(processesQueue, p);
                fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);
                fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), p->memSize, p->id, p->memStart, p->memEnd);
            }
            else
            {
                enqueue(diskQueue, p);
            }
        }
    }

    // if (AlgoType == SHORTEST_JOB_FIRST)
    // {
    //     bool freeSpace = false;

    //     if (!isEmpty(diskQueue))
    //     {
    //         printf("Disk queue is not empty\n");
    //         printf("Disk queue size: %d\n", getQueueSize(diskQueue));
    //         for (int i = 0; i < getQueueSize(diskQueue); i++)
    //         {
    //             struct Process *temp = dequeue(diskQueue);
    //             freeSpace = checkMemory(temp);
    //             if (freeSpace)
    //             {
    //                 insertByRuntime(processesQueue, temp);
    //                 fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), temp->memSize, temp->id, temp->memStart, temp->memEnd);
    //                 fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), temp->id, getProcessStateText(temp->state), temp->arrivalTime, temp->runTime, temp->remainingTime, temp->waitTime);
    //             }
    //             else
    //             {
    //                 enqueue(diskQueue, temp);
    //             }
    //         }
    //     }
    // }
    // else if (AlgoType == ROUND_ROBIN)
    // {
    //     bool freeSpace = false;
    //     if (!isEmpty(diskQueue))
    //     {
    //         for (int i = 0; i < getQueueSize(diskQueue); i++)
    //         {
    //             struct Process *temp = dequeue(diskQueue);
    //             freeSpace = checkMemory(temp);
    //             if (freeSpace)
    //             {
    //                 fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), temp->memSize, temp->id, temp->memStart, temp->memEnd);
    //                 fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);
    //                 enqueue(processesQueue, temp);
    //             }
    //             else
    //             {
    //                 enqueue(diskQueue, temp);
    //             }
    //         }
    //     }
    // }
}

void recieveProcessHPF()
{
    rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 0, IPC_NOWAIT);
    if (rec_process == -1)
    {
        rec_process = 1;
    }
    else
    {

        p = createProcess(message.process.id, message.process.priority, message.process.runTime, message.process.arrivalTime, message.process.memSize);

        // If a process with higher priority arrives, interrupt the current running process
        // if (processSend && p->priority < processSend->priority)
        // {
        //     processSend->remainingTime = runningProcessRemainingTime - getClk() + processSend->startTime;
        //     kill(runningProcessPid, SIGSTOP);
        //     processSend->state = STOPPED;
        //     processSend->stoppingTime = getClk();
        //     fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
        //     currentRunning = false;
        //     insertByPriority(processesQueue, processSend);
        // }

        bool freeSpace = false;
        // if (!isEmpty(diskQueue))
        // {
        //     for (int i = 0; i < getQueueSize(diskQueue); i++)
        //     {
        //         struct Process *temp = dequeue(diskQueue);
        //         freeSpace = checkMemory(temp);
        //         if (freeSpace)
        //         {
        //             // If a process with higher priority arrives, interrupt the current running process

        //             if (processSend && temp->priority < processSend->priority)
        //             {
        //                 processSend->remainingTime = runningProcessRemainingTime - getClk() + processSend->startTime;
        //                 kill(runningProcessPid, SIGSTOP);
        //                 processSend->state = STOPPED;
        //                 processSend->stoppingTime = getClk();
        //                 fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
        //                 currentRunning = false;
        //                 insertByPriority(processesQueue, processSend);
        //             }
        //             insertByPriority(processesQueue, temp);
        //             fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), temp->memSize, temp->id, temp->memStart, temp->memEnd);
        //             fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);
        //         }
        //         else
        //         {
        //             enqueue(diskQueue, temp);
        //         }
        //     }
        // }
        freeSpace = checkMemory(p);
        if (freeSpace)
        {
            // If a process with higher priority arrives, interrupt the current running process
            if (processSend && p->priority < processSend->priority)
            {
                processSend->remainingTime = runningProcessRemainingTime - getClk() + processSend->startTime;
                kill(runningProcessPid, SIGSTOP);
                processSend->state = STOPPED;
                processSend->stoppingTime = getClk();
                fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
                currentRunning = false;
                insertByPriority(processesQueue, processSend);
            }
            insertByPriority(processesQueue, p);
            fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), p->memSize, p->id, p->memStart, p->memEnd);
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);
        }
        else
            enqueue(diskQueue, p);
    }

    // bool freeSpace = false;
    // if (!isEmpty(diskQueue))
    // {
    //     for (int i = 0; i < getQueueSize(diskQueue); i++)
    //     {
    //         struct Process *temp = dequeue(diskQueue);
    //         freeSpace = checkMemory(temp);
    //         if (freeSpace)
    //         {
    //             // If a process with higher priority arrives, interrupt the current running process

    //             if (processSend && temp->priority < processSend->priority)
    //             {
    //                 processSend->remainingTime = runningProcessRemainingTime - getClk() + processSend->startTime;
    //                 kill(runningProcessPid, SIGSTOP);
    //                 processSend->state = STOPPED;
    //                 processSend->stoppingTime = getClk();
    //                 fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
    //                 currentRunning = false;
    //                 insertByPriority(processesQueue, processSend);
    //             }
    //             insertByPriority(processesQueue, temp);
    //             fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), temp->memSize, temp->id, temp->memStart, temp->memEnd);
    //             fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);
    //         }
    //         else
    //         {
    //             enqueue(diskQueue, temp);
    //         }
    //     }
    // }
}

void recieveMultiLevelProcesses()
{
    while (rec_process != -1)
    {
        rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 0, IPC_NOWAIT);
        if (rec_process == -1)
        {
            rec_process = 1;
            break;
        }
        else
        {

            p = createProcess(message.process.id, message.process.priority, message.process.runTime, message.process.arrivalTime, message.process.memSize);
            bool freeSpace = false;
            // if (!isEmpty(diskQueue))
            // {
            //     for (int i = 0; i < getQueueSize(diskQueue); i++)
            //     {
            //         struct Process *temp = dequeue(diskQueue);
            //         freeSpace = checkMemory(temp);
            //         if (freeSpace)
            //         {
            //             enqueue(multiLevelQueue[temp->priority], p);
            //             fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), temp->memSize, temp->id, temp->memStart, temp->memEnd);
            //             fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);

            //             // Check if a process of higher priority arrived
            //             if (currentRunning && processSend && p->priority < processSend->priority)
            //             {
            //                 processSend->remainingTime = runningProcessRemainingTime - getClk() + processSend->startTime;
            //                 kill(runningProcessPid, SIGSTOP);
            //                 processSend->state = STOPPED;
            //                 processSend->stoppingTime = getClk();
            //                 fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
            //                 currentRunning = false;
            //                 enqueue(multiLevelQueue[processSend->priority], processSend);
            //             }
            //         }
            //         else
            //         {
            //             enqueue(diskQueue, temp);
            //         }
            //     }
            // }

            freeSpace = checkMemory(p);
            if (freeSpace)
            {
                enqueue(multiLevelQueue[p->priority], p);
                fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), p->memSize, p->id, p->memStart, p->memEnd);
                fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);

                // Check if a process of higher priority arrived
                if (currentRunning && processSend && p->priority < processSend->priority)
                {
                    processSend->remainingTime = runningProcessRemainingTime - getClk() + processSend->startTime;
                    kill(runningProcessPid, SIGSTOP);
                    processSend->state = STOPPED;
                    processSend->stoppingTime = getClk();
                    fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
                    currentRunning = false;
                    enqueue(multiLevelQueue[processSend->priority], processSend);
                }
            }
            else
                enqueue(diskQueue, p);

            // // Check if a process of higher priority arrived
            // if (currentRunning && processSend && p->priority < processSend->priority)
            // {
            //     processSend->remainingTime = runningProcessRemainingTime - getClk() + processSend->startTime;
            //     kill(runningProcessPid, SIGSTOP);
            //     processSend->state = STOPPED;
            //     processSend->stoppingTime = getClk();
            //     fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
            //     currentRunning = false;
            //     enqueue(multiLevelQueue[processSend->priority], processSend);
            // }
            break;
        }
    }

    // bool freeSpace = false;
    // if (!isEmpty(diskQueue))
    // {
    //     for (int i = 0; i < getQueueSize(diskQueue); i++)
    //     {
    //         struct Process *temp = dequeue(diskQueue);
    //         freeSpace = checkMemory(temp);
    //         if (freeSpace)
    //         {
    //             enqueue(multiLevelQueue[temp->priority], p);
    //             fprintf(memoryLog, "#At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), temp->memSize, temp->id, temp->memStart, temp->memEnd);
    //             fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);

    //             // Check if a process of higher priority arrived
    //             if (currentRunning && processSend && p->priority < processSend->priority)
    //             {
    //                 processSend->remainingTime = runningProcessRemainingTime - getClk() + processSend->startTime;
    //                 kill(runningProcessPid, SIGSTOP);
    //                 processSend->state = STOPPED;
    //                 processSend->stoppingTime = getClk();
    //                 fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
    //                 currentRunning = false;
    //                 enqueue(multiLevelQueue[processSend->priority], processSend);
    //             }
    //         }
    //         else
    //         {
    //             enqueue(diskQueue, temp);
    //         }
    //     }
    // }
}

void InitializeMultiLevelQueue()
{
    multiLevelQueue = (struct Queue **)malloc(sizeof(struct Queue *) * (PRIORITY_LEVELS + 1));
    for (int i = 0; i <= PRIORITY_LEVELS; i++)
    {
        multiLevelQueue[i] = createQueue();
    }
}

// Function used to check if current running process finished its quantum
void stoppingHandler()
{
    if (currentRunning && processSend->remainingTime != 0 && getClk() - processSend->startTime == quantum)
    {
        kill(runningProcessPid, SIGSTOP);
        currentRunning = false;
        processSend->state = STOPPED;
        processSend->stoppingTime = getClk();
        fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);

        // In case of multi level feedback queue, we move the process to the next level
        if (AlgoType == MULTILEVEL_FEEDBACK_QUEUE)
        {
            recieveMultiLevelProcesses();
            int nextLevel = processSend->priority + 1 <= PRIORITY_LEVELS ? ++processSend->priority : PRIORITY_LEVELS;
            enqueue(multiLevelQueue[nextLevel], processSend);
        }
        else
        {
            recieveProcess();
            enqueue(processesQueue, processSend);
        }
    }
}

// Used to count clock cycles
void countClockCycles()
{
    if (currentClock < getClk())
    {
        currentClock = getClk();

        if (!currentRunning && !recivedAllProcesses)
        {
            idleClockCycles++;
        }
        totalClockCycles++;
        printf("Total Cycles: %d\n", totalClockCycles);
        printf("Idle Cycles: %d\n", idleClockCycles);
    }
}
bool checkMemory(struct Process *p)
{
    struct Pair *iterator = memoryQueue->front;
    int min = INT_MAX;
    int start = 0;
    int c = 0;
    while (iterator != NULL)
    {
        if (iterator->full)
            iterator = iterator->next;

        if (iterator->end - iterator->start >= p->memSize)
        {
            if (iterator->end - iterator->start < min)
            {
                min = iterator->end - iterator->start;
                start = iterator->start;
            }
            iterator = iterator->next;
        }
    }
    if (min == INT_MAX)
    {
        printf("No space for process %d\n", p->id);
        return false;
    }
    iterator = memoryQueue->front;
    while (iterator != NULL)
    {
        if (iterator->start == start)
        {
            while ((iterator->end - iterator->start) / 2 >= p->memSize)
            {
                struct Pair *new = createPair();
                addPair(memoryQueue, iterator, new);
            }
            iterator->full = true;
            p->memStart = start;
            p->memEnd = start + p->actualMemSize;
        }
        iterator = iterator->next;
    }
    return true;

    // int min = INT_MAX;
    // int c = 0;
    // int start = 0;
    // int end = 0;
    // for (int i = 0; i < MAXMEMSIZE; i++)
    // {
    //     if (memory[i] == false && i < MAXMEMSIZE - 1)
    //     {
    //         c++;
    //     }
    //     else
    //     {

    //         if (c >= p->memSize)
    //         {
    //             if (c < min)
    //             {
    //                 start = i - c;
    //                 end = start + p->actualMemSize - 1;
    //                 min = c;
    //             }
    //         }
    //         c = 0;
    //     }
    // }
    // if (min == INT_MAX)
    // {
    //     printf("No space for process %d\n", p->id);
    //     return false;
    // }
    // else
    // {
    //     for (int i = start; i < end + 1; i++)
    //     {
    //         memory[i] = true;
    //     }

    //     p->memStart = start;
    //     p->memEnd = end;
    //     return true;
    // }
}