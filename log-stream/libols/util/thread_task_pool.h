#pragma once

#include <string.h>
#include "task.h"

// 线程池结构体
// shutdown标志位
#define POOL_ACTIVE 0
#define POOL_SHUTDOWN 1 

// 池默认工作线程数组容量
#define DEFAULT_THREAD_POOL_WORKER_SIZE 8
// 池默认任务队列容量
#define DEFAULT_TASK_QUEUE_CAP 16

// 暂无线程池扩容，创建时线程数量是多少就是多少线程
typedef struct fixed_thread_pool_s fixed_thread_pool_t;

/**
 *线程池函数
 */
// 创建线程池
fixed_thread_pool_t * create_fixed_thread_pool(int task_queue_cap, int worker_arr_size);
// 工作线程，循环消费任务队列
// todo 消费
void * worker(void * arg);
// 线程池持有者，生产任务到任务队列
// todo 生产
void thread_pool_task_add(fixed_thread_pool_t * pool, void*(* func)(void*), void * arg);
// 关闭线程池
void pool_shutdown(fixed_thread_pool_t * pool);