#include "headers.h"
#include<string.h>

void clearResources(int);
void SJF();
void HPF();
void RR(int quantum);
void MLFQ();

void handler(int);
void alarmHandler(int);
void childHandlerSJF(int);

struct Queue *processesQueue;
bool recivedAllProcesses = false;
bool currentRunning = false;
int pGeneratorToSchedulerQueue;
struct Process *processSend;
FILE *schedulerLog;

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    signal(SIGUSR1, handler);

    initClk();
    int AlgoType = atoi(argv[1]);
    int quantum = atoi(argv[2]);
        SJF();
        break;
    case 2:
        HPF();
        break;
    case 3:
        RR(quantum);
        break;
    case 4:
        MLFQ();
        break;
    default:
        break;
    }
    // TODO: upon termination release the clock resources.
    fprintf(schedulerLog, "At time %d all processes finished\n", getClk());
    fclose(schedulerLog);
    //printf("%s", logFile);
    raise(SIGINT);
    return 0;
}

// shortest job first algorithm
void SJF()
{
    fprintf(schedulerLog, "At time x process y state arr w total z remain y wait k\n\n");
    //strcat(logFile, "At time x process y state arr w total z remain y wait k\n");
    processesQueue = createQueue();
    struct Process *p = NULL;
    pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    signal(SIGCHLD, childHandlerSJF);

    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
    }
    while (1)
    {
        struct processMsgBuff message;

        int rec_process = 1;
        while (rec_process != -1)
        {
            rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 0, IPC_NOWAIT);
            if (rec_process == -1)
            {
                break;
            }
            else
            {
                p = createProcess(message.process.id, message.process.priority, message.process.runTime, message.process.arrivalTime);
                insertByShortestRunTime(processesQueue, p);
                break;
            }
            // printf("At time %d process %d %s arr %d total %d remain %d wait %d", getClk(), p->id, getProcessStateText(p->state), p->arrivalTime, p->runTime, p->remainingTime, p->waitTime);
        }

        if (!currentRunning && !isEmpty(processesQueue))
        {
            processSend = dequeue(processesQueue);
            processSend->state = STARTED;
            // printf("At time %d process %d %s arr %d total %d remain %d wait %d", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
            //fprintf(schedulerLog, "At time %d ", getClk());
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
    // int rec_process = 0;
    // struct Process *temp = NULL;
    // int c = 0;
    // int pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    // if (pGeneratorToSchedulerQueue == -1)
    // {
    //     perror("Error in create");
    //     exit(-1);
    // }
    // struct Queue *q = createQueue();
    // struct processMsgBuff message;

    // while (!rec_process)
    // {
    //     rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, IPC_NOWAIT);
    //     if (rec_process == -1)
    //         printf("Error in receive");
    //     else
    //     {
    //         rec_process = 0;
    //         printf("\nMessage received: %d\n", message.process.id);
    //         enqueue(q, &message.process);
    //         printf("\nQueue front: %d\n  Queue rear %d \n ", q->front->id, q->rear->id);
    //     }
    // }
    // while (1)
    // {
    //     while (!isEmpty(q))
    //     {
    //         q->front->remainingTime -= 1;
    //         printf("\nProcess %d remaining time: %d\n", q->front->id, q->front->remainingTime);
    //         sleep(1);
    //         c++;
    //         if (c == quantum)
    //         {
    //             if ((q->front->remainingTime) != 0)
    //             {
    //                 temp = dequeue(q);
    //                 while (!rec_process)
    //                 {
    //                     rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, IPC_NOWAIT);
    //                     printf("\nMessage received: %d\n", message.process.id);
    //                     enqueue(q, &message.process);
    //                 }
    //                 enqueue(q, temp);
    //             }
    //             else
    //             {
    //                 temp = dequeue(q);
    //                 printf("\nProcess %d finished\n", temp->id);
    //             }
    //             c = 0;
    //         }
    //         while (!rec_process)
    //         {
    //             rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, IPC_NOWAIT);
    //             printf("\nMessage received: %d\n", message.process.id);
    //             enqueue(q, &message.process);
    //         }
    //     }
    //     while (!rec_process)
    //     {
    //         rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, IPC_NOWAIT);
    //         printf("\nMessage received: %d\n", message.process.id);
    //         enqueue(q, &message.process);
    //     }
    // }
}

void MLFQ()
{
}

void alarmHandler(int signum)
{
}

void handler(int signum)
{
    recivedAllProcesses = true;
}

void childHandlerSJF(int signum)
{
    currentRunning = false;
    if (processSend->remainingTime == 0)
    {
        processSend->state = FINISHED;
        //printf("At time %d process %d %s arr %d total %d remain %d wait %d", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
        //fprintf(schedulerLog, "At time %d process %d %s arr %d total %d remain %d wait %d", getClk(), processSend->id, getProcessStateText(processSend->state), processSend->arrivalTime, processSend->runTime, processSend->remainingTime, processSend->waitTime);
    }
    else
    {
        processSend->state = STOPPED;
    }
}

void clearResources(int signum)
{
    msgctl(pGeneratorToSchedulerQueue, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
    exit(0);
}