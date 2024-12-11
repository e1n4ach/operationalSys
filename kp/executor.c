#include "dag.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <unistd.h>

int find_job_index(const Config *config, const char *name);
int find_barrier_index(const Config *config, const char *name);
int topological_sort(const Config *config, int *sorted_indices);

int execute_dag(const Config *config);

typedef struct {
    const Config *config;
    int *sorted;
    int done[MAX_JOBS];
    int remainingDeps[MAX_JOBS];

    int barrier_remaining[MAX_BARRIERS];
    int barrier_passed[MAX_BARRIERS];

    int ready_queue[MAX_JOBS];
    int rq_front, rq_back;

    pthread_mutex_t m;
    pthread_cond_t cv;

    int runningJobs;
    int maxParallel;
    atomic_bool stop_all;
    int total_jobs;

    int job_barrier_deps[MAX_JOBS][MAX_BARRIERS];
} ExecutorState;

static int compute_barrier_deps(const Config *config, int *sorted, int job_barrier_deps[MAX_JOBS][MAX_BARRIERS]) {
    for(int i=0;i<config->job_count;i++) {
        for(int b=0;b<config->barrier_count;b++) {
            job_barrier_deps[i][b] = 0;
        }
    }

    for(int i=0;i<config->job_count;i++) {
        int u = sorted[i];
        for(int b=0;b<config->barrier_count;b++) {
            if(strcmp(config->jobs[u].barrier, config->barriers[b].name) != 0) {
                for(int d=0; d < config->jobs[u].dep_count; d++) {
                    int di = find_job_index(config, config->jobs[u].deps[d]);
                    if(di < 0) {
                        fprintf(stderr,"Error: dependency '%s' for job '%s' not found.\n",
                            config->jobs[u].deps[d], config->jobs[u].name);
                        return -1;
                    }
                    if(strcmp(config->jobs[di].barrier, config->barriers[b].name) == 0) {
                        job_barrier_deps[u][b] = 1;
                    }
                }
            }
        }
    }
    return 0;
}

static int all_barriers_passed_for_job(ExecutorState *st, int jobi) {
    if(jobi<0 || jobi>=st->config->job_count) {
        fprintf(stderr,"all_barriers_passed_for_job: invalid job index %d\n", jobi);
        return 0;
    }
    for(int b=0;b<st->config->barrier_count;b++) {
        if(st->job_barrier_deps[jobi][b] && !st->barrier_passed[b]) {
            return 0;
        }
    }
    return 1;
}

static void enqueue_job(ExecutorState *st,int jobi) {
    if(st->rq_back>=MAX_JOBS) {
        fprintf(stderr,"enqueue_job: too many jobs in queue\n");
        return;
    }
    st->ready_queue[st->rq_back++] = jobi;
}

static int dequeue_job(ExecutorState *st,int *jobi) {
    if(st->rq_front<st->rq_back) {
        *jobi=st->ready_queue[st->rq_front++];
        return 1;
    }
    return 0;
}

static void job_finished(ExecutorState *st, int jobi, int success);

static int run_job(ExecutorState *st,int jobi) {
    if(jobi<0 || jobi>=st->config->job_count) {
        fprintf(stderr,"run_job: invalid job index %d\n", jobi);
        return 0;
    }
    printf("Running job: %s\n", st->config->jobs[jobi].name);
    usleep(200000);
    return 1;
    //ПРОВЕРКА НА ЗАВЕРШЕНИЕ ПРИ ОШИБКЕ В ДЖОБЕ
    //static int run_count = 0;
    //run_count++;
    //if (strcmp(st->config->jobs[jobi].name, "job2") == 0 && run_count == 1) {
    //    fprintf(stderr, "Simulating failure in job2\n");
    //    return 0; // Возвращаем неуспех.
    //}

}

static void barrier_passed(ExecutorState *st,int barrier_i) {
    if(barrier_i<0 || barrier_i>=st->config->barrier_count) {
        fprintf(stderr,"barrier_passed: invalid barrier index %d\n", barrier_i);
        return;
    }
    for(int i=0;i<st->config->job_count;i++) {
        if(!st->done[i] && st->remainingDeps[i]==0 && all_barriers_passed_for_job(st,i)) {
            enqueue_job(st,i);
        }
    }
}

static void job_finished(ExecutorState *st, int jobi, int success) {
    pthread_mutex_lock(&st->m);
    st->runningJobs--;
    if(!success) {
        atomic_store(&st->stop_all,1);
        pthread_cond_broadcast(&st->cv);
        pthread_mutex_unlock(&st->m);
        return;
    }

    st->done[jobi]=1;
    char *bname = st->config->jobs[jobi].barrier;
    if(strlen(bname)>0) {
        int bi = find_barrier_index(st->config,bname);
        if(bi>=0) {
            st->barrier_remaining[bi]--;
            if(st->barrier_remaining[bi]==0) {
                st->barrier_passed[bi]=1;
                barrier_passed(st,bi);
            }
        }
    }

    for(int j=0;j<st->config->job_count;j++) {
        if(!st->done[j]) {
            for(int d=0;d<st->config->jobs[j].dep_count;d++) {
                if(strcmp(st->config->jobs[j].deps[d], st->config->jobs[jobi].name)==0) {
                    st->remainingDeps[j]--;
                    if(st->remainingDeps[j]==0 && all_barriers_passed_for_job(st,j)) {
                        enqueue_job(st,j);
                    }
                }
            }
        }
    }

    int completed=0;
    for(int i=0;i<st->config->job_count;i++) {
        if(st->done[i]) completed++;
    }
    if(completed==st->config->job_count) {
        atomic_store(&st->stop_all,1);
    }

    pthread_cond_broadcast(&st->cv);
    pthread_mutex_unlock(&st->m);
}

static void *worker_thread(void *arg) {
    ExecutorState *st=(ExecutorState*)arg;
    while(!atomic_load(&st->stop_all)) {
        int jobi;
        pthread_mutex_lock(&st->m);
        while(!atomic_load(&st->stop_all) && ((st->rq_front==st->rq_back)||(st->runningJobs>=st->maxParallel))) {
            pthread_cond_wait(&st->cv,&st->m);
        }
        if(atomic_load(&st->stop_all)) {
            pthread_mutex_unlock(&st->m);
            break;
        }
        if(!dequeue_job(st,&jobi)) {
            pthread_mutex_unlock(&st->m);
            continue;
        }
        st->runningJobs++;
        pthread_mutex_unlock(&st->m);

        int success = run_job(st,jobi);
        job_finished(st,jobi,success);
    }
    return NULL;
}

int execute_dag(const Config *config) {
    if(config->job_count<=0) {
        fprintf(stderr,"No jobs to execute.\n");
        return -1;
    }
    int *sorted = malloc(sizeof(int)*config->job_count);
    if(!sorted) {
        fprintf(stderr,"Memory allocation failed for sorted.\n");
        return -1;
    }
    if(topological_sort(config,sorted)!=0) {
        free(sorted);
        return -1;
    }

    printf("Topological order of jobs:\n");
    for(int i=0;i<config->job_count;i++) {
        printf("%d: %s\n", i, config->jobs[sorted[i]].name);
    }

    ExecutorState st;
    memset(&st,0,sizeof(st));
    st.config=config;
    st.sorted=sorted;
    st.total_jobs=config->job_count;
    st.maxParallel=config->max_parallel_jobs;

    for(int i=0;i<config->job_count;i++) {
        st.done[i]=0;
        st.remainingDeps[i]=config->jobs[i].dep_count;
    }
    for(int b=0;b<config->barrier_count;b++) {
        st.barrier_remaining[b]=config->barriers[b].remaining;
        st.barrier_passed[b]=0;
    }

    pthread_mutex_init(&st.m,NULL);
    pthread_cond_init(&st.cv,NULL);

    if(compute_barrier_deps(config,sorted,st.job_barrier_deps)<0) {
        fprintf(stderr,"Error computing barrier dependencies\n");
        free(sorted);
        pthread_mutex_destroy(&st.m);
        pthread_cond_destroy(&st.cv);
        return -1;
    }

    for(int i=0;i<config->job_count;i++) {
        if(st.remainingDeps[i]==0 && all_barriers_passed_for_job(&st,i)) {
            enqueue_job(&st,i);
        }
    }

    int num_threads = (st.maxParallel>0 ? st.maxParallel:1);
    pthread_t *threads=malloc(sizeof(pthread_t)*num_threads);
    if(!threads) {
        fprintf(stderr,"Memory allocation failed for threads.\n");
        free(sorted);
        pthread_mutex_destroy(&st.m);
        pthread_cond_destroy(&st.cv);
        return -1;
    }
    atomic_store(&st.stop_all,0);
    for(int i=0;i<num_threads;i++) {
        if(pthread_create(&threads[i],NULL,worker_thread,&st)!=0) {
            fprintf(stderr,"Failed to create thread\n");
            atomic_store(&st.stop_all,1);
            pthread_cond_broadcast(&st.cv);
            for(int k=0;k<i;k++) pthread_join(threads[k],NULL);
            free(threads);
            free(sorted);
            pthread_mutex_destroy(&st.m);
            pthread_cond_destroy(&st.cv);
            return -1;
        }
    }

    pthread_mutex_lock(&st.m);
    while(!atomic_load(&st.stop_all)) {
        pthread_cond_wait(&st.cv,&st.m);
    }
    pthread_mutex_unlock(&st.m);

    pthread_cond_broadcast(&st.cv);

    for(int i=0;i<num_threads;i++) {
        pthread_join(threads[i],NULL);
    }

    int all_done=1;
    for(int i=0;i<config->job_count;i++) {
        if(!st.done[i]) {all_done=0;break;}
    }

    pthread_mutex_destroy(&st.m);
    pthread_cond_destroy(&st.cv);
    free(threads);
    free(sorted);

    if(!all_done) {
        fprintf(stderr,"DAG execution stopped due to an error.\n");
        return -1;
    } else {
        printf("DAG execution completed successfully.\n");
        return 0;
    }
}
