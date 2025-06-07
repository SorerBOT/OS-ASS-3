#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

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
void InitProcessesFromCSV(const char* path, Process oprocs[], int* oprocsCount);
Process ParseProcess(const char* line);
void SortProcesses(Process procs[], int procCount, int (*predicate)(Process, Process));
int ProcCmpArrivalTime(Process a, Process b);

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
    ReadyQueue queue =
    {
        .CmpPriority = CmpPriorityNull,
        .size = 0,
    };



    /*
     * Start timer
     */
    struct timespec startingTime, currentTime;
    clock_gettime(CLOCK_MONOTONIC, &startingTime);

    sleep(5);

    clock_gettime(CLOCK_MONOTONIC, &currentTime);
    double elapsed = (currentTime.tv_sec - startingTime.tv_sec) +
        (currentTime.tv_nsec - startingTime.tv_nsec) / 1e9;
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
    Process firstProcess = queue->procs[0];

    queue->size--;
    for (int i = 0; i < queue->size; i++)
        queue[i] = queue[i+1];

    return firstProcess;
}

void Enqueue(ReadyQueue* queue, Process item)
{
    queue->procs[queue->size] = item;
    queue->size++;
    SortProcesses(queue->procs, queue->size, queue->CmpPriority);
}
