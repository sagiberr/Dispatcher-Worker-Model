#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>

#define MAX_COUNTER_SIZE 100
#define MAX_THREAD_SIZE 4096
#define MAX_LINE_SIZE 1024
#define CLOCK_REALTIME 0
#define MAX_QUEUE_SIZE 4096
#define MAX_JOBS 4096 // Adjust this to the expected maximum number of jobs


typedef struct
{
    char jobs[MAX_THREAD_SIZE][MAX_LINE_SIZE];; 
    int front, rear, size;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    
} Jobs;
Jobs jobs_queue;


// Function declarations
void initialize_queue();
void dis_msleep(int x);
void dis_wait();
void create_counter_files();
void update_counter_file(const char *filename, int counter_index, int value);
long long current_time_ms();
void log_worker(int thread_id, const char *status, const char *job);
void process_job(char *job);
void *worker_function(void *arg);
void create_threads();
void enqueue(char *job);
char *dequeue();
void dispatcher();
void print_statistics();
void free_threads();




