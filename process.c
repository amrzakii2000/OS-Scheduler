#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

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
        // remainingtime = ??;
    }

    destroyClk(false);

    return 0;
}
