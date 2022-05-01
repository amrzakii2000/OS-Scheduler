#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

///==============================
// don't mess with this variable//
int *shmaddr; //
//===============================

int getClk()
{
    return *shmaddr;
}

/*
 * All processes call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
 */
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        // Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All processes call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
 */

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}

enum ProccessState
{
    ARRIVED,
    STARTED,
    RESUMED,
    STOPPED,
    FINISHED
};

struct Process
{
    int id;
    int priority;
    int runTime;
    int arrivalTime;
    int waitTime;
    int finishTime;
    int remainingTime;
    enum ProccessState state;
    struct Process *next;
};

struct Process *createProcess(int id, int priority, int runTime, int arrivalTime)
{
    struct Process *p = malloc(sizeof *p);
    p->id = id;
    p->priority = priority;
    p->runTime = p->remainingTime = runTime;
    p->arrivalTime = arrivalTime;
    p->waitTime = 0;
    p->finishTime = 0;
    p->state = ARRIVED;
    p->next = NULL;

    return p;
}

struct Queue
{
    struct Process *front;
    struct Process *rear;
};

struct Queue *createQueue()
{
    struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}

void enqueue(struct Queue *q, struct Process *p)
{
    struct Process *front = q->front;
    struct Process *rear = q->rear;

    if (front == NULL)
    {
        front = rear = p;
    }
    else
    {
        rear->next = p;
        rear = p;
    }
}

struct Process *dequeue(struct Queue *q)
{
    struct Process *front = q->front;
    struct Process *rear = q->rear;
    if (front == NULL)
    {
        return NULL;
    }
    else
    {
        struct Process *p = front;
        front = front->next;
        return p;
    }
}

void insertByPriority(struct Queue *q, struct Process *p)
{
    struct Process *front = q->front;
    struct Process *rear = q->rear;

    if (front == NULL)
    {
        front = rear = p;
    }
    else
    {
        struct Process *temp = front;
        if (p->priority < temp->priority)
        {
            p->next = temp;
            front = p;
        }
        else
        {
            while (temp->next != NULL && temp->next->priority < p->priority)
            {
                temp = temp->next;
            }
            p->next = temp->next;
            temp->next = p;
        }
    }
}

void insertByRuntime(struct Queue *q, struct Process *p)
{
    struct Process *front = q->front;
    struct Process *rear = q->rear;

    if (front == NULL)
    {
        front = rear = p;
    }
    else
    {
        struct Process *temp = front;
        if (p->runTime < temp->runTime)
        {
            p->next = temp;
            front = p;
        }
        else
        {
            while (temp->next != NULL && temp->next->runTime < p->runTime)
            {
                temp = temp->next;
            }
            p->next = temp->next;
            temp->next = p;
        }
    }
}