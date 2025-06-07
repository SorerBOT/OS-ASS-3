#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

void InitProcessesFromCSV(const char* path, Process oprocs[], int* oprocsCount);
Process ParseProcess(const char* line);

void HandleCPUScheduler(const char* processesCsvFilePath, int timeQuantum)
{
    int procsCount = 0;
    Process procs[MAX_PROC];


    /*
     * GET PROCS FROM FILE
     */
    InitProcessesFromCSV(processesCsvFilePath, procs, &procsCount);



    /*
     * 
     */

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
        oprocs[*oprocsCount] = ParseProcess(line);
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
