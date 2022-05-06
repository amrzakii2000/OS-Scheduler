#include "headers.h"

void SJF();
void HPF();
void RR(int quantum);
void MLFQ();
int main(int argc, char *argv[])
{
    int AlgoType = -1;
    int quantum = -1;
    initClk();
    // TODO: implement the scheduler.
    receiveInt(&AlgoType);
    receiveInt(&quantum);
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
        printf("Invalid Algorithm Type\n");
        break;
    }
    // TODO: upon termination release the clock resources.
    destroyClk(true);
}

// shortest job first algorithm
void SJF()
{
    int pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    struct processMsgBuff message;
    while (1)
    {
        rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, !IPC_NOWAIT);
        if (rec_process == -1)
            perror("Error in receive");
        else
        {
            printf("\nMessage received: %s\n", message.mtext);
            struct Process p = message.process;
            int pid = fork();
            if (pid == 0)
            {
                // child process
            }
            else
            {
                // parent process
            }
        }
    }
}

// Round Robin algorithm
void RR(int quantum)
{
    struct process temp;
    int c = 0;
    int pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    struct queue *q = createQueue();
    struct processMsgBuff message;

    while (!rec_process)
    {
        rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, !IPC_NOWAIT);
        printf("\nMessage received: %s\n", message.mtext);
        enqueue(q, message.process);
    }
    while (1)
    {
        while (!isEmpty(q))
        {
            q.front.remaining_time -= 1;
            sleep(1);
            c++;
            if (c == quantum)
            {
                if (q.front.remaining_time != 0)
                {
                    temp = dequeue(q);
                    while (!rec_process)
                    {
                        rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, IPC_NOWAIT);
                        printf("\nMessage received: %s\n", message.mtext);
                        enqueue(q, message.process);
                    }
                    enqueue(q, temp);
                }
                else
                {
                    temp = dequeue(q);
                    printf("\nProcess %d finished\n", temp.pid);
                }
                c = 0;
            }
            while (!rec_process)
            {
                rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, IPC_NOWAIT);
                printf("\nMessage received: %s\n", message.mtext);
                enqueue(q, message.process);
            }
        }
        while (!rec_process)
        {
            rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.process), 1, IPC_NOWAIT);
            printf("\nMessage received: %s\n", message.mtext);
            enqueue(q, message.process);
        }
    }
}