#include "utils.h"

void init_sem(pthread_mutex_t *sem)
{
    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0)
    {
        syserr("pthread_mutexattr_init");
    }
    if (pthread_mutex_init(sem, &attr) != 0)
    {
        syserr("pthread_mutex_init");
    }
    if (pthread_mutexattr_destroy(&attr) != 0)
    {
        syserr("pthread_mutexattr_destroy");
    }
}

void init_sem_recursive(pthread_mutex_t *sem) {
    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0)
    {
        syserr("pthread_mutexattr_init");
    }
    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
        syserr("pthread_mutexattr_settype");
    }
    if (pthread_mutex_init(sem, &attr) != 0)
    {
        syserr("pthread_mutex_init");
    }
    if (pthread_mutexattr_destroy(&attr) != 0)
    {
        syserr("pthread_mutexattr_destroy");
    }
}

void notify_all(pthread_cond_t *cond)
{
    if (pthread_cond_broadcast(cond) != 0)
    {
        syserr("pthread_cond_braodcast");
    }
}

void init_cond(pthread_cond_t *cond)
{
    pthread_condattr_t attr;
    if (pthread_condattr_init(&attr) != 0)
    {
        syserr("pthread_condattr_init");
    }
    if (pthread_cond_init(cond, &attr) != 0)
    {
        syserr("pthread_cond_init");
    }
    if (pthread_condattr_destroy(&attr) != 0)
    {
        syserr("pthread_condattr_destroy");
    }
}

void destroy_sem(pthread_mutex_t *sem)
{
    if (pthread_mutex_destroy(sem) != 0)
    {
        syserr("pthread_mutex_destroy");
    }
}

void destroy_cond(pthread_cond_t *cond)
{
    if (pthread_cond_destroy(cond) != 0)
    {
        syserr("pthread_cond_destroy");
    }
}

void P(pthread_mutex_t *sem)
{
    int err;
    if ((err = pthread_mutex_lock(sem)) != 0)
    {
        syserr("pthread_mutex_lock");
    }
}

int P_(pthread_mutex_t *sem)
{
    return pthread_mutex_lock(sem);
}

void V(pthread_mutex_t *sem)
{
    int err;
    if ((err = pthread_mutex_unlock(sem)) != 0)
    {
        syserr("pthread_mutex_unlock");
    }
}

int V_(pthread_mutex_t *sem)
{
    return pthread_mutex_unlock(sem);
}

void wait_cond(pthread_cond_t *cond, pthread_mutex_t *sem)
{
    if (pthread_cond_wait(cond, sem) != 0)
    {
        syserr("pthread_cond_wait");
    }
}

void signal_cond(pthread_cond_t *cond)
{
    if (pthread_cond_signal(cond) != 0)
    {
        syserr("pthread_cond_signal");
    }
}

int signal_(pthread_cond_t *cond)
{
    return pthread_cond_signal(cond);
}