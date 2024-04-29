#include "ols-pad.h"

static void ols_pad_init(ols_pad *pad) {}

/* called when setting the pad inactive. It removes all sticky events from
 * the pad. must be called with object lock */
static void remove_events(OlsPad *pad) {
  guint i, len;
  GArray *events;
  gboolean notify = FALSE;

  events = pad->priv->events;

  len = events->len;
  for (i = 0; i < len; i++) {
    PadEvent *ev = &g_array_index(events, PadEvent, i);
    OlsEvent *event = ev->event;

    ev->event = NULL;

    if (event && OLS_EVENT_TYPE(event) == OLS_EVENT_CAPS)
      notify = TRUE;

    ols_event_unref(event);
  }

  OLS_OBJECT_FLAG_UNSET(pad, OLS_PAD_FLAG_PENDING_EVENTS);
  g_array_set_size(events, 0);
  pad->priv->events_cookie++;

  if (notify) {
    OLS_OBJECT_UNLOCK(pad);

    OLS_DEBUG_OBJECT(pad, "notify caps");
    g_object_notify_by_pspec((GObject *)pad, pspec_caps);

    OLS_OBJECT_LOCK(pad);
  }
}

#define _to_sticky_order(t) ols_event_type_to_sticky_ordering(t)

/* should be called with object lock */
static PadEvent *find_event_by_type(OlsPad *pad, OlsEventType type, guint idx) {
  guint i, len;
  GArray *events;
  PadEvent *ev;
  guint last_sticky_order = _to_sticky_order(type);

  events = pad->priv->events;
  len = events->len;

  for (i = 0; i < len; i++) {
    ev = &g_array_index(events, PadEvent, i);
    if (ev->event == NULL)
      continue;

    if (OLS_EVENT_TYPE(ev->event) == type) {
      if (idx == 0)
        goto found;
      idx--;
    } else if (ev->sticky_order > last_sticky_order) {
      break;
    }
  }
  ev = NULL;
found:
  return ev;
}

/* should be called with OBJECT lock */
static PadEvent *find_event(OlsPad *pad, OlsEvent *event) {
  guint i, len;
  GArray *events;
  PadEvent *ev;

  events = pad->priv->events;
  len = events->len;

  guint sticky_order = _to_sticky_order(OLS_EVENT_TYPE(event));
  for (i = 0; i < len; i++) {
    ev = &g_array_index(events, PadEvent, i);
    if (event == ev->event)
      goto found;
    else if (ev->sticky_order > sticky_order)
      break;
  }
  ev = NULL;
found:
  return ev;
}

/* should be called with OBJECT lock */
static void remove_event_by_type(OlsPad *pad, OlsEventType type) {
  guint i, len;
  GArray *events;
  PadEvent *ev;

  events = pad->priv->events;
  len = events->len;

  guint last_sticky_order = _to_sticky_order(type);

  i = 0;
  while (i < len) {
    ev = &g_array_index(events, PadEvent, i);
    if (ev->event == NULL)
      goto next;

    if (ev->sticky_order > last_sticky_order)
      break;
    else if (OLS_EVENT_TYPE(ev->event) != type)
      goto next;

    ols_event_unref(ev->event);
    g_array_remove_index(events, i);
    len--;
    pad->priv->events_cookie++;
    continue;

  next:
    i++;
  }
}

/* check all events on srcpad against those on sinkpad. All events that are not
 * on sinkpad are marked as received=%FALSE and the PENDING_EVENTS is set on the
 * srcpad so that the events will be sent next time */
/* should be called with srcpad and sinkpad LOCKS */
static void schedule_events(OlsPad *srcpad, OlsPad *sinkpad) {
  gint i, len;
  GArray *events;
  PadEvent *ev;
  gboolean pending = FALSE;

  events = srcpad->priv->events;
  len = events->len;

  for (i = 0; i < len; i++) {
    ev = &g_array_index(events, PadEvent, i);
    if (ev->event == NULL)
      continue;

    if (sinkpad == NULL || !find_event(sinkpad, ev->event)) {
      ev->received = FALSE;
      pending = TRUE;
    }
  }
  if (pending)
    OLS_OBJECT_FLAG_SET(srcpad, OLS_PAD_FLAG_PENDING_EVENTS);
}

typedef gboolean (*PadEventFunction)(OlsPad *pad, PadEvent *ev,
                                     gpointer user_data);

/* should be called with pad LOCK */
static void events_foreach(OlsPad *pad, PadEventFunction func,
                           gpointer user_data) {
  guint i, len;
  GArray *events;
  gboolean ret;
  guint cookie;

  events = pad->priv->events;

restart:
  cookie = pad->priv->events_cookie;
  i = 0;
  len = events->len;
  while (i < len) {
    PadEvent *ev, ev_ret;

    ev = &g_array_index(events, PadEvent, i);
    if (G_UNLIKELY(ev->event == NULL))
      goto next;

    /* take additional ref, func might release the lock */
    ev_ret.sticky_order = ev->sticky_order;
    ev_ret.event = ols_event_ref(ev->event);
    ev_ret.received = ev->received;

    ret = func(pad, &ev_ret, user_data);

    /* recheck the cookie, lock might have been released and the list could have
     * changed */
    if (G_UNLIKELY(cookie != pad->priv->events_cookie)) {
      if (G_LIKELY(ev_ret.event))
        ols_event_unref(ev_ret.event);
      goto restart;
    }

    /* store the received state */
    ev->received = ev_ret.received;

    /* if the event changed, we need to do something */
    if (G_UNLIKELY(ev->event != ev_ret.event)) {
      if (G_UNLIKELY(ev_ret.event == NULL)) {
        /* function unreffed and set the event to NULL, remove it */
        ols_event_unref(ev->event);
        g_array_remove_index(events, i);
        len--;
        cookie = ++pad->priv->events_cookie;
        continue;
      } else {
        /* function gave a new event for us */
        ols_event_take(&ev->event, ev_ret.event);
      }
    } else {
      /* just unref, nothing changed */
      ols_event_unref(ev_ret.event);
    }
    if (!ret)
      break;
  next:
    i++;
  }
}

/* should be called with LOCK */
static OlsEvent *_apply_pad_offset(OlsPad *pad, OlsEvent *event,
                                   gboolean upstream, gint64 pad_offset) {
  gint64 offset;

  OLS_DEBUG_OBJECT(pad, "apply pad offset %" OLS_STIME_FORMAT,
                   OLS_STIME_ARGS(pad_offset));

  if (OLS_EVENT_TYPE(event) == OLS_EVENT_SEGMENT) {
    OlsSegment segment;
    guint32 seqnum;

    g_assert(!upstream);

    /* copy segment values */
    ols_event_copy_segment(event, &segment);
    seqnum = ols_event_get_seqnum(event);
    ols_event_unref(event);

    ols_segment_offset_running_time(&segment, segment.format, pad_offset);
    event = ols_event_new_segment(&segment);
    ols_event_set_seqnum(event, seqnum);
  }

  event = ols_event_make_writable(event);
  offset = ols_event_get_running_time_offset(event);
  if (upstream)
    offset -= pad_offset;
  else
    offset += pad_offset;
  ols_event_set_running_time_offset(event, offset);

  return event;
}

#define ACQUIRE_PARENT(pad, parent, label)                                     \
  G_STMT_START {                                                               \
    if (G_LIKELY((parent = OLS_OBJECT_PARENT(pad))))                           \
      ols_object_ref(parent);                                                  \
    else if (G_LIKELY(OLS_PAD_NEEDS_PARENT(pad)))                              \
      goto label;                                                              \
  }                                                                            \
  G_STMT_END

#define RELEASE_PARENT(parent)                                                 \
  G_STMT_START {                                                               \
    if (G_LIKELY(parent))                                                      \
      ols_object_unref(parent);                                                \
  }                                                                            \
  G_STMT_END

/**
 * ols_pad_mode_get_name:
 * @mode: the pad mode
 *
 * Return the name of a pad mode, for use in debug messages mostly.
 *
 * Returns: short mnemonic for pad mode @mode
 */
const char *ols_pad_mode_get_name(OlsPadMode mode) {
  switch (mode) {
  case OLS_PAD_MODE_NONE:
    return "none";
  case OLS_PAD_MODE_PUSH:
    return "push";
  case OLS_PAD_MODE_PULL:
    return "pull";
  default:
    break;
  }
  return "unknown";
}

/**
 * ols_pad_is_blocking:
 * @pad: the #OlsPad to query
 *
 * Checks if the pad is blocking or not. This is a guaranteed state
 * of whether the pad is actually blocking on a #OlsBuffer or a #OlsEvent.
 *
 * Returns: %TRUE if the pad is blocking.
 *
 * MT safe.
 */
gboolean ols_pad_is_blocking(OlsPad *pad) {
  gboolean result = FALSE;

  g_return_val_if_fail(OLS_IS_PAD(pad), result);

  OLS_OBJECT_LOCK(pad);
  /* the blocking flag is only valid if the pad is not flushing */
  result = OLS_PAD_IS_BLOCKING(pad) && !OLS_PAD_IS_FLUSHING(pad);
  OLS_OBJECT_UNLOCK(pad);

  return result;
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
void ols_pad_set_chain_function_full(OlsPad *pad, OlsPadChainFunction chain,
                                     gpointer user_data,
                                     GDestroyNotify notify) {
  g_return_if_fail(OLS_IS_PAD(pad));
  g_return_if_fail(OLS_PAD_IS_SINK(pad));

  if (pad->chainnotify)
    pad->chainnotify(pad->chaindata);
  OLS_PAD_CHAINFUNC(pad) = chain;
  pad->chaindata = user_data;
  pad->chainnotify = notify;

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "chainfunc set to %s",
                       OLS_DEBUG_FUNCPTR_NAME(chain));
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
void ols_pad_set_chain_list_function_full(OlsPad *pad,
                                          OlsPadChainListFunction chainlist,
                                          gpointer user_data,
                                          GDestroyNotify notify) {
  g_return_if_fail(OLS_IS_PAD(pad));
  g_return_if_fail(OLS_PAD_IS_SINK(pad));

  if (pad->chainlistnotify)
    pad->chainlistnotify(pad->chainlistdata);
  OLS_PAD_CHAINLISTFUNC(pad) = chainlist;
  pad->chainlistdata = user_data;
  pad->chainlistnotify = notify;

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "chainlistfunc set to %s",
                       OLS_DEBUG_FUNCPTR_NAME(chainlist));
}

/**
 * ols_pad_set_getrange_function:
 * @p: a source #OlsPad.
 * @f: the #OlsPadGetRangeFunction to set.
 *
 * Calls ols_pad_set_getrange_function_full() with %NULL for the user_data and
 * notify.
 */
/**
 * ols_pad_set_getrange_function_full:
 * @pad: a source #OlsPad.
 * @get: the #OlsPadGetRangeFunction to set.
 * @user_data: user_data passed to @notify
 * @notify: notify called when @get will not be used anymore.
 *
 * Sets the given getrange function for the pad. The getrange function is
 * called to produce a new #OlsBuffer to start the processing pipeline. see
 * #OlsPadGetRangeFunction for a description of the getrange function.
 */
void ols_pad_set_getrange_function_full(OlsPad *pad, OlsPadGetRangeFunction get,
                                        gpointer user_data,
                                        GDestroyNotify notify) {

  if (pad->getrangenotify)
    pad->getrangenotify(pad->getrangedata);
  OLS_PAD_GETRANGEFUNC(pad) = get;
  pad->getrangedata = user_data;
  pad->getrangenotify = notify;

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "getrangefunc set to %s",
                       OLS_DEBUG_FUNCPTR_NAME(get));
}

/**
 * ols_pad_set_event_function:
 * @p: a #OlsPad of either direction.
 * @f: the #OlsPadEventFunction to set.
 *
 * Calls ols_pad_set_event_function_full() with %NULL for the user_data and
 * notify.
 */
/**
 * ols_pad_set_event_function_full:
 * @pad: a #OlsPad of either direction.
 * @event: the #OlsPadEventFunction to set.
 * @user_data: user_data passed to @notify
 * @notify: notify called when @event will not be used anymore.
 *
 * Sets the given event handler for the pad.
 */
void ols_pad_set_event_function_full(OlsPad *pad, OlsPadEventFunction event,
                                     gpointer user_data,
                                     GDestroyNotify notify) {
  g_return_if_fail(OLS_IS_PAD(pad));

  if (pad->eventnotify)
    pad->eventnotify(pad->eventdata);
  OLS_PAD_EVENTFUNC(pad) = event;
  pad->eventdata = user_data;
  pad->eventnotify = notify;

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "eventfunc for set to %s",
                       OLS_DEBUG_FUNCPTR_NAME(event));
}

/**
 * ols_pad_set_event_full_function:
 * @p: a #OlsPad of either direction.
 * @f: the #OlsPadEventFullFunction to set.
 *
 * Calls ols_pad_set_event_full_function_full() with %NULL for the user_data and
 * notify.
 */
/**
 * ols_pad_set_event_full_function_full:
 * @pad: a #OlsPad of either direction.
 * @event: the #OlsPadEventFullFunction to set.
 * @user_data: user_data passed to @notify
 * @notify: notify called when @event will not be used anymore.
 *
 * Sets the given event handler for the pad.
 *
 * Since: 1.8
 */
void ols_pad_set_event_full_function_full(OlsPad *pad,
                                          OlsPadEventFullFunction event,
                                          gpointer user_data,
                                          GDestroyNotify notify) {
  g_return_if_fail(OLS_IS_PAD(pad));

  if (pad->eventnotify)
    pad->eventnotify(pad->eventdata);
  OLS_PAD_EVENTFULLFUNC(pad) = event;
  OLS_PAD_EVENTFUNC(pad) = event_wrap;
  pad->eventdata = user_data;
  pad->eventnotify = notify;

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "eventfullfunc for set to %s",
                       OLS_DEBUG_FUNCPTR_NAME(event));
}

/**
 * ols_pad_set_query_function:
 * @p: a #OlsPad of either direction.
 * @f: the #OlsPadQueryFunction to set.
 *
 * Calls ols_pad_set_query_function_full() with %NULL for the user_data and
 * notify.
 */
/**
 * ols_pad_set_query_function_full:
 * @pad: a #OlsPad of either direction.
 * @query: the #OlsPadQueryFunction to set.
 * @user_data: user_data passed to @notify
 * @notify: notify called when @query will not be used anymore.
 *
 * Set the given query function for the pad.
 */
void ols_pad_set_query_function_full(OlsPad *pad, OlsPadQueryFunction query,
                                     gpointer user_data,
                                     GDestroyNotify notify) {
  g_return_if_fail(OLS_IS_PAD(pad));

  if (pad->querynotify)
    pad->querynotify(pad->querydata);
  OLS_PAD_QUERYFUNC(pad) = query;
  pad->querydata = user_data;
  pad->querynotify = notify;

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "queryfunc set to %s",
                       OLS_DEBUG_FUNCPTR_NAME(query));
}

/**
 * ols_pad_set_iterate_internal_links_function:
 * @p: a #OlsPad of either direction.
 * @f: the #OlsPadIterIntLinkFunction to set.
 *
 * Calls ols_pad_set_iterate_internal_links_function_full() with %NULL
 * for the user_data and notify.
 */
/**
 * ols_pad_set_iterate_internal_links_function_full:
 * @pad: a #OlsPad of either direction.
 * @iterintlink: the #OlsPadIterIntLinkFunction to set.
 * @user_data: user_data passed to @notify
 * @notify: notify called when @iterintlink will not be used anymore.
 *
 * Sets the given internal link iterator function for the pad.
 */
void ols_pad_set_iterate_internal_links_function_full(
    OlsPad *pad, OlsPadIterIntLinkFunction iterintlink, gpointer user_data,
    GDestroyNotify notify) {
  g_return_if_fail(OLS_IS_PAD(pad));

  if (pad->iterintlinknotify)
    pad->iterintlinknotify(pad->iterintlinkdata);
  OLS_PAD_ITERINTLINKFUNC(pad) = iterintlink;
  pad->iterintlinkdata = user_data;
  pad->iterintlinknotify = notify;

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "internal link iterator set to %s",
                       OLS_DEBUG_FUNCPTR_NAME(iterintlink));
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
void ols_pad_set_link_function_full(OlsPad *pad, OlsPadLinkFunction link,
                                    gpointer user_data, GDestroyNotify notify) {
  g_return_if_fail(OLS_IS_PAD(pad));

  if (pad->linknotify)
    pad->linknotify(pad->linkdata);
  OLS_PAD_LINKFUNC(pad) = link;
  pad->linkdata = user_data;
  pad->linknotify = notify;

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "linkfunc set to %s",
                       OLS_DEBUG_FUNCPTR_NAME(link));
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
void ols_pad_set_unlink_function_full(OlsPad *pad, OlsPadUnlinkFunction unlink,
                                      gpointer user_data,
                                      GDestroyNotify notify) {
  g_return_if_fail(OLS_IS_PAD(pad));

  if (pad->unlinknotify)
    pad->unlinknotify(pad->unlinkdata);
  OLS_PAD_UNLINKFUNC(pad) = unlink;
  pad->unlinkdata = user_data;
  pad->unlinknotify = notify;

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "unlinkfunc set to %s",
                       OLS_DEBUG_FUNCPTR_NAME(unlink));
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
gboolean ols_pad_unlink(OlsPad *srcpad, OlsPad *sinkpad) {
  gboolean result = FALSE;
  OlsElement *parent = NULL;

  g_return_val_if_fail(OLS_IS_PAD(srcpad), FALSE);
  g_return_val_if_fail(OLS_PAD_IS_SRC(srcpad), FALSE);
  g_return_val_if_fail(OLS_IS_PAD(sinkpad), FALSE);
  g_return_val_if_fail(OLS_PAD_IS_SINK(sinkpad), FALSE);

  OLS_TRACER_PAD_UNLINK_PRE(srcpad, sinkpad);

  OLS_CAT_INFO(OLS_CAT_ELEMENT_PADS, "unlinking %s:%s(%p) and %s:%s(%p)",
               OLS_DEBUG_PAD_NAME(srcpad), srcpad, OLS_DEBUG_PAD_NAME(sinkpad),
               sinkpad);

  /* We need to notify the parent before taking any pad locks as the bin in
   * question might be waiting for a lock on the pad while holding its lock
   * that our message will try to take. */
  if ((parent = OLS_ELEMENT_CAST(ols_pad_get_parent(srcpad)))) {
    if (OLS_IS_ELEMENT(parent)) {
      ols_element_post_message(parent, ols_message_new_structure_change(
                                           OLS_OBJECT_CAST(sinkpad),
                                           OLS_STRUCTURE_CHANGE_TYPE_PAD_UNLINK,
                                           parent, TRUE));
    } else {
      ols_object_unref(parent);
      parent = NULL;
    }
  }

  OLS_OBJECT_LOCK(srcpad);
  OLS_OBJECT_LOCK(sinkpad);

  if (G_UNLIKELY(OLS_PAD_PEER(srcpad) != sinkpad))
    goto not_linked_together;

  if (OLS_PAD_UNLINKFUNC(srcpad)) {
    OlsObject *tmpparent;

    ACQUIRE_PARENT(srcpad, tmpparent, no_src_parent);

    OLS_PAD_UNLINKFUNC(srcpad)(srcpad, tmpparent);
    RELEASE_PARENT(tmpparent);
  }
no_src_parent:
  if (OLS_PAD_UNLINKFUNC(sinkpad)) {
    OlsObject *tmpparent;

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
  g_signal_emit(srcpad, ols_pad_signals[PAD_UNLINKED], 0, sinkpad);
  g_signal_emit(sinkpad, ols_pad_signals[PAD_UNLINKED], 0, srcpad);

  OLS_CAT_INFO(OLS_CAT_ELEMENT_PADS, "unlinked %s:%s and %s:%s",
               OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad));

  result = TRUE;

done:
  if (parent != NULL) {
    ols_element_post_message(parent, ols_message_new_structure_change(
                                         OLS_OBJECT_CAST(sinkpad),
                                         OLS_STRUCTURE_CHANGE_TYPE_PAD_UNLINK,
                                         parent, FALSE));
    ols_object_unref(parent);
  }
  OLS_TRACER_PAD_UNLINK_POST(srcpad, sinkpad, result);
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
gboolean ols_pad_is_linked(OlsPad *pad) {
  gboolean result;

  g_return_val_if_fail(OLS_IS_PAD(pad), FALSE);

  OLS_OBJECT_LOCK(pad);
  result = (OLS_PAD_PEER(pad) != NULL);
  OLS_OBJECT_UNLOCK(pad);

  return result;
}

/* get the caps from both pads and see if the intersection
 * is not empty.
 *
 * This function should be called with the pad LOCK on both
 * pads
 */
static gboolean ols_pad_link_check_compatible_unlocked(OlsPad *src,
                                                       OlsPad *sink,
                                                       OlsPadLinkCheck flags) {
  OlsCaps *srccaps = NULL;
  OlsCaps *sinkcaps = NULL;
  gboolean compatible = FALSE;

  if (!(flags & (OLS_PAD_LINK_CHECK_CAPS | OLS_PAD_LINK_CHECK_TEMPLATE_CAPS)))
    return TRUE;

  /* Doing the expensive caps checking takes priority over only checking the
   * template caps */
  if (flags & OLS_PAD_LINK_CHECK_CAPS) {
    OLS_OBJECT_UNLOCK(sink);
    OLS_OBJECT_UNLOCK(src);

    srccaps = ols_pad_query_caps(src, NULL);
    sinkcaps = ols_pad_query_caps(sink, NULL);

    OLS_OBJECT_LOCK(src);
    OLS_OBJECT_LOCK(sink);
  } else {
    /* If one of the two pads doesn't have a template, consider the intersection
     * as valid.*/
    if (G_UNLIKELY((OLS_PAD_PAD_TEMPLATE(src) == NULL) ||
                   (OLS_PAD_PAD_TEMPLATE(sink) == NULL))) {
      compatible = TRUE;
      goto done;
    }
    srccaps = ols_caps_ref(OLS_PAD_TEMPLATE_CAPS(OLS_PAD_PAD_TEMPLATE(src)));
    sinkcaps = ols_caps_ref(OLS_PAD_TEMPLATE_CAPS(OLS_PAD_PAD_TEMPLATE(sink)));
  }

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_CAPS, src, "src caps %" OLS_PTR_FORMAT, srccaps);
  OLS_CAT_DEBUG_OBJECT(OLS_CAT_CAPS, sink, "sink caps %" OLS_PTR_FORMAT,
                       sinkcaps);

  /* if we have caps on both pads we can check the intersection. If one
   * of the caps is %NULL, we return %TRUE. */
  if (G_UNLIKELY(srccaps == NULL || sinkcaps == NULL)) {
    if (srccaps)
      ols_caps_unref(srccaps);
    if (sinkcaps)
      ols_caps_unref(sinkcaps);
    goto done;
  }

  compatible = ols_caps_can_intersect(srccaps, sinkcaps);
  ols_caps_unref(srccaps);
  ols_caps_unref(sinkcaps);

done:
  OLS_CAT_DEBUG(OLS_CAT_CAPS, "caps are %scompatible",
                (compatible ? "" : "not "));

  return compatible;
}

/* check if the grandparents of both pads are the same.
 * This check is required so that we don't try to link
 * pads from elements in different bins without ghostpads.
 *
 * The LOCK should be held on both pads
 */
static gboolean ols_pad_link_check_hierarchy(OlsPad *src, OlsPad *sink) {
  OlsObject *psrc, *psink;

  psrc = OLS_OBJECT_PARENT(src);
  psink = OLS_OBJECT_PARENT(sink);

  /* if one of the pads has no parent, we allow the link */
  if (G_UNLIKELY(psrc == NULL || psink == NULL))
    goto no_parent;

  /* only care about parents that are elements */
  if (G_UNLIKELY(!OLS_IS_ELEMENT(psrc) || !OLS_IS_ELEMENT(psink)))
    goto no_element_parent;

  /* if the parents are the same, we have a loop */
  if (G_UNLIKELY(psrc == psink))
    goto same_parents;

  /* if they both have a parent, we check the grandparents. We can not lock
   * the parent because we hold on the child (pad) and the locking order is
   * parent >> child. */
  psrc = OLS_OBJECT_PARENT(psrc);
  psink = OLS_OBJECT_PARENT(psink);

  /* if they have grandparents but they are not the same */
  if (G_UNLIKELY(psrc != psink))
    goto wrong_grandparents;

  return TRUE;

  /* ERRORS */
no_parent : {
  OLS_CAT_DEBUG(OLS_CAT_CAPS,
                "one of the pads has no parent %" OLS_PTR_FORMAT
                " and %" OLS_PTR_FORMAT,
                psrc, psink);
  return TRUE;
}
no_element_parent : {
  OLS_CAT_DEBUG(OLS_CAT_CAPS,
                "one of the pads has no element parent %" OLS_PTR_FORMAT
                " and %" OLS_PTR_FORMAT,
                psrc, psink);
  return TRUE;
}
same_parents : {
  OLS_CAT_DEBUG(OLS_CAT_CAPS, "pads have same parent %" OLS_PTR_FORMAT, psrc);
  return FALSE;
}
wrong_grandparents : {
  OLS_CAT_DEBUG(OLS_CAT_CAPS,
                "pads have different grandparents %" OLS_PTR_FORMAT
                " and %" OLS_PTR_FORMAT,
                psrc, psink);
  return FALSE;
}
}

/**
 * ols_pad_can_link:
 * @srcpad: the source #OlsPad.
 * @sinkpad: the sink #OlsPad.
 *
 * Checks if the source pad and the sink pad are compatible so they can be
 * linked.
 *
 * Returns: %TRUE if the pads can be linked.
 */
gboolean ols_pad_can_link(OlsPad *srcpad, OlsPad *sinkpad) {
  OlsPadLinkReturn result;

  /* generic checks */
  g_return_val_if_fail(OLS_IS_PAD(srcpad), FALSE);
  g_return_val_if_fail(OLS_IS_PAD(sinkpad), FALSE);

  OLS_CAT_INFO(OLS_CAT_PADS, "check if %s:%s can link with %s:%s",
               OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad));

  /* ols_pad_link_prepare does everything for us, we only release the locks
   * on the pads that it gets us. If this function returns !OK the locks are not
   * taken anymore. */
  result = ols_pad_link_prepare(srcpad, sinkpad, OLS_PAD_LINK_CHECK_DEFAULT);
  if (result != OLS_PAD_LINK_OK)
    goto done;

  OLS_OBJECT_UNLOCK(srcpad);
  OLS_OBJECT_UNLOCK(sinkpad);

done:
  return result == OLS_PAD_LINK_OK;
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
OlsPadLinkReturn ols_pad_link_full(OlsPad *srcpad, OlsPad *sinkpad,
                                   OlsPadLinkCheck flags) {
  OlsPadLinkReturn result;
  OlsElement *parent;
  OlsPadLinkFunction srcfunc, sinkfunc;

  g_return_val_if_fail(OLS_IS_PAD(srcpad), OLS_PAD_LINK_REFUSED);
  g_return_val_if_fail(OLS_PAD_IS_SRC(srcpad), OLS_PAD_LINK_WRONG_DIRECTION);
  g_return_val_if_fail(OLS_IS_PAD(sinkpad), OLS_PAD_LINK_REFUSED);
  g_return_val_if_fail(OLS_PAD_IS_SINK(sinkpad), OLS_PAD_LINK_WRONG_DIRECTION);

  OLS_TRACER_PAD_LINK_PRE(srcpad, sinkpad);

  /* Notify the parent early. See ols_pad_unlink for details. */
  if (G_LIKELY((parent = OLS_ELEMENT_CAST(ols_pad_get_parent(srcpad))))) {
    if (G_LIKELY(OLS_IS_ELEMENT(parent))) {
      ols_element_post_message(parent, ols_message_new_structure_change(
                                           OLS_OBJECT_CAST(sinkpad),
                                           OLS_STRUCTURE_CHANGE_TYPE_PAD_LINK,
                                           parent, TRUE));
    } else {
      ols_object_unref(parent);
      parent = NULL;
    }
  }

  /* prepare will also lock the two pads */
  result = ols_pad_link_prepare(srcpad, sinkpad, flags);

  if (G_UNLIKELY(result != OLS_PAD_LINK_OK)) {
    OLS_CAT_INFO(OLS_CAT_PADS, "link between %s:%s and %s:%s failed: %s",
                 OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad),
                 ols_pad_link_get_name(result));
    goto done;
  }

  /* must set peers before calling the link function */
  OLS_PAD_PEER(srcpad) = sinkpad;
  OLS_PAD_PEER(sinkpad) = srcpad;

  /* check events, when something is different, mark pending */
  schedule_events(srcpad, sinkpad);

  /* get the link functions */
  srcfunc = OLS_PAD_LINKFUNC(srcpad);
  sinkfunc = OLS_PAD_LINKFUNC(sinkpad);

  if (G_UNLIKELY(srcfunc || sinkfunc)) {
    /* custom link functions, execute them */
    OLS_OBJECT_UNLOCK(sinkpad);
    OLS_OBJECT_UNLOCK(srcpad);

    if (srcfunc) {
      OlsObject *tmpparent;

      ACQUIRE_PARENT(srcpad, tmpparent, no_parent);
      /* this one will call the peer link function */
      result = srcfunc(srcpad, tmpparent, sinkpad);
      RELEASE_PARENT(tmpparent);
    } else if (sinkfunc) {
      OlsObject *tmpparent;

      ACQUIRE_PARENT(sinkpad, tmpparent, no_parent);
      /* if no source link function, we need to call the sink link
       * function ourselves. */
      result = sinkfunc(sinkpad, tmpparent, srcpad);
      RELEASE_PARENT(tmpparent);
    }
  no_parent:

    OLS_OBJECT_LOCK(srcpad);
    OLS_OBJECT_LOCK(sinkpad);

    /* we released the lock, check if the same pads are linked still */
    if (OLS_PAD_PEER(srcpad) != sinkpad || OLS_PAD_PEER(sinkpad) != srcpad)
      goto concurrent_link;

    if (G_UNLIKELY(result != OLS_PAD_LINK_OK))
      goto link_failed;
  }
  OLS_OBJECT_UNLOCK(sinkpad);
  OLS_OBJECT_UNLOCK(srcpad);

  /* fire off a signal to each of the pads telling them
   * that they've been linked */
  g_signal_emit(srcpad, ols_pad_signals[PAD_LINKED], 0, sinkpad);
  g_signal_emit(sinkpad, ols_pad_signals[PAD_LINKED], 0, srcpad);

  OLS_CAT_INFO(OLS_CAT_PADS, "linked %s:%s and %s:%s, successful",
               OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad));

  if (!(flags & OLS_PAD_LINK_CHECK_NO_RECONFIGURE))
    ols_pad_send_event(srcpad, ols_event_new_reconfigure());

done:
  if (G_LIKELY(parent)) {
    ols_element_post_message(parent, ols_message_new_structure_change(
                                         OLS_OBJECT_CAST(sinkpad),
                                         OLS_STRUCTURE_CHANGE_TYPE_PAD_LINK,
                                         parent, FALSE));
    ols_object_unref(parent);
  }

  OLS_TRACER_PAD_LINK_POST(srcpad, sinkpad, result);
  return result;

  /* ERRORS */
concurrent_link : {
  OLS_CAT_INFO(OLS_CAT_PADS, "concurrent link between %s:%s and %s:%s",
               OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad));
  OLS_OBJECT_UNLOCK(sinkpad);
  OLS_OBJECT_UNLOCK(srcpad);

  /* The other link operation succeeded first */
  result = OLS_PAD_LINK_WAS_LINKED;
  goto done;
}
link_failed : {
  OLS_CAT_INFO(OLS_CAT_PADS, "link between %s:%s and %s:%s failed: %s",
               OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad),
               ols_pad_link_get_name(result));

  OLS_PAD_PEER(srcpad) = NULL;
  OLS_PAD_PEER(sinkpad) = NULL;

  OLS_OBJECT_UNLOCK(sinkpad);
  OLS_OBJECT_UNLOCK(srcpad);

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
OlsPadLinkReturn ols_pad_link(OlsPad *srcpad, OlsPad *sinkpad) {
  return ols_pad_link_full(srcpad, sinkpad, OLS_PAD_LINK_CHECK_DEFAULT);
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
OlsPad *ols_pad_get_peer(OlsPad *pad) {
  OlsPad *result;

  g_return_val_if_fail(OLS_IS_PAD(pad), NULL);

  OLS_OBJECT_LOCK(pad);
  result = OLS_PAD_PEER(pad);
  if (result)
    ols_object_ref(result);
  OLS_OBJECT_UNLOCK(pad);

  return result;
}

/**
 * ols_pad_get_allowed_caps:
 * @pad: a #OlsPad.
 *
 * Gets the capabilities of the allowed media types that can flow through
 * @pad and its peer.
 *
 * The allowed capabilities is calculated as the intersection of the results of
 * calling ols_pad_query_caps() on @pad and its peer. The caller owns a
 * reference on the resulting caps.
 *
 * Returns: (transfer full) (nullable): the allowed #OlsCaps of the
 *     pad link. Unref the caps when you no longer need it. This
 *     function returns %NULL when @pad has no peer.
 *
 * MT safe.
 */
OlsCaps *ols_pad_get_allowed_caps(OlsPad *pad) {
  OlsCaps *mycaps;
  OlsCaps *caps = NULL;
  OlsQuery *query;

  g_return_val_if_fail(OLS_IS_PAD(pad), NULL);

  OLS_OBJECT_LOCK(pad);
  if (G_UNLIKELY(OLS_PAD_PEER(pad) == NULL))
    goto no_peer;
  OLS_OBJECT_UNLOCK(pad);

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_PROPERTIES, pad, "getting allowed caps");

  mycaps = ols_pad_query_caps(pad, NULL);

  /* Query peer caps */
  query = ols_query_new_caps(mycaps);
  if (!ols_pad_peer_query(pad, query)) {
    OLS_CAT_DEBUG_OBJECT(OLS_CAT_CAPS, pad, "Caps query failed");
    goto end;
  }

  ols_query_parse_caps_result(query, &caps);
  if (caps == NULL) {
    g_warn_if_fail(caps != NULL);
    goto end;
  }
  ols_caps_ref(caps);

  OLS_CAT_DEBUG_OBJECT(OLS_CAT_CAPS, pad, "allowed caps %" OLS_PTR_FORMAT,
                       caps);

end:
  ols_query_unref(query);
  ols_caps_unref(mycaps);

  return caps;

no_peer : {
  OLS_CAT_DEBUG_OBJECT(OLS_CAT_PROPERTIES, pad, "no peer");
  OLS_OBJECT_UNLOCK(pad);

  return NULL;
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
gboolean ols_pad_push_event(OlsPad *pad, OlsEvent *event) {
  gboolean res = FALSE;
  OlsPadProbeType type;
  gboolean sticky, serialized;

  g_return_val_if_fail(OLS_IS_PAD(pad), FALSE);
  g_return_val_if_fail(OLS_IS_EVENT(event), FALSE);

  OLS_TRACER_PAD_PUSH_EVENT_PRE(pad, event);

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
    /* srcpad sticky events are stored immediately, the received flag is set
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
     * Note that we do not do this for non-serialized sticky events since it
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
      res = TRUE;
    ols_event_unref(event);
  }
  OLS_OBJECT_UNLOCK(pad);

  OLS_TRACER_PAD_PUSH_EVENT_POST(pad, res);
  return res;

  /* ERROR handling */
wrong_direction : {
  g_warning("pad %s:%s pushing %s event in wrong direction",
            OLS_DEBUG_PAD_NAME(pad), OLS_EVENT_TYPE_NAME(event));
  ols_event_unref(event);
  goto done;
}
unknown_direction : {
  g_warning("pad %s:%s has invalid direction", OLS_DEBUG_PAD_NAME(pad));
  ols_event_unref(event);
  goto done;
}
flushed : {
  OLS_DEBUG_OBJECT(pad, "We're flushing");
  OLS_OBJECT_UNLOCK(pad);
  ols_event_unref(event);
  goto done;
}
eos : {
  OLS_DEBUG_OBJECT(pad, "We're EOS");
  OLS_OBJECT_UNLOCK(pad);
  ols_event_unref(event);
  goto done;
}
done:
  OLS_TRACER_PAD_PUSH_EVENT_POST(pad, FALSE);
  return FALSE;
}
