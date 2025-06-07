#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#define TICK_TIME 1e5

#define MAX_NAME 51
#define MAX_DESC 101
#define MAX_PROC 1000
#define MAX_LINE 257

#define CSV_DELIMS ","

typedef struct
{
    char name[MAX_NAME];
    char desc[MAX_DESC];
    int arrival_time;
    int burst_time;
    int priority;
    int original_idx;
} Process;



typedef struct
{
    Process procs[MAX_PROC];
    int size;
    int (*CmpPriority)(Process, Process);
} ReadyQueue;



int CmpPriorityNull(Process _, Process __);
int ProcCmpPriority(Process a, Process b, int (*getProcessPriority)(Process));
void Enqueue(ReadyQueue* queue, Process item);
Process Dequeue(ReadyQueue* queue);
bool IsEmpty(ReadyQueue queue);
void InitProcessesFromCSV(const char* path, Process oprocs[], int* oprocsCount);
Process ParseProcess(const char* line);
void SortProcesses(Process procs[], int procCount, int (*predicate)(Process, Process));
int ProcCmpArrivalTime(Process a, Process b);
double GetTimeElapsed(struct timespec startingTime);
void EnqueueNewArrivals(ReadyQueue* queue, Process procs[], int* startingIdx, int procCount, struct timespec startingTime);
void SigAlarmHandler();



void HandleCPUScheduler(const char* processesCsvFilePath, int timeQuantum)
{
    int procsCount = 0;
    Process procs[MAX_PROC];


    /*
     * Get procs from file
     */
    InitProcessesFromCSV(processesCsvFilePath, procs, &procsCount);



    /*
     * Sort procs (stable)
     */
    SortProcesses(procs, procsCount, ProcCmpArrivalTime);



    /*
     * Initialise Ready Queue (FCFS)
     */
    ReadyQueue queue;
    queue.size = 0;
    queue.CmpPriority = CmpPriorityNull;
    if (queue.CmpPriority == NULL)
    {
        fprintf(stderr, "null after init\n");
        exit(EXIT_FAILURE);
    }



    /*
     * Ignoring sigalarms, we use them as a waiting mechanism
     */
    struct sigaction act;
    act.sa_handler = SigAlarmHandler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);



    /*
     * Start timer
     */
    struct timespec startingTime;
    if (clock_gettime(CLOCK_MONOTONIC, &startingTime) != 0)
    {
        perror("clock_gettime() error");
        exit(EXIT_FAILURE);
    }



    /*
     * Main loop which runs every tick (1 second) and:
     * Enqueues new processes
     * Checks if the process currently running has finished
     *
     */
    int startingIdx = 0;
    Process runningProcess;
    bool isProcessRunning = false;
    bool isIdling = false;
    struct timespec processStartingTime;
    bool isProcessNotArrived = startingIdx < procsCount;
    int turnaroundTime = 0;
    int iteration = 0;

    while (isProcessNotArrived || IsEmpty(queue) || isProcessRunning)
    {
        isProcessNotArrived = startingIdx < procsCount;



        if (isProcessNotArrived)
        {
            if (queue.CmpPriority == NULL)
            {
                fprintf(stderr, "Argument null error in function HandleCPUScheduler, 'queue.CmpPriority' cannot be null (iteration %d)\n", iteration);
                exit(EXIT_FAILURE);
            }
            EnqueueNewArrivals(&queue, procs, &startingIdx, procsCount, startingTime);
        }



        if (isProcessRunning)
        {
            int timeElapsed = GetTimeElapsed(processStartingTime);
            if (timeElapsed >= runningProcess.burst_time)
            {
                isProcessRunning = false;
                if (!isProcessNotArrived && IsEmpty(queue))
                {
                    turnaroundTime = GetTimeElapsed(startingTime);
                    break;
                }
            }
        }



        if (!isProcessRunning && !IsEmpty(queue))
        {
            if (isIdling)
            {
                isIdling = false;
            }
            isProcessRunning = true;
            if (clock_gettime(CLOCK_MONOTONIC, &processStartingTime) != 0)
            {
                perror("clock_gettime() error");
                exit(EXIT_FAILURE);
            }
            runningProcess = Dequeue(&queue);
        }




        if (!isProcessRunning && IsEmpty(queue) && isProcessNotArrived)
        {
            isIdling = true;
        }


        iteration++;
        ualarm((int)1e5, 0);
        pause();
    }



    printf("Turnaround time: %d\n", turnaroundTime);
}



void InitProcessesFromCSV(const char* path, Process oprocs[], int* oprocsCount)
{
    FILE* file = NULL;

    if ((file = fopen(path, "r")) == NULL)
    {
        perror("fopen() error:");
        exit(EXIT_FAILURE);
    }



    /*
     * GETTING PROC INFORMATION
     */
    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, file) != NULL)
    {
        Process proc = ParseProcess(line);
        proc.original_idx = *oprocsCount;
        oprocs[*oprocsCount] = proc;
        (*oprocsCount)++;
    }
    if (ferror(file))
    {
        perror("fgets() error:");
        exit(EXIT_FAILURE);
    }
}



Process ParseProcess(const char* line)
{
    char* save_ptr = NULL;
    char* dup = strdup(line);
    char* currentValue = NULL;
    Process proc = { 0 };


    /*
     * GETTING NAME
     */
    if ((currentValue = strtok_r(dup, CSV_DELIMS, &save_ptr)) == NULL)
    {
        perror("strtok_r() error");
        exit(EXIT_FAILURE);
    }
    strcpy(proc.name, currentValue);


    /*
     * GETTING DESC
     */
    if ((currentValue = strtok_r(NULL, CSV_DELIMS, &save_ptr)) == NULL)
    {
        perror("strtok_r() error");
        exit(EXIT_FAILURE);
    }
    strcpy(proc.desc, currentValue);



    /*
     * GETTING ARRIVAL TIME
     */
    if ((currentValue = strtok_r(NULL, CSV_DELIMS, &save_ptr)) == NULL)
    {
        perror("strtok_r() error");
        exit(EXIT_FAILURE);
    }
    proc.arrival_time = atoi(currentValue);



    /*
     * GETTING BURST TIME
     */
    if ((currentValue = strtok_r(NULL, CSV_DELIMS, &save_ptr)) == NULL)
    {
        perror("strtok_r() error");
        exit(EXIT_FAILURE);
    }
    proc.burst_time = atoi(currentValue);



    /*
     * GETTING PRIORITY
     */
    if ((currentValue = strtok_r(NULL, CSV_DELIMS, &save_ptr)) == NULL)
    {
        perror("strtok_r() error");
        exit(EXIT_FAILURE);
    }
    proc.priority = atoi(currentValue);



    free(dup);



    return proc;
}

int CmpPriorityNull(Process _, Process __)
{
    return 0;
}

int ProcCmpArrivalTime(Process a, Process b)
{
    return a.arrival_time - b.arrival_time;
}

void SortProcesses(Process procs[], int procCount, int (*predicate)(Process, Process))
{
    if (predicate == NULL)
    {
        fprintf(stderr, "Argument null error in function SortProcesses, 'predicate' cannot be null\n\n");
        exit(EXIT_FAILURE);
    }

    bool didSwap;
    do
    {
        didSwap = false;
        for (int i = 0; i < procCount - 1; i++)
        {
            int cmpRes = predicate(procs[i], procs[i+1]);
            if (cmpRes > 0)
            {
                Process temp = procs[i];
                procs[i] = procs[i+1];
                procs[i+1] = temp;
                didSwap = true;
            }
        }
    } while (didSwap);
}

Process Dequeue(ReadyQueue* queue)
{
    if (IsEmpty(*queue))
    {
        fprintf(stderr, "Invalid operation error: queue is already empty\n");
        exit(EXIT_FAILURE);
    }

    Process firstProcess = queue->procs[0];

    queue->size--;
    for (int i = 0; i < queue->size; i++)
        queue->procs[i] = queue->procs[i+1];

    return firstProcess;
}

void Enqueue(ReadyQueue* queue, Process item)
{
    if (queue->size >= MAX_PROC)
    {
        fprintf(stderr, "Invalid operation error: queue is full\n");
        exit(EXIT_FAILURE);
    }
    if (queue->CmpPriority == NULL)
    {
        fprintf(stderr, "Argument null error in function Enqueue, 'queue->CmpPriority' cannot be null\n");
        exit(EXIT_FAILURE);
    }

    queue->procs[queue->size] = item;
    queue->size++;
    SortProcesses(queue->procs, queue->size, queue->CmpPriority);
}

double GetTimeElapsed(struct timespec startingTime)
{
    struct timespec currentTime;

    if (clock_gettime(CLOCK_MONOTONIC, &currentTime) != 0)
    {
        perror("clock_gettime error");
        exit(EXIT_FAILURE);
    }

    return  (currentTime.tv_sec - startingTime.tv_sec) +
            (currentTime.tv_nsec - startingTime.tv_nsec) / 1e9;
}

void EnqueueNewArrivals(ReadyQueue* queue, Process procs[], int* startingIdx, int procCount, struct timespec startingTime)
{
    /*
     * Validating that there is a process to add
     */
    if (*startingIdx >= procCount)
    {
        fprintf(stderr, "Invalid argument error: startingIdx = %d when there are only %d processes\n", *startingIdx, procCount);
        exit(EXIT_FAILURE);
    }



    /*
     * Getting scheduler uptime
     */
    double uptime = GetTimeElapsed(startingTime);



    /*
     * Adding processes to ReadyQueue
     */
    for (; *startingIdx < procCount; (*startingIdx)++)
        if (procs[*startingIdx].arrival_time < uptime)
        {
            if (queue->CmpPriority == NULL)
            {
                fprintf(stderr, "Argument null error in function EnqueueNewArrivals, 'queue->CmpPriority' cannot be null\n");
                exit(EXIT_FAILURE);
            }
            Enqueue(queue, procs[*startingIdx]);
        }
        else break;
}

bool IsEmpty(ReadyQueue queue)
{
    return queue.size == 0;
}

void SigAlarmHandler() {  }
