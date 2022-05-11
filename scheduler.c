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
void recieveProcessHPF();
void stoppingHandler();
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
int runningProcessPid;
int runningProcessRemainingTime;

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    signal(SIGUSR1, handler);
    signal(SIGUSR2, childHandler);

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
        if (recivedAllProcesses && isEmpty(processesQueue) && !currentRunning)
        {
            break;
        }
    }
}

void HPF()
{
    fprintf(schedulerLog, "----------          HPF algorithm started          ---------\n");
    fprintf(schedulerLog, "At time x process y state arr w total z remain y wait k\n\n");
    processesQueue = createQueue();
    pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);

    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
    }
    while (1)
    {
        recieveProcessHPF();
        if (!currentRunning && !isEmpty(processesQueue))
        {
            processSend = dequeue(processesQueue);
            processSend->startTime = getClk();
            if (processSend->state == STOPPED) {
                processSend->waitTime += getClk() - processSend->stoppingTime;
            } else  {
                processSend->waitTime += getClk() - processSend->stoppingTime;
            }
            processSend->state = STARTED;
            
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
            runningProcessRemainingTime=processSend->remainingTime;
            processSend->remainingTime = 0;
           // processSend->runTime;
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
        if (recivedAllProcesses && isEmpty(processesQueue) && !currentRunning)
        {
            break;
        }
    }
}

void RR(int quantum)
{
    fprintf(schedulerLog, "----------          RR algorithm started          ---------\n");
    fprintf(schedulerLog, "At time x process y state arr w total z remain y wait k\n\n");
    processesQueue = createQueue();
    pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    // signal(SIGCHLD, childHandler);

    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
    }
    while (1)
    {
        recieveProcess();
        stoppingHandler();
        if (!currentRunning && !isEmpty(processesQueue))
        {
            currentRunning = true;
            processSend = dequeue(processesQueue);
            processSend->state = STARTED;
            processSend->startTime = getClk();
            processSend->waitTime += getClk() - processSend->stoppingTime;
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);

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
                    currentRunning = true;
                    processSend->pid = runningProcessPid;
                }
            }
            else
            {
                kill(processSend->pid, SIGCONT);
                currentRunning = true;
                runningProcessPid = processSend->pid;
                // printf("Sending continue to process %d at time %d\n", processSend->id, getClk());
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

    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
    }

    InitializeMultiLevelQueue();
    while (1)
    {
        recieveMultiLevelProcesses();
        processesQueue = getPriorityQueue(multiLevelQueue, PRIORITY_LEVELS);

        if (processesQueue != NULL && !currentRunning && !isEmpty(processesQueue))
        {
            processSend = dequeue(processesQueue);
            processSend->state = STARTED;
            processSend->waitTime += getClk() - processSend->stoppingTime;
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);

            runningProcessRemainingTime = processSend->remainingTime;
            if (processSend->remainingTime >= quantum)
                processSend->remainingTime -= quantum;
            else
                processSend->remainingTime = 0;

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
                currentRunning = true;
            }
        }
        if (recivedAllProcesses && (processesQueue == NULL || isEmpty(processesQueue)) && !currentRunning)
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
    if (processSend->remainingTime == 0)
    {
        currentRunning = false;
        processSend->state = FINISHED;
        processSend->finishTime = getClk();
        float turnAround = processSend->finishTime - processSend->arrivalTime;
        float weightedTurnAround = turnAround / processSend->runTime;
        fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d TA %.2f WTA %.2f\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime, turnAround, weightedTurnAround);
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

void recieveProcessHPF()
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
            // printf("Recieved process of priority %d at clock: %d\n", p->priority, getClk());
            if(processSend){
                // printf("Current process of priority %d at clock: %d\n", processSend->priority, getClk());
            }
            if (processSend && p->priority < processSend->priority)
            {
                // printf("Higher priority process received\n");
                processSend->remainingTime=runningProcessRemainingTime-getClk()+processSend->startTime;
                processSend->runTime -= (getClk() - processSend->startTime);
                kill(runningProcessPid, SIGSTOP);
                processSend->state = STOPPED;
                processSend->stoppingTime = getClk();
                fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
                currentRunning = false;
                insertByPriority(processesQueue, processSend);
            }
            insertByPriority(processesQueue, p);
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
            enqueue(multiLevelQueue[p->priority], p);
            fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);
            if (currentRunning && p->priority > processSend->priority)
            {
                kill(runningProcessPid, SIGTERM);
                processSend->remainingTime = runningProcessRemainingTime - getClk();
            }
            break;
        }
    }
}

void InitializeMultiLevelQueue()
{
    multiLevelQueue = (struct Queue **)malloc(sizeof(struct Queue *) * (PRIORITY_LEVELS + 1));
    for (int i = 0; i <= PRIORITY_LEVELS; i++)
    {
        multiLevelQueue[i] = createQueue();
    }
}

void stoppingHandler()
{
    if (currentRunning && processSend->remainingTime != 0 && getClk() - processSend->startTime == quantum)
    {
        kill(runningProcessPid, SIGSTOP);
        currentRunning = false;
        processSend->state = STOPPED;
        processSend->stoppingTime = getClk();
        fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);

        if (AlgoType == MULTILEVEL_FEEDBACK_QUEUE)
        {
            recieveMultiLevelProcesses();
            for (int i = 0; i <= PRIORITY_LEVELS; i++)
            {
                printf("Level %d\n", i);
                printQueue(multiLevelQueue[i]);
            }

            int nextLevel = processSend->priority + 1 <= PRIORITY_LEVELS ? ++processSend->priority : PRIORITY_LEVELS;
            enqueue(multiLevelQueue[nextLevel], processSend);

            printf("Queues after enqueue\n");
            for (int i = 0; i <= PRIORITY_LEVELS; i++)
            {
                printf("Level %d\n", i);
                printQueue(multiLevelQueue[i]);
            }
        }
        else
        {
            recieveProcess();
            enqueue(processesQueue, processSend);
        }
    }
}