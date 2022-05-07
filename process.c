#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int timeQuantum;
void alarmHandler();

int main(int agrc, char *argv[])
{
    initClk();

    // TODO The process needs to get the remaining time from somewhere
    remainingtime = atoi(argv[1]);
    timeQuantum = atoi(argv[2]);
    if(remainingtime>=timeQuantum){
    alarm(timeQuantum);
    pause();
    remainingtime = remainingtime - timeQuantum;
    }
    else{
        alarm(remainingtime);
        pause();
        remainingtime = 0;
        
    }
    destroyClk(false);
    printf("process finished after time: %d and remaining time: %d \n", timeQuantum,remainingtime);
    return 0;
}
void alarmHandler() {}
