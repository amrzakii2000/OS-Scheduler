#include "headers.h"

int main(int argc, char *argv[])
{
    int AlgoType = -1;
    int quantum = -1;
    initClk();
    // TODO: implement the scheduler.
    receiveInt(&AlgoType);
    receiveInt(&quantum);
    int pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    struct processMsgBuff message;
    while (1)
    {
        rec_process = msgrcv(pGeneratorToSchedulerQueue, &message, sizeof(message.mtext), 1, !IPC_NOWAIT);

    }
    // TODO: upon termination release the clock resources.

    destroyClk(true);
}
