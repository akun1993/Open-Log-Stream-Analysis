
 #ifndef __OLS_TASK_H__
 #define __OLS_TASK_H__
 
#include "util/task.h"
#include "util/threading.h"
 
#ifdef __cplusplus
extern "C" {
#endif
 /**
  * ols_task_function:
  * @user_data: user data passed to the function
  *
  * A function that will repeatedly be called in the thread created by
  * a #ols_task.
  */
 typedef void (*ols_task_function)(void * user_data);
 
 typedef struct ols_task  ols_task_t;

 /* --- standard type macros --- */

 #define OLS_TASK_CAST(task)             ((ols_task_t*)(task))

 /**
  * OlsTaskState:
  * @OLS_TASK_STARTED: the task is started and running
  * @OLS_TASK_STOPPED: the task is stopped
  * @OLS_TASK_PAUSED: the task is paused
  *
  * The different states a task can be in
  */
 typedef enum {
   OLS_TASK_STARTED,
   OLS_TASK_STOPPED,
   OLS_TASK_PAUSED
 } OlsTaskState;
 
 /**
  * OLS_TASK_STATE:
  * @task: Task to get the state of
  *
  * Get access to the state of the task.
  */
 #define OLS_TASK_STATE(task)            (OLS_TASK_CAST(task)->state)
 
 /**
  * OLS_TASK_GET_COND:
  * @task: Task to get the cond of
  *
  * Get access to the cond of the task.
  */
 #define OLS_TASK_GET_COND(task)         (&OLS_TASK_CAST(task)->cond)


#define OLS_TASK_GET_LOCK(task) (&OLS_TASK_CAST(task)->tsk_lck)

 /**
  * OLS_TASK_WAIT:
  * @task: Task to wait for
  *
  * Wait for the task cond to be signalled
  */
 #define OLS_TASK_WAIT(task)            (pthread_cond_wait(OLS_TASK_GET_COND (task), OLS_TASK_GET_LOCK (task))) 
 /**
  * OLS_TASK_SIGNAL:
  * @task: Task to signal
  *
  * Signal the task cond
  */
 #define OLS_TASK_SIGNAL(task)           (pthread_cond_signal(OLS_TASK_GET_COND (task)))
 /**
  * OLS_TASK_BROADCAST:
  * @task: Task to broadcast
  *
  * Send a broadcast signal to all waiting task conds
  */
 #define OLS_TASK_BROADCAST(task)        (pthread_cond_broadcast(OLS_TASK_GET_COND (task)))
 
 /**
  * OLS_TASK_GET_LOCK:
  * @task: Task to get the lock of
  *
  * Get access to the task lock.
  */
 #define OLS_TASK_GET_REC_LOCK(task)         (OLS_TASK_CAST(task)->rec_lock)
 
 /**
  * GstTaskThreadFunc:
  * @task: The #GstTask
  * @thread: The #GThread
  * @user_data: user data
  *
  * Custom GstTask thread callback functions that can be installed.
  */
 typedef void (*ols_task_thread_func) (ols_task_function *task, void *user_data);
 
 /**
  * GstTask:
  * @state: the state of the task
  * @cond: used to pause/resume the task
  * @lock: The lock taken when iterating the task function
  * @func: the function executed by this task
  * @user_data: user_data passed to the task function
  * @notify: GDestroyNotify for @user_data
  * @running: a flag indicating that the task is running
  *
  */

struct ols_task {
   /*< public >*/ /* with LOCK */

   pthread_mutex_t  tsk_lck;
   os_task_queue_t * os_task;

   OlsTaskState      state;
   pthread_cond_t    cond;
   pthread_mutex_t  *rec_lock;
   ols_task_function  func;
   void *             user_data;
   bool               running;
 };
 
 
 ols_task_t*  ols_task_new (ols_task_function func,void * user_data);
 void         ols_task_set_lock       (ols_task_t *task, pthread_mutex_t *mutex);

 OlsTaskState ols_task_get_state      (ols_task_t *task);
 bool        ols_task_set_state      (ols_task_t *task, OlsTaskState state);
 bool        ols_task_start          (ols_task_t *task);
 bool        ols_task_stop           (ols_task_t *task);
 bool        ols_task_pause          (ols_task_t *task);
 
 bool        ols_task_join           (ols_task_t *task);
#ifdef __cplusplus
}
#endif
 
#endif /* __OLS_TASK_H__ */
 