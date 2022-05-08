#include "headers.h"

/* Modify this file as needed*/
int remainingTime = 0;
int waitTime = 0;
int finishTime = 0;
int timeQuantum = 0;

void alarmHandler(int signum);

int main(int agrc, char *argv[])
{
    initClk();
    
    remainingTime = atoi(argv[1]);
    timeQuantum = atoi(argv[2]);

    signal(SIGALRM, alarmHandler);

    if (remainingTime >= timeQuantum)
    {
        alarm(timeQuantum);
        pause();
        remainingTime = remainingTime - timeQuantum;
    }
    else
    {
        alarm(remainingTime);
        pause();
        remainingTime = 0;
    }
    destroyClk(false);
    return 0;
}
void alarmHandler(int signum) {}
