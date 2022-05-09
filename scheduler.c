#include "headers.h"
#include <string.h>

#define PRIORITY_LEVELS 10
#define SHORTEST_JOB_FIRST 1
#define HIGHEST_PRIORITY_FIRST 2
#define ROUND_ROBIN 3
#define MULTILEVEL_FEEDBACK_QUEUE 4

void clearResources(int);
void SJF();
void HPF();
void RR(int);
void MLFQ(int);

void handler(int);
void alarmHandler(int);
void childHandler(int);
void recieveProcess();
void InitializeMultiLevelQueue(); 
void recieveMultiLevelProcesses();

int AlgoType;
int quantum; 
struct Queue *processesQueue;
bool recivedAllProcesses = false;
bool currentRunning = false;
int pGeneratorToSchedulerQueue;
struct Process *processSend;
FILE *schedulerLog;
int rec_process = 1;
struct Process *p = NULL;
struct processMsgBuff message;
struct Queue **multiLevelQueue;

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    signal(SIGUSR1, handler);

    initClk();
    AlgoType = atoi(argv[1]);
    quantum = atoi(argv[2]);
    schedulerLog = fopen("scheduler.log", "w");
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
    raise(SIGINT);
    return 0;
}

// shortest job first algorithm
void SJF()
{
    fprintf(schedulerLog, "----------          SJF algorithm started          ---------\n");
    fprintf(schedulerLog, "At time x process y state arr w total z remain y wait k\n\n");
    processesQueue = createQueue();
    pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    signal(SIGCHLD, childHandler);

    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
    }
    while (1)
    {
        recieveProcess();
        if (!currentRunning && !isEmpty(processesQueue))
        {
            processSend = dequeue(processesQueue);
            processSend->state = STARTED;
            processSend->waitTime += getClk() - processSend->stoppingTime;
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
            processSend->remainingTime = 0;
            int pid = fork();
            if (pid == 0)
            {
                char runTimeValue[5];
                sprintf(runTimeValue, "%d", processSend->runTime);
                execl("./process.out", "./process.out", runTimeValue, runTimeValue, NULL);
            }
            else if (pid != -1)
            {
                currentRunning = true;
            }
        }
        if (recivedAllProcesses && isEmpty(processesQueue) && !currentRunning)
        {
            break;
        }
    }
}

void HPF()
{
}

void RR(int quantum)
{
    fprintf(schedulerLog, "----------          RR algorithm started          ---------\n");
    fprintf(schedulerLog, "At time x process y state arr w total z remain y wait k\n\n");
    processesQueue = createQueue();
    pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    signal(SIGCHLD, childHandler);

    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
    }
    while (1)
    {
        recieveProcess();
        if (!currentRunning && !isEmpty(processesQueue))
        {
            processSend = dequeue(processesQueue);
            processSend->state = STARTED;
            processSend->waitTime += getClk() - processSend->stoppingTime;
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);

            if (processSend->remainingTime >= quantum)
                processSend->remainingTime -= quantum;
            else
                processSend->remainingTime = 0;

            int pid = fork();
            if (pid == 0)
            {
                char remainingTimeValue[5];
                char quantumTimeValue[5];

                sprintf(remainingTimeValue, "%d", processSend->runTime);
                sprintf(quantumTimeValue, "%d", quantum);
                execl("./process.out", "./process.out", remainingTimeValue, quantumTimeValue, NULL);
            }
            else if (pid != -1)
            {
                // printf("Process %d started its quantum at %d\n", processSend->id, getClk());
                currentRunning = true;
            }
        }
        if (recivedAllProcesses && isEmpty(processesQueue) && !currentRunning)
        {
            break;
        }
    }
}

void MLFQ(int quantum)
{
    fprintf(schedulerLog, "----------          MLFQ algorithm started          ---------\n");
    fprintf(schedulerLog, "At time x process y state arr w total z remain y wait k\n\n");
    pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    signal(SIGCHLD, childHandler);

    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
    }
    InitializeMultiLevelQueue();
    while (1)
    {
        recieveMultiLevelProcesses();
        processesQueue = getPriorityQueue(multiLevelQueue, PRIORITY_LEVELS);

        if (!currentRunning && !isEmpty(processesQueue))
        {
            processSend = dequeue(processesQueue);
            processSend->state = STARTED;
            processSend->waitTime += getClk() - processSend->stoppingTime;
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);

            if (processSend->remainingTime >= quantum)
                processSend->remainingTime -= quantum;
            else
                processSend->remainingTime = 0;

            int pid = fork();
            if (pid == 0)
            {
                char remainingTimeValue[5];
                char quantumTimeValue[5];
                sprintf(remainingTimeValue, "%d", processSend->runTime);
                sprintf(quantumTimeValue, "%d", quantum);
                execl("./process.out", "./process.out", remainingTimeValue, quantumTimeValue, NULL);
            }
            else if (pid != -1)
            {
                currentRunning = true;
            }
        }
        if (recivedAllProcesses && isEmpty(processesQueue) && !currentRunning)
        {
            break;
        }
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
    currentRunning = false;
    if (processSend->remainingTime == 0)
    {
        processSend->state = FINISHED;
        processSend->finishTime = getClk();
        float turnAround = processSend->finishTime - processSend->arrivalTime;
        float weightedTurnAround = turnAround / processSend->runTime;
        fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d TA %.2f WTA %.2f\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime, turnAround, weightedTurnAround);
    }
    else
    {
        processSend->state = STOPPED;
        processSend->stoppingTime = getClk();
        fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
        
        if(AlgoType == MULTILEVEL_FEEDBACK_QUEUE)
        {
            recieveMultiLevelProcesses();
            int nextLevel = processSend->priority + 1 > PRIORITY_LEVELS ? PRIORITY_LEVELS : processSend->priority + 1; 
            enqueue(multiLevelQueue[nextLevel], processSend);
        }
        else
        {
            recieveProcess();
            enqueue(processesQueue, processSend);
        }
    }
}

void clearResources(int signum)
{
    msgctl(pGeneratorToSchedulerQueue, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
    exit(0);
}

void recieveProcess()
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

            p = createProcess(message.process.id, message.process.priority, message.process.runTime, message.process.arrivalTime);
            enqueue(processesQueue, p);
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);
            break;
        }
    }
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

            p = createProcess(message.process.id, message.process.priority, message.process.runTime, message.process.arrivalTime);
            printf("Process %d recieved\n", p->id);
            enqueue(multiLevelQueue[p->priority], p);
            printf("Process %d recieved\n", p->id);
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);
            break;
        }
    }
}

void InitializeMultiLevelQueue()
{
    multiLevelQueue = (struct Queue **)malloc(sizeof(struct Queue *) * (PRIORITY_LEVELS + 1) );
    for (int i = 0; i <= PRIORITY_LEVELS; i++)
    {
        multiLevelQueue[i] = createQueue();
    }
}