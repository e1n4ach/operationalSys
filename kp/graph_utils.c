#include "dag.h"
#include <string.h>
#include <stdio.h>

int find_job_index(const Config *config, const char *name) {
    for(int i=0;i<config->job_count;i++) {
        if(strcmp(config->jobs[i].name,name)==0) return i;
    }
    return -1;
}

int find_barrier_index(const Config *config, const char *name) {
    for(int i=0;i<config->barrier_count;i++) {
        if(strcmp(config->barriers[i].name,name)==0) return i;
    }
    return -1;
}

int topological_sort(const Config *config, int *sorted_indices) {
    int indeg[MAX_JOBS];
    for(int i=0;i<config->job_count;i++) indeg[i]=config->jobs[i].dep_count;

    int queue[MAX_JOBS];
    int front=0,back=0;
    for(int i=0;i<config->job_count;i++)
        if(indeg[i]==0) queue[back++]=i;

    int count=0;
    while(front<back) {
        int u=queue[front++];
        sorted_indices[count++]=u;
        for(int j=0;j<config->job_count;j++) {
            if(j==u) continue;
            for(int d=0;d<config->jobs[j].dep_count;d++) {
                if(strcmp(config->jobs[j].deps[d],config->jobs[u].name)==0) {
                    indeg[j]--;
                    if(indeg[j]==0) {
                        queue[back++]=j;
                    }
                }
            }
        }
    }
    if(count<config->job_count) {
        fprintf(stderr,"Error: DAG has a cycle.\n");
        return -1;
    }
    return 0;
}
