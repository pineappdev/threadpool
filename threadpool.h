#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

typedef struct runnable
{
	void (*function)(void *, size_t);
	void *arg;
	size_t argsz;
} runnable_t;

// ******************************************* TASK QUEUE *******************************************

typedef struct subQ
{
    struct subQ *next;
    runnable_t task;
} subQ;

typedef struct Q
{
    subQ *first;
    subQ *last;
} Q;

// **************************************** THREAD_POOL *********************************************

typedef struct thread_pool
{
	uint64_t pool_size;
	pthread_t* pool;
	Q tasks; // Queue of tasks (runnable_t's)
	pthread_mutex_t mutex;
	pthread_cond_t for_task;
	bool stop;	// flag indicating that no defer will be accepted for this pool
} thread_pool_t;

int thread_pool_init(thread_pool_t *pool, size_t pool_size);

void thread_pool_destroy(thread_pool_t *pool);

int defer(thread_pool_t *pool, runnable_t runnable);

// ********************************************* POOL ARRAY ***********************************************
typedef struct pool_array
{
    thread_pool_t **q;
    uint64_t size;
    uint64_t capacity;
    float shrink_coef;
} pool_array;

// ********************************************** GLOBALS *************************************************

extern pool_array pool_all_father;	// array of all thread_pools, useful for sigint to call destroy
extern pthread_mutex_t for_paf;		// mutex for access to the above array

void before_main() __attribute__((constructor));	// inits globals
void after_main() __attribute__((destructor));		// destroys globals

#endif
