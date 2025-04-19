#include "hw2.h"

long long job_start_times[MAX_JOBS];
long long job_end_times[MAX_JOBS];
//int completed_jobs = 0; 

int running_flag = 1; //check if the dispatcher still working
int active_jobs = 0; 
int total_jobs = 0;// Track the number of completed jobs

//variables for statistics text
pthread_cond_t all_jobs_done;
 long long job_time;
long long start_time;
long long total_time;
long long sum_turnaround = 0;
long long min_turnaround = LLONG_MAX;
long long max_turnaround = LLONG_MIN;
double avg_turnaround;

pthread_t* threads[MAX_THREAD_SIZE]; //from num_of_threds = NULL
int num_threads;
int num_files;
FILE *files[MAX_COUNTER_SIZE]; //from num_of_files = NULL
int log_enable;
FILE *cmd;




void initialize_queue() { // queue initializer
    jobs_queue.front = 0;
    jobs_queue.rear = -1;
    jobs_queue.size = 0;
    pthread_mutex_init(&jobs_queue.lock, NULL);
    pthread_cond_init(&jobs_queue.not_empty, NULL);
    pthread_cond_init(&all_jobs_done, NULL);
    
}

void dis_msleep(int x) { // generic msleep function
    usleep(x * 1000);
}

void dis_wait() { // dispatcher wait function
    pthread_mutex_lock(&jobs_queue.lock);
    while (active_jobs > 0) { // Wait until all active jobs are done
        pthread_cond_wait(&all_jobs_done, &jobs_queue.lock);
    }
    pthread_mutex_unlock(&jobs_queue.lock);
}

void create_counter_files() { // initialize counter files
    char filename[20];
    for (int i = 0; i < num_files; i++) {
        sprintf(filename, "count%02d.txt", i);
        FILE *file_i = fopen(filename, "w");
        if (file_i != NULL) {
            fprintf(file_i, "0\n");
            fclose(file_i);
        } else {
            perror("Error creating counter file");
        }
    }
}

void update_counter_file(const char *filename, int counter_index, int value) {
    long long counter = 0;
    FILE *file = fopen(filename, "r+"); // open the counter file according to value
    if (!file) {
        perror("Error opening counter file");
        return;
    }

    if (fscanf(file, "%lld", &counter) != 1) { // read counter
        perror("Error reading counter value");
        fclose(file);
        return;
    }

    counter += value; // update value

    fseek(file, 0, SEEK_SET);
    fprintf(file, "%lld\n", counter); // write updated value to the beginning of file
    fclose(file); // close file
}

long long current_time_ms() { // Get current time in milliseconds
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void log_worker(int thread_id, const char *status, const char *job) {
    if (log_enable) {
        char filename[20];
        sprintf(filename, "thread%02d.txt", thread_id);  // Format the filename as "threadXX.txt"
        FILE *log_file = fopen(filename, "a");
        if (log_file) {
            fprintf(log_file, "TIME %lld: %s job %s\n", current_time_ms(), status, job); // Write the log entry with timestamp, status, and job information
            fclose(log_file);
        }
    }
}

void process_job(char *job) {
    int arg;
    char filename[20];
    if (strncmp(job, "worker", 6) == 0) {
        job += 7; // skip the worker prefix
    }

    
    if (strncmp(job, "msleep", 6) == 0) {
        arg = atoi(job + 7);
        dis_msleep(arg); //msleep functionality
    } else if (strncmp(job, "increment", 9) == 0) {
        arg = atoi(job + 10); 
        sprintf(filename, "count%02d.txt", arg);
        update_counter_file(filename, arg, 1); //incrememnt funtinality,update flie with +1
    } else if (strncmp(job, "decrement", 9) == 0) {
        arg = atoi(job + 10);
        sprintf(filename, "count%02d.txt", arg);
        update_counter_file(filename, arg, -1); //incrememnt funtinality,update flie with -1
    } else if (strncmp(job, "repeat", 6) == 0) {
        int repeat_count;
        char *commands_start;

        // Extract the repeat count
    sscanf(job + 7, "%d", &repeat_count);
    // Locate the start of commands after the semicolon
    commands_start = strchr(job, ';');
    if (commands_start) {
        commands_start++; // Move past the semicolon
        for (int i = 0; i < repeat_count; i++) {
            char *current_command = commands_start;
            char *next_command = NULL;
            // Process each command in the sequence using strtok-like logic
            do {
                next_command = strchr(current_command, ';');
                if (next_command) {
                    *next_command = '\0'; // Temporarily terminate the current command
                }
                current_command++;
                process_job(current_command); // Process the current command

                if (next_command) {
                    *next_command = ';'; // Restore the semicolon
                    current_command = next_command + 1; // Move to the next command
                } else {
                    current_command = NULL; // No more commands
                }
            } while (current_command);
        }
    }
}
}
    

void *worker_function(void *arg) { // this function is given to the worker thread
    int thread_id = *((int *)arg);
    free(arg);
   
    
    while (1) {
        pthread_mutex_lock(&jobs_queue.lock);

        // Wait for jobs or shutdown signal
        while (jobs_queue.size == 0 && running_flag) {
            pthread_cond_wait(&jobs_queue.not_empty, &jobs_queue.lock);
        }

        // Exit if no more jobs and running is false
        if (jobs_queue.size == 0 && !running_flag) {
            pthread_mutex_unlock(&jobs_queue.lock);
            break;
        }

        // Dequeue the job
        char *job = dequeue();
        pthread_mutex_unlock(&jobs_queue.lock);

        if (job != NULL) {
            
            start_time= current_time_ms();
            log_worker(thread_id, "START", job);
            process_job(job);
            log_worker(thread_id, "END", job);
            job_time= current_time_ms() - start_time;  // Calculate the time taken to complete the job
            // update the variables for the statistics file

       
            if(job_time> max_turnaround)
            {
                max_turnaround= job_time;
            }
            
            if(job_time<min_turnaround && job_time > 0)
            {
                min_turnaround= job_time;
            }
            
         
            sum_turnaround+=job_time;


            pthread_mutex_lock(&jobs_queue.lock);
            
            active_jobs--;
            // If there are no active jobs remaining, signal the condition variable
            if (active_jobs == 0) {
                pthread_cond_signal(&all_jobs_done);
            }
            pthread_mutex_unlock(&jobs_queue.lock);
        }
    }
    return NULL;
}

void create_threads() { // create worker threads
    for (int i = 0; i < num_threads; i++) {
        threads[i] = malloc(sizeof(pthread_t));
        if (threads[i] == NULL) {
            perror("Malloc Error");
        }
        int *thread_id = malloc(sizeof(int));
        if (thread_id == NULL) {
            perror("Malloc Error");
        }
        *thread_id = i;
        int stat = pthread_create(threads[i], NULL, worker_function, (void *)thread_id);
        if (stat != 0) {
            perror("Thread creation error");
        }
    }
}

void enqueue(char *job) { // enter worker job to queue and update pointers
    pthread_mutex_lock(&jobs_queue.lock);
    if (jobs_queue.size < MAX_QUEUE_SIZE) {
        jobs_queue.rear = (jobs_queue.rear + 1) % MAX_QUEUE_SIZE; // Fixed circular queue logic
        strcpy(jobs_queue.jobs[jobs_queue.rear], job);
        jobs_queue.jobs[jobs_queue.rear][MAX_LINE_SIZE - 1] = '\0';
        jobs_queue.size++;
        pthread_cond_broadcast(&jobs_queue.not_empty); // Signal all worker threads that the jobs queue is not empty
    }
    pthread_mutex_unlock(&jobs_queue.lock);
}

char *dequeue() { // remove worker job from queue and update pointers
    char *job = jobs_queue.jobs[jobs_queue.front];
    jobs_queue.front = (jobs_queue.front + 1) % MAX_QUEUE_SIZE; // Fixed circular queue logic
    jobs_queue.size--;
    return job;
}

void dispatcher() { // handle dispatcher functions
    char line[MAX_LINE_SIZE];
    FILE* dis_file;
    dis_file = fopen("dispatcher.txt", "w");
    while (fgets(line, MAX_LINE_SIZE, cmd)) {
        long long curr_time = current_time_ms();
        fprintf(dis_file, "TIME %lld: read cmd line: %s\n", curr_time, line);
        if (strncmp(line, "dispatcher_msleep", 17) == 0) {
            int x;
            sscanf(line, "dispatcher_msleep %d", &x);
            dis_msleep(x);
        } else if (strncmp(line, "dispatcher_wait", 15) == 0) {
            dis_wait();
        } else if (strncmp(line, "worker", 6) == 0) {
            enqueue(line);
            active_jobs++;
            total_jobs++;
        }
    }
    pthread_mutex_lock(&jobs_queue.lock);
    running_flag = 0;
    pthread_cond_broadcast(&jobs_queue.not_empty); // Wake up all waiting worker threads
    pthread_mutex_unlock(&jobs_queue.lock);

    // Wait for all worker threads to finish execution
    for (int i = 0; i < num_threads; i++) {
        pthread_join(*threads[i], NULL);
    }

    fclose(dis_file);
}

void print_statistics() { // Print statistics at the end
    
    long long total_time = current_time_ms();

    avg_turnaround = (double)sum_turnaround / total_jobs;

    FILE *stats = fopen("stats.txt", "w");
    if (stats) {
        fprintf(stats, "total running time: %lld milliseconds\n", total_time);
        fprintf(stats, "sum of jobs turnaround time: %lld milliseconds\n", sum_turnaround);
        fprintf(stats, "min job turnaround time: %lld milliseconds\n", min_turnaround);
        fprintf(stats, "average job turnaround time: %.2f milliseconds\n", avg_turnaround);
        fprintf(stats, "max job turnaround time: %lld milliseconds\n", max_turnaround);
        fclose(stats);
    } else {
        perror("Error opening stats file");
    }
}

void free_threads() { // Free thread pointers
    for (int i = 0; i < num_threads; i++) {
        free(threads[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        exit(1);
    }
    cmd = fopen(argv[1], "r");
    if (!cmd) {
        perror("Error opening command file");
        exit(EXIT_FAILURE);
    }

    num_threads = atoi(argv[2]);
    num_files = atoi(argv[3]);
    log_enable = atoi(argv[4]);

    create_counter_files();
    initialize_queue();
    create_threads();
    dispatcher(); // Start the dispatcher
    
    print_statistics(); // Print statistics at the end
    free_threads(); // Free thread pointers
    return 0;
}
