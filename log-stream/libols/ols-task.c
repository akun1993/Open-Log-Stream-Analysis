

/**
 * SECTION:gsttask
 * @title: GstTask
 * @short_description: Abstraction of GStreamer streaming threads.
 * @see_also: #GstElement, #GstPad
 *
 * #GstTask is used by #GstElement and #GstPad to provide the data passing
 * threads in a #GstPipeline.
 *
 * A #GstPad will typically start a #GstTask to push or pull data to/from the
 * peer pads. Most source elements start a #GstTask to push data. In some cases
 * a demuxer element can start a #GstTask to pull data from a peer element. This
 * is typically done when the demuxer can perform random access on the upstream
 * peer element for improved performance.
 *
 * Although convenience functions exist on #GstPad to start/pause/stop tasks, it
 * might sometimes be needed to create a #GstTask manually if it is not related
 * to a #GstPad.
 *
 * Before the #GstTask can be run, it needs a #GRecMutex that can be set with
 * ols_task_set_lock().
 *
 * The task can be started, paused and stopped with ols_task_start(),
 * ols_task_pause() and ols_task_stop() respectively or with the
 * ols_task_set_state() function.
 *
 * A #GstTask will repeatedly call the #GstTaskFunction with the user data
 * that was provided when creating the task with ols_task_new(). While calling
 * the function it will acquire the provided lock. The provided lock is released
 * when the task pauses or stops.
 *
 * Stopping a task with ols_task_stop() will not immediately make sure the task
 * is not running anymore. Use ols_task_join() to make sure the task is
 * completely stopped and the thread is stopped.
 *
 * After creating a #GstTask, use ols_object_unref() to free its resources. This
 * can only be done when the task is not running anymore.
 *
 * Task functions can send a #GstMessage to send out-of-band data to the
 * application. The application can receive messages from the #GstBus in its
 * mainloop.
 *
 * For debugging purposes, the task will configure its object name as the thread
 * name on Linux. Please note that the object name should be configured before
 * the task is started; changing the object name after the task has been
 * started, has no effect on the thread name.
 */

#include "ols-task.h"

#include <stdio.h>

#include "util/bmem.h"

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#define OLS_TASK_LOCK(t) (pthread_mutex_lock(OLS_TASK_GET_LOCK(t)))
#define OLS_TASK_UNLOCK(t) (pthread_mutex_unlock(OLS_TASK_GET_LOCK(t)))
#define SET_TASK_STATE(t, s) \
  (os_atomic_set_long((volatile long *)&OLS_TASK_STATE(t), (s)))
#define GET_TASK_STATE(t) \
  ((OlsTaskState)os_atomic_load_long((volatile long *)&OLS_TASK_STATE(t)))

static void ols_task_func(ols_task_t *task);

static void ols_task_init(ols_task_t *task) {
  // task->priv = OLS_TASK_GET_PRIVATE (task);
  task->running = false;
  // task->thread = NULL;
  task->rec_lock = NULL;
  pthread_cond_init(&task->cond, NULL);
  pthread_mutex_init(&task->tsk_lck, NULL);
  SET_TASK_STATE(task, OLS_TASK_STOPPED);
  task->os_task = os_task_queue_create();
  /* use the default klass pool for this task, users can
   * override this later */
  //  g_mutex_lock (&pool_lock);

  //  g_mutex_unlock (&pool_lock);

  /* clear floating flag */
  //  ols_object_ref_sink (task);
}

static void ols_task_func(ols_task_t *task) {
  pthread_mutex_t *lock;
  //  GThread *tself;
  //  GstTaskPrivate *priv;

  //  priv = task->priv;

  //  tself = g_thread_self ();

  // OLS_DEBUG ("Entering task %p, thread %p", task, tself);

  /* we have to grab the lock to get the mutex. We also
   * mark our state running so that nobody can mess with
   * the mutex. */

  OLS_TASK_LOCK(task);
  if (GET_TASK_STATE(task) == OLS_TASK_STOPPED) goto exit;
  lock = OLS_TASK_GET_REC_LOCK(task);

  if (lock == NULL) goto no_lock;

  OLS_TASK_UNLOCK(task);

  /* fire the enter_func callback when we need to */
  /* locking order is TASK_LOCK, LOCK */
  pthread_mutex_lock(lock);
  /* configure the thread name now */
  // ols_task_configure_name (task);

  while ((GET_TASK_STATE(task) != OLS_TASK_STOPPED)) {
    OLS_TASK_LOCK(task);
    while ((OLS_TASK_STATE(task) == OLS_TASK_PAUSED)) {
      pthread_mutex_unlock(lock);

      OLS_TASK_SIGNAL(task);
      // OLS_INFO_OBJECT (task, "Task going to paused");
      OLS_TASK_WAIT(task);
      // OLS_INFO_OBJECT (task, "Task resume from paused");
      OLS_TASK_UNLOCK(task);
      /* locking order.. */
      pthread_mutex_lock(lock);
      OLS_TASK_LOCK(task);
    }

    if ((GET_TASK_STATE(task) == OLS_TASK_STOPPED)) {
      OLS_TASK_UNLOCK(task);
      break;
    } else {
      OLS_TASK_UNLOCK(task);
    }

    task->func(task->user_data);
  }
  pthread_mutex_unlock(lock);

  OLS_TASK_LOCK(task);
  // task->thread = NULL;

exit:
  // if (priv->leave_func) {
  /* fire the leave_func callback when we need to. We need to do this before
   * we signal the task and with the task lock released. */
  // OLS_OBJECT_UNLOCK (task);
  // priv->leave_func (task, tself, priv->leave_user_data);
  // OLS_OBJECT_LOCK (task);
  //}
  /* now we allow messing with the lock again by setting the running flag to
   * %FALSE. Together with the SIGNAL this is the sign for the _join() to
   * complete.
   * Note that we still have not dropped the final ref on the task. We could
   * check here if there is a pending join() going on and drop the last ref
   * before releasing the lock as we can be sure that a ref is held by the
   * caller of the join(). */
  task->running = false;
  OLS_TASK_SIGNAL(task);
  OLS_TASK_UNLOCK(task);

  // OLS_DEBUG ("Exit task %p, thread %p", task, g_thread_self ());

  // ols_object_unref (task);
  return;

no_lock: {
  // g_warning ("starting task without a lock");
  goto exit;
}
}

/**
 * ols_task_new:
 * @func: The #GstTaskFunction to use
 * @user_data: User data to pass to @func
 * @notify: the function to call when @user_data is no longer needed.
 *
 * Create a new Task that will repeatedly call the provided @func
 * with @user_data as a parameter. Typically the task will run in
 * a new thread.
 *
 * The function cannot be changed after the task has been created. You
 * must create a new #GstTask to change the function.
 *
 * This function will not yet create and start a thread. Use ols_task_start() or
 * ols_task_pause() to create and start the GThread.
 *
 * Before the task can be used, a #GRecMutex must be configured using the
 * ols_task_set_lock() function. This lock will always be acquired while
 * @func is called.
 *
 * Returns: (transfer full): A new #GstTask.
 *
 * MT safe.
 */
ols_task_t *ols_task_new(ols_task_function func, void *user_data) {
  ols_task_t *task;

  task = (ols_task_t *)bmalloc(sizeof(ols_task_t));
  ols_task_init(task);

  task->func = func;
  task->user_data = user_data;
  // OLS_DEBUG ("Created task %p", task);

  return task;
}

/**
 * ols_task_set_lock:
 * @task: The #GstTask to use
 * @mutex: The #GRecMutex to use
 *
 * Set the mutex used by the task. The mutex will be acquired before
 * calling the #GstTaskFunction.
 *
 * This function has to be called before calling ols_task_pause() or
 * ols_task_start().
 *
 * MT safe.
 */
void ols_task_set_lock(ols_task_t *task, pthread_mutex_t *mutex) {
  // g_return_if_fail (OLS_IS_TASK (task));

  OLS_TASK_LOCK(task);
  if (task->running) goto is_running;
  // OLS_INFO ("setting stream lock %p on task %p", mutex, task);
  OLS_TASK_GET_REC_LOCK(task) = mutex;
  OLS_TASK_UNLOCK(task);

  return;

  /* ERRORS */
is_running: {
  OLS_TASK_UNLOCK(task);
  // g_warning ("cannot call set_lock on a running task");
}
}

/**
 * ols_task_get_state:
 * @task: The #GstTask to query
 *
 * Get the current state of the task.
 *
 * Returns: The #GstTaskState of the task
 *
 * MT safe.
 */
OlsTaskState ols_task_get_state(ols_task_t *task) {
  OlsTaskState result;

  // g_return_val_if_fail (OLS_IS_TASK (task), OLS_TASK_STOPPED);

  result = GET_TASK_STATE(task);

  return result;
}

/* make sure the task is running and start a thread if it's not.
 * This function must be called with the task LOCK. */
static bool start_task(ols_task_t *task) {
  bool res = true;
  //  GError *error = NULL;
  //  GstTaskPrivate *priv;

  // priv = task->priv;

  /* new task, We ref before so that it remains alive while
   * the thread is running. */
  // ols_object_ref (task);
  /* mark task as running so that a join will wait until we schedule
   * and exit the task function. */
  task->running = true;

  os_task_queue_queue_task(task->os_task, (os_task_t)ols_task_func, task);

  /* push on the thread pool, we remember the original pool because the user
   * could change it later on and then we join to the wrong pool. */
  //  priv->pool_id = ols_object_ref (priv->pool);
  //  priv->id =
  //      ols_task_pool_push (priv->pool_id, (GstTaskPoolFunction)
  //      ols_task_func, task, &error);

  // if (error != NULL) {
  //  g_warning ("failed to create thread: %s", error->message);
  //  g_error_free (error);
  //  res = false;
  //}
  return res;
}

/**
 * ols_task_set_state:
 * @task: a #GstTask
 * @state: the new task state
 *
 * Sets the state of @task to @state.
 *
 * The @task must have a lock associated with it using
 * ols_task_set_lock() when going to OLS_TASK_STARTED or OLS_TASK_PAUSED or
 * this function will return %FALSE.
 *
 * MT safe.
 *
 * Returns: %TRUE if the state could be changed.
 */
bool ols_task_set_state(ols_task_t *task, OlsTaskState state) {
  OlsTaskState old;
  bool res = true;

  OLS_TASK_LOCK(task);
  if (state != OLS_TASK_STOPPED)
    if ((OLS_TASK_GET_REC_LOCK(task) == NULL)) goto no_lock;

  /* if the state changed, do our thing */
  old = GET_TASK_STATE(task);
  if (old != state) {
    SET_TASK_STATE(task, state);
    switch (old) {
      case OLS_TASK_STOPPED:
        /* If the task already has a thread scheduled we don't have to do
         * anything. */
        if ((!task->running)) res = start_task(task);
        break;
      case OLS_TASK_PAUSED:
        /* when we are paused, signal to go to the new state */
        OLS_TASK_SIGNAL(task);
        break;
      case OLS_TASK_STARTED:
        /* if we were started, we'll go to the new state after the next
         * iteration. */
        break;
    }
  }
  OLS_TASK_UNLOCK(task);

  return res;

  /* ERRORS */
no_lock: {
  // OLS_WARNING_OBJECT (task, "state %d set on task without a lock", state);
  OLS_TASK_UNLOCK(task);
  // g_warning ("task without a lock can't be set to state %d", state);
  return false;
}
}

/**
 * ols_task_start:
 * @task: The #GstTask to start
 *
 * Starts @task. The @task must have a lock associated with it using
 * ols_task_set_lock() or this function will return %FALSE.
 *
 * Returns: %TRUE if the task could be started.
 *
 * MT safe.
 */
bool ols_task_start(ols_task_t *task) {
  return ols_task_set_state(task, OLS_TASK_STARTED);
}

/**
 * ols_task_stop:
 * @task: The #GstTask to stop
 *
 * Stops @task. This method merely schedules the task to stop and
 * will not wait for the task to have completely stopped. Use
 * ols_task_join() to stop and wait for completion.
 *
 * Returns: %TRUE if the task could be stopped.
 *
 * MT safe.
 */
bool ols_task_stop(ols_task_t *task) {
  return ols_task_set_state(task, OLS_TASK_STOPPED);
}

/**
 * ols_task_pause:
 * @task: The #GstTask to pause
 *
 * Pauses @task. This method can also be called on a task in the
 * stopped state, in which case a thread will be started and will remain
 * in the paused state. This function does not wait for the task to complete
 * the paused state.
 *
 * Returns: %TRUE if the task could be paused.
 *
 * MT safe.
 */
bool ols_task_pause(ols_task_t *task) {
  return ols_task_set_state(task, OLS_TASK_PAUSED);
}

/**
 * ols_task_join:
 * @task: The #GstTask to join
 *
 * Joins @task. After this call, it is safe to unref the task
 * and clean up the lock set with ols_task_set_lock().
 *
 * The task will automatically be stopped with this call.
 *
 * This function cannot be called from within a task function as this
 * would cause a deadlock. The function will detect this and print a
 * g_warning.
 *
 * Returns: %TRUE if the task could be joined.
 *
 * MT safe.
 */
bool ols_task_join(ols_task_t *task) {
  // g_return_val_if_fail (OLS_IS_TASK (task), FALSE);

  /* we don't use a real thread join here because we are using
   * thread pools */
  OLS_TASK_LOCK(task);

  SET_TASK_STATE(task, OLS_TASK_STOPPED);
  /* signal the state change for when it was blocked in PAUSED. */
  OLS_TASK_SIGNAL(task);
  /* we set the running flag when pushing the task on the thread pool.
   * This means that the task function might not be called when we try
   * to join it here. */
  while (task->running) OLS_TASK_WAIT(task);
  /* clean the thread */
  // task->thread = NULL;
  /* get the id and pool to join */
  //    pool = priv->pool_id;
  //    id = priv->id;
  // priv->pool_id = NULL;
  // priv->id = NULL;
  OLS_TASK_UNLOCK(task);

  //    if (pool) {
  //      if (id)
  //        ols_task_pool_join (pool, id);
  //      ols_object_unref (pool);
  //    }

  // OLS_DEBUG_OBJECT (task, "Joined task %p", task);

  return true;

  /* ERRORS */
  // joining_self:
  //{
  //   OLS_WARNING_OBJECT (task, "trying to join task from its thread");
  //   OLS_OBJECT_UNLOCK (task);
  //   g_warning ("\nTrying to join task %p from its thread would deadlock.\n"
  //       "You cannot change the state of an element from its streaming\n"
  //       "thread. Use g_idle_add() or post a GstMessage on the bus to\n"
  //       "schedule the state change from the main thread.\n", task);
  // return false;
  //}
}
