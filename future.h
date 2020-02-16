#ifndef FUTURE_H
#define FUTURE_H

#include "threadpool.h"

typedef struct callable
{
	void *(*function)(void *, size_t, size_t *);
	void *arg;
	size_t argsz;
} callable_t;

typedef struct future
{
	pthread_mutex_t mutex;
	pthread_cond_t for_outcome;
	bool is_finished;
	void *outcome;
	size_t size;
} future_t;

/*
* Inits future arg, adds adds callable.function as a runnable to the pool's tasks
*/
int async(thread_pool_t *pool, future_t *future, callable_t callable);

/*
* Add a new task to the pool - call <function> on argument <from> and write result in <future>
*/
int map(thread_pool_t *pool, future_t *future, future_t *from,
		void *(*function)(void *, size_t, size_t *));

/*
* Waits for the future's outcome
*/
void *await(future_t *future);

#endif
