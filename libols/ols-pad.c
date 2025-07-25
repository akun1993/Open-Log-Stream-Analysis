#include "ols-pad.h"

#include <stdbool.h>

#include "ols-internal.h"

static OlsFlowReturn ols_pad_send_event_unchecked(ols_pad_t *pad,
                                                  ols_event_t *event,
                                                  OlsPadProbeType type);
static OlsFlowReturn ols_pad_push_event_unchecked(ols_pad_t *pad,
                                                  ols_event_t *event,
                                                  OlsPadProbeType type);

static OlsFlowReturn ols_pad_chain_list_default(ols_pad_t *pad,
                                                ols_object_t *parent,
                                                ols_buffer_list_t *list);

static bool ols_pad_event_default(ols_pad_t *pad, ols_object_t *parent,
                                  ols_event_t *event);

#define ACQUIRE_PARENT(pad, parent, label)                                     \
  do {                                                                         \
    if ((parent = OLS_PAD_PARENT(pad)))                                        \
      ols_object_get_ref(parent);                                              \
    else                                                                       \
      goto label;                                                              \
  } while (0)

#define RELEASE_PARENT(parent)                                                 \
  do {                                                                         \
    if (parent)                                                                \
      ols_object_release(parent);                                              \
  } while (0)

#define OLS_PAD_CAST(obj) ((ols_pad_t *)(obj))

/**
 * ols_pad_event_default:
 * @pad: a #GstPad to call the default event handler on.
 * @parent: (allow-none): the parent of @pad or %NULL
 * @event: (transfer full): the #GstEvent to handle.
 *
 * Invokes the default event handler for the given pad.
 *
 * The EOS event will pause the task associated with @pad before it is forwarded
 * to all internally linked pads,
 *
 * The event is sent to all pads internally linked to @pad. This function
 * takes ownership of @event.
 *
 * Returns: %TRUE if the event was sent successfully.
 */
static bool ols_pad_event_default(ols_pad_t *pad, ols_object_t *parent,
                                  ols_event_t *event) {
  bool result = true;
  UNUSED_PARAMETER(pad);
  UNUSED_PARAMETER(parent);
  UNUSED_PARAMETER(event);
  // g_return_val_if_fail(GST_IS_PAD(pad), FALSE);
  // g_return_val_if_fail(event != NULL, FALSE);

  // GST_LOG_OBJECT(pad, "default event handler for event %" GST_PTR_FORMAT,
  //                event);

  // switch (GST_EVENT_TYPE(event)) {
  // case GST_EVENT_CAPS:
  //   forward = GST_PAD_IS_PROXY_CAPS(pad);
  //   result = TRUE;
  //   break;
  // default:
  //   break;
  // }

  // if (forward) {
  //   EventData data;

  //   data.event = event;
  //   data.dispatched = FALSE;
  //   data.result = FALSE;

  //   gst_pad_forward(pad, (GstPadForwardFunction)event_forward_func, &data);

  //   /* for sinkpads without a parent element or without internal links,
  //   nothing
  //    * will be dispatched but we still want to return TRUE. */
  //   if (data.dispatched)
  //     result = data.result;
  //   else
  //     result = TRUE;
  // }

  // gst_event_unref(event);

  return result;
}

static void ols_pad_init(ols_pad_t *pad,const char *name) {
  // pad->priv = ols_pad_get_instance_private(pad);

  OLS_PAD_NAME(pad) =  bstrdup(name);

  OLS_PAD_DIRECTION(pad) = OLS_PAD_UNKNOWN;

  OLS_PAD_EVENTFUNC(pad) = ols_pad_event_default;

  OLS_PAD_CHAINLISTFUNC(pad) = ols_pad_chain_list_default;

  // OLS_PAD_LINKFUNC(pad) =

  OLS_PAD_PEER(pad) = NULL;

  OLS_PAD_TASK(pad) = NULL;

  // g_rec_mutex_init(&pad->stream_rec_lock);

  pthread_cond_init(&pad->block_cond, NULL);

  pthread_mutex_init(&pad->mutex, NULL);

  pthread_mutex_init_recursive(&pad->stream_rec_lock);

  // g_hook_list_init(&pad->probes, sizeof(OLSProbe));

  // pad->priv->events = g_array_sized_new(FALSE, TRUE, sizeof(PadEvent), 16);
  // pad->priv->events_cookie = 0;
  // pad->priv->last_cookie = -1;
  // g_cond_init(&pad->priv->activation_cond);
}

bool ols_pad_set_parent(ols_pad_t *pad, ols_object_t *parent) {
  OLS_PAD_LOCK(pad);
  if ((pad->parent != NULL))
    goto had_parent;

  pad->parent = parent;

  blog(LOG_DEBUG, "pad's  parent is set to %p", parent);

  OLS_PAD_UNLOCK(pad);

  return true;

  /* ERROR handling */
had_parent: {
  blog(LOG_ERROR, "pad has parent already");
  OLS_PAD_UNLOCK(pad);
  return false;
}
}

ols_object_t *ols_pad_get_parent(ols_pad_t *pad) {
  ols_object_t *result = NULL;
  OLS_PAD_LOCK(pad);
  result = pad->parent;
  // if (G_LIKELY (result))
  //   gst_object_ref (result);
  OLS_PAD_UNLOCK(pad);

  return result;
}

/**
 * ols_pad_new:
 * @name: (allow-none): the name of the new pad.
 * @direction: the #GstPadDirection of the pad.
 *
 * Creates a new pad with the given name in the given direction.
 * If name is %NULL, a guaranteed unique name (across all pads)
 * will be assigned.
 * This function makes a copy of the name so you can safely free the name.
 *
 * Returns: (transfer floating) (nullable): a new #GstPad, or %NULL in
 * case of an error.
 *
 * MT safe.
 */
ols_pad_t *ols_pad_new(const char *name, OLSPadDirection direction) {
  ols_pad_t *pad = (ols_pad_t *)bzalloc(sizeof(ols_pad_t));

  if (pad) {
    ols_pad_init(pad,name);
    pad->direction = direction;
  }

  return pad;
}

void ols_pad_destory( ols_pad_t *pad) {

  if(!pad){
    return;
  }

  bfree(pad->name);

  pthread_cond_destroy(&pad->block_cond);
  
  pthread_mutex_destroy(&pad->mutex);


  pthread_mutex_destroy(&pad->stream_rec_lock);

  bfree(pad);

}

bool ols_pad_start_task(ols_pad_t *pad, os_task_t func, void *user_data) {
  ols_task_t *task;
  bool res;

  OLS_PAD_LOCK(pad);
  task = OLS_PAD_TASK(pad);
  if (task == NULL) {
    task = ols_task_new(func, user_data);
    ols_task_set_lock(task, OLS_PAD_GET_STREAM_LOCK(pad));

    OLS_PAD_TASK(pad) = task;

    /* release lock to post the message */
    OLS_PAD_UNLOCK(pad);

    OLS_PAD_LOCK(pad);
    /* nobody else is supposed to have changed the pad now */
    if (OLS_PAD_TASK(pad) != task)
      goto concurrent_stop;
  }

  res = ols_task_set_state(task, OLS_TASK_STARTED);

  OLS_PAD_UNLOCK(pad);

  return res;
  /* ERRORS */
concurrent_stop: {
  OLS_PAD_UNLOCK(pad);
  return true;
}
}

bool ols_pad_pause_task(ols_pad_t *pad) {
  ols_task_t *task;
  bool res;

  OLS_PAD_LOCK(pad);
  task = OLS_PAD_TASK(pad);
  if (task == NULL)
    goto no_task;

  res = ols_task_set_state(task, OLS_TASK_PAUSED);
  /* unblock activation waits if any */
  // pad->priv->in_activation = FALSE;
  // g_cond_broadcast (&pad->priv->activation_cond);
  OLS_PAD_UNLOCK(pad);

  /* wait for task function to finish, this lock is recursive so it does nothing
   * when the pause is called from the task itself */

  OLS_PAD_STREAM_LOCK(pad);
  OLS_PAD_STREAM_UNLOCK(pad);

  return res;

no_task: {
  blog(LOG_DEBUG, "pad[%p] pad has no task", pad);
  OLS_PAD_UNLOCK(pad);
  return false;
}
}

bool ols_pad_stop_task(ols_pad_t *pad) {
  ols_task_t *task;
  bool res;

  OLS_PAD_LOCK(pad);
  task = OLS_PAD_TASK(pad);
  if (task == NULL)
    goto no_task;
  OLS_PAD_TASK(pad) = NULL;
  res = ols_task_set_state(task, OLS_TASK_STOPPED);
  /* unblock activation waits if any */
  // pad->priv->in_activation = false;
  // g_cond_broadcast (&pad->priv->activation_cond);
  OLS_PAD_UNLOCK(pad);
  OLS_PAD_STREAM_LOCK(pad);
  OLS_PAD_STREAM_UNLOCK(pad);

  if (!ols_task_join(task))
    goto join_failed;

  // gst_object_unref (task);

  return res;

no_task: {
  // GST_DEBUG_OBJECT (pad, "no task");
  OLS_PAD_UNLOCK(pad);

  OLS_PAD_STREAM_LOCK(pad);
  OLS_PAD_STREAM_UNLOCK(pad);

  /* this is not an error */
  return true;
}
join_failed: {
  /* this is bad, possibly the application tried to join the task from
   * the task's thread. We install the task again so that it will be stopped
   * again from the right thread next time hopefully. */
  OLS_PAD_LOCK(pad);
  // GST_DEBUG_OBJECT (pad, "join failed");
  /* we can only install this task if there was no other task */
  if (OLS_PAD_TASK(pad) == NULL)
    OLS_PAD_TASK(pad) = task;
  OLS_PAD_UNLOCK(pad);

  return false;
}
}

bool pad_link_maybe_ghosting(ols_pad_t *src, ols_pad_t *sink) {
  // GSList *pads_created = NULL;
  
  
  bool ret;

  // if (!prepare_link_maybe_ghosting(&src, &sink, &pads_created)) {
  //   ret = false;
  // } else {
  ret = (ols_pad_link_full(src, sink) == OLS_PAD_LINK_OK);

  if (!ret) {
    blog(LOG_DEBUG, "pad link failed");
  }
  // g_slist_free(pads_created);

  return ret;
}

/**
 * ols_pad_set_chain_function:
 * @p: a sink #OlsPad.
 * @f: the #OlsPadChainFunction to set.
 *
 * Calls ols_pad_set_chain_function_full() with %NULL for the user_data and
 * notify.
 */
/**
 * ols_pad_set_chain_function_full:
 * @pad: a sink #OlsPad.
 * @chain: the #OlsPadChainFunction to set.
 * @user_data: user_data passed to @notify
 * @notify: notify called when @chain will not be used anymore.
 *
 * Sets the given chain function for the pad. The chain function is called to
 * process a #OlsBuffer input buffer. see #OlsPadChainFunction for more details.
 */
void ols_pad_set_chain_function_full(ols_pad_t *pad,
                                     ols_pad_chain_function chain,
                                     void *user_data) {
  OLS_PAD_CHAINFUNC(pad) = chain;
  pad->chaindata = user_data;

  // OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "chainfunc set to %s",
  //                      OLS_DEBUG_FUNCPTR_NAME(chain));
}

/**
 * ols_pad_set_chain_list_function:
 * @p: a sink #OlsPad.
 * @f: the #OlsPadChainListFunction to set.
 *
 * Calls ols_pad_set_chain_list_function_full() with %NULL for the user_data and
 * notify.
 */
/**
 * ols_pad_set_chain_list_function_full:
 * @pad: a sink #OlsPad.
 * @chainlist: the #OlsPadChainListFunction to set.
 * @user_data: user_data passed to @notify
 * @notify: notify called when @chainlist will not be used anymore.
 *
 * Sets the given chain list function for the pad. The chainlist function is
 * called to process a #OlsBufferList input buffer list. See
 * #OlsPadChainListFunction for more details.
 */
void ols_pad_set_chain_list_function_full(ols_pad_t *pad,
                                          ols_pad_chain_list_function chainlist,
                                          void *user_data) {
  OLS_PAD_CHAINLISTFUNC(pad) = chainlist;
  pad->chaindata = user_data;

  // OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "chainlistfunc set to %s",
  //                      OLS_DEBUG_FUNCPTR_NAME(chainlist));
}

/**
 * ols_pad_set_link_function:
 * @p: a #OlsPad.
 * @f: the #OlsPadLinkFunction to set.
 *
 * Calls ols_pad_set_link_function_full() with %NULL
 * for the user_data and notify.
 */
/**
 * ols_pad_set_link_function_full:
 * @pad: a #OlsPad.
 * @link: the #OlsPadLinkFunction to set.
 * @user_data: user_data passed to @notify
 * @notify: notify called when @link will not be used anymore.
 *
 * Sets the given link function for the pad. It will be called when
 * the pad is linked with another pad.
 *
 * The return value #OLS_PAD_LINK_OK should be used when the connection can be
 * made.
 *
 * The return value #OLS_PAD_LINK_REFUSED should be used when the connection
 * cannot be made for some reason.
 *
 * If @link is installed on a source pad, it should call the #OlsPadLinkFunction
 * of the peer sink pad, if present.
 */
void ols_pad_set_link_function_full(ols_pad_t *pad, ols_pad_link_function link,
                                    void *user_data) {
  //   g_return_if_fail(OLS_IS_PAD(pad));

  //   if (pad->linknotify)
  //     pad->linknotify(pad->linkdata);
  OLS_PAD_LINKFUNC(pad) = link;
  pad->linkdata = user_data;
  //   OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "linkfunc set to %s",
  //                        OLS_DEBUG_FUNCPTR_NAME(link));
  //
}

/**
 * ols_pad_set_unlink_function:
 * @p: a #OlsPad.
 * @f: the #OlsPadUnlinkFunction to set.
 *
 * Calls ols_pad_set_unlink_function_full() with %NULL
 * for the user_data and notify.
 */
/**
 * ols_pad_set_unlink_function_full:
 * @pad: a #OlsPad.
 * @unlink: the #OlsPadUnlinkFunction to set.
 * @user_data: user_data passed to @notify
 * @notify: notify called when @unlink will not be used anymore.
 *
 * Sets the given unlink function for the pad. It will be called
 * when the pad is unlinked.
 *
 * Note that the pad's lock is already held when the unlink
 * function is called, so most pad functions cannot be called
 * from within the callback.
 */
void ols_pad_set_unlink_function_full(ols_pad_t *pad,
                                      ols_pad_unlink_function unlink,
                                      void *user_data) {
  // g_return_if_fail(OLS_IS_PAD(pad));

  // if (pad->unlinknotify)
  //   pad->unlinknotify(pad->unlinkdata);
  OLS_PAD_UNLINKFUNC(pad) = unlink;
  pad->unlinkdata = user_data;
  // pad->unlinknotify = notify;

  // OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "unlinkfunc set to %s",
  //                      OLS_DEBUG_FUNCPTR_NAME(unlink));
}

/**
 * ols_pad_unlink:
 * @srcpad: the source #OlsPad to unlink.
 * @sinkpad: the sink #OlsPad to unlink.
 *
 * Unlinks the source pad from the sink pad. Will emit the #OlsPad::unlinked
 * signal on both pads.
 *
 * Returns: %TRUE if the pads were unlinked. This function returns %FALSE if
 * the pads were not linked together.
 *
 * MT safe.
 */
bool ols_pad_unlink(ols_pad_t *srcpad, ols_pad_t *sinkpad) {
  bool result = false;
  // OLSElement *parent = NULL;

  // OLS_TRACER_PAD_UNLINK_PRE(srcpad, sinkpad);

  // OLS_CAT_INFO(OLS_CAT_ELEMENT_PADS, "unlinking %s:%s(%p) and %s:%s(%p)",
  //              OLS_DEBUG_PAD_NAME(srcpad), srcpad,
  //              OLS_DEBUG_PAD_NAME(sinkpad), sinkpad);

  /* We need to notify the parent before taking any pad locks as the bin in
   * question might be waiting for a lock on the pad while holding its lock
   * that our message will try to take. */
  // if ((parent = OLS_ELEMENT_CAST(ols_pad_get_parent(srcpad)))) {
  //   if (OLS_IS_ELEMENT(parent)) {
  //     ols_element_post_message(parent, ols_message_new_structure_change(
  //                                          OLS_OBJECT_CAST(sinkpad),
  //                                          OLS_STRUCTURE_CHANGE_TYPE_PAD_UNLINK,
  //                                          parent, TRUE));
  //   } else {
  //     ols_object_unref(parent);
  //     parent = NULL;
  //   }
  // }

  OLS_PAD_LOCK(srcpad);
  OLS_PAD_LOCK(sinkpad);

  if (OLS_PAD_PEER(srcpad) != sinkpad)
    goto not_linked_together;

  if (OLS_PAD_UNLINKFUNC(srcpad)) {
    ols_object_t *tmpparent;

    ACQUIRE_PARENT(srcpad, tmpparent, no_src_parent);

    OLS_PAD_UNLINKFUNC(srcpad)(srcpad, tmpparent);
    RELEASE_PARENT(tmpparent);
  }
no_src_parent:
  if (OLS_PAD_UNLINKFUNC(sinkpad)) {
    ols_object_t *tmpparent;

    ACQUIRE_PARENT(sinkpad, tmpparent, no_sink_parent);

    OLS_PAD_UNLINKFUNC(sinkpad)(sinkpad, tmpparent);
    RELEASE_PARENT(tmpparent);
  }
no_sink_parent:

  /* first clear peers */
  OLS_PAD_PEER(srcpad) = NULL;
  OLS_PAD_PEER(sinkpad) = NULL;

  OLS_PAD_UNLOCK(sinkpad);
  OLS_PAD_UNLOCK(srcpad);

  /* fire off a signal to each of the pads telling them
   * that they've been unlinked */
  // g_signal_emit(srcpad, ols_pad_signals[PAD_UNLINKED], 0, sinkpad);
  // g_signal_emit(sinkpad, ols_pad_signals[PAD_UNLINKED], 0, srcpad);

  // OLS_CAT_INFO(OLS_CAT_ELEMENT_PADS, "unlinked %s:%s and %s:%s",
  //              OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad));

  result = true;

done:
  // if (parent != NULL) {
  // ols_element_post_message(parent, ols_message_new_structure_change(
  //                                      OLS_OBJECT_CAST(sinkpad),
  //                                      OLS_STRUCTURE_CHANGE_TYPE_PAD_UNLINK,
  //                                      parent, FALSE));
  // ols_object_unref(parent);
  //}
  // OLS_TRACER_PAD_UNLINK_POST(srcpad, sinkpad, result);
  return result;

  /* ERRORS */
not_linked_together: {
  /* we do not emit a warning in this case because unlinking cannot
   * be made MT safe.*/
  OLS_PAD_UNLOCK(sinkpad);
  OLS_PAD_UNLOCK(srcpad);
  goto done;
}
}

/**
 * ols_pad_is_linked:
 * @pad: pad to check
 *
 * Checks if a @pad is linked to another pad or not.
 *
 * Returns: %TRUE if the pad is linked, %FALSE otherwise.
 *
 * MT safe.
 */
bool ols_pad_is_linked(ols_pad_t *pad) {
  bool result = false;
  // g_return_val_if_fail(OLS_IS_PAD(pad), FALSE);

  OLS_PAD_LOCK(pad);
  result = (OLS_PAD_PEER(pad) != NULL);
  OLS_PAD_UNLOCK(pad);

  return result;
}

/**
 * ols_pad_link_full:
 * @srcpad: the source #OlsPad to link.
 * @sinkpad: the sink #OlsPad to link.
 * @flags: the checks to validate when linking
 *
 * Links the source pad and the sink pad.
 *
 * This variant of #ols_pad_link provides a more granular control on the
 * checks being done when linking. While providing some considerable speedups
 * the caller of this method must be aware that wrong usage of those flags
 * can cause severe issues. Refer to the documentation of #OlsPadLinkCheck
 * for more information.
 *
 * MT Safe.
 *
 * Returns: A result code indicating if the connection worked or
 *          what went wrong.
 */
OlsPadLinkReturn ols_pad_link_full(ols_pad_t *srcpad, ols_pad_t *sinkpad) {
  OlsPadLinkReturn result = OLS_PAD_LINK_OK;
  ols_object_t *parent;
  ols_pad_link_function srcfunc, sinkfunc;

  // g_return_val_if_fail(OLS_IS_PAD(srcpad), OLS_PAD_LINK_REFUSED);
  // g_return_val_if_fail(OLS_PAD_IS_SRC(srcpad), OLS_PAD_LINK_WRONG_DIRECTION);
  // g_return_val_if_fail(OLS_IS_PAD(sinkpad), OLS_PAD_LINK_REFUSED);
  // g_return_val_if_fail(OLS_PAD_IS_SINK(sinkpad),
  // OLS_PAD_LINK_WRONG_DIRECTION);

  if (!OLS_PAD_IS_SRC(srcpad)) {
    return OLS_PAD_LINK_WRONG_DIRECTION;
  }

  if (!OLS_PAD_IS_SINK(sinkpad)) {
    return OLS_PAD_LINK_WRONG_DIRECTION;
  }

  // OLS_TRACER_PAD_LINK_PRE(srcpad, sinkpad);

  /* Notify the parent early. See ols_pad_unlink for details. */
  // if ((parent = OLS_ELEMENT_CAST(ols_pad_get_parent(srcpad)))) {
  //   if (OLS_IS_ELEMENT(parent)) {
  //     // ols_element_post_message(parent, ols_message_new_structure_change(
  //     //                                      OLS_OBJECT_CAST(sinkpad),
  //     // OLS_STRUCTURE_CHANGE_TYPE_PAD_LINK,
  //     //                                      parent, TRUE));
  //   } else {
  //     ols_object_unref(parent);
  //     parent = NULL;
  //   }
  // }

  /* prepare will also lock the two pads */
  // result = ols_pad_link_prepare(srcpad, sinkpad, flags);
  OLS_PAD_LOCK(srcpad);
  OLS_PAD_LOCK(sinkpad);

  /* must set peers before calling the link function */
  OLS_PAD_PEER(srcpad) = sinkpad;
  OLS_PAD_PEER(sinkpad) = srcpad;

  /* check events, when something is different, mark pending */
  // schedule_events(srcpad, sinkpad);

  /* get the link functions */
  srcfunc = OLS_PAD_LINKFUNC(srcpad);
  sinkfunc = OLS_PAD_LINKFUNC(sinkpad);

  if (srcfunc || sinkfunc) {
    /* custom link functions, execute them */
    OLS_PAD_UNLOCK(sinkpad);
    OLS_PAD_UNLOCK(srcpad);

    if (srcfunc) {
      ols_object_t *tmpparent;

      ACQUIRE_PARENT(srcpad, tmpparent, no_parent);
      /* this one will call the peer link function */
      result = srcfunc(srcpad, tmpparent, sinkpad);
      RELEASE_PARENT(tmpparent);
    } else if (sinkfunc) {
      ols_object_t *tmpparent;

      ACQUIRE_PARENT(sinkpad, tmpparent, no_parent);
      /* if no source link function, we need to call the sink link
       * function ourselves. */
      result = sinkfunc(sinkpad, tmpparent, srcpad);
      RELEASE_PARENT(tmpparent);
    }
  no_parent:

    OLS_PAD_LOCK(srcpad);
    OLS_PAD_LOCK(sinkpad);

    /* we released the lock, check if the same pads are linked still */
    if (OLS_PAD_PEER(srcpad) != sinkpad || OLS_PAD_PEER(sinkpad) != srcpad)
      goto concurrent_link;

    if (result != OLS_PAD_LINK_OK)
      goto link_failed;
  }
  OLS_PAD_UNLOCK(sinkpad);
  OLS_PAD_UNLOCK(srcpad);

  /* fire off a signal to each of the pads telling them
   * that they've been linked */
  // g_signal_emit(srcpad, ols_pad_signals[PAD_LINKED], 0, sinkpad);
  // g_signal_emit(sinkpad, ols_pad_signals[PAD_LINKED], 0, srcpad);

  // OLS_CAT_INFO(OLS_CAT_PADS, "linked %s:%s and %s:%s, successful",
  //              OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad));

  // if (!(flags & OLS_PAD_LINK_CHECK_NO_RECONFIGURE))
  //   ols_pad_send_event(srcpad, ols_event_new_reconfigure());

done:
  // if (parent) {
  // ols_element_post_message(parent, ols_message_new_structure_change(
  //                                      OLS_OBJECT_CAST(sinkpad),
  //                                      OLS_STRUCTURE_CHANGE_TYPE_PAD_LINK,
  //                                      parent, FALSE));
  //  ols_object_unref(parent);
  //}

  // OLS_TRACER_PAD_LINK_POST(srcpad, sinkpad, result);
  return result;

  /* ERRORS */
concurrent_link: {
  // OLS_CAT_INFO(OLS_CAT_PADS, "concurrent link between %s:%s and %s:%s",
  //              OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad));
  OLS_PAD_UNLOCK(sinkpad);
  OLS_PAD_UNLOCK(srcpad);

  /* The other link operation succeeded first */
  result = OLS_PAD_LINK_WAS_LINKED;
  goto done;
}
link_failed: {
  // OLS_CAT_INFO(OLS_CAT_PADS, "link between %s:%s and %s:%s failed: %s",
  //              OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad),
  //              ols_pad_link_get_name(result));

  OLS_PAD_PEER(srcpad) = NULL;
  OLS_PAD_PEER(sinkpad) = NULL;

  OLS_PAD_UNLOCK(sinkpad);
  OLS_PAD_UNLOCK(srcpad);

  goto done;
}
}

/**
 * ols_pad_link:
 * @srcpad: the source #OlsPad to link.
 * @sinkpad: the sink #OlsPad to link.
 *
 * Links the source pad and the sink pad.
 *
 * Returns: A result code indicating if the connection worked or
 *          what went wrong.
 *
 * MT Safe.
 */
OlsPadLinkReturn ols_pad_link(ols_pad_t *srcpad, ols_pad_t *sinkpad) {
  return ols_pad_link_full(srcpad, sinkpad);
}

/**********************************************************************
 * Data passing functions
 */

/* this is the chain function that does not perform the additional argument
 * checking for that little extra speed.
 */
static inline OlsFlowReturn
ols_pad_chain_data_unchecked(ols_pad_t *pad, OlsPadProbeType type, void *data) {
  OlsFlowReturn ret;
  ols_object_t *parent;
  bool handled = false;

  // if (type & OLS_PAD_PROBE_TYPE_BUFFER_LIST) {
  //   OLS_TRACER_PAD_CHAIN_LIST_PRE(pad, data);
  // } else {
  //   OLS_TRACER_PAD_CHAIN_PRE(pad, data);
  // }

  // OLS_PAD_STREAM_LOCK(pad);

  OLS_PAD_LOCK(pad);

  if (OLS_PAD_IS_EOS(pad))
    goto eos;

  // if (OLS_PAD_MODE(pad) != OLS_PAD_MODE_PUSH)
  //   goto wrong_mode;

  ACQUIRE_PARENT(pad, parent, no_parent);
  OLS_PAD_UNLOCK(pad);

  /* NOTE: we read the chainfunc unlocked.
   * we cannot hold the lock for the pad so we might send
   * the data to the wrong function. This is not really a
   * problem since functions are assigned at creation time
   * and don't change that often... */
  if (type & OLS_PAD_PROBE_TYPE_BUFFER) {
    ols_pad_chain_function chainfunc;

    if ((chainfunc = OLS_PAD_CHAINFUNC(pad)) == NULL)
      goto no_function;

    // OLS_CAT_DEBUG_OBJECT(
    //     OLS_CAT_SCHEDULING, pad,
    //     "calling chainfunction &%s with buffer %" OLS_PTR_FORMAT,
    //     OLS_DEBUG_FUNCPTR_NAME(chainfunc), OLS_BUFFER(data));

    ret = chainfunc(pad, parent, OLS_BUFFER_CAST(data));

    // OLS_CAT_DEBUG_OBJECT(OLS_CAT_SCHEDULING, pad,
    //                      "called chainfunction &%s with buffer %p, returned
    //                      %s", OLS_DEBUG_FUNCPTR_NAME(chainfunc), data,
    //                      ols_flow_get_name(ret));
  } else {
    ols_pad_chain_list_function chainlistfunc;

    if ((chainlistfunc = OLS_PAD_CHAINLISTFUNC(pad)) == NULL)
      goto no_function;

    // OLS_CAT_DEBUG_OBJECT(OLS_CAT_SCHEDULING, pad,
    //                      "calling chainlistfunction &%s",
    //                      OLS_DEBUG_FUNCPTR_NAME(chainlistfunc));

    ret = chainlistfunc(pad, parent, OLS_BUFFER_LIST_CAST(data));

    // OLS_CAT_DEBUG_OBJECT(
    //     OLS_CAT_SCHEDULING, pad, "called chainlistfunction &%s, returned %s",
    //     OLS_DEBUG_FUNCPTR_NAME(chainlistfunc), ols_flow_get_name(ret));
  }

  // pad->ABI.abi.last_flowret = ret;

  RELEASE_PARENT(parent);

  // OLS_PAD_STREAM_UNLOCK(pad);

out:
  // if (type & OLS_PAD_PROBE_TYPE_BUFFER_LIST) {
  //   OLS_TRACER_PAD_CHAIN_LIST_POST(pad, ret);
  // } else {
  //   OLS_TRACER_PAD_CHAIN_POST(pad, ret);
  // }
  return ret;

  /* ERRORS */
flushing: {
  blog(LOG_ERROR, "chaining, but pad was flushing"); 
  OLS_PAD_UNLOCK(pad);
  // OLS_PAD_STREAM_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  ret = OLS_FLOW_FLUSHING;
  goto out;
}

eos: {
  blog(LOG_WARNING,  "chaining, but pad was EOS");
  // pad->ABI.abi.last_flowret = OLS_FLOW_EOS;
  OLS_PAD_UNLOCK(pad);
  // OLS_PAD_STREAM_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  ret = OLS_FLOW_EOS;
  goto out;
}
wrong_mode: {
  blog(LOG_ERROR,"chain on pad %s but it was not in push mode",OLS_PAD_NAME(pad));
  // pad->ABI.abi.last_flowret = OLS_FLOW_ERROR;
  OLS_PAD_UNLOCK(pad);
  // OLS_PAD_STREAM_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  ret = OLS_FLOW_ERROR;
  goto out;
}
probe_stopped: {
  /* We unref the buffer, except if the probe handled it (CUSTOM_SUCCESS_1) */
  if (!handled)
    ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));

  switch (ret) {
  case OLS_FLOW_CUSTOM_SUCCESS:
  case OLS_FLOW_CUSTOM_SUCCESS_1:
    // OLS_DEBUG_OBJECT(pad, "dropped or handled buffer");
    ret = OLS_FLOW_OK;
    break;
  default:
    // OLS_DEBUG_OBJECT(pad, "an error occurred %s", ols_flow_get_name(ret));
    break;
  }
  // pad->ABI.abi.last_flowret = ret;
  OLS_PAD_UNLOCK(pad);
  // OLS_PAD_STREAM_UNLOCK(pad);
  goto out;
}

no_parent: {
  // OLS_DEBUG_OBJECT(pad, "No parent when chaining %" OLS_PTR_FORMAT, data);
  // pad->ABI.abi.last_flowret = OLS_FLOW_FLUSHING;
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  OLS_PAD_UNLOCK(pad);
  // OLS_PAD_STREAM_UNLOCK(pad);
  ret = OLS_FLOW_FLUSHING;
  goto out;
}

no_function: {
  // pad->ABI.abi.last_flowret = OLS_FLOW_NOT_SUPPORTED;
  RELEASE_PARENT(parent);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  // g_critical("chain on pad %s:%s but it has no chainfunction",
  //            OLS_DEBUG_PAD_NAME(pad));
  // OLS_PAD_STREAM_UNLOCK(pad);
  ret = OLS_FLOW_NOT_SUPPORTED;
  goto out;
}
}

static OlsFlowReturn ols_pad_chain_list_default(ols_pad_t *pad,
                                                ols_object_t *parent,
                                                ols_buffer_list_t *list) {
  UNUSED_PARAMETER(parent);
  uint32_t i, len;
  ols_buffer_t *buffer;
  OlsFlowReturn ret;

  // OLS_LOG_OBJECT(pad, "chaining each buffer in list individually");

  len = ols_buffer_list_length(list);

  ret = OLS_FLOW_OK;
  for (i = 0; i < len; i++) {
    buffer = ols_buffer_list_get(list, i);
    ret = ols_pad_chain_data_unchecked(
        pad, OLS_PAD_PROBE_TYPE_BUFFER | OLS_PAD_PROBE_TYPE_PUSH,
        ols_buffer_ref(buffer));
    if (ret != OLS_FLOW_OK)
      break;
  }
  ols_buffer_list_unref(list);

  return ret;
}

static OlsFlowReturn ols_pad_push_data(ols_pad_t *pad, OlsPadProbeType type,
                                       void *data) {
  ols_pad_t *peer;
  OlsFlowReturn ret;
  bool handled = false;

  OLS_PAD_LOCK(pad);

  if (OLS_PAD_IS_EOS(pad))
    goto eos;

  // if (OLS_PAD_MODE(pad) != OLS_PAD_MODE_PUSH)
  //   goto wrong_mode;

  if ((peer = OLS_PAD_PEER(pad)) == NULL)
    goto not_linked;

  /* take ref to peer pad before releasing the lock */
  ols_mini_object_ref(OLS_MINI_OBJECT_CAST(peer));
  // pad->priv->using ++;
  OLS_PAD_UNLOCK(pad);

  ret = ols_pad_chain_data_unchecked(peer, type, data);
  data = NULL;

  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(peer));

  return ret;

eos: {
   blog(LOG_ERROR,  "pushing on pad %s, but pad was EOS", OLS_PAD_NAME(pad));
  // pad->ABI.abi.last_flowret = OLS_FLOW_EOS;
  OLS_PAD_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  return OLS_FLOW_EOS;
}

wrong_mode: {
  blog(LOG_ERROR,"pushing on pad %s but it was not activated in push mode",  OLS_PAD_NAME(pad));
  // pad->ABI.abi.last_flowret = OLS_FLOW_ERROR;
  OLS_PAD_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  return OLS_FLOW_ERROR;
}

not_linked: {
  blog(LOG_ERROR, "pushing on pad %s, but it was not linked",OLS_PAD_NAME(pad));// pad->ABI.abi.last_flowret = OLS_FLOW_NOT_LINKED;
  OLS_PAD_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  return OLS_FLOW_NOT_LINKED;
}
}

/**
 * ols_pad_push:
 * @pad: a source #ols_pad_t, returns #OLS_FLOW_ERROR if not.
 * @buffer: (transfer full): the #OLSBuffer to push returns OLS_FLOW_ERROR
 *     if not.
 *
 * Pushes a buffer to the peer of @pad.
 *
 * This function will call installed block probes before triggering any
 * installed data probes.
 *
 * The function proceeds calling ols_pad_chain() on the peer pad and returns
 * the value from that function. If @pad has no peer, #OLS_FLOW_NOT_LINKED will
 * be returned.
 *
 * In all cases, success or failure, the caller loses its reference to @buffer
 * after calling this function.
 *
 * Returns: a #OLSFlowReturn from the peer pad.
 *
 * MT safe.
 */
OlsFlowReturn ols_pad_push(ols_pad_t *pad, ols_buffer_t *buffer) {
  OlsFlowReturn res;

  // return_val_if_fail(OLS_IS_PAD(pad), OLS_FLOW_ERROR);
  // return_val_if_fail(OLS_PAD_IS_SRC(pad), OLS_FLOW_ERROR);
  // return_val_if_fail(OLS_IS_BUFFER(buffer), OLS_FLOW_ERROR);

  // OLS_TRACER_PAD_PUSH_PRE(pad, buffer);
  res = ols_pad_push_data(
      pad, OLS_PAD_PROBE_TYPE_BUFFER | OLS_PAD_PROBE_TYPE_PUSH, buffer);
  // OLS_TRACER_PAD_PUSH_POST(pad, res);
  return res;
}

/**
 * ols_pad_get_peer:
 * @pad: a #OlsPad to get the peer of.
 *
 * Gets the peer of @pad. This function refs the peer pad so
 * you need to unref it after use.
 *
 * Returns: (transfer full) (nullable): the peer #OlsPad. Unref after usage.
 *
 * MT safe.
 */
ols_pad_t *ols_pad_get_peer(ols_pad_t *pad) {
  ols_pad_t *result = NULL;

  OLS_PAD_LOCK(pad);
  result = OLS_PAD_PEER(pad);
  if (result)
    ols_mini_object_ref(OLS_MINI_OBJECT_CAST(pad));
  OLS_PAD_UNLOCK(pad);

  return result;
}

static OlsFlowReturn ols_pad_send_event_unchecked(ols_pad_t *pad,
                                                  ols_event_t *event,
                                                  OlsPadProbeType type) {
  UNUSED_PARAMETER(type);

  OlsFlowReturn ret;
  // ols_event_type event_type;
  // bool serialized, need_unlock = false;
  ols_pad_event_function eventfunc;
  ols_object_t *parent;
  // int64_t old_pad_offset;

  OLS_PAD_LOCK(pad);

  // old_pad_offset = pad->offset;
  // event = apply_pad_offset(pad, event, OLS_PAD_IS_SRC(pad));

  // if (OLS_PAD_IS_SINK(pad))
  //   serialized = OLS_EVENT_IS_SERIALIZED(event);
  // else
  //   serialized = false;

  // event_type = OLS_EVENT_TYPE(event);

  /* now do the probe */
  // PROBE_PUSH(pad, type | OLS_PAD_PROBE_TYPE_PUSH | OLS_PAD_PROBE_TYPE_BLOCK,
  //            event, probe_stopped);

  // PROBE_PUSH(pad, type | OLS_PAD_PROBE_TYPE_PUSH, event, probe_stopped);

  /* the pad offset might've been changed by any of the probes above. It
   * would've been taken into account when repushing any of the sticky events
   * above but not for our current event here */
  // if (old_pad_offset != pad->offset) {
  //   event = _apply_pad_offset(pad, event, OLS_PAD_IS_SRC(pad),
  //                             pad->offset - old_pad_offset);
  // }

  // eventfullfunc = OLS_PAD_EVENTFULLFUNC(pad);
  eventfunc = OLS_PAD_EVENTFUNC(pad);
  if (eventfunc == NULL)
    goto no_function;

  ACQUIRE_PARENT(pad, parent, no_parent);
  OLS_PAD_UNLOCK(pad);

  if (eventfunc(pad, parent, event)) {
    ret = OLS_FLOW_OK;
  } else {
    /* something went wrong */
    ret = OLS_FLOW_ERROR;
  }
  RELEASE_PARENT(parent);

  // OLS_DEBUG_OBJECT(pad, "sent event, ret %s", ols_flow_get_name(ret));

  return ret;

  /* ERROR handling */
eos: {
  OLS_PAD_UNLOCK(pad);
  ols_event_unref(event);
  return OLS_FLOW_EOS;
}
no_function: {
  // g_warning("pad %s:%s has no event handler, file a bug.",
  //           OLS_DEBUG_PAD_NAME(pad));
  OLS_PAD_UNLOCK(pad);
  ols_event_unref(event);
  return OLS_FLOW_NOT_SUPPORTED;
}
no_parent: {
  // OLS_DEBUG_OBJECT(pad, "no parent");
  OLS_PAD_UNLOCK(pad);
  ols_event_unref(event);
  return OLS_FLOW_FLUSHING;
}
}

/* should be called with pad LOCK */
static OlsFlowReturn ols_pad_push_event_unchecked(ols_pad_t *pad,
                                                  ols_event_t *event,
                                                  OlsPadProbeType type) {
  OlsFlowReturn ret;
  ols_pad_t *peerpad;
  ols_event_type event_type;
  // int64_t old_pad_offset = pad->offset;

  /* now check the peer pad */
  peerpad = OLS_PAD_PEER(pad);
  if (peerpad == NULL)
    goto not_linked;

  ols_mini_object_ref(OLS_MINI_OBJECT_CAST(peerpad));
  // ols_object_ref(peerpad);
  // pad->priv->using ++;
  OLS_PAD_UNLOCK(pad);

  // OLS_LOG_OBJECT(
  //     pad, "sending event %" OLS_PTR_FORMAT " to peerpad %" OLS_PTR_FORMAT,
  //     event, peerpad);

  ret = ols_pad_send_event_unchecked(peerpad, event, type);

  /* Note: we gave away ownership of the event at this point but we can still
   * print the old pointer */
  // OLS_LOG_OBJECT(
  //     pad, "sent event %p (%s) to peerpad %" OLS_PTR_FORMAT ", ret %s",
  //     event, ols_event_type_get_name(event_type), peerpad,
  //     ols_flow_get_name(ret));
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(peerpad));
  // ols_object_unref(peerpad);

  OLS_PAD_LOCK(pad);
  // pad->priv->using --;
  // if (pad->priv->using == 0) {
  //   /* pad is not active anymore, trigger idle callbacks */
  //   PROBE_NO_DATA(pad, OLS_PAD_PROBE_TYPE_PUSH | OLS_PAD_PROBE_TYPE_IDLE,
  //                 idle_probe_stopped, ret);
  // }
  return ret;

  /* ERROR handling */
flushed: {
  // OLS_DEBUG_OBJECT(pad, "We're flushing");
  ols_event_unref(event);
  return OLS_FLOW_FLUSHING;
}
inactive: {
  // OLS_DEBUG_OBJECT(pad, "flush-stop on inactive pad");
  ols_event_unref(event);
  return OLS_FLOW_FLUSHING;
}

not_linked: {
  // OLS_DEBUG_OBJECT(pad, "Dropping event %s because pad is not linked",
  //                  ols_event_type_get_name(OLS_EVENT_TYPE(event)));
  // OLS_OBJECT_FLAG_SET(pad, OLS_PAD_FLAG_PENDING_EVENTS);
  ols_event_unref(event);

  return OLS_FLOW_NOT_LINKED;
}
}

/**
 * ols_pad_push_event:
 * @pad: a #OlsPad to push the event to.
 * @event: (transfer full): the #OlsEvent to send to the pad.
 *
 * Sends the event to the peer of the given pad. This function is
 * mainly used by elements to send events to their peer
 * elements.
 *
 * This function takes ownership of the provided event so you should
 * ols_event_ref() it if you want to reuse the event after this call.
 *
 * Returns: %TRUE if the event was handled.
 *
 * MT safe.
 */
bool ols_pad_push_event(ols_pad_t *pad, ols_event_t *event) {
  bool res = false;

  OlsPadProbeType type;
  bool serialized;

  // g_return_val_if_fail(OLS_IS_PAD(pad), FALSE);
  // g_return_val_if_fail(OLS_IS_EVENT(event), FALSE);

  // OLS_TRACER_PAD_PUSH_EVENT_PRE(pad, event);

  if (OLS_PAD_IS_SRC(pad)) {
    if (!OLS_EVENT_IS_DOWNSTREAM(event))
      goto wrong_direction;
    type = OLS_PAD_PROBE_TYPE_EVENT_DOWNSTREAM;
  } else if (OLS_PAD_IS_SINK(pad)) {
    if (!OLS_EVENT_IS_UPSTREAM(event))
      goto wrong_direction;
    /* events pushed on sinkpad never are sticky */
    type = OLS_PAD_PROBE_TYPE_EVENT_UPSTREAM;
  } else
    goto unknown_direction;

  OLS_PAD_LOCK(pad);

  serialized = OLS_EVENT_IS_SERIALIZED(event);

  if (!serialized) {
    OlsFlowReturn ret;

    /* non-serialized and non-sticky events are pushed right away. */
    ret = ols_pad_push_event_unchecked(pad, event, type);
    /* dropped events by a probe are not an error */
    res = (ret == OLS_FLOW_OK || ret == OLS_FLOW_CUSTOM_SUCCESS ||
           ret == OLS_FLOW_CUSTOM_SUCCESS_1);
  } else {
    /* Errors in sticky event pushing are no problem and ignored here
     * as they will cause more meaningful errors during data flow.
     * For EOS events, that are not followed by data flow, we still
     * return FALSE here though.
     */
    if (OLS_EVENT_TYPE(event) != OLS_EVENT_EOS)
      res = true;
    ols_event_unref(event);
  }
  OLS_PAD_UNLOCK(pad);
  // OLS_TRACER_PAD_PUSH_EVENT_POST(pad, res);
  return res;

  /* ERROR handling */
wrong_direction: {
  // g_warning("pad %s:%s pushing %s event in wrong direction",
  //           OLS_DEBUG_PAD_NAME(pad), OLS_EVENT_TYPE_NAME(event));
  ols_event_unref(event);
  goto done;
}
unknown_direction: {
  // g_warning("pad %s:%s has invalid direction", OLS_DEBUG_PAD_NAME(pad));
  ols_event_unref(event);
  goto done;
}
flushed: {
  // OLS_DEBUG_OBJECT(pad, "We're flushing");
  OLS_PAD_UNLOCK(pad);
  ols_event_unref(event);
  goto done;
}
eos: {
  // OLS_DEBUG_OBJECT(pad, "We're EOS");
  OLS_PAD_UNLOCK(pad);
  ols_event_unref(event);
  goto done;
}
done:
  // OLS_TRACER_PAD_PUSH_EVENT_POST(pad, FALSE);
  return res;
}
