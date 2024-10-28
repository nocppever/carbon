#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdatomic.h>
#include "error.h"

#define MAX_THREADS 10
#define MAX_QUEUE 100

typedef struct {
    void (*function)(void*);
    void* argument;
} Task;

typedef struct {
    Task tasks[MAX_QUEUE];
    int front;
    int rear;
    int count;
    pthread_t threads[MAX_THREADS];
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    atomic_bool running;
    int thread_count;
} ThreadPool;

static inline void* worker_thread(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    Task task;
    
    while (atomic_load(&pool->running)) {
        pthread_mutex_lock(&pool->lock);
        
        while (pool->count == 0 && atomic_load(&pool->running)) {
            pthread_cond_wait(&pool->not_empty, &pool->lock);
        }
        
        if (!atomic_load(&pool->running)) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }
        
        task = pool->tasks[pool->front];
        pool->front = (pool->front + 1) % MAX_QUEUE;
        pool->count--;
        
        pthread_cond_signal(&pool->not_full);
        pthread_mutex_unlock(&pool->lock);
        
        (*(task.function))(task.argument);
    }
    
    return NULL;
}

static inline ErrorCode thread_pool_init(ThreadPool* pool, int num_threads) {
    if (num_threads > MAX_THREADS) num_threads = MAX_THREADS;
    
    pool->front = pool->rear = pool->count = 0;
    pool->thread_count = num_threads;
    atomic_store(&pool->running, true);
    
    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        return ERROR_THREAD;
    }
    
    if (pthread_cond_init(&pool->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&pool->lock);
        return ERROR_THREAD;
    }
    
    if (pthread_cond_init(&pool->not_full, NULL) != 0) {
        pthread_mutex_destroy(&pool->lock);
        pthread_cond_destroy(&pool->not_empty);
        return ERROR_THREAD;
    }
    
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            pthread_mutex_destroy(&pool->lock);
            pthread_cond_destroy(&pool->not_empty);
            pthread_cond_destroy(&pool->not_full);
            return ERROR_THREAD;
        }
    }
    
    return ERROR_NONE;
}

static inline ErrorCode thread_pool_add_task(ThreadPool* pool, void (*function)(void*), void* argument) {
    pthread_mutex_lock(&pool->lock);
    
    while (pool->count == MAX_QUEUE && atomic_load(&pool->running)) {
        pthread_cond_wait(&pool->not_full, &pool->lock);
    }
    
    if (!atomic_load(&pool->running)) {
        pthread_mutex_unlock(&pool->lock);
        return ERROR_THREAD;
    }
    
    pool->tasks[pool->rear].function = function;
    pool->tasks[pool->rear].argument = argument;
    pool->rear = (pool->rear + 1) % MAX_QUEUE;
    pool->count++;
    
    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);
    
    return ERROR_NONE;
}

static inline void thread_pool_destroy(ThreadPool* pool) {
    atomic_store(&pool->running, false);
    
    pthread_cond_broadcast(&pool->not_empty);
    pthread_cond_broadcast(&pool->not_full);
    
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);
}

#endif