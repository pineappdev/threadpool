#include "threadpool.h"
#include "err.h"
#include "utils.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <math.h>

// Necessary declarations
static void thread_pool_destroy_no_paf(struct thread_pool *pool);

// ************************************** THREADPOOL ARRAY ***************************************
static uint64_t custom_ceil(double val) {
    if (val == (double)((int64_t)val)) {
        return val;
    }
    else {
        return (int64_t)(val + 1);
    }
}

static bool should_shrink(pool_array *a)
{
    return (a->size != 0 && custom_ceil((double)a->size * a->shrink_coef) <= a->capacity && a->capacity != 1);
}

static void shrink_array(pool_array *a)
{
    a->q = realloc(a->q, a->size * sizeof(thread_pool_t *));
    if (a->q == NULL)
    {
        syserr("realloc");
    }
    a->capacity = a->size;
}

static void init_array(pool_array *a)
{
    a->size = 0;
    a->capacity = 1;
    a->q = malloc(sizeof(thread_pool_t *) * a->capacity);
    if (a->q == NULL)
    {
        syserr("malloc");
    }
    a->shrink_coef = 2;
}

static void add_pool(pool_array *a, thread_pool_t *pool)
{
    if (a->size == a->capacity)
    {
        a->q = realloc(a->q, a->capacity * a->shrink_coef * sizeof(thread_pool_t *));
        if (a->q == NULL)
        {
            syserr("realloc");
        }
        a->capacity = custom_ceil((double)a->capacity * a->shrink_coef);
    }

    a->q[(a->size)++] = pool;
}

static thread_pool_t *pop_array(pool_array *a)
{
    if (a->size == 0)
    {
        syserr("tried to pop on an empty q");
    }

    thread_pool_t *top = a->q[--(a->size)];

    if (should_shrink(a))
    {
        shrink_array(a);
    }

    return top;
}

static void remove_pool(pool_array *a, thread_pool_t *pool)
{
    for (uint64_t i = 0; i < a->size; i++)
    {
        if (a->q[i] == pool)
        {
            a->q[i] = a->q[--(a->size)];
            break;
        }
    }
    if (should_shrink(a))
    {
        shrink_array(a);
    }
}

static bool is_empty_array(pool_array *a)
{
    return a->size == 0;
}

static void destroy_array(pool_array *a)
{
    free(a->q);
}

// ***************************************** TASK_QUEUE *****************************************
static void init_q(Q *guardian)
{
    guardian->first = NULL;
    guardian->last = NULL;
}

static bool is_empty(Q *guardian)
{
    return guardian->first == NULL;
}

static runnable_t pop_q(Q *guardian)
{
    if (is_empty(guardian))
    {
        fatal("Tried to pop from an empty q");
    }
    runnable_t task = guardian->first->task;
    subQ *temp = guardian->first;
    guardian->first = guardian->first->next;
    free(temp);
    return task;
}

static void destroy_q(Q *guardian)
{
    while (!is_empty(guardian))
    {
        pop_q(guardian);
    }
}

static void add_q(Q *guardian, runnable_t *task)
{
    if (is_empty(guardian))
    {
        guardian->last = malloc(sizeof(subQ));
        if (guardian->last == NULL)
        {
            syserr("malloc");
        }
        guardian->first = guardian->last;
    }
    else
    {
        guardian->last->next = malloc(sizeof(subQ));
        if (guardian->last->next == NULL)
        {
            syserr("malloc");
        }
        guardian->last = guardian->last->next;
    }
    guardian->last->next = NULL;
    guardian->last->task = *task;
}

// ******************************************** PAF **********************************************

pool_array pool_all_father;
pthread_mutex_t for_paf;

static void add_to_paf(thread_pool_t *pool)
{
    P(&for_paf);
    add_pool(&pool_all_father, pool);
    V(&for_paf);
}

static void remove_from_paf(thread_pool_t *pool)
{
    P(&for_paf);
    remove_pool(&pool_all_father, pool);
    V(&for_paf);
}

static void destroy_paf()
{
    thread_pool_t *temp;
    P(&for_paf);
    while (!is_empty_array(&pool_all_father))
    {
        temp = pop_array(&pool_all_father);
        thread_pool_destroy_no_paf(temp);
    }
    V(&for_paf);
}

static void init_paf()
{
    init_sem(&for_paf);
    P(&for_paf);
    init_array(&pool_all_father);
    V(&for_paf);
}

static void sigint_handler(int signum)
{
    if (signum == 0)
    {
        signum = 0;
    } // to prevent compilation warnings
    destroy_paf();
}

static void sigaction_sigint()
{
    sigset_t block_mask;
    struct sigaction action;
    sigemptyset(&block_mask);
    if(sigaddset(&block_mask, SIGINT) != 0) {
        syserr("sigaddset");
    }

    action.sa_handler = sigint_handler;
    action.sa_flags = 0;
    action.sa_mask = block_mask;
    if (sigaction(SIGINT, &action, NULL) != 0)
    {
        syserr("sigaction");
    }
}

// ***************************************** THREADPOOL ******************************************

static void *thread_func(void *mothership_)
{
    thread_pool_t *mothership = mothership_;
    pthread_mutex_t *mutex = &(mothership->mutex);
    pthread_cond_t *for_task = &(mothership->for_task);

    while (true)
    {
        P(mutex);
        while (is_empty(&(mothership->tasks)) && !mothership->stop)
        {
            wait_cond(for_task, mutex);
        }

        // here we should check whether to break (because destroy was called) or not
        if (&(mothership->stop) && is_empty(&(mothership->tasks)))
        {
            V(mutex);
            break;
        }

        runnable_t task = pop_q(&(mothership->tasks));

        V(mutex);

        task.function(task.arg, task.argsz);
    }
    return NULL;
}

static void set_stop_flag(thread_pool_t *pool)
{
    P(&(pool->mutex));
    pool->stop = true;
    notify_all(&(pool->for_task));
    V(&(pool->mutex));
}

static void create_threads(thread_pool_t *pool, size_t no_threads)
{
    pthread_attr_t attr;
    int err;

    if ((err = pthread_attr_init(&attr)) != 0)
    {
        syserr("attr_init");
    }

    if ((err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)) != 0)
    {
        syserr("setdetach");
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * no_threads);
    if (threads == NULL)
    {
        syserr("malloc");
    }

    for (unsigned int i = 0; i < no_threads; i++)
    {
        if ((err = pthread_create(&threads[i], &attr, &thread_func, (void *)pool)) != 0)
        {
            syserr("pthread_create");
        }
    }

    if ((err = pthread_attr_destroy(&attr)) != 0)
    {
        syserr("attr_destroy");
    }

    pool->pool = threads;
    pool->pool_size = no_threads;
}

static void destroy_threads(thread_pool_t *pool)
{
    int err, *status;

    set_stop_flag(pool);

    for (size_t i = 0; i < pool->pool_size; i++)
    {
        if ((err = pthread_join(pool->pool[i], (void **)&status)) != 0)
        {
            syserr("pthread_join");
        }
    }

    free(pool->pool);
}

int thread_pool_init(thread_pool_t *pool, size_t num_threads)
{
    add_to_paf(pool);
    pool->stop = false;
    init_q(&(pool->tasks));
    init_sem(&(pool->mutex));
    init_cond(&(pool->for_task));
    create_threads(pool, num_threads);
    return 0;
}

static void thread_pool_destroy_no_paf(struct thread_pool *pool) {
    destroy_threads(pool);
    destroy_q(&(pool->tasks));
    destroy_sem(&(pool->mutex));
    destroy_cond(&(pool->for_task));
}

void thread_pool_destroy(struct thread_pool *pool)
{
    remove_from_paf(pool);
    thread_pool_destroy_no_paf(pool);
}

int defer(struct thread_pool *pool, runnable_t runnable)
{
    int ret = 0;
    if ((ret = P_(&(pool->mutex))) != 0)
    {
        return ret;
    }
    if (!pool->stop)
    {
        add_q(&(pool->tasks), &runnable);
        if ((ret = signal_(&(pool->for_task))) != 0)
        {
            V_(&(pool->mutex));
            return ret;
        }
    }
    else
    {
        ret = -1;
        V_(&(pool->mutex));
        return ret;
    }
    ret = V_(&(pool->mutex));
    return ret;
}


void before_main() {
	sigaction_sigint();
	init_paf();
}

void after_main() {
    destroy_array(&pool_all_father);
    destroy_sem(&for_paf);
}