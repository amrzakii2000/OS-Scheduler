#include "headers.h"

/* Modify this file as needed*/
int runTime = 0;
int timeQuantum = 0;
int currentClk;

void contHandler(int);

int main(int agrc, char *argv[])
{
    initClk();
    signal(SIGCONT, contHandler);
    runTime = atoi(argv[1]);
    timeQuantum = atoi(argv[2]);
    // printf("Current of process %d Runtime: %d at clock: %d\n", getpid(), runTime, getClk());

    currentClk = getClk();
    while (runTime > 0)
    {

        // save the currentclk to be always equal to actual clk amd reduce the remainingtime
        if (currentClk < getClk())
        {
            currentClk = getClk();
            --runTime;
            // printf ("\ncurrent clk is %d and remaining time is %d\n", currentClk, remainingTime);
        }
    }
    kill(getppid(), SIGUSR2);
    destroyClk(false);
    return 0;
}

void contHandler(int signum)
{
    currentClk = getClk();
    signal(SIGCONT, contHandler);
}
