[Assignment 2: Scheduling Policy Demonstration Program](https://hackmd.io/@shihh/os-assignment-2)
### Describe how you implemented the program in detail.
#### 1. Parse program arguments
```cpp
/* Main Thread */
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
```
- `getopt(int argc, char *const argv[], const char *optstring)` 
    - `optstring` 中的每個字元表示一個參數選項 `-n` `-t` 等等，`:`表示後面需要帶一個參數
    - 每個選項的參數會存於 `extern char *optarg`
    - 若parse 有誤會跳到 `case '?':`
- `strtok(char *str, const *delim)` 將字串切割後待 set thread attribute
- `CheckNumOfThread(int threadNum, int value , const char* msg)` 驗證thread數量是否跟其他參數數量符合
#### 2. Create <num_threads> worker threads 
```cpp
/* Main Thread */
thread_info_t thread_info[num_threads];
pthread_barrier_init(&barrier, NULL, num_threads);
for (int i = 0; i < num_threads; i++) {
    thread_info[i].thread_num = i;
    thread_info[i].sched_policy = policies[i];
    if( thread_info[i].sched_policy ==  SCHED_NORMAL)
        thread_info[i].sched_priority = 0;
    else
        thread_info[i].sched_priority = priorities[i];
}
```
- 把剛剛parse的參數assign給threads
- `SCHED_NORMAL` 的 priority defalut 為 0
- 初始化barrir，等待 create `<num_threads>` threads 才一起放行
    - `pthread_barrier_init(&barrier, NULL, num_threads)`

#### 3. Set CPU affinity 
```cpp
/* Main Thread */
int cpu_id = 3; // set threads to the same CPU core(3)
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(cpu_id, &cpuset);
pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
```
- 使thread跑在指定CPU core上
    - `pthread_setaffinity_np(pthread_t id, size_t cpusetsize, cpu_set_t cpuset)` 

#### 4. Set the attributes to each thread
```cpp
/* Main Thread */
pthread_attr_t attr;
sched_param param;

for (int i = 0; i < num_threads; i++) {
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
```
- 設定 thread attribution 是否繼承 parent thread 的 schedule_policy
    - `int pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)`
    - 不繼承(`PTHREAD_EXPLICIT_SCHED`) 才可設定 thread policy
- 設定 thread attribution 的 schedule policy
    - `pthread_attr_setschedpolicy(&attr, <schedule_policy>)`
        - `SCHED_RR`、`SCHED_FIFO`、`SCHED_OTHER`(`SCHED_NORMAL`)
- 設定 thread attribution 的 優先值
    - `param.sched_priority = <priority_value>`
        - `SCHED_NORMAL`: 0
        - `SCHED_FIFO`: 1~99
    - `pthread_attr_setschedparam(&attr, &param)`
- 用 attribution 建立 thread 
    - `pthread_create(pthread_t *thread_id, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg)`
- 設定每個 child threads 跑在指定的 CPU core 上

#### 5. Start all threads at once
```cpp
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
```
- 等待 threads 達到指定數量後一起執行
    - `pthread_barrier_wait(&barrier)`
- 使用 `mutex` 避免多個 thread 混著執行造成每次結果順序不同
    - 若其他的 thread 同時也要執行該處的程式碼時，就必須等待先前的 thread 執行完之後，才能接著進入
    - 加入mutex `pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER`
    - `pthread_mutex_lock(&mutex)`、`pthread_mutex_unlock(&mutex)`
- Busy for <time_wait> seconds
    - 用 `while-loop` 和 時間函式庫`chrono` 設定loop直到 `time_wait` 
    - `duration_cast<seconds>(high_resolution_clock::now() - start_time).count()`
#### 6. Wait for all threads to finish
```cpp
/* Main Thread */
for (int i = 0; i < num_threads; i++) {
    pthread_join(thread_info[i].thread_id, NULL);
}
pthread_exit(NULL); 
```
- 等待所有 child threads 完成工作 `pthread_join`
- 確認所有 thread 完成 `pthread_exit(NULL)`

### Describe the results of ./sched_demo -n 3 -t 1.0 -s NORMAL,FIFO,FIFO -p -1,10,30 and what causes that. 
    $ sudo ./sched_demo -n 3 -t 1.0 -s NORMAL,FIFO,FIFO -p -1,10,30
    Thread 2 is running
    Thread 2 is running
    Thread 2 is running
    Thread 1 is running
    Thread 1 is running
    Thread 1 is running
    Thread 0 is running
    Thread 0 is running
    Thread 0 is running
Thread1、Thread2 為 FIFO，表示他們執行時不需要 time slicing 且 Thread2 的 priority 比 Thread1 高，所以優先執行 Thread2，執行完再執行Thread1，最後才是Thread0

### Describe the results of ./sched_demo -n 4 -t 0.5 -s NORMAL,FIFO,NORMAL,FIFO -p -1,10,-1,30, and what causes that. 
    $ sudo ./sched_demo -n 4 -t 0.5 -s NORMAL,FIFO,NORMAL,FIFO -p -1,10,-1,30
    Thread 3 is running
    Thread 3 is running
    Thread 3 is running
    Thread 1 is running
    Thread 1 is running
    Thread 1 is running
    Thread 2 is running
    Thread 0 is running
    Thread 2 is running
    Thread 0 is running
    Thread 2 is running
    Thread 0 is running
Thread1、Thread3 為 FIFO，表示他們執行時不需要 time slicing 且 Thread2 的 priority 比 Thread1 高，所以優先執行 Thread2，執行完再執行Thread1。
接者才會輪到需要time slicing的 Thread0、Thread2，linux 系統下的 time slice 區間不固定且無法完全保證順序，兩者輪流 time sharing 直到完成 task。
### Describe how did you implement n-second-busy-waiting? 
使用`chrono`，進入 while-loop 直到 n-second 到，並且用 `mutex` 保證當前 thread 執行時不會被其他thread干擾。
```cpp!
pthread_mutex_lock(&mutex);
printf("Thread %d is running\n", ((thread_info_t *)arg)->thread_num);

auto start_time = high_resolution_clock::now();
while (duration_cast<seconds>(high_resolution_clock::now() - start_time).count() < time_wait ) {
    /* Busy for <time_wait> seconds */
}
pthread_mutex_unlock(&mutex);
```