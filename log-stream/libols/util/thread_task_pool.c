#include "thread_task_pool.h"
#include "exception.h"

// 任务结构体
struct task_s {
    void * (* func) (void* arg);
    void * arg;
};

// 任务队列结构体
struct task_queue_s {
    int task_queue_cap; // 容量
    int task_queue_size; // 当前任务队列大小
    int task_queue_head; // 队头 出任务
    int task_queue_tail; // 队尾 入任务

    task_t tasks[]; // C99 柔性数组
};

// 线程池结构体
struct fixed_thread_pool_s {
    // 线程池是否关闭
    int is_shutdown;

    // 工作者线程数量
    int worker_arr_cap;
    // 工作者线程数组
    pthread_t * worker_id_arr;

    // todo 因为不需要扩容线程池工作线程，所以不需要 管理者和 监控工作线程忙数量
    /*
    //工作者线程 忙数量
    __attribute__((unused)) int workker_busy_num;
    // 工作者线程 忙数量 锁
    __attribute__((unused)) pthread_mutex_t mutex_busy_num; // 可以用cas锁优化

    // 管理者线程
    __attribute__((unused)) pthread_t manager_id ;
     */

    // 线程池锁
    pthread_mutex_t mutex_pool;

    // 任务队列
    task_queue_t * task_queue;
    // 条件变量：任务队列是否满
    pthread_cond_t tq_is_full;
    // 条件变量：任务队列是否空
    pthread_cond_t tq_is_empty;
};

fixed_thread_pool_t * create_fixed_thread_pool(int worker_arr_cap, int task_queue_cap){
    fixed_thread_pool_t * pool = NULL; // 线程池
    task_queue_t * task_queue = NULL; // 任务队列
    pthread_t * worker_id_arr = NULL; // 工作线程数组
    TRY_BEGIN:
        // 1. 线程池 的 创建和初始化
        pool = malloc(sizeof(fixed_thread_pool_t));
        if(pool == NULL){
            fprintf(stderr, "线程池 堆内存分配失败");
            CATCH_EXCEPTION;
        }
        pool->is_shutdown = POOL_ACTIVE;
        pool->worker_arr_cap = worker_arr_cap;

        // 2. 任务队列 的 创建和初始化
        task_queue = malloc(sizeof(task_queue_t) + task_queue_cap * sizeof(task_t));
        if(task_queue == NULL){
            fprintf(stderr, "任务队列 堆内存分配失败");
            CATCH_EXCEPTION;
        }
        pool->task_queue = task_queue;

        memset(task_queue, 0, sizeof(task_queue_t));
        task_queue->task_queue_cap = task_queue_cap;

        // 3. 工作线程 的创建
        worker_id_arr = malloc(worker_arr_cap * sizeof(pthread_t));
        if(worker_id_arr == NULL){
            fprintf(stderr, "工作线程数组 堆内存分配失败");
            CATCH_EXCEPTION;
        }
        pool->worker_id_arr = worker_id_arr;

        for(int i = 0; i < worker_arr_cap; i++){
            pthread_create(&pool->worker_id_arr[i], NULL, worker, pool);
        }
        return pool;
    TRY_END;

    free(pool);
    free(task_queue);
    free(worker_id_arr);
    return NULL;
}

/* __attribute__((unused)) void * manager(void * arg){
 * } // 暂时不需要
 */

void * worker(void * arg){
    // 获取自己的id
    pthread_t self_id = pthread_self();
    // 获取线程池实例
    fixed_thread_pool_t * pool = (fixed_thread_pool_t*) arg;

    while(1) {
        // todo 1 使用线程池，加锁
        pthread_mutex_lock(&pool->mutex_pool);

        // todo 2 判断 任务队列 和 线程池 的状态
        while(pool->task_queue->task_queue_size == 0 && pool->is_shutdown == POOL_ACTIVE){
            // 用 任务队列空条件 阻塞自己
            pthread_cond_wait(&pool->tq_is_empty, &pool->mutex_pool);
        }

        // 此时队列非空，可以获取任务
        // todo 3 但是不知道阻塞期间，线程池是不是被关闭了
        if(pool->is_shutdown == POOL_SHUTDOWN){
            // 池关闭了，此时解锁, 防止死锁
            pthread_mutex_unlock(&pool->mutex_pool);
        }

        // todo 4 获取任务
        task_queue_t * task_queue = pool->task_queue;
        int queue_head = task_queue->task_queue_head;
        task_t task;
        // 取出 任务队列头节点任务
        task.func = task_queue->tasks[queue_head].func;
        task.arg = task_queue->tasks[queue_head].arg;
        // 移动 任务队列头结点，任务队列当前大小 - 1
        task_queue->task_queue_head = (queue_head + 1) % task_queue->task_queue_cap;
        --task_queue->task_queue_size;

        // todo 5 用完线程池，解锁
        pthread_mutex_unlock(&pool->mutex_pool);

        // todo 6 执行任务

        printf(INFO "线程：%ld 开始执行任务\n", self_id);

        (* task.func)(task.arg); // 管理者的任务 应该是传入的堆内存，否则不能 free
        //free(task.arg);
        //task.arg = NULL;

        printf(INFO "线程：%ld 任务执行完成\n", self_id);
    }
}

void thread_pool_task_add(fixed_thread_pool_t * pool, void* (* func)(void*), void * arg){
    printf(INFO "添加任务到线程池：\n");
    // todo 线程池加锁
    pthread_mutex_lock(&pool->mutex_pool);
    task_queue_t * tq = pool->task_queue;
    // todo 判断 任务队列 和 线程池 的状态
    while (tq->task_queue_size == tq->task_queue_cap && pool->is_shutdown == POOL_ACTIVE){
        pthread_cond_wait(&pool->tq_is_full, &pool->mutex_pool);
    }
    // 任务队列有空余，再看看是不是线程池已经关闭
    if(pool->is_shutdown){
        pthread_mutex_unlock(&pool->mutex_pool);
        return;
    }
    // todo 添加任务
    // 访问队尾，移动尾指针，任务队列个数修改
    task_t * tasks = tq->tasks;
    tasks[tq->task_queue_tail].func = func;
    tasks[tq->task_queue_tail].arg = arg;
    tq->task_queue_tail = (tq->task_queue_tail + 1) % tq->task_queue_cap;
    tq->task_queue_size ++;

    // todo 唤醒阻塞的消费者
    // 当任务队列为空， worker线程 使用 任务队列空条件 阻塞自己
    // 所以需要唤醒
    pthread_cond_signal(&pool->tq_is_empty);
    // todo 任务添加完毕，解锁线程池
    pthread_mutex_unlock(&pool->mutex_pool);
}

void pool_shutdown(fixed_thread_pool_t * pool){
    pool->is_shutdown = POOL_SHUTDOWN;
}