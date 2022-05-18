#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
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

struct Pair
{
    int start;
    int end;
    struct Pair *next;
    struct Pair *prev;
    bool full;
};

struct Pair *createPair()
{
    struct Pair *p = (struct Pair *)malloc(sizeof(struct Pair));
    p->start = -1;
    p->end = -1;
    p->next = NULL;
    p->prev = NULL;
    p->full = false;
    return p;
};

struct memQueue
{
    struct Pair *front;
    struct Pair *rear;
};

struct memQueue *createMemQueue()
{
    struct Pair *p = createPair();
    p->start = 0;
    p->end = 1023;
    struct memQueue *q = (struct memQueue *)malloc(sizeof(struct memQueue));
    q->front = p;
    q->rear = q->front;
    return q;
};

void addPair(struct memQueue *q, struct Pair *exist, struct Pair *added)
{
    int temp = (exist->end - exist->start) / 2;
    exist->end = exist->start + temp;
    added->start = exist->end;
    added->end = added->start + temp;
    added->next = exist->next;
    added->prev = exist;
    exist->next = added;
};



struct Process
{
    int id;
    int pid;
    int priority;
    int runTime;
    int arrivalTime;
    int waitTime;
    int stoppingTime;
    int startTime;
    int finishTime;
    int remainingTime;
    int memSize;
    int actualMemSize;
    int memStart;
    int memEnd;
    enum ProccessState state;
    struct Process *next;
};

struct Process *createProcess(int id, int priority, int runTime, int arrivalTime, int memSize)
{
    struct Process *p = (struct Process *)malloc(sizeof(struct Process));
    p->id = id;
    p->priority = priority;
    p->runTime = p->remainingTime = runTime;
    p->arrivalTime = p->stoppingTime = arrivalTime;
    p->waitTime = 0;
    p->finishTime = 0;
    p->state = ARRIVED;
    p->next = NULL;
    p->pid = -1;
    p->memSize = memSize;
    for (int i = 0; i < 10; i++)
    {
        int temp = 1;
        for (int j = 0; j < i; j++)
        {
            temp = temp * 2;
        }

        if (temp >= memSize)
        {
            p->actualMemSize = temp;
            break;
        }
    }
    // p->actualMemSize = pow(2, ceil(log(memSize)/log(2)));
    p->memStart = -1;
    p->memEnd = -1;
    return p;
};

struct processMsgBuff
{
    long mtype;
    struct Process process;
};
struct msgBuff
{
    long mtype;
    int intMsg;
};

void sendProcess(struct Process *p)
{
    int pGeneratorToSchedulerQueue = msgget(1234, 0666 | IPC_CREAT);
    if (pGeneratorToSchedulerQueue == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    struct processMsgBuff message;
    message.mtype = 1;
    message.process = *p;
    int send_val = msgsnd(pGeneratorToSchedulerQueue, &message, sizeof(message.process), !IPC_NOWAIT);
    if (send_val == -1)
        perror("Errror in send");
};

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
    p->next = NULL;
    if (q->front == NULL)
    {
        q->front = q->rear = p;
    }
    else
    {
        if (q->front == q->rear)
        {
            q->front->next = p;
        }
        q->rear->next = p;
        q->rear = p;
    }
}

struct Process *dequeue(struct Queue *q)
{
    if (q->front == NULL)
    {
        return NULL;
    }
    else
    {
        struct Process *p = q->front;
        if (q->front == q->rear)
        {
            q->front = q->rear = NULL;
        }
        else
        {
            q->front = q->front->next;
        }
        return p;
    }
}

void insertByPriority(struct Queue *q, struct Process *p)
{
    if (q->front == NULL)
    {
        p->next = NULL;
        q->front = q->rear = p;
    }
    else
    {
        struct Process *temp = q->front;
        if (temp->priority > p->priority)
        {
            p->next = temp;
            q->front = p;
        }
        else
        {
            while (temp->next != NULL && temp->next->priority < p->priority)
            {
                temp = temp->next;
            }
            p->next = temp->next;
            temp->next = p;
            if (temp == q->rear)
            {
                q->rear = p;
            }
        }
    }
}
void insertByShortestRunTime(struct Queue *q, struct Process *p)
{
    if (q->front == NULL)
    {
        p->next = NULL;
        q->front = q->rear = p;
    }
    else
    {
        struct Process *temp = q->front;
        if (temp->runTime > p->runTime)
        {
            p->next = temp;
            q->front = p;
        }
        else
        {
            while (temp->next != NULL && temp->next->runTime < p->runTime)
            {
                temp = temp->next;
            }
            p->next = temp->next;
            temp->next = p;
            if (temp == q->rear)
            {
                q->rear = p;
            }
        }
    }
}

void insertByRuntime(struct Queue *q, struct Process *p)
{
    if (q->front == NULL)
    {
        p->next = NULL;
        q->front = q->rear = p;
    }
    else
    {
        struct Process *temp = q->front;
        if (temp->runTime > p->runTime)
        {
            p->next = temp;
            q->front = p;
        }
        else
        {
            while (temp->next != NULL && temp->next->runTime < p->runTime)
            {
                temp = temp->next;
            }
            p->next = temp->next;
            temp->next = p;
            if (temp == q->rear)
            {
                q->rear = p;
            }
        }
    }
}

bool isEmpty(struct Queue *q)
{
    return q->front == NULL;
}

char *getProcessStateText(enum ProccessState state)
{
    switch (state)
    {
    case ARRIVED:
        return "ARRIVED";
    case STARTED:
        return "STARTED";
    case RESUMED:
        return "RESUMED";
    case STOPPED:
        return "STOPPED";
    case FINISHED:
        return "FINISHED";
    default:
        return "UNKNOWN";
    }
}

// Print Queue
void printQueue(struct Queue *q)
{
    struct Process *p = q->front;
    while (p != NULL)
    {
        printf("%d ", p->id);
        p = p->next;
    }
    printf("\n");
}

struct Queue *getPriorityQueue(struct Queue **q, int size)
{
    for (int i = 0; i <= size; i++)
    {
        if (!isEmpty(q[i]))
        {
            return q[i];
        }
    }
    return NULL;
}

int getQueueSize(struct Queue *q)
{
    int size = 0;
    struct Process *p = q->front;
    while (p != NULL)
    {
        size++;
        p = p->next;
    }
    return size;
}