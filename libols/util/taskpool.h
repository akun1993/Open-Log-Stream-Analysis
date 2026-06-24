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

#pragma once

#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file taskpool.h
 * @brief Thread pool implementation based on os_task_queue
 *
 * This thread pool manages multiple worker threads that can execute
 * tasks concurrently. It provides functionality for task submission,
 * waiting for completion, and graceful shutdown.
 */

/* Forward declarations */
struct os_taskpool;
typedef struct os_taskpool os_taskpool_t;

/* Task function type - same as os_task_t for compatibility */
typedef void (*os_taskpool_task_t)(void *param);

/**
 * @brief Create a new thread pool
 *
 * @param num_threads Number of worker threads to create.
 *                    If 0, uses the number of CPU cores available.
 * @return Pointer to the thread pool, or NULL on failure
 */
EXPORT os_taskpool_t *os_taskpool_create(size_t num_threads);

/**
 * @brief Destroy the thread pool
 *
 * Waits for all currently executing tasks to complete, then shuts down
 * all worker threads and frees resources.
 *
 * @param pool The thread pool to destroy
 */
EXPORT void os_taskpool_destroy(os_taskpool_t *pool);

/**
 * @brief Submit a task to the thread pool
 *
 * The task will be executed by one of the worker threads in the pool.
 * Tasks are executed in FIFO order.
 *
 * @param pool The thread pool
 * @param task The task function to execute
 * @param param Parameter to pass to the task function
 * @return true if the task was successfully queued, false on error
 */
EXPORT bool os_taskpool_queue_task(os_taskpool_t *pool, os_taskpool_task_t task, void *param);

/**
 * @brief Wait for all queued tasks to complete
 *
 * Blocks until all currently queued tasks have finished executing.
 * Note: New tasks added during the wait may or may not be waited for.
 *
 * @param pool The thread pool
 * @return true on success, false on error
 */
EXPORT bool os_taskpool_wait(os_taskpool_t *pool);

/**
 * @brief Get the number of worker threads in the pool
 *
 * @param pool The thread pool
 * @return Number of worker threads, or 0 on error
 */
EXPORT size_t os_taskpool_num_threads(os_taskpool_t *pool);

/**
 * @brief Check if the current thread is a worker thread of this pool
 *
 * @param pool The thread pool
 * @return true if called from within a worker thread of this pool
 */
EXPORT bool os_taskpool_inside(os_taskpool_t *pool);

/**
 * @brief Get the default thread pool instance
 *
 * Returns a lazily-initialized global thread pool with a default
 * number of threads. The pool is automatically destroyed on program exit.
 *
 * @return Pointer to the default thread pool
 */
EXPORT os_taskpool_t *os_taskpool_get_default(void);

#ifdef __cplusplus
}
#endif
