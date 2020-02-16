#include "threadpool.h"
#include "utils.h"
#include "future.h"
#include "err.h"
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <signal.h>

typedef struct args
{
    int milis;
    int val;
    int *sum;
    pthread_mutex_t *mutex;
} args_t;

void function(void *args, size_t size)
{
    if (size == 0)
    {
        size = 0;
    } // for no warnings

    args_t *arg = (args_t *)args;

    sleep(arg->milis / 1000);

    P(arg->mutex);
    *(arg->sum) += arg->val;
    V(arg->mutex);
}

void init_mutexes(pthread_mutex_t *mutexes, int size)
{
    for (int i = 0; i < size; i++)
    {
        init_sem(&mutexes[i]);
    }
}

int main()
{
    int rows, cols; // rows and cols
    if (scanf("%d", &rows) != 1)
    {
        syserr("scanf");
    }
    if (scanf("%d", &cols) != 1)
    {
        syserr("scanf");
    }

    int val, milis;

    pthread_mutex_t *mutexes = malloc(sizeof(pthread_mutex_t) * rows);
    if (mutexes == NULL)
    {
        syserr("malloc");
    }
    init_mutexes(mutexes, rows);

    runnable_t *runnables = malloc(sizeof(runnable_t) * rows * cols);
    if (runnables == NULL)
    {
        syserr("malloc");
    }

    int *rows_sums = malloc(sizeof(int) * rows);
    if (rows_sums == NULL)
    {
        syserr("malloc");
    }
    memset(rows_sums, 0, rows * sizeof(int));

    args_t *arg = malloc(sizeof(args_t) * rows * cols);
    if (arg == NULL)
    {
        syserr("malloc");
    }

    for (int i = 0; i < rows * cols; i++)
    {
        if (scanf("%d %d", &val, &milis) != 2)
        {
            syserr("scanf");
        }
        arg[i].milis = milis;
        arg[i].val = val;
        arg[i].mutex = &mutexes[i / cols];
        arg[i].sum = rows_sums + i / cols;
        runnables[i].arg = (void *)&arg[i];
        runnables[i].argsz = 0;
        runnables[i].function = function;
    }

    thread_pool_t pool;
    thread_pool_init(&pool, 4);

    for (int i = 0; i < rows * cols; i++)
    {
        if(defer(&pool, runnables[i]) != 0) {
            syserr("defer");
        }
    }

    thread_pool_destroy(&pool);

    for (int i = 0; i < rows; i++)
    {
        printf("%d\n", rows_sums[i]);
    }

    for (int i = 0; i < rows; i++)
    {
        destroy_sem(&mutexes[i]);
    }

    free(mutexes);
    free(runnables);
    free(rows_sums);
    free(arg);

    return 0;
}
