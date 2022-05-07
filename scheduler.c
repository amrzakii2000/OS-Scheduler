#include "headers.h"

void clearResources(int);
void SJF();
void HPF();
void RR(int quantum);
void MLFQ();
void alarmHandler();

struct Queue *processesQueue;

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    int AlgoType = atoi(argv[1]);
    int quantum = atoi(argv[2]);

    // TODO: implement the scheduler.
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
        MLFQ();
        break;
    default:
        break;
    }
    // TODO: upon termination release the clock resources.
    return 0;
}

//shortest job first algorithm
void SJF()
{
    processesQueue = createQueue();
    int c = 0;
    int pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    struct processMsgBuff message;
    while (1)
    {
        int rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, !IPC_NOWAIT);
        if (rec_process == -1)
            perror("Error in receive");
        else
        {
            struct Process p = message.process;
            enqueue(processesQueue, &p);
            printf("Process %d has arrived\n", p.id);
            c++;
        }
        if (c == 6)
        {
            raise(SIGINT);
        }
        
    }
}

void HPF()
{
    // int pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
}

void RR(int quantum)
{
    int rec_process = 0;
    struct Process *temp = NULL;
    int c = 0;
    int pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    struct Queue *q = createQueue();
    struct processMsgBuff message;

    while (!rec_process)
    {
        rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, IPC_NOWAIT);
        if (rec_process == -1)
            printf("Error in receive");
        else
        {
            rec_process = 0;
            printf("\nMessage received: %d\n", message.process.id);
            enqueue(q, &message.process);
            printf("\nQueue front: %d\n  Queue rear %d \n ", q->front->id, q->rear->id);
        }
    }
    while (1)
    {
        while (!isEmpty(q))
        {
            q->front->remainingTime -= 1;
            printf("\nProcess %d remaining time: %d\n", q->front->id, q->front->remainingTime);
            sleep(1);
            c++;
            if (c == quantum)
            {
                if ((q->front->remainingTime) != 0)
                {
                    temp = dequeue(q);
                    while (!rec_process)
                    {
                        rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, IPC_NOWAIT);
                        printf("\nMessage received: %d\n", message.process.id);
                        enqueue(q, &message.process);
                    }
                    enqueue(q, temp);
                }
                else
                {
                    temp = dequeue(q);
                    printf("\nProcess %d finished\n", temp->id);
                }
                c = 0;
            }
            while (!rec_process)
            {
                rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, IPC_NOWAIT);
                printf("\nMessage received: %d\n", message.process.id);
                enqueue(q, &message.process);
            }
        }
        while (!rec_process)
        {
            rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, IPC_NOWAIT);
            printf("\nMessage received: %d\n", message.process.id);
            enqueue(q, &message.process);
        }
    }
}

void MLFQ()
{

}

void alarmHandler()
{
}

void clearResources(int signum)
{
    destroyClk(true);
    exit(0);
}