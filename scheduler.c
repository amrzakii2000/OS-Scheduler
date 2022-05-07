#include "headers.h"

void clearResources(int);
void SJF();
void HPF();
void RR(int quantum);
void MLFQ();

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
    while(1)
    {
        int x = getClk();
        printf("\nCurrent time: %d\n", x);
    }
}

void MLFQ()
{

}

void clearResources(int signum)
{
    destroyClk(true);
    exit(0);
}