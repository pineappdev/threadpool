#include "future.h"
#include "utils.h"
#include "err.h"
#include <string.h>

typedef void *(*function_t)(void *);

struct args_t
{
	future_t *future;
	callable_t callable;
};

static void func(void *args, size_t size)
{
	if (size == 0)
	{
		size = 0;
	} // to prevent warnings

	struct args_t *true_args = (struct args_t *)args;
	future_t *future = true_args->future;
	callable_t callable = true_args->callable;

	future->outcome = callable.function(callable.arg, callable.argsz, &(future->size));

	P(&(future->mutex));
	future->is_finished = true;
	notify_all(&(future->for_outcome));
	V(&(future->mutex));

	free(args);
}

static runnable_t to_runnable(future_t *future, callable_t callable)
{
	runnable_t res;
	res.function = func;
	struct args_t args;
	args.callable = callable;
	args.future = future;

	res.arg = malloc(sizeof(struct args_t));
	if (res.arg == NULL)
	{
		syserr("malloc");
	}
	memcpy(res.arg, &args, sizeof(struct args_t));
	res.argsz = 0;
	return res;
}

/*
* Inits future arg, adds adds callable.function as a runnable to the pool's tasks
* Has to call defer
*/
int async(thread_pool_t *pool, future_t *future, callable_t callable)
{
	future->is_finished = false;
	init_sem(&(future->mutex));
	init_cond(&(future->for_outcome));
	runnable_t runnable = to_runnable(future, callable);
	return defer(pool, runnable);
}

/*
* This function assumes that future has already finished and yielded the result.
*/
static size_t get_outcome_size(future_t *future)
{
	P(&(future->mutex));
	size_t res = future->size;
	V(&(future->mutex));
	return res;
}

struct map_arg
{
	future_t *from;
	void *(*function)(void *, size_t, size_t *);
};

static void *func_map(void *arg, size_t arg_size, size_t *out_size)
{
	if(arg_size == 0) {
		arg_size = 0;
	}

	struct map_arg *map_arg = arg;

	future_t *from = map_arg->from;

	// I have no idea whether the type's all right
	void *(*func_inner)(void *, size_t, size_t *) = map_arg->function;

	void *outcome = await(from);
	size_t outcome_size = get_outcome_size(from);

	free(arg);

	return (*func_inner)(outcome, outcome_size, out_size);
}

// Non-blocking version of map
int map(thread_pool_t *pool, future_t *future, future_t *from,
		void *(*function)(void *, size_t, size_t *))
{
	struct map_arg *map_arg = malloc(sizeof(struct map_arg));
	if(map_arg == NULL) {
		syserr("malloc");
	}
	map_arg->from = from;
	map_arg->function = function;

	callable_t callable;
	callable.arg = map_arg;
	callable.argsz = 0;
	callable.function = func_map;

	return async(pool, future, callable);
}

/*
* Waits for the future's outcome
*/
void *await(future_t *future)
{
	P(&(future->mutex));
	while (!future->is_finished)
	{
		wait_cond(&(future->for_outcome), &(future->mutex));
	}
	void *res = future->outcome;
	V(&(future->mutex));
	return res;
}
