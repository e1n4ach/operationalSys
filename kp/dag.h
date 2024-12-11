#ifndef DAG_H
#define DAG_H

#include <stddef.h>

#define MAX_NAME_LEN 128
#define MAX_JOBS 100
#define MAX_BARRIERS 100
#define MAX_DEPS 50

typedef struct {
    char name[MAX_NAME_LEN];
    int dep_count;
    char deps[MAX_DEPS][MAX_NAME_LEN];
    char barrier[MAX_NAME_LEN]; 
} Job;

typedef struct {
    char name[MAX_NAME_LEN];
    int job_count;
    char jobs[MAX_DEPS][MAX_NAME_LEN];
    int remaining;
    int passed; 
} BarrierInfo;

typedef struct {
    int max_parallel_jobs;
    int job_count;
    Job jobs[MAX_JOBS];
    int barrier_count;
    BarrierInfo barriers[MAX_BARRIERS];
} Config;

#endif
