#include <unistd.h>  // getopt
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <cstdlib>
#include <pthread.h>
#include <vector> 
#include <chrono>
using namespace std;
using namespace chrono;

/* Fair scheduing policies: SCHED_NORMAL (CFS, SCHED_OTHER in POSIX), SCHED_BATCH */
#define SCHED_NORMAL SCHED_OTHER		

pthread_barrier_t barrier;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

double time_wait = 0;

/* struct for collecting required thread information. */
typedef struct {
    pthread_t thread_id;
    int thread_num;
    int sched_policy;
    int sched_priority;
} thread_info_t;

const char *usage_msg = 
        "Usage:\n"
        "\t./sched_demo -n <num_thread> -t <time_wait> -s <policies> -p <priorities>\n"
        "Options:\n"
        "\t-n <num_threads>  Number of threads to run simultaneously\n"
        "\t-t <time_wait>    Duration of \"busy\" period\n"
        "\t-s <policies>     Scheduling policy for each thread,\n"
        "\t                  currently only NORMAL(SCHED_NORMAL) and FIFO(SCHED_FIFO)\n"
        "\t                  scheduling policies are supported.\n"
        "\t-p <priorities>   Real-time thread priority for real-time threads\n"
        "Example:\n"
        "\t./sched_demo -n 4 -t 0.5 -s NORMAL,FIFO,NORMAL,FIFO -p -1,10,-1,30\n";

void PrintVec(vector<int>& vec){
    printf("vector print: ");
    for (int i= 0; i < vec.size(); i++){
        printf("%d ", vec[i]);
    }
    printf("\n");
}

void CheckNumOfThread(int threadNum, int value , const char* msg){
    if (threadNum != value){ 
        printf("The number of %s did not match the number of thread\n",msg);
        exit(0);
    }
}

/* Worker Thread */
void *thread_func(void *arg)
{
    /* 1. Wait until all threads are ready */
    pthread_barrier_wait(&barrier);
    
    /* 2. Do the task */ 
    for (int i = 0; i < 3; i++) {
        pthread_mutex_lock(&mutex);
        printf("Thread %d is running\n", ((thread_info_t *)arg)->thread_num);

        auto start_time = high_resolution_clock::now();
        while (duration_cast<seconds>(high_resolution_clock::now() - start_time).count() 
            < time_wait ) {
            /* Busy for <time_wait> seconds */
        }
        pthread_mutex_unlock(&mutex);

    }
    /* 3. Exit the function  */
    pthread_exit(NULL);
}

/* Main Thread */
int main(int argc, char **argv) {
    // e.g. ./sched_demo -n 4 -t 0.5 -s NORMAL,FIFO,NORMAL,FIFO -p -1,10,-1,30
    int cmd_arg = 0;
    int num_threads = 0;
    vector<int> policies;
    vector<int> priorities;

    /* 1. Parse program arguments */
    while( (cmd_arg = getopt(argc, argv, "n:t:s:p:")) != -1 ){
        switch(cmd_arg){
            case 'n':
                num_threads = atoi(optarg);
                break;
            case 't':
                time_wait = atof(optarg);
                break;
            case 's':{
                    char* token = strtok(optarg, ",");
                    while ( token != NULL ){
                        policies.push_back( (strcmp(token, "NORMAL") == 0) ? SCHED_NORMAL : SCHED_FIFO );
                        token = strtok(NULL, ",");
                    }
                    CheckNumOfThread(num_threads, policies.size(), "policies");
                }
                break;
            case 'p':{
                    char* token = strtok(optarg, ",");
                    while ( token != NULL ){
                        priorities.push_back( atoi(token) );
                        token = strtok(NULL, ",");
                    }
                    CheckNumOfThread(num_threads, priorities.size(), "priorities");
                }
                break;
            
            /* Error handle: Mainly missing arg or illegal option */
            case '?':
                printf("Unknown option: %c\n",(char)optopt);
                break;
        }
    }
    if (num_threads == 0){
        printf("%s",usage_msg);
        exit(0);
    }

    /* 2. Create <num_threads> worker threads */
    thread_info_t thread_info[num_threads];
    pthread_barrier_init(&barrier, NULL, num_threads);
    for (int i = 0; i < num_threads; i++) {
        // thread_id is assigned by system
        thread_info[i].thread_num = i;
        thread_info[i].sched_policy = policies[i];
        if( thread_info[i].sched_policy ==  SCHED_NORMAL)
            thread_info[i].sched_priority = 0;
        else
            thread_info[i].sched_priority = priorities[i];
    }

    /* 3. Set CPU affinity */
    int cpu_id = 3; // set thread to the same CPU core(3)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    
    pthread_attr_t attr;
    sched_param param;
    for (int i = 0; i < num_threads; i++) {
        /* 4. Set the attributes to each thread */
        pthread_attr_init(&attr); // thread's attriburte init

     	if( pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0){
     		perror("setinherit failed\n");
     		return -1;
      	}
        
     	if(pthread_attr_setschedpolicy(&attr, thread_info[i].sched_policy) != 0){
     		perror("setpolicy failed\n");
     		return -1;
      	}
        
        param.sched_priority = thread_info[i].sched_priority;
        
        if(pthread_attr_setschedparam(&attr, &param) != 0 ){
     		printf("setparam[%d] failed\n", param.sched_priority);
            return -1;
        }

        if( pthread_create(&thread_info[i].thread_id, &attr, thread_func, (void *)&thread_info[i])!= 0 ){
            printf("create thread[%d] failed\n", thread_info[i].thread_num);
            return -1;
        }
        pthread_setaffinity_np(thread_info[i].thread_id, sizeof(cpuset), &cpuset);

    }

    /* 5. Start all threads at once */

    /* 6. Wait for all threads to finish  */ 
    for (int i = 0; i < num_threads; i++) {
        pthread_join(thread_info[i].thread_id, NULL);
    }
    pthread_exit(NULL); 

}

//g++ sched_demo_311581039.cpp -o sched_demo_311581039 -pthread