#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h> 

#define N 100000 // length of massive
#define K 1000   // numbers of massive

int arrays[K][N]; 
int result[N];  
pthread_mutex_t mutex;

typedef struct {
    int start;
    int end;
} thread_args;

void* sum_arrays(void* arg) {
    thread_args* args = (thread_args*) arg;
    int start = args->start;
    int end = args->end;

    for (int i = start; i < end; i++) {
        int sum = 0;
        for (int j = 0; j < K; j++) {
            sum += arrays[j][i];
        }

        pthread_mutex_lock(&mutex); 
        result[i] = sum;
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_threads>\n", argv[0]);
        return 1;
    }

    int max_threads = atoi(argv[1]); 
    if (max_threads <= 0) {
        printf("Invalid number of threads. It should be greater than 0.\n");
        return 1;
    }

    pthread_t threads[max_threads];
    pthread_mutex_init(&mutex, NULL);

    // initialize massive random numbers
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < N; j++) {
            arrays[i][j] = rand() % 10;
        }
    }

    // **PRINT ARRAYS
    //printf("Initial arrays:\n");
    //for (int i = 0; i < K; i++) {
    //    printf("Array %d: ", i);
    //    for (int j = 0; j < N; j++) {
    //        printf("%d ", arrays[i][j]);
    //    }
    //    printf("\n");
    //}
    //printf("\n");

    struct timeval start, end;
    gettimeofday(&start, NULL);

    int portion = N / max_threads;
    thread_args args[max_threads];

    // creat threads
    for (int i = 0; i < max_threads; i++) {
        args[i].start = i * portion;
        args[i].end = (i == max_threads - 1) ? N : (i + 1) * portion;

        pthread_create(&threads[i], NULL, sum_arrays, &args[i]);
    }

    // join threads
    for (int i = 0; i < max_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);

    pthread_mutex_destroy(&mutex);

    //double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("Resulting array:\n");
    for (int i = 0; i < N; i++) {
        printf("%d ", result[i]);
    }

    //printf("\n");
    
    //printf("Elapsed time: %.6f seconds\n", elapsed_time); 

    return 0;
}