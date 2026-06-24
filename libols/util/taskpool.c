/*
 * Copyright (c) 2024
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "taskpool.h"
#include "task.h"
#include "bmem.h"
#include "threading.h"

#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

/* Internal task structure for tracking pending tasks */
struct taskpool_task_info {
    os_taskpool_task_t task;
    void *param;
};

/* Thread pool structure */
struct os_taskpool {
    os_task_queue_t **queues; /* Array of task queues (one per thread) */
    size_t num_threads;       /* Number of worker threads */
    size_t next_queue;        /* Next queue to assign task to (round-robin) */
    pthread_mutex_t mutex;    /* Mutex for round-robin counter */

    bool initialized; /* Whether the pool is fully initialized */
};

/* Default global thread pool */
static os_taskpool_t *default_pool = NULL;
static pthread_mutex_t default_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Get the optimal number of threads based on CPU cores
 */
static size_t get_optimal_thread_count(void) {
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return (size_t)sysinfo.dwNumberOfProcessors;
#else
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    return (nprocs > 0) ? (size_t)nprocs : 4;
#endif
}

/**
 * Increment pending task counter wrapper
 */
static void task_wrapper(void *param) {
    struct taskpool_task_info *ti = (struct taskpool_task_info *)param;
    if (ti) {
        ti->task(ti->param);
        bfree(ti);
    }
}

os_taskpool_t *os_taskpool_create(size_t num_threads) {
    struct os_taskpool *pool;
    size_t i;

    /* Use optimal thread count if not specified */
    if (num_threads == 0) {
        num_threads = get_optimal_thread_count();
    }

    /* Allocate pool structure */
    pool = bzalloc(sizeof(*pool));
    if (!pool) {
        return NULL;
    }

    pool->num_threads = num_threads;
    pool->next_queue = 0;
    pool->initialized = false;

    /* Initialize mutex for round-robin assignment */
    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        bfree(pool);
        return NULL;
    }

    /* Allocate array of task queue pointers */
    pool->queues = bzalloc(sizeof(os_task_queue_t *) * num_threads);
    if (!pool->queues) {
        pthread_mutex_destroy(&pool->mutex);
        bfree(pool);
        return NULL;
    }

    /* Create task queues (one per thread) */
    for (i = 0; i < num_threads; i++) {
        pool->queues[i] = os_task_queue_create();
        if (!pool->queues[i]) {
            /* Clean up already created queues */
            for (size_t j = 0; j < i; j++) {
                os_task_queue_destroy(pool->queues[j]);
            }
            bfree(pool->queues);
            pthread_mutex_destroy(&pool->mutex);
            bfree(pool);
            return NULL;
        }
    }

    pool->initialized = true;
    return pool;
}

void os_taskpool_destroy(os_taskpool_t *pool) {
    size_t i;

    if (!pool) {
        return;
    }

    /* Destroy all task queues */
    for (i = 0; i < pool->num_threads; i++) {
        if (pool->queues[i]) {
            os_task_queue_destroy(pool->queues[i]);
        }
    }

    bfree(pool->queues);
    pthread_mutex_destroy(&pool->mutex);
    bfree(pool);
}

bool os_taskpool_queue_task(os_taskpool_t *pool, os_taskpool_task_t task, void *param) {
    size_t queue_idx;
    bool result;

    if (!pool || !pool->initialized || !task) {
        return false;
    }

    /* Allocate task info for wrapper */
    struct taskpool_task_info *ti = bmalloc(sizeof(*ti));
    if (!ti) {
        return false;
    }
    ti->task = task;
    ti->param = param;

    /* Select queue using round-robin */
    pthread_mutex_lock(&pool->mutex);
    queue_idx = pool->next_queue;
    pool->next_queue = (pool->next_queue + 1) % pool->num_threads;
    pthread_mutex_unlock(&pool->mutex);

    /* Queue the task */
    result = os_task_queue_queue_task(pool->queues[queue_idx], task_wrapper, ti);
    if (!result) {
        bfree(ti);
    }

    return result;
}

bool os_taskpool_wait(os_taskpool_t *pool) {
    size_t i;
    bool success = true;

    if (!pool || !pool->initialized) {
        return false;
    }

    /* Wait on all queues */
    for (i = 0; i < pool->num_threads; i++) {
        if (!os_task_queue_wait(pool->queues[i])) {
            success = false;
        }
    }

    return success;
}

size_t os_taskpool_num_threads(os_taskpool_t *pool) {
    if (!pool) {
        return 0;
    }
    return pool->num_threads;
}

bool os_taskpool_inside(os_taskpool_t *pool) {
    size_t i;

    if (!pool || !pool->initialized) {
        return false;
    }

    /* Check if current thread belongs to any queue in this pool */
    for (i = 0; i < pool->num_threads; i++) {
        if (os_task_queue_inside(pool->queues[i])) {
            return true;
        }
    }

    return false;
}

os_taskpool_t *os_taskpool_get_default(void) {
    if (default_pool) {
        return default_pool;
    }

    pthread_mutex_lock(&default_pool_mutex);
    if (!default_pool) {
        default_pool = os_taskpool_create(0);
    }
    pthread_mutex_unlock(&default_pool_mutex);

    return default_pool;
}

/**
 * Cleanup function for default pool (called at program exit)
 */
static void cleanup_default_pool(void) {
    if (default_pool) {
        os_taskpool_destroy(default_pool);
        default_pool = NULL;
    }
}

#ifdef _MSC_VER
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU"))
#endif
static void init_default_pool_cleanup(void) {
    atexit(cleanup_default_pool);
}

#ifndef _MSC_VER
__attribute__((constructor)) static void init_default_pool_cleanup2(void) {
    atexit(cleanup_default_pool);
}
#endif
