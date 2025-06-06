#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Focus-Mode.c"
#include "CPU-Scheduler.c"

#define REQUIRED_ARGS              2
#define FOCUS_MODE_CMD             "Focus-Mode"
#define CPU_SCHEDULER_CMD          "CPU-Scheduler"
#define USAGE                      "Usage: %s <Focus-Mode/CPU-Schedule> <Num-Of-Rounds/Processes.csv> <Round-Duration/Time-Quantum>"

int main(const int argc, const char* const * argv)
{
    if (argc < REQUIRED_ARGS)
    {
        printf(USAGE, argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], FOCUS_MODE_CMD) == 0)
    {
        int numOfRounds = atoi(argv[2]);
        int duration = atoi(argv[3]);

        HandleFocusMode(numOfRounds, duration);
        exit(0);
    }
    else if (strcmp(argv[1], CPU_SCHEDULER_CMD) == 0)
    {
        const char* processesCsvFilePath = argv[2];
        int timeQuantum = atoi(argv[3]);

        HandleCPUScheduler(processesCsvFilePath, timeQuantum);
        exit(0);
    }
    else
    {
        printf(USAGE, argv[0]);
        exit(1);
    }
}


