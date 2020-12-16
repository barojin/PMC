#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>  /* required for rand() */
#include <semaphore.h>
#include <unistd.h> /* provides access to the POSIX */
#include "buffer.h"
#include <signal.h> /* SIGUSR1 */

void *consumer(void *param);
void *producer(void *param);
void *middleman(void *param);
void *watch(void *param);

/* to check the output */
int idx1, idx2;
int res1[200], res2[200];
int total_in, total_out;

int running; // for while loops in thread to terminate simoutaneously
int count, count2; // points buffer is full or not.
int in, in2; //in points to the next free spot in the buffer
int out, out2; //out points to the first filled spot
int a,b,c; // store arguments in the int type
#define BUFFER_SIZE 5
buffer_item buffer[BUFFER_SIZE]; // used in producer part
buffer_item buffer2[BUFFER_SIZE]; // used in consumer part
sem_t mutex, full, empty, mutex2, full2, empty2;

int main(int argc, char *argv[])
{
    /* 1. Get command line arguments */
    // argv[1] = the length of time
    a = atoi( argv[2]); // argv[2] = # of producer
    b = atoi( argv[3]); // argv[3] = # of middleman
    c = atoi( argv[4]); // argv[4] = # of consumer
    pthread_t thread[a]; /* the producer identifier */
    pthread_t thread2[b]; /* the middleman identifier */
    pthread_t thread3[c]; /* the consumer identifier */
    pthread_t threadwatch; /* the watcher identifier */
    
    /* Initialize values */
    idx1 = idx2 = 0;
    in = 0;
    out = 0;
    count = 0;
    in2 = 0;
    out2 = 0;
    count2 = 0;
    total_in = 0;
    total_out = 0;
    for(int i=0; i<200; i++){
        res1[i] = 0;
        res2[i] = 0;
    }
    pthread_attr_t attr; /* set of attributes for the thread */
    pthread_attr_init(&attr); /* get the default attributes */
    
    running = 1; /*TRUE var for while loop in thread */
    
    sem_init(&mutex, 0, 1); //A semaphore mutex initialized to the value 1
    sem_init(&full, 0, 0); //A semaphore full initialized to the value 0
    sem_init(&empty, 0, BUFFER_SIZE); //A semaphore empty initialized to the value N
    sem_init(&mutex2, 0, 1); //A semaphore mutex initialized to the value 1
    sem_init(&full2, 0, 0); //A semaphore full initialized to the value 0
    sem_init(&empty2, 0, BUFFER_SIZE); //A semaphore empty initialized to the value N
    
    /* 2. Initialize buffer */
    for(int i=0; i<a; i++){
        buffer[i] = 0;
        buffer2[i] = 0;
    }
    
    /* 3. Create producer threads. */
    for(int i=0; i<a; i++){
        if(pthread_create(&thread[i], &attr, producer, NULL) < 0){
            perror("producer pthread_create failed");
            exit(1);
        }
        printf("Creating producer thread with id %lu\n", thread[i]);
    }
    
    /* 4. Create middleman threads. */
    for(int i=0; i<b; i++){
        if(pthread_create(&thread2[i], &attr, middleman, NULL) < 0){
            perror("middleman pthread_create failed");
            exit(1);
        }
        printf("Creating middleman thread with id %lu\n", thread2[i]);
    }
    
    /* 5. Create consumer threads.  */
    for(int i=0; i<c; i++){
        if(pthread_create(&thread3[i], &attr, consumer, NULL) < 0){
            perror("consumer pthread_create failed");
            exit(1);
        }
        printf("Creating consumer thread with id %lu\n", thread3[i]);
    }
    
    /* Create watch thread
     main function waits all of threads join together
     so I made the threadwatch function to count the time and kill all of threads */
    if(pthread_create(&threadwatch, &attr, watch, argv[1]) < 0){
        perror("threadwatch pthread_create failed");
        exit(1);
    }
    
    /* Join. */
    for(int i=0; i<a; i++){
        pthread_join(thread[i], NULL);
    }
    for(int i=0; i<b; i++){
        pthread_join(thread2[i], NULL);
    }
    for(int i=0; i<c; i++){
        pthread_join(thread3[i], NULL);
    }
    
    /* 6.  Sleep. I think I need pthread_cond_t and using the
    pthread_cond_broadcast (condition) to unlock and kill threads
     but this way is hard to apply on here, I used another way.
     Instead, I made a threadwatch join with other threads for sychronization
     and stop other threads setting the global value 'int running' to 0 */
    pthread_join(threadwatch, NULL);
    
    /* 7.  Kill threads and exit. Threads are already killed by running value,
     otherwise, use the pthread_kill signal option. */
    for(int i=0; i<a; i++){
        if(pthread_kill(thread[i], SIGUSR1)<0)
            perror("pthread_kill failed");
    }
    for(int i=0; i<b; i++){
        if(pthread_kill(thread2[i], SIGUSR1)<0)
            perror("pthread_kill failed");
    }
    for(int i=0; i<c; i++){
        if(pthread_kill(thread3[i], SIGUSR1)<0)
            perror("pthread_kill failed");
    }
    pthread_attr_destroy(&attr);
    sem_destroy(&mutex);
    sem_destroy(&full);
    sem_destroy(&empty);
    sem_destroy(&mutex2);
    sem_destroy(&full2);
    sem_destroy(&empty2);
    printf("Main thread exiting.\n");
    printf("total in = %d, total out = %d\n", total_in, total_out);
    printf("buf1|");
    for(int i=0; i<200; i++){
        printf("%d |", res1[i]);
    }
    printf("\n");
    printf("buf2|");
    for(int i=0; i<200; i++){
        printf("%d |",res2[i]);
    }
    pthread_exit(NULL);
    return 0;
}

/* This thread is created and join with other threads and kill all of threads created in the main thread
 using 'running' global variable was set to 1 and it is used in while loop condtion of all of threads.
    */
void *watch(void *param){
    int time = atoi(param);
    printf("Main thread sleeping for %d seconds\n",time);
    sleep(time);
    running = 0;
    pthread_exit(0);
}

/* insert item into the buffer (item from producer)*/
int insert_item(buffer_item item, char name[]) {
    int result = 0;
    // to return 0 when insert value to buffer otherwise -1
    
    /* sem_wait() if locks value is 0 then the call blocks until available or signal handler interrupts
      reference: linux man page */
    sem_wait(&empty);
    /* empty and full two counting semaphore sem_t tracks the state of the buffer. In detail, empty is the number of empty places in the buffer, and full is the number of items in the buffer so they can maintain consistency */
    
    sem_wait(&mutex);
    /* The binary semaphore allows only one thread attempting to the critical section to insert value to the buffer. */
    
    if(count == BUFFER_SIZE){
        printf("1 is full\n");
        result = -1;
    }
    else{
        buffer[in] = item;
        printf("Insert_item inserted item %d at position %d\n", item, in);
        in = (in + 1) % BUFFER_SIZE;
        count ++;
        total_in ++;
    }
    printf("in_1[%d][%d][%d][%d][%d], in = %d, out = %d queue = %s\n", buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],in,out,name);
    
    // signal
    /* sem_post() increments unlocks value. If the value of a semaphore is currently zero, then a sem_wait() operation will block until the value becomes greater than zero. reference: linux man page*/
    sem_post(&mutex);
    sem_post(&full);
    
    return result;
}

/* remove item from the buffer (called by middleman)*/
int remove_item(char name[]) {
    int result = -1;
    // wait
    sem_wait(&full);
    sem_wait(&mutex);
    
    if(count == 0){
        printf("1 is empty\n");
    }
    else{
        result = buffer[out];
        buffer[out] = 0;
        printf("Remove_item inserted item %d at position %d\n", result, out);
        out = (out + 1) % BUFFER_SIZE;
        count--;
        res1[idx1] = result;
        idx1 += 1;
    }
    printf("ou_1[%d][%d][%d][%d][%d], in = %d, out = %d queue = %s\n", buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],in,out,name);

    // signal
    sem_post(&mutex);
    sem_post(&empty);
    
    return result;
}

/* insert item into the buffer2 (item from the buffer and called by middleman)*/
int insert_item2(buffer_item item, char name[]) {
    int result = 0;
    // wait
    sem_wait(&empty2);
    sem_wait(&mutex2);
    
    if(count2 == BUFFER_SIZE){
        printf("2 is full\n");
        result = -1;
    }
    else{
        buffer2[in2] = item;
        printf("Insert_item inserted item %d at position %d\n", item, in2);
        in2 = (in2 + 1) % BUFFER_SIZE;
        count2 ++;
        res2[idx2] = item;
        idx2 += 1;
    }
    printf("in_2[%d][%d][%d][%d][%d], in = %d, out = %d queue = %s\n", buffer2[0],buffer2[1],buffer2[2],buffer2[3],buffer2[4],in2,out2,name);
    
    // signal
    sem_post(&mutex2);
    sem_post(&full2);
    
    return result;
}

/* remove item from the buffer2 (called by consumer)*/
int remove_item2(char name[]) {
    int result = -1;
    // wait
    sem_wait(&full2);
    sem_wait(&mutex2);

    if(count2 == 0){
        printf("2 is empty\n");
        result = -1;
    }
    else{
        result = buffer2[out2];
        buffer2[out2] = 0;
        printf("Remove_item inserted item %d at position %d\n", result, out2);
        out2 = (out2 + 1) % BUFFER_SIZE;
        count2--;
        total_out ++;
    }
    printf("ou_2[%d][%d][%d][%d][%d], in = %d, out = %d queue = %s\n", buffer2[0],buffer2[1],buffer2[2],buffer2[3],buffer2[4],in2,out2,name);
    
    // signal
    sem_post(&mutex2);
    sem_post(&empty2);
    
    return result;
}

/* call insert_item() */
void *producer(void *param) {
    buffer_item item;
    int cnt = 0;
    while(running) {
        /* generate random number */
        srand(time(NULL)); /* initialize random seed: */
        int rn = rand() % 3 + 1; /* rn between 1 and 3 */
        printf("Producer thread %lu sleeping for %d seconds\n",  pthread_self(), rn);
        sleep(rn); /* sleep for random amount of time */
        /* generate a random number - this is the producer's product */
        item = rand() % 50 + 1;
        printf("Producer thread %lu inserted value %d\n", pthread_self(), item);
        if (insert_item(item, "Producer") < 0) printf("Producer error\n");
    }
    pthread_exit(0);
}

 /* move one item from the producer buffer(buffer) to the consumer buffer(buffer2) and repeat. */
void *middleman(void *param)
{
    buffer_item item;
        while(running) {
            srand(time(NULL));
            int rn = rand() % 3 + 1;
            sleep(rn);
            printf("Middleman thread %lu sleeping for %d seconds\n",  pthread_self(), rn);
            
            // fetch the item from the producer buffer and remove it
            item = remove_item("Middleman");
            if (item< 0)
                printf("Middleman error\n");
            else
                printf("Middleman thread %lu removed value %d\n", pthread_self(), item);
            
            // insert that item into the consumer buffer
            if (insert_item2(item,"Middleman") < 0) printf("Middleman error\n");
            else printf("Middleman thread %lu inserted value %d\n", pthread_self(), item);
        }
   pthread_exit(0);
}

/* call remove_item2 */
void *consumer(void *param)
{
    buffer_item item;
         while(running) {
             srand(time(NULL));
             int rn = rand() % 3 + 1;
             sleep(rn);
             printf("Consumer thread %lu sleeping for %d seconds\n",  pthread_self(), rn);
             item = remove_item2("Consumer");
             if (item < 0) printf("Consumer error\n");
             else printf("Consumer thread %lu removed value %d\n", pthread_self(), item);
         }
    pthread_exit(0);
}
