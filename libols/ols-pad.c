#include "ols-pad.h"
#include "ols-internal.h"
#include <stdbool.h>

static OlsFlowReturn ols_pad_send_event_unchecked(ols_pad_t *pad,
                                                  ols_event_t *event,
                                                  OlsPadProbeType type);
static OlsFlowReturn ols_pad_push_event_unchecked(ols_pad_t *pad,
                                                  ols_event_t *event,
                                                  OlsPadProbeType type);

#define ACQUIRE_PARENT(pad, parent, label)                                     \
  do {                                                                         \
    if ((parent = OLS_PAD_PARENT(pad)))                                        \
      ols_object_get_ref(parent);                                              \
    else                                                                       \
      goto label;                                                              \
  } while (0)

#define RELEASE_PARENT(parent)                                                 \
  do {                                                                         \
    if ((parent = OLS_PAD_PARENT(pad)))                                        \
      ols_object_release(parent);                                              \
  } while (0)

static void ols_pad_init(ols_pad_t *pad) {
  // pad->priv = ols_pad_get_instance_private(pad);

  OLS_PAD_DIRECTION(pad) = OLS_PAD_UNKNOWN;

  OLS_PAD_EVENTFUNC(pad) = ols_pad_event_default;
  // OLS_PAD_QUERYFUNC(pad) = ols_pad_query_default;
  // OLS_PAD_ITERINTLINKFUNC(pad) = ols_pad_iterate_internal_links_default;
  OLS_PAD_CHAINLISTFUNC(pad) = ols_pad_chain_list_default;

  // g_rec_mutex_init(&pad->stream_rec_lock);

  pthread_cond_init(&pad->block_cond, NULL);

  pthread_mutex_init_recursive(&pad->stream_rec_lock);

  // g_hook_list_init(&pad->probes, sizeof(OLSProbe));

  // pad->priv->events = g_array_sized_new(FALSE, TRUE, sizeof(PadEvent), 16);
  // pad->priv->events_cookie = 0;
  // pad->priv->last_cookie = -1;
  // g_cond_init(&pad->priv->activation_cond);
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

  OLS_OBJECT_LOCK(srcpad);
  OLS_OBJECT_LOCK(sinkpad);

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

  OLS_OBJECT_UNLOCK(sinkpad);
  OLS_OBJECT_UNLOCK(srcpad);

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
not_linked_together : {
  /* we do not emit a warning in this case because unlinking cannot
   * be made MT safe.*/
  OLS_OBJECT_UNLOCK(sinkpad);
  OLS_OBJECT_UNLOCK(srcpad);
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

  OLS_OBJECT_LOCK(pad);
  result = (OLS_PAD_PEER(pad) != NULL);
  OLS_OBJECT_UNLOCK(pad);

  return result;
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

  OLS_PAD_STREAM_LOCK(pad);

  OLS_OBJECT_LOCK(pad);

  if (OLS_PAD_IS_EOS(pad))
    goto eos;

  if (OLS_PAD_MODE(pad) != OLS_PAD_MODE_PUSH)
    goto wrong_mode;

  ACQUIRE_PARENT(pad, parent, no_parent);
  OLS_OBJECT_UNLOCK(pad);

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

  OLS_PAD_STREAM_UNLOCK(pad);

out:
  // if (type & OLS_PAD_PROBE_TYPE_BUFFER_LIST) {
  //   OLS_TRACER_PAD_CHAIN_LIST_POST(pad, ret);
  // } else {
  //   OLS_TRACER_PAD_CHAIN_POST(pad, ret);
  // }

  return ret;

  /* ERRORS */
flushing : {
  // OLS_CAT_LOG_OBJECT(OLS_CAT_SCHEDULING, pad, "chaining, but pad was
  // flushing"); pad->ABI.abi.last_flowret = OLS_FLOW_FLUSHING;
  OLS_OBJECT_UNLOCK(pad);
  OLS_PAD_STREAM_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  ret = OLS_FLOW_FLUSHING;
  goto out;
}
eos : {
  // OLS_CAT_LOG_OBJECT(OLS_CAT_SCHEDULING, pad, "chaining, but pad was EOS");
  // pad->ABI.abi.last_flowret = OLS_FLOW_EOS;
  OLS_OBJECT_UNLOCK(pad);
  OLS_PAD_STREAM_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  ret = OLS_FLOW_EOS;
  goto out;
}
wrong_mode : {
  // g_critical("chain on pad %s:%s but it was not in push mode",
  //            OLS_DEBUG_PAD_NAME(pad));
  // pad->ABI.abi.last_flowret = OLS_FLOW_ERROR;
  OLS_OBJECT_UNLOCK(pad);
  OLS_PAD_STREAM_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  ret = OLS_FLOW_ERROR;
  goto out;
}
probe_stopped : {
  /* We unref the buffer, except if the probe handled it (CUSTOM_SUCCESS_1) */
  if (!handled)
    ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));

  switch (ret) {
  case OLS_FLOW_CUSTOM_SUCCESS:
  case OLS_FLOW_CUSTOM_SUCCESS_1:
    OLS_DEBUG_OBJECT(pad, "dropped or handled buffer");
    ret = OLS_FLOW_OK;
    break;
  default:
    OLS_DEBUG_OBJECT(pad, "an error occurred %s", ols_flow_get_name(ret));
    break;
  }
  // pad->ABI.abi.last_flowret = ret;
  OLS_OBJECT_UNLOCK(pad);
  OLS_PAD_STREAM_UNLOCK(pad);
  goto out;
}
no_parent : {
  // OLS_DEBUG_OBJECT(pad, "No parent when chaining %" OLS_PTR_FORMAT, data);
  // pad->ABI.abi.last_flowret = OLS_FLOW_FLUSHING;
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  OLS_OBJECT_UNLOCK(pad);
  OLS_PAD_STREAM_UNLOCK(pad);
  ret = OLS_FLOW_FLUSHING;
  goto out;
}
no_function : {
  // pad->ABI.abi.last_flowret = OLS_FLOW_NOT_SUPPORTED;
  RELEASE_PARENT(parent);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  // g_critical("chain on pad %s:%s but it has no chainfunction",
  //            OLS_DEBUG_PAD_NAME(pad));
  OLS_PAD_STREAM_UNLOCK(pad);
  ret = OLS_FLOW_NOT_SUPPORTED;
  goto out;
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

  // g_return_val_if_fail(OLS_IS_PAD(pad), OLS_FLOW_ERROR);
  // g_return_val_if_fail(OLS_PAD_IS_SRC(pad), OLS_FLOW_ERROR);
  // g_return_val_if_fail(OLS_IS_BUFFER(buffer), OLS_FLOW_ERROR);

  // OLS_TRACER_PAD_PUSH_PRE(pad, buffer);
  res = ols_pad_push_data(
      pad, OLS_PAD_PROBE_TYPE_BUFFER | OLS_PAD_PROBE_TYPE_PUSH, buffer);
  // OLS_TRACER_PAD_PUSH_POST(pad, res);
  return res;
}

static OlsFlowReturn ols_pad_push_data(ols_pad_t *pad, OlsPadProbeType type,
                                       void *data) {
  ols_pad_t *peer;
  OlsFlowReturn ret;
  bool handled = false;

  OLS_OBJECT_LOCK(pad);

  if (OLS_PAD_IS_EOS(pad))
    goto eos;

  if (OLS_PAD_MODE(pad) != OLS_PAD_MODE_PUSH)
    goto wrong_mode;

  if ((peer = OLS_PAD_PEER(pad)) == NULL)
    goto not_linked;

  /* take ref to peer pad before releasing the lock */
  ols_object_ref(peer);
  // pad->priv->using ++;
  OLS_OBJECT_UNLOCK(pad);

  ret = ols_pad_chain_data_unchecked(peer, type, data);
  data = NULL;

  ols_object_unref(peer);

  return ret;

eos : {
  // OLS_CAT_LOG_OBJECT(OLS_CAT_SCHEDULING, pad, "pushing, but pad was EOS");
  // pad->ABI.abi.last_flowret = OLS_FLOW_EOS;
  OLS_OBJECT_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  return OLS_FLOW_EOS;
}
wrong_mode : {
  // g_critical("pushing on pad %s:%s but it was not activated in push mode",
  //            OLS_DEBUG_PAD_NAME(pad));
  // pad->ABI.abi.last_flowret = OLS_FLOW_ERROR;
  OLS_OBJECT_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  return OLS_FLOW_ERROR;
}

not_linked : {
  // OLS_CAT_LOG_OBJECT(OLS_CAT_SCHEDULING, pad, "pushing, but it was not
  // linked"); pad->ABI.abi.last_flowret = OLS_FLOW_NOT_LINKED;
  OLS_OBJECT_UNLOCK(pad);
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
  return OLS_FLOW_NOT_LINKED;
}
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

  OLS_OBJECT_LOCK(pad);
  result = OLS_PAD_PEER(pad);
  if (result)
    ols_object_ref(result);
  OLS_OBJECT_UNLOCK(pad);

  return result;
}

static OlsFlowReturn ols_pad_send_event_unchecked(ols_pad_t *pad,
                                                  ols_event_t *event,
                                                  OlsPadProbeType type) {
  OlsFlowReturn ret;
  ols_event_type event_type;
  bool serialized, need_unlock = false, sticky;
  ols_pad_event_function eventfunc;
  ols_pad_event_full_function eventfullfunc = NULL;
  ols_object_t *parent;
  int64_t old_pad_offset;

  OLS_OBJECT_LOCK(pad);

  old_pad_offset = pad->offset;
  event = apply_pad_offset(pad, event, OLS_PAD_IS_SRC(pad));

  if (OLS_PAD_IS_SINK(pad))
    serialized = OLS_EVENT_IS_SERIALIZED(event);
  else
    serialized = false;
  sticky = OLS_EVENT_IS_STICKY(event);
  event_type = OLS_EVENT_TYPE(event);
  switch (event_type) {
  case OLS_EVENT_FLUSH_START:
    // OLS_CAT_DEBUG_OBJECT(OLS_CAT_EVENT, pad, "have event type %d
    // (FLUSH_START)",
    //                      OLS_EVENT_TYPE(event));

    /* can't even accept a flush begin event when flushing */
    if (OLS_PAD_IS_FLUSHING(pad))
      goto flushing;

    OLS_PAD_SET_FLUSHING(pad);
    // OLS_CAT_DEBUG_OBJECT(OLS_CAT_EVENT, pad, "set flush flag");
    OLS_PAD_BLOCK_BROADCAST(pad);
    type |= OLS_PAD_PROBE_TYPE_EVENT_FLUSH;
    break;
  case OLS_EVENT_FLUSH_STOP:
    /* we can't accept flush-stop on inactive pads else the flushing flag
     * would be cleared and it would look like the pad can accept data.
     * Also, some elements restart a streaming thread in flush-stop which we
     * can't allow on inactive pads */
    if (!OLS_PAD_IS_ACTIVE(pad))
      goto inactive;

    OLS_PAD_UNSET_FLUSHING(pad);
    // OLS_CAT_DEBUG_OBJECT(OLS_CAT_EVENT, pad, "cleared flush flag");
    /* Remove pending EOS events */
    // OLS_LOG_OBJECT(pad, "Removing pending EOS and SEGMENT events");
    remove_event_by_type(pad, OLS_EVENT_EOS);
    // remove_event_by_type(pad, OLS_EVENT_STREAM_GROUP_DONE);
    // remove_event_by_type(pad, OLS_EVENT_SEGMENT);
    // OLS_OBJECT_FLAG_UNSET(pad, OLS_PAD_FLAG_EOS);
    // pad->ABI.abi.last_flowret = OLS_FLOW_OK;

    OLS_OBJECT_UNLOCK(pad);
    /* grab stream lock */
    OLS_PAD_STREAM_LOCK(pad);
    need_unlock = true;
    OLS_OBJECT_LOCK(pad);
    if (OLS_PAD_IS_FLUSHING(pad))
      goto flushing;
    break;
  default:
    // OLS_CAT_DEBUG_OBJECT(OLS_CAT_EVENT, pad, "have event type %"
    // OLS_PTR_FORMAT,
    //                      event);

    if (OLS_PAD_IS_FLUSHING(pad))
      goto flushing;

    switch (event_type) {
    case OLS_EVENT_STREAM_START:
      /* Take the stream lock to unset the EOS status. This is to ensure
       * there isn't any other serialized event passing through while this
       * EOS status is being unset */
      /* lock order: STREAM_LOCK, LOCK, recheck flushing. */
      OLS_OBJECT_UNLOCK(pad);
      OLS_PAD_STREAM_LOCK(pad);
      need_unlock = true;
      OLS_OBJECT_LOCK(pad);
      if (OLS_PAD_IS_FLUSHING(pad))
        goto flushing;

      /* Remove sticky EOS events */
      OLS_LOG_OBJECT(pad, "Removing pending EOS events");
      remove_event_by_type(pad, OLS_EVENT_EOS);
      // remove_event_by_type(pad, OLS_EVENT_STREAM_GROUP_DONE);
      remove_event_by_type(pad, OLS_EVENT_TAG);
      // OLS_OBJECT_FLAG_UNSET(pad, OLS_PAD_FLAG_EOS);
      break;
    default:
      if (serialized) {
        /* Take the stream lock to check the EOS status and drop the event
         * if that is the case. */
        /* lock order: STREAM_LOCK, LOCK, recheck flushing. */
        OLS_OBJECT_UNLOCK(pad);
        OLS_PAD_STREAM_LOCK(pad);
        need_unlock = true;
        OLS_OBJECT_LOCK(pad);
        if (OLS_PAD_IS_FLUSHING(pad))
          goto flushing;

        if (OLS_PAD_IS_EOS(pad))
          goto eos;
      }
      break;
    }
    break;
  }

  /* now do the probe */
  // PROBE_PUSH(pad, type | OLS_PAD_PROBE_TYPE_PUSH | OLS_PAD_PROBE_TYPE_BLOCK,
  //            event, probe_stopped);

  // PROBE_PUSH(pad, type | OLS_PAD_PROBE_TYPE_PUSH, event, probe_stopped);

  /* the pad offset might've been changed by any of the probes above. It
   * would've been taken into account when repushing any of the sticky events
   * above but not for our current event here */
  if (old_pad_offset != pad->offset) {
    event = _apply_pad_offset(pad, event, OLS_PAD_IS_SRC(pad),
                              pad->offset - old_pad_offset);
  }

  eventfullfunc = OLS_PAD_EVENTFULLFUNC(pad);
  eventfunc = OLS_PAD_EVENTFUNC(pad);
  if (eventfunc == NULL && eventfullfunc == NULL)
    goto no_function;

  ACQUIRE_PARENT(pad, parent, no_parent);
  OLS_OBJECT_UNLOCK(pad);

  ret = pre_eventfunc_check(pad, event);
  if (ret != OLS_FLOW_OK)
    goto precheck_failed;

  if (sticky)
    ols_event_ref(event);

  if (eventfullfunc) {
    ret = eventfullfunc(pad, parent, event);
  } else if (eventfunc(pad, parent, event)) {
    ret = OLS_FLOW_OK;
  } else {
    /* something went wrong */
    switch (event_type) {
    case OLS_EVENT_CAPS:
      ret = OLS_FLOW_NOT_NEGOTIATED;
      break;
    default:
      ret = OLS_FLOW_ERROR;
      break;
    }
  }
  RELEASE_PARENT(parent);

  OLS_DEBUG_OBJECT(pad, "sent event, ret %s", ols_flow_get_name(ret));

  if (sticky) {
    if (ret == OLS_FLOW_OK) {
      OLS_OBJECT_LOCK(pad);
      /* after the event function accepted the event, we can store the sticky
       * event on the pad */
      switch (store_sticky_event(pad, event)) {
      case OLS_FLOW_FLUSHING:
        goto flushing;
      case OLS_FLOW_EOS:
        goto eos;
      default:
        break;
      }
      OLS_OBJECT_UNLOCK(pad);
    }
    ols_event_unref(event);
  }

  if (need_unlock)
    OLS_PAD_STREAM_UNLOCK(pad);

  return ret;

  /* ERROR handling */
flushing : {
  OLS_OBJECT_UNLOCK(pad);
  if (need_unlock)
    OLS_PAD_STREAM_UNLOCK(pad);
  // OLS_CAT_INFO_OBJECT(OLS_CAT_EVENT, pad,
  //                     "Received event on flushing pad. Discarding");
  ols_event_unref(event);
  return OLS_FLOW_FLUSHING;
}
inactive : {
  OLS_OBJECT_UNLOCK(pad);
  if (need_unlock)
    OLS_PAD_STREAM_UNLOCK(pad);
  // OLS_CAT_INFO_OBJECT(OLS_CAT_EVENT, pad,
  //                     "Received flush-stop on inactive pad. Discarding");
  ols_event_unref(event);
  return OLS_FLOW_FLUSHING;
}
eos : {
  OLS_OBJECT_UNLOCK(pad);
  if (need_unlock)
    OLS_PAD_STREAM_UNLOCK(pad);
  // OLS_CAT_INFO_OBJECT(OLS_CAT_EVENT, pad,
  //                     "Received event on EOS pad. Discarding");
  ols_event_unref(event);
  return OLS_FLOW_EOS;
}
probe_stopped : {
  OLS_OBJECT_UNLOCK(pad);
  if (need_unlock)
    OLS_PAD_STREAM_UNLOCK(pad);
  /* Only unref if unhandled */
  if (ret != OLS_FLOW_CUSTOM_SUCCESS_1)
    ols_event_unref(event);

  switch (ret) {
  case OLS_FLOW_CUSTOM_SUCCESS_1:
  case OLS_FLOW_CUSTOM_SUCCESS:
    OLS_DEBUG_OBJECT(pad, "dropped or handled event");
    ret = OLS_FLOW_OK;
    break;
  default:
    OLS_DEBUG_OBJECT(pad, "an error occurred %s", ols_flow_get_name(ret));
    break;
  }
  return ret;
}
no_function : {
  // g_warning("pad %s:%s has no event handler, file a bug.",
  //           OLS_DEBUG_PAD_NAME(pad));
  OLS_OBJECT_UNLOCK(pad);
  if (need_unlock)
    OLS_PAD_STREAM_UNLOCK(pad);
  ols_event_unref(event);
  return OLS_FLOW_NOT_SUPPORTED;
}
no_parent : {
  // OLS_DEBUG_OBJECT(pad, "no parent");
  OLS_OBJECT_UNLOCK(pad);
  if (need_unlock)
    OLS_PAD_STREAM_UNLOCK(pad);
  ols_event_unref(event);
  return OLS_FLOW_FLUSHING;
}
precheck_failed : {
  // OLS_DEBUG_OBJECT(pad, "pre event check failed");
  RELEASE_PARENT(parent);
  if (need_unlock)
    OLS_PAD_STREAM_UNLOCK(pad);
  ols_event_unref(event);
  return ret;
}
}

/* should be called with pad LOCK */
static OlsFlowReturn ols_pad_push_event_unchecked(ols_pad_t *pad,
                                                  ols_event_t *event,
                                                  OlsPadProbeType type) {
  OlsFlowReturn ret;
  ols_pad_t *peerpad;
  ols_event_type event_type;
  int64_t old_pad_offset = pad->offset;

  /* pass the adjusted event on. We need to do this even if
   * there is no peer pad because of the probes. */
  event = apply_pad_offset(pad, event, OLS_PAD_IS_SINK(pad));

  /* Two checks to be made:
   * . (un)set the FLUSHING flag for flushing events,
   * . handle pad blocking */
  event_type = OLS_EVENT_TYPE(event);
  switch (event_type) {
  case OLS_EVENT_FLUSH_START:
    OLS_PAD_SET_FLUSHING(pad);

    OLS_PAD_BLOCK_BROADCAST(pad);
    type |= OLS_PAD_PROBE_TYPE_EVENT_FLUSH;
    break;
  case OLS_EVENT_FLUSH_STOP:
    if (G_UNLIKELY(!OLS_PAD_IS_ACTIVE(pad)))
      goto inactive;

    OLS_PAD_UNSET_FLUSHING(pad);

    /* Remove sticky EOS events */
    // OLS_LOG_OBJECT(pad, "Removing pending EOS events");
    remove_event_by_type(pad, OLS_EVENT_EOS);
    // remove_event_by_type(pad, OLS_EVENT_STREAM_GROUP_DONE);
    // remove_event_by_type(pad, OLS_EVENT_SEGMENT);
    OLS_OBJECT_FLAG_UNSET(pad, OLS_PAD_FLAG_EOS);
    // pad->ABI.abi.last_flowret = OLS_FLOW_OK;

    type |= OLS_PAD_PROBE_TYPE_EVENT_FLUSH;
    break;
  default: {
    if (G_UNLIKELY(OLS_PAD_IS_FLUSHING(pad)))
      goto flushed;

    /* No need to check for EOS here as either the caller (ols_pad_push_event())
     * checked already or this is called as part of pushing sticky events,
     * in which case we still want to forward the EOS event downstream.
     */

    switch (OLS_EVENT_TYPE(event)) {
    case OLS_EVENT_RECONFIGURE:
      if (OLS_PAD_IS_SINK(pad))
        OLS_OBJECT_FLAG_SET(pad, OLS_PAD_FLAG_NEED_RECONFIGURE);
      break;
    default:
      break;
    }
    PROBE_PUSH(pad, type | OLS_PAD_PROBE_TYPE_PUSH | OLS_PAD_PROBE_TYPE_BLOCK,
               event, probe_stopped);
    /* recheck sticky events because the probe might have cause a relink */
    if (OLS_PAD_HAS_PENDING_EVENTS(pad) && OLS_PAD_IS_SRC(pad) &&
        (OLS_EVENT_IS_SERIALIZED(event))) {
      PushStickyData data = {OLS_FLOW_OK, FALSE, event};
      OLS_OBJECT_FLAG_UNSET(pad, OLS_PAD_FLAG_PENDING_EVENTS);

      /* Push all sticky events before our current one
       * that have changed */
      events_foreach(pad, sticky_changed, &data);
    }
    break;
  }
  }

  /* send probes after modifying the events above */
  PROBE_PUSH(pad, type | OLS_PAD_PROBE_TYPE_PUSH, event, probe_stopped);

  /* recheck sticky events because the probe might have cause a relink */
  if (OLS_PAD_HAS_PENDING_EVENTS(pad) && OLS_PAD_IS_SRC(pad) &&
      (OLS_EVENT_IS_SERIALIZED(event))) {
    PushStickyData data = {OLS_FLOW_OK, FALSE, event};
    OLS_OBJECT_FLAG_UNSET(pad, OLS_PAD_FLAG_PENDING_EVENTS);

    /* Push all sticky events before our current one
     * that have changed */
    events_foreach(pad, sticky_changed, &data);
  }

  /* the pad offset might've been changed by any of the probes above. It
   * would've been taken into account when repushing any of the sticky events
   * above but not for our current event here */
  if (G_UNLIKELY(old_pad_offset != pad->offset)) {
    event = _apply_pad_offset(pad, event, OLS_PAD_IS_SINK(pad),
                              pad->offset - old_pad_offset);
  }

  /* now check the peer pad */
  peerpad = OLS_PAD_PEER(pad);
  if (peerpad == NULL)
    goto not_linked;

  ols_object_ref(peerpad);
  pad->priv->using ++;
  OLS_OBJECT_UNLOCK(pad);

  OLS_LOG_OBJECT(
      pad, "sending event %" OLS_PTR_FORMAT " to peerpad %" OLS_PTR_FORMAT,
      event, peerpad);

  ret = ols_pad_send_event_unchecked(peerpad, event, type);

  /* Note: we gave away ownership of the event at this point but we can still
   * print the old pointer */
  OLS_LOG_OBJECT(
      pad, "sent event %p (%s) to peerpad %" OLS_PTR_FORMAT ", ret %s", event,
      ols_event_type_get_name(event_type), peerpad, ols_flow_get_name(ret));

  ols_object_unref(peerpad);

  OLS_OBJECT_LOCK(pad);
  pad->priv->using --;
  if (pad->priv->using == 0) {
    /* pad is not active anymore, trigger idle callbacks */
    PROBE_NO_DATA(pad, OLS_PAD_PROBE_TYPE_PUSH | OLS_PAD_PROBE_TYPE_IDLE,
                  idle_probe_stopped, ret);
  }
  return ret;

  /* ERROR handling */
flushed : {
  OLS_DEBUG_OBJECT(pad, "We're flushing");
  ols_event_unref(event);
  return OLS_FLOW_FLUSHING;
}
inactive : {
  OLS_DEBUG_OBJECT(pad, "flush-stop on inactive pad");
  ols_event_unref(event);
  return OLS_FLOW_FLUSHING;
}
probe_stopped : {
  OLS_OBJECT_FLAG_SET(pad, OLS_PAD_FLAG_PENDING_EVENTS);
  if (ret != OLS_FLOW_CUSTOM_SUCCESS_1)
    ols_event_unref(event);

  switch (ret) {
  case OLS_FLOW_CUSTOM_SUCCESS_1:
    OLS_DEBUG_OBJECT(pad, "handled event");
    break;
  case OLS_FLOW_CUSTOM_SUCCESS:
    OLS_DEBUG_OBJECT(pad, "dropped event");
    break;
  default:
    OLS_DEBUG_OBJECT(pad, "an error occurred %s", ols_flow_get_name(ret));
    break;
  }
  return ret;
}
not_linked : {
  // OLS_DEBUG_OBJECT(pad, "Dropping event %s because pad is not linked",
  //                  ols_event_type_get_name(OLS_EVENT_TYPE(event)));
  // OLS_OBJECT_FLAG_SET(pad, OLS_PAD_FLAG_PENDING_EVENTS);
  ols_event_unref(event);

  return OLS_FLOW_NOT_LINKED;
}
idle_probe_stopped : {
  // OLS_DEBUG_OBJECT(pad, "Idle probe returned %s", ols_flow_get_name(ret));
  return ret;
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
  bool sticky, serialized;

  // g_return_val_if_fail(OLS_IS_PAD(pad), FALSE);
  // g_return_val_if_fail(OLS_IS_EVENT(event), FALSE);

  // OLS_TRACER_PAD_PUSH_EVENT_PRE(pad, event);

  if (OLS_PAD_IS_SRC(pad)) {
    if (G_UNLIKELY(!OLS_EVENT_IS_DOWNSTREAM(event)))
      goto wrong_direction;
    type = OLS_PAD_PROBE_TYPE_EVENT_DOWNSTREAM;
  } else if (OLS_PAD_IS_SINK(pad)) {
    if (G_UNLIKELY(!OLS_EVENT_IS_UPSTREAM(event)))
      goto wrong_direction;
    /* events pushed on sinkpad never are sticky */
    type = OLS_PAD_PROBE_TYPE_EVENT_UPSTREAM;
  } else
    goto unknown_direction;

  OLS_OBJECT_LOCK(pad);
  sticky = OLS_EVENT_IS_STICKY(event);
  serialized = OLS_EVENT_IS_SERIALIZED(event);

  if (sticky) {
    /* srcpad sticky events are stored immediately, the received flag is
    set
     * to FALSE and will be set to TRUE when we can successfully push the
     * event to the peer pad */
    switch (store_sticky_event(pad, event)) {
    case OLS_FLOW_FLUSHING:
      goto flushed;
    case OLS_FLOW_EOS:
      goto eos;
    default:
      break;
    }
  }
  if (OLS_PAD_IS_SRC(pad) && serialized) {
    /* All serialized events on the srcpad trigger push of sticky events.
     *
     * Note that we do not do this for non-serialized sticky events since
     it
     * could potentially block. */
    res = (check_sticky(pad, event) == OLS_FLOW_OK);
  }
  if (!serialized || !sticky) {
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
  OLS_OBJECT_UNLOCK(pad);

  // OLS_TRACER_PAD_PUSH_EVENT_POST(pad, res);
  return res;

  /* ERROR handling */
wrong_direction : {
  // g_warning("pad %s:%s pushing %s event in wrong direction",
  //           OLS_DEBUG_PAD_NAME(pad), OLS_EVENT_TYPE_NAME(event));
  ols_event_unref(event);
  goto done;
}
unknown_direction : {
  // g_warning("pad %s:%s has invalid direction", OLS_DEBUG_PAD_NAME(pad));
  ols_event_unref(event);
  goto done;
}
flushed : {
  // OLS_DEBUG_OBJECT(pad, "We're flushing");
  OLS_OBJECT_UNLOCK(pad);
  ols_event_unref(event);
  goto done;
}
eos : {
  // OLS_DEBUG_OBJECT(pad, "We're EOS");
  OLS_OBJECT_UNLOCK(pad);
  ols_event_unref(event);
  goto done;
}
done:
  // OLS_TRACER_PAD_PUSH_EVENT_POST(pad, FALSE);
  return res;
}
