#include "ols-pad.h"
#include "ols-internal.h"
#include <stdbool.h>

static void ols_pad_init(ols_pad_t *pad) { UNUSED_PARAMETER(pad); }

#define _to_sticky_order(t) ols_event_type_to_sticky_ordering(t)

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

  UNUSED_PARAMETER(pad);
  UNUSED_PARAMETER(chain);
  UNUSED_PARAMETER(user_data);
  // if (pad->chainnotify)
  //   pad->chainnotify(pad->chaindata);
  // OLS_PAD_CHAINFUNC(pad) = chain;
  // pad->chaindata = user_data;
  // pad->chainnotify = notify;

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

  UNUSED_PARAMETER(pad);
  UNUSED_PARAMETER(chainlist);
  UNUSED_PARAMETER(user_data);
  // if (pad->chainlistnotify)
  //   pad->chainlistnotify(pad->chainlistdata);
  // OLS_PAD_CHAINLISTFUNC(pad) = chainlist;
  // pad->chainlistdata = user_data;
  // pad->chainlistnotify = notify;

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

  UNUSED_PARAMETER(pad);
  UNUSED_PARAMETER(link);
  UNUSED_PARAMETER(user_data);
  //   if (pad->linknotify)
  //     pad->linknotify(pad->linkdata);
  //   OLS_PAD_LINKFUNC(pad) = link;
  //   pad->linkdata = user_data;
  //   pad->linknotify = notify;

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

  UNUSED_PARAMETER(pad);
  UNUSED_PARAMETER(unlink);
  UNUSED_PARAMETER(user_data);
  // if (pad->unlinknotify)
  //   pad->unlinknotify(pad->unlinkdata);
  // OLS_PAD_UNLINKFUNC(pad) = unlink;
  // pad->unlinkdata = user_data;
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
  UNUSED_PARAMETER(srcpad);
  UNUSED_PARAMETER(sinkpad);
  //   OLS_TRACER_PAD_UNLINK_PRE(srcpad, sinkpad);

  //   OLS_CAT_INFO(OLS_CAT_ELEMENT_PADS, "unlinking %s:%s(%p) and %s:%s(%p)",
  //                OLS_DEBUG_PAD_NAME(srcpad), srcpad,
  //                OLS_DEBUG_PAD_NAME(sinkpad), sinkpad);

  //   /* We need to notify the parent before taking any pad locks as the bin in
  //    * question might be waiting for a lock on the pad while holding its lock
  //    * that our message will try to take. */
  //   if ((parent = OLS_ELEMENT_CAST(ols_pad_get_parent(srcpad)))) {
  //     if (OLS_IS_ELEMENT(parent)) {
  //       ols_element_post_message(parent, ols_message_new_structure_change(
  //                                            OLS_OBJECT_CAST(sinkpad),
  //                                            OLS_STRUCTURE_CHANGE_TYPE_PAD_UNLINK,
  //                                            parent, TRUE));
  //     } else {
  //       ols_object_unref(parent);
  //       parent = NULL;
  //     }
  //   }

  //   OLS_OBJECT_LOCK(srcpad);
  //   OLS_OBJECT_LOCK(sinkpad);

  //   if (G_UNLIKELY(OLS_PAD_PEER(srcpad) != sinkpad))
  //     goto not_linked_together;

  //   if (OLS_PAD_UNLINKFUNC(srcpad)) {
  //     OlsObject *tmpparent;

  //     ACQUIRE_PARENT(srcpad, tmpparent, no_src_parent);

  //     OLS_PAD_UNLINKFUNC(srcpad)(srcpad, tmpparent);
  //     RELEASE_PARENT(tmpparent);
  //   }
  // no_src_parent:
  //   if (OLS_PAD_UNLINKFUNC(sinkpad)) {
  //     OlsObject *tmpparent;

  //     ACQUIRE_PARENT(sinkpad, tmpparent, no_sink_parent);

  //     OLS_PAD_UNLINKFUNC(sinkpad)(sinkpad, tmpparent);
  //     RELEASE_PARENT(tmpparent);
  //   }
  // no_sink_parent:

  //   /* first clear peers */
  //   OLS_PAD_PEER(srcpad) = NULL;
  //   OLS_PAD_PEER(sinkpad) = NULL;

  //   OLS_OBJECT_UNLOCK(sinkpad);
  //   OLS_OBJECT_UNLOCK(srcpad);

  //   /* fire off a signal to each of the pads telling them
  //    * that they've been unlinked */
  //   g_signal_emit(srcpad, ols_pad_signals[PAD_UNLINKED], 0, sinkpad);
  //   g_signal_emit(sinkpad, ols_pad_signals[PAD_UNLINKED], 0, srcpad);

  //   OLS_CAT_INFO(OLS_CAT_ELEMENT_PADS, "unlinked %s:%s and %s:%s",
  //                OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad));

  //   result = TRUE;

  // done:
  //   if (parent != NULL) {
  //     ols_element_post_message(parent, ols_message_new_structure_change(
  //                                          OLS_OBJECT_CAST(sinkpad),
  //                                          OLS_STRUCTURE_CHANGE_TYPE_PAD_UNLINK,
  //                                          parent, FALSE));
  //     ols_object_unref(parent);
  //   }
  //   OLS_TRACER_PAD_UNLINK_POST(srcpad, sinkpad, result);
  return result;

  //   /* ERRORS */
  // not_linked_together : {
  //   /* we do not emit a warning in this case because unlinking cannot
  //    * be made MT safe.*/
  //   OLS_OBJECT_UNLOCK(sinkpad);
  //   OLS_OBJECT_UNLOCK(srcpad);
  //   goto done;
  // }
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
  ;
  UNUSED_PARAMETER(pad);

  // g_return_val_if_fail(OLS_IS_PAD(pad), FALSE);

  // OLS_OBJECT_LOCK(pad);
  // result = (OLS_PAD_PEER(pad) != NULL);
  // OLS_OBJECT_UNLOCK(pad);

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
  OlsPadLinkReturn result = false;
  ;
  // OlsElement *parent;
  // ols_pad_link_function srcfunc, sinkfunc;
  UNUSED_PARAMETER(srcpad);
  UNUSED_PARAMETER(sinkpad);
  // g_return_val_if_fail(OLS_IS_PAD(srcpad), OLS_PAD_LINK_REFUSED);
  // g_return_val_if_fail(OLS_PAD_IS_SRC(srcpad), OLS_PAD_LINK_WRONG_DIRECTION);
  // g_return_val_if_fail(OLS_IS_PAD(sinkpad), OLS_PAD_LINK_REFUSED);
  // g_return_val_if_fail(OLS_PAD_IS_SINK(sinkpad),
  // OLS_PAD_LINK_WRONG_DIRECTION);

  // OLS_TRACER_PAD_LINK_PRE(srcpad, sinkpad);

  /* Notify the parent early. See ols_pad_unlink for details. */
  // if (G_LIKELY((parent = OLS_ELEMENT_CAST(ols_pad_get_parent(srcpad))))) {
  //   if (G_LIKELY(OLS_IS_ELEMENT(parent))) {
  //     ols_element_post_message(parent, ols_message_new_structure_change(
  //                                          OLS_OBJECT_CAST(sinkpad),
  //                                          OLS_STRUCTURE_CHANGE_TYPE_PAD_LINK,
  //                                          parent, TRUE));
  //   } else {
  //     ols_object_unref(parent);
  //     parent = NULL;
  //   }
  // }

  /* prepare will also lock the two pads */
  // result = ols_pad_link_prepare(srcpad, sinkpad, flags);

  // if (G_UNLIKELY(result != OLS_PAD_LINK_OK)) {
  //   OLS_CAT_INFO(OLS_CAT_PADS, "link between %s:%s and %s:%s failed: %s",
  //                OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad),
  //                ols_pad_link_get_name(result));
  //   goto done;
  // }

  /* must set peers before calling the link function */
  // OLS_PAD_PEER(srcpad) = sinkpad;
  // OLS_PAD_PEER(sinkpad) = srcpad;

  /* check events, when something is different, mark pending */
  // schedule_events(srcpad, sinkpad);

  /* get the link functions */
  //   srcfunc = OLS_PAD_LINKFUNC(srcpad);
  //   sinkfunc = OLS_PAD_LINKFUNC(sinkpad);

  //   if (G_UNLIKELY(srcfunc || sinkfunc)) {
  //     /* custom link functions, execute them */
  //     OLS_OBJECT_UNLOCK(sinkpad);
  //     OLS_OBJECT_UNLOCK(srcpad);

  //     if (srcfunc) {
  //       OlsObject *tmpparent;

  //       ACQUIRE_PARENT(srcpad, tmpparent, no_parent);
  //       /* this one will call the peer link function */
  //       result = srcfunc(srcpad, tmpparent, sinkpad);
  //       RELEASE_PARENT(tmpparent);
  //     } else if (sinkfunc) {
  //       OlsObject *tmpparent;

  //       ACQUIRE_PARENT(sinkpad, tmpparent, no_parent);
  //       /* if no source link function, we need to call the sink link
  //        * function ourselves. */
  //       result = sinkfunc(sinkpad, tmpparent, srcpad);
  //       RELEASE_PARENT(tmpparent);
  //     }
  //   no_parent:

  //     OLS_OBJECT_LOCK(srcpad);
  //     OLS_OBJECT_LOCK(sinkpad);

  //     /* we released the lock, check if the same pads are linked still */
  //     if (OLS_PAD_PEER(srcpad) != sinkpad || OLS_PAD_PEER(sinkpad) != srcpad)
  //       goto concurrent_link;

  //     if (G_UNLIKELY(result != OLS_PAD_LINK_OK))
  //       goto link_failed;
  //   }
  //   OLS_OBJECT_UNLOCK(sinkpad);
  //   OLS_OBJECT_UNLOCK(srcpad);

  //   /* fire off a signal to each of the pads telling them
  //    * that they've been linked */
  //   g_signal_emit(srcpad, ols_pad_signals[PAD_LINKED], 0, sinkpad);
  //   g_signal_emit(sinkpad, ols_pad_signals[PAD_LINKED], 0, srcpad);

  //   OLS_CAT_INFO(OLS_CAT_PADS, "linked %s:%s and %s:%s, successful",
  //                OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad));

  //   if (!(flags & OLS_PAD_LINK_CHECK_NO_RECONFIGURE))
  //     ols_pad_send_event(srcpad, ols_event_new_reconfigure());

  // done:
  //   if (G_LIKELY(parent)) {
  //     ols_element_post_message(parent, ols_message_new_structure_change(
  //                                          OLS_OBJECT_CAST(sinkpad),
  //                                          OLS_STRUCTURE_CHANGE_TYPE_PAD_LINK,
  //                                          parent, FALSE));
  //     ols_object_unref(parent);
  //   }

  //   OLS_TRACER_PAD_LINK_POST(srcpad, sinkpad, result);
  return result;

  //   /* ERRORS */
  // concurrent_link : {
  //   OLS_CAT_INFO(OLS_CAT_PADS, "concurrent link between %s:%s and %s:%s",
  //                OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad));
  //   OLS_OBJECT_UNLOCK(sinkpad);
  //   OLS_OBJECT_UNLOCK(srcpad);

  //   /* The other link operation succeeded first */
  //   result = OLS_PAD_LINK_WAS_LINKED;
  //   goto done;
  // }
  // link_failed : {
  //   OLS_CAT_INFO(OLS_CAT_PADS, "link between %s:%s and %s:%s failed: %s",
  //                OLS_DEBUG_PAD_NAME(srcpad), OLS_DEBUG_PAD_NAME(sinkpad),
  //                ols_pad_link_get_name(result));

  //   OLS_PAD_PEER(srcpad) = NULL;
  //   OLS_PAD_PEER(sinkpad) = NULL;

  //   OLS_OBJECT_UNLOCK(sinkpad);
  //   OLS_OBJECT_UNLOCK(srcpad);

  //   goto done;
  // }
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
  UNUSED_PARAMETER(pad);
  // OLS_OBJECT_LOCK(pad);
  // result = OLS_PAD_PEER(pad);
  // if (result)
  //   ols_object_ref(result);
  // OLS_OBJECT_UNLOCK(pad);

  return result;
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
  UNUSED_PARAMETER(pad);
  UNUSED_PARAMETER(event);
  //   OlsPadProbeType type;
  //   bool sticky, serialized;

  //   g_return_val_if_fail(OLS_IS_PAD(pad), FALSE);
  //   g_return_val_if_fail(OLS_IS_EVENT(event), FALSE);

  //   OLS_TRACER_PAD_PUSH_EVENT_PRE(pad, event);

  //   if (OLS_PAD_IS_SRC(pad)) {
  //     if (G_UNLIKELY(!OLS_EVENT_IS_DOWNSTREAM(event)))
  //       goto wrong_direction;
  //     type = OLS_PAD_PROBE_TYPE_EVENT_DOWNSTREAM;
  //   } else if (OLS_PAD_IS_SINK(pad)) {
  //     if (G_UNLIKELY(!OLS_EVENT_IS_UPSTREAM(event)))
  //       goto wrong_direction;
  //     /* events pushed on sinkpad never are sticky */
  //     type = OLS_PAD_PROBE_TYPE_EVENT_UPSTREAM;
  //   } else
  //     goto unknown_direction;

  //   OLS_OBJECT_LOCK(pad);
  //   sticky = OLS_EVENT_IS_STICKY(event);
  //   serialized = OLS_EVENT_IS_SERIALIZED(event);

  //   if (sticky) {
  //     /* srcpad sticky events are stored immediately, the received flag is
  //     set
  //      * to FALSE and will be set to TRUE when we can successfully push the
  //      * event to the peer pad */
  //     switch (store_sticky_event(pad, event)) {
  //     case OLS_FLOW_FLUSHING:
  //       goto flushed;
  //     case OLS_FLOW_EOS:
  //       goto eos;
  //     default:
  //       break;
  //     }
  //   }
  //   if (OLS_PAD_IS_SRC(pad) && serialized) {
  //     /* All serialized events on the srcpad trigger push of sticky events.
  //      *
  //      * Note that we do not do this for non-serialized sticky events since
  //      it
  //      * could potentially block. */
  //     res = (check_sticky(pad, event) == OLS_FLOW_OK);
  //   }
  //   if (!serialized || !sticky) {
  //     OlsFlowReturn ret;

  //     /* non-serialized and non-sticky events are pushed right away. */
  //     ret = ols_pad_push_event_unchecked(pad, event, type);
  //     /* dropped events by a probe are not an error */
  //     res = (ret == OLS_FLOW_OK || ret == OLS_FLOW_CUSTOM_SUCCESS ||
  //            ret == OLS_FLOW_CUSTOM_SUCCESS_1);
  //   } else {
  //     /* Errors in sticky event pushing are no problem and ignored here
  //      * as they will cause more meaningful errors during data flow.
  //      * For EOS events, that are not followed by data flow, we still
  //      * return FALSE here though.
  //      */
  //     if (OLS_EVENT_TYPE(event) != OLS_EVENT_EOS)
  //       res = TRUE;
  //     ols_event_unref(event);
  //   }
  //   OLS_OBJECT_UNLOCK(pad);

  //   OLS_TRACER_PAD_PUSH_EVENT_POST(pad, res);
  //   return res;

  //   /* ERROR handling */
  // wrong_direction : {
  //   g_warning("pad %s:%s pushing %s event in wrong direction",
  //             OLS_DEBUG_PAD_NAME(pad), OLS_EVENT_TYPE_NAME(event));
  //   ols_event_unref(event);
  //   goto done;
  // }
  // unknown_direction : {
  //   g_warning("pad %s:%s has invalid direction", OLS_DEBUG_PAD_NAME(pad));
  //   ols_event_unref(event);
  //   goto done;
  // }
  // flushed : {
  //   OLS_DEBUG_OBJECT(pad, "We're flushing");
  //   OLS_OBJECT_UNLOCK(pad);
  //   ols_event_unref(event);
  //   goto done;
  // }
  // eos : {
  //   OLS_DEBUG_OBJECT(pad, "We're EOS");
  //   OLS_OBJECT_UNLOCK(pad);
  //   ols_event_unref(event);
  //   goto done;
  // }
  // done:
  //   OLS_TRACER_PAD_PUSH_EVENT_POST(pad, FALSE);
  return res;
}
