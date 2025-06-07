#include <math.h>
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

#define PROC_LOG "%d → %d: %s Running %s.\n"
#define IDLE_LOG "%d → %d: Idle.\n"
#define ALGORITHM_FCFS "FCFS"
#define ALGORITHM_SJF "SJF"
#define ALGORITHM_PRIORITY "Priority"
#define ALGORITHM_RR "Round Robin"
#define SCHEDULER_INTRO \
"══════════════════════════════════════════════\n" \
">> Scheduler Mode : %s\n" \
">> Engine Status  : Initialized\n" \
"──────────────────────────────────────────────\n\n"
#define SCHEDULER_OUTRO_TOTAL_WAIT \
"\n──────────────────────────────────────────────\n" \
">> Engine Status  : Completed\n" \
">> Summary        :\n" \
"   └─ Average Waiting Time : %.2f time units\n" \
">> End of Report\n" \
"══════════════════════════════════════════════\n\n"

#define SCHEDULER_OUTRO_TURNAROUND \
"\n──────────────────────────────────────────────\n" \
">> Engine Status  : Completed\n" \
">> Summary        :\n" \
"   └─ Total Turnaround Time : %d time units\n\n" \
">> End of Report\n" \
"══════════════════════════════════════════════\n\n"


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


typedef struct
{
    int (*CmpPriority)(Process, Process);
    bool shouldPrintTurnaround;
    bool shouldPrintTotalWait;
    char* name;
    int maxUptime;
} AlgorithmData;


int CmpPriorityNull(Process _, Process __);
int CmpLowerPriority(Process a, Process b);
int CmpShortestBurst(Process a, Process b);
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
void RunAlgorithm(AlgorithmData algorithm, Process procs[], int procsCount);



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
     * FCFS alg
     */
    AlgorithmData fcfs;
    fcfs.CmpPriority = CmpPriorityNull;
    fcfs.shouldPrintTotalWait = true;
    fcfs.shouldPrintTurnaround = false;
    fcfs.name = ALGORITHM_FCFS;
    fcfs.maxUptime = -1;
    RunAlgorithm(fcfs, procs, procsCount);



    /*
     * SJF alg
     */
    AlgorithmData sjf;
    sjf.CmpPriority = CmpShortestBurst;
    sjf.shouldPrintTotalWait = true;
    sjf.shouldPrintTurnaround = false;
    sjf.name = ALGORITHM_SJF;
    sjf.maxUptime = -1;
    RunAlgorithm(sjf, procs, procsCount);



    /*
     * Priority alg
     */
    AlgorithmData priorityAlg;
    priorityAlg.CmpPriority = CmpLowerPriority;
    priorityAlg.shouldPrintTotalWait = true;
    priorityAlg.shouldPrintTurnaround = false;
    priorityAlg.name = ALGORITHM_PRIORITY;
    priorityAlg.maxUptime = -1;
    RunAlgorithm(priorityAlg, procs, procsCount);



    /*
     * RR alg
     */
    AlgorithmData roundRobinAlg;
    roundRobinAlg.CmpPriority = CmpPriorityNull;
    roundRobinAlg.shouldPrintTotalWait = false;
    roundRobinAlg.shouldPrintTurnaround = true;
    roundRobinAlg.name = ALGORITHM_RR;
    roundRobinAlg.maxUptime = timeQuantum;
    RunAlgorithm(roundRobinAlg, procs, procsCount);
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

int CmpShortestBurst(Process a, Process b)
{
    return a.burst_time - b.burst_time;
}

int CmpLowerPriority(Process a, Process b)
{
    return a.priority - b.priority;
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

    return  (double) (currentTime.tv_sec - startingTime.tv_sec) +
            (double) (currentTime.tv_nsec - startingTime.tv_nsec) / 1e9;
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
    int uptime = (int)GetTimeElapsed(startingTime);



    /*
     * Adding processes to ReadyQueue
     */
    for (; *startingIdx < procCount; (*startingIdx)++)
        if (procs[*startingIdx].arrival_time <= uptime)
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


void RunAlgorithm(AlgorithmData algorithm, Process procs[], int procsCount)
{
    /*
     * Initialise Ready Queue (FCFS)
     */
    ReadyQueue queue;
    queue.size = 0;
    queue.CmpPriority = algorithm.CmpPriority;
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
     * Print introduction
     */
    printf(SCHEDULER_INTRO, algorithm.name);



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
    int totalWaitingTime = 0;
    int iteration = 0;
    int idleTimeStart = -1;

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
            int processUptime = (int)GetTimeElapsed(processStartingTime);
            int schedulerUptime = (int)GetTimeElapsed(startingTime);

            if (processUptime >= runningProcess.burst_time)
            {
                /*
                 * Adding to totalWaitingTime
                 */
                totalWaitingTime += schedulerUptime - runningProcess.burst_time - runningProcess.arrival_time;
                isProcessRunning = false;



                /*
                 * Printing process log
                 */
                printf(PROC_LOG, schedulerUptime - runningProcess.burst_time, schedulerUptime, runningProcess.name, runningProcess.desc);



                /*
                 * Checks if we've finished with all the processes
                 */
                if (!isProcessNotArrived && IsEmpty(queue))
                {
                    turnaroundTime = (int)GetTimeElapsed(startingTime);
                    break;
                }
            }
            else if (algorithm.maxUptime != -1)
            {
                if (processUptime >= algorithm.maxUptime)
                {
                    /*
                     * Adding to totalWaitingTime
                     */
                    totalWaitingTime += schedulerUptime - algorithm.maxUptime - runningProcess.arrival_time;
                    isProcessRunning = false;



                    /*
                     * Printing process log
                     */
                    printf(PROC_LOG, schedulerUptime - algorithm.maxUptime, schedulerUptime, runningProcess.name, runningProcess.desc);



                    /*
                     * Process did not finish entire burst. Adjusting it and re-adding to queue
                     */
                    Process modifiedProcess = runningProcess;
                    modifiedProcess.arrival_time = schedulerUptime;
                    modifiedProcess.burst_time -= algorithm.maxUptime;
                    Enqueue(&queue, modifiedProcess);
                }
            }
        }


        if (!isProcessRunning && !IsEmpty(queue))
        {
            if (isIdling)
            {
                /*
                 * Printing idle log
                 */
                int schedulerUptime = (int)GetTimeElapsed(startingTime);
                printf(IDLE_LOG, idleTimeStart, schedulerUptime);
                isIdling = false;
                idleTimeStart = -1;
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
            idleTimeStart = (int)GetTimeElapsed(startingTime);
        }


        iteration++;
        ualarm((int)1e5, 0);
        pause();
    }



    if (algorithm.shouldPrintTotalWait)
        printf(SCHEDULER_OUTRO_TOTAL_WAIT, (double)totalWaitingTime / procsCount);
    if (algorithm.shouldPrintTurnaround)
        printf(SCHEDULER_OUTRO_TURNAROUND, turnaroundTime);



    /*
     * Restoring the sigalarm handler
     */
    struct sigaction act_restore;
    act_restore.sa_handler = SIG_DFL;
    act_restore.sa_flags = 0;
    sigemptyset(&act_restore.sa_mask);
    sigaction(SIGALRM, &act_restore, NULL);
}
