#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <windows.h>
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
    HANDLE threads[MAX_THREADS];
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE not_empty;
    CONDITION_VARIABLE not_full;
    volatile LONG running;  // Using Windows atomic operations
    int thread_count;
} ThreadPool;

static DWORD WINAPI worker_thread(LPVOID arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    Task task;
    
    while (InterlockedCompareExchange(&pool->running, 1, 1)) {
        EnterCriticalSection(&pool->lock);
        
        while (pool->count == 0 && InterlockedCompareExchange(&pool->running, 1, 1)) {
            SleepConditionVariableCS(&pool->not_empty, &pool->lock, INFINITE);
        }
        
        if (!InterlockedCompareExchange(&pool->running, 1, 1)) {
            LeaveCriticalSection(&pool->lock);
            break;
        }
        
        task = pool->tasks[pool->front];
        pool->front = (pool->front + 1) % MAX_QUEUE;
        pool->count--;
        
        WakeConditionVariable(&pool->not_full);
        LeaveCriticalSection(&pool->lock);
        
        (*(task.function))(task.argument);
    }
    
    return 0;
}

static inline ErrorCode thread_pool_init(ThreadPool* pool, int num_threads) {
    if (num_threads > MAX_THREADS) num_threads = MAX_THREADS;
    
    pool->front = pool->rear = pool->count = 0;
    pool->thread_count = num_threads;
    InterlockedExchange(&pool->running, 1);
    
    InitializeCriticalSection(&pool->lock);
    InitializeConditionVariable(&pool->not_empty);
    InitializeConditionVariable(&pool->not_full);
    
    for (int i = 0; i < num_threads; i++) {
        pool->threads[i] = CreateThread(
            NULL,
            0,
            worker_thread,
            pool,
            0,
            NULL
        );
        
        if (pool->threads[i] == NULL) {
            // Cleanup on failure
            InterlockedExchange(&pool->running, 0);
            WakeAllConditionVariable(&pool->not_empty);
            WakeAllConditionVariable(&pool->not_full);
            
            for (int j = 0; j < i; j++) {
                WaitForSingleObject(pool->threads[j], INFINITE);
                CloseHandle(pool->threads[j]);
            }
            
            DeleteCriticalSection(&pool->lock);
            return ERROR_THREAD;
        }
    }
    
    return ERROR_NONE;
}

static inline ErrorCode thread_pool_add_task(ThreadPool* pool, void (*function)(void*), void* argument) {
    EnterCriticalSection(&pool->lock);
    
    while (pool->count == MAX_QUEUE && InterlockedCompareExchange(&pool->running, 1, 1)) {
        SleepConditionVariableCS(&pool->not_full, &pool->lock, INFINITE);
    }
    
    if (!InterlockedCompareExchange(&pool->running, 1, 1)) {
        LeaveCriticalSection(&pool->lock);
        return ERROR_THREAD;
    }
    
    pool->tasks[pool->rear].function = function;
    pool->tasks[pool->rear].argument = argument;
    pool->rear = (pool->rear + 1) % MAX_QUEUE;
    pool->count++;
    
    WakeConditionVariable(&pool->not_empty);
    LeaveCriticalSection(&pool->lock);
    
    return ERROR_NONE;
}

static inline void thread_pool_destroy(ThreadPool* pool) {
    InterlockedExchange(&pool->running, 0);
    
    WakeAllConditionVariable(&pool->not_empty);
    WakeAllConditionVariable(&pool->not_full);
    
    for (int i = 0; i < pool->thread_count; i++) {
        WaitForSingleObject(pool->threads[i], INFINITE);
        CloseHandle(pool->threads[i]);
    }
    
    DeleteCriticalSection(&pool->lock);
}

#endif