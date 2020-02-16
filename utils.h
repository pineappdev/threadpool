#ifndef UTILS_H
#define UTILS_H

#include "err.h"
#include <pthread.h>

// Some wrappers for the sake of comfort

void init_sem(pthread_mutex_t *sem);

void init_sem_recursive(pthread_mutex_t *sem);

void notify_all(pthread_cond_t *cond);

void init_cond(pthread_cond_t *cond);

void destroy_sem(pthread_mutex_t *sem);

void destroy_cond(pthread_cond_t *cond);

void P(pthread_mutex_t *sem);

int P_(pthread_mutex_t *sem);

void V(pthread_mutex_t *sem);

int V_(pthread_mutex_t *sem);

void wait_cond(pthread_cond_t *cond, pthread_mutex_t *sem);

void signal_cond(pthread_cond_t *cond);

int signal_(pthread_cond_t *cond);

#endif