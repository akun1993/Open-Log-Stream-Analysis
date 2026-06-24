#include "ols-pad.h"
#include <stdbool.h>
#include "ols-internal.h"
#include "util/debug.h"

static OlsFlowReturn ols_pad_send_event_unchecked(ols_pad_t *pad, ols_event_t *event, OlsPadProbeType type);
static OlsFlowReturn ols_pad_push_event_unchecked(ols_pad_t *pad, ols_event_t *event, OlsPadProbeType type);

static OlsFlowReturn ols_pad_chain_list_default(ols_pad_t *pad, ols_object_t *parent, ols_buffer_list_t *list);

static bool ols_pad_event_default(ols_pad_t *pad, ols_object_t *parent, ols_event_t *event);

#define ACQUIRE_PARENT(pad, parent, label)  \
    do {                                    \
        if ((parent = OLS_PAD_PARENT(pad))) \
            ols_object_get_ref(parent);     \
        else                                \
            goto label;                     \
    } while (0)

#define RELEASE_PARENT(parent)                  \
    do {                                        \
        if (parent) ols_object_release(parent); \
    } while (0)

#define OLS_PAD_CAST(obj) ((ols_pad_t *)(obj))

/**
 * ols_pad_forward:
 * @pad: a #ols_pad_t
 * @forward: (scope call): a #ols_pad_forward_function
 * @user_data: user data passed to @forward
 *
 * Calls @forward for all internally linked pads of @pad. This function deals with
 * dynamically changing internal pads and will make sure that the @forward
 * function is only called once for each pad.
 *
 * When @forward returns %TRUE, no further pads will be processed.
 *
 * Returns: %TRUE if one of the dispatcher functions returned %TRUE.
 */
bool ols_pad_forward(ols_pad_t *pad, ols_pad_forward_function forward, void *user_data) {
    bool result = false;
    ols_object_t *parent = NULL;
    struct darray *pad_array;
    size_t i;

    OLS_PAD_LOCK(pad);
    ACQUIRE_PARENT(pad, parent, no_parent);
    OLS_PAD_UNLOCK(pad);

    /* 根据 pad 方向选择对应的 pads 数组:
     * SRC pad 转发到 parent 的 sinkpads
     * SINK pad 转发到 parent 的 srcpads
     */
    if (pad->direction == OLS_PAD_SRC) {
        pad_array = &parent->sinkpads.da;
    } else {
        pad_array = &parent->srcpads.da;
    }

    for (i = 0; i < pad_array->num && !result; i++) {
        ols_pad_t *intpad = ((ols_pad_t **)pad_array->array)[i];
        if (intpad == NULL) {
            continue; /* 跳过空槽位，继续处理后续 pad */
        }
        /* forward 返回 true 时停止迭代 */
        result = forward(intpad, user_data);
    }

    RELEASE_PARENT(parent);
    return result;

no_parent:
    OLS_PAD_UNLOCK(pad);
    return result;
}

typedef struct {
    ols_event_t *event;
    bool result;
    bool dispatched;
} EventData;

static bool event_forward_func(ols_pad_t *pad, EventData *data) {
    /* for each pad we send to, we should ref the event; it's up
     * to downstream to unref again when handled. */
    // OLS_LOG_OBJECT (pad, "Reffing and pushing event %p (%s) to %s:%s",
    //     data->event, GST_EVENT_TYPE_NAME (data->event), GST_DEBUG_PAD_NAME (pad));

    data->result |= ols_pad_push_event(pad, ols_event_ref(data->event));

    data->dispatched = true;

    /* don't stop */
    return false;
}

/**
 * gst_pad_event_default:
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
bool ols_pad_event_default(ols_pad_t *pad, ols_object_t *parent, ols_event_t *event) {
    EventData data;
    bool result;

    return_val_if_fail(pad != NULL, false);
    return_val_if_fail(event != NULL, false);

    data.event = event;
    data.dispatched = false;
    data.result = false;

    ols_pad_forward(pad, (ols_pad_forward_function)event_forward_func, &data);

    /* 对于没有父元素或没有内部链接的 sinkpad，
     * 不会有任何分发，但我们仍然返回 TRUE */
    result = data.dispatched ? data.result : true;

    ols_event_unref(event);

    return result;
}

static void ols_pad_init(ols_pad_t *pad, const char *name) {
    // pad->priv = ols_pad_get_instance_private(pad);

    OLS_PAD_NAME(pad) = bstrdup(name);

    OLS_PAD_DIRECTION(pad) = OLS_PAD_UNKNOWN;

    OLS_PAD_EVENTFUNC(pad) = ols_pad_event_default;

    OLS_PAD_CHAINLISTFUNC(pad) = ols_pad_chain_list_default;

    OLS_PAD_CHAINFUNC(pad) = NULL;

    OLS_PAD_LINKFUNC(pad) = NULL;

    OLS_PAD_UNLINKFUNC(pad) = NULL;

    OLS_PAD_CAPS(pad) = NULL;
    // OLS_PAD_LINKFUNC(pad) =

    OLS_PAD_PEER(pad) = NULL;

    OLS_PAD_TASK(pad) = NULL;

    // g_rec_mutex_init(&pad->stream_rec_lock);

    pthread_cond_init(&pad->block_cond, NULL);

    pthread_mutex_init(&pad->mutex, NULL);

    pthread_mutex_init_recursive(&pad->stream_rec_lock);

    // pad->priv->events = g_array_sized_new(FALSE, TRUE, sizeof(PadEvent), 16);
    // pad->priv->events_cookie = 0;
    // pad->priv->last_cookie = -1;
    // g_cond_init(&pad->priv->activation_cond);
}

bool ols_pad_set_parent(ols_pad_t *pad, ols_object_t *parent) {
    OLS_PAD_LOCK(pad);
    if ((pad->parent != NULL)) goto had_parent;

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

void ols_pad_set_caps(ols_pad_t *pad, ols_caps_t *caps) {
    OLS_PAD_LOCK(pad);
    if (pad->caps) {
        ols_caps_unref(pad->caps);
    }
    pad->caps = caps;

    // blog(LOG_ERROR, "pad %p caps is %p",pad,pad->caps);
    //  if (G_LIKELY (result))
    //    gst_object_ref (result);
    OLS_PAD_UNLOCK(pad);
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
        ols_pad_init(pad, name);
        pad->direction = direction;
    }

    return pad;
}

void ols_pad_destory(ols_pad_t *pad) {
    if (!pad) {
        return;
    }

    bfree(pad->name);

    if (pad->caps) {
        ols_caps_unref(pad->caps);
    }

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
        if (OLS_PAD_TASK(pad) != task) goto concurrent_stop;
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
    if (task == NULL) goto no_task;

    res = ols_task_set_state(task, OLS_TASK_PAUSED);
    /* unblock activation waits if any */
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
    if (task == NULL) goto no_task;
    OLS_PAD_TASK(pad) = NULL;
    res = ols_task_set_state(task, OLS_TASK_STOPPED);
    /* unblock activation waits if any */
    // pad->priv->in_activation = false;
    // g_cond_broadcast (&pad->priv->activation_cond);
    OLS_PAD_UNLOCK(pad);
    OLS_PAD_STREAM_LOCK(pad);
    OLS_PAD_STREAM_UNLOCK(pad);

    if (!ols_task_join(task)) goto join_failed;

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
    if (OLS_PAD_TASK(pad) == NULL) OLS_PAD_TASK(pad) = task;
    OLS_PAD_UNLOCK(pad);

    return false;
}
}

bool pad_link_maybe_ghosting(ols_pad_t *src, ols_pad_t *sink) {
    bool ret;

    // if (!prepare_link_maybe_ghosting(&src, &sink, &pads_created)) {
    //   ret = false;
    // } else {
    ret = (ols_pad_link_full(src, sink) == OLS_PAD_LINK_OK);

    if (!ret) {
        blog(LOG_DEBUG, "pad link failed");
    }

    return ret;
}

/**
 * ols_pad_set_chain_function_full:
 * @pad: a sink #ols_pad_t.
 * @chain: the #ols_pad_chain_function to set.
 * @user_data: user_data passed to @notify
 *
 * Sets the given chain function for the pad. The chain function is called to
 * process a #OlsBuffer input buffer. see #ols_pad_chain_function for more details.
 */
void ols_pad_set_chain_function_full(ols_pad_t *pad, ols_pad_chain_function chain, void *user_data) {
    OLS_PAD_CHAINFUNC(pad) = chain;
    pad->chaindata = user_data;

    // OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "chainfunc set to %s",
    //                      OLS_DEBUG_FUNCPTR_NAME(chain));
}

/**
 * ols_pad_set_event_function_full:
 * @pad: a sink #ols_pad_t.
 * @chain: the #ols_pad_event_function to set.
 * @user_data: user_data passed to @notify
 *
 * Sets the given event function for the pad.
 */
void ols_pad_set_event_function_full(ols_pad_t *pad, ols_pad_event_function event, void *user_data) {
    OLS_PAD_EVENTFUNC(pad) = event;
    pad->eventdata = user_data;
}

/**
 * ols_pad_set_chain_list_function_full:
 * @pad: a sink #ols_pad_t.
 * @chainlist: the #ols_pad_chain_list_function to set.
 * @user_data: user_data passed to @notify
 *
 * Sets the given chain list function for the pad. The chainlist function is
 * called to process a #OlsBufferList input buffer list. See
 * #ols_pad_chain_list_function for more details.
 */
void ols_pad_set_chain_list_function_full(ols_pad_t *pad, ols_pad_chain_list_function chainlist, void *user_data) {
    OLS_PAD_CHAINLISTFUNC(pad) = chainlist;
    pad->chaindata = user_data;
    // OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "chainlistfunc set to %s",
    //                      OLS_DEBUG_FUNCPTR_NAME(chainlist));
}

/**
 * ols_pad_set_link_function_full:
 * @pad: a #ols_pad_t.
 * @link: the #ols_pad_link_function to set.
 * @user_data: user_data passed to @notify
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
 * If @link is installed on a source pad, it should call the #ols_pad_link_function
 * of the peer sink pad, if present.
 */
void ols_pad_set_link_function_full(ols_pad_t *pad, ols_pad_link_function link, void *user_data) {
    // return_if_fail(OLS_IS_PAD(pad));

    OLS_PAD_LINKFUNC(pad) = link;
    pad->linkdata = user_data;
    //   OLS_CAT_DEBUG_OBJECT(OLS_CAT_PADS, pad, "linkfunc set to %s",
    //                        OLS_DEBUG_FUNCPTR_NAME(link));
    //
}

/**
 * ols_pad_set_unlink_function_full:
 * @pad: a #ols_pad_t.
 * @unlink: the #ols_pad_unlink_function to set.
 * @user_data: user_data passed to @notify
 *
 * Sets the given unlink function for the pad. It will be called
 * when the pad is unlinked.
 *
 * Note that the pad's lock is already held when the unlink
 * function is called, so most pad functions cannot be called
 * from within the callback.
 */
void ols_pad_set_unlink_function_full(ols_pad_t *pad, ols_pad_unlink_function unlink, void *user_data) {
    // return_if_fail(OLS_IS_PAD(pad));

    OLS_PAD_UNLINKFUNC(pad) = unlink;
    pad->unlinkdata = user_data;

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

    OLS_PAD_LOCK(srcpad);
    OLS_PAD_LOCK(sinkpad);

    /* 检查两个 pad 是否确实链接在一起 */
    if (OLS_PAD_PEER(srcpad) != sinkpad) {
        /* pads 未链接在一起，直接返回 */
        OLS_PAD_UNLOCK(sinkpad);
        OLS_PAD_UNLOCK(srcpad);
        return result;
    }

    /* 调用 srcpad 的 unlink 回调 */
    if (OLS_PAD_UNLINKFUNC(srcpad)) {
        ols_object_t *tmpparent;
        ACQUIRE_PARENT(srcpad, tmpparent, skip_src_unlink);
        OLS_PAD_UNLINKFUNC(srcpad)(srcpad, tmpparent);
        RELEASE_PARENT(tmpparent);
    }
skip_src_unlink:

    /* 调用 sinkpad 的 unlink 回调 */
    if (OLS_PAD_UNLINKFUNC(sinkpad)) {
        ols_object_t *tmpparent;
        ACQUIRE_PARENT(sinkpad, tmpparent, skip_sink_unlink);
        OLS_PAD_UNLINKFUNC(sinkpad)(sinkpad, tmpparent);
        RELEASE_PARENT(tmpparent);
    }
skip_sink_unlink:

    /* 清除 peer 引用 */
    OLS_PAD_PEER(srcpad) = NULL;
    OLS_PAD_PEER(sinkpad) = NULL;

    OLS_PAD_UNLOCK(sinkpad);
    OLS_PAD_UNLOCK(srcpad);

    return true;
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
    ols_pad_link_function srcfunc, sinkfunc;

    return_val_if_fail(OLS_PAD_IS_SRC(srcpad), OLS_PAD_LINK_WRONG_DIRECTION);
    return_val_if_fail(OLS_PAD_IS_SINK(sinkpad), OLS_PAD_LINK_WRONG_DIRECTION);

    OLS_PAD_LOCK(srcpad);
    OLS_PAD_LOCK(sinkpad);

    /* 在调用 link 函数之前设置 peer */
    OLS_PAD_PEER(srcpad) = sinkpad;
    OLS_PAD_PEER(sinkpad) = srcpad;

    /* 获取 link 函数 */
    srcfunc = OLS_PAD_LINKFUNC(srcpad);
    sinkfunc = OLS_PAD_LINKFUNC(sinkpad);

    if (srcfunc || sinkfunc) {
        /* 自定义 link 函数，执行它们（需要释放锁） */
        OLS_PAD_UNLOCK(sinkpad);
        OLS_PAD_UNLOCK(srcpad);

        if (srcfunc) {
            ols_object_t *tmpparent;
            ACQUIRE_PARENT(srcpad, tmpparent, link_check);
            result = srcfunc(srcpad, tmpparent, sinkpad);
            RELEASE_PARENT(tmpparent);
        } else if (sinkfunc) {
            ols_object_t *tmpparent;
            ACQUIRE_PARENT(sinkpad, tmpparent, link_check);
            result = sinkfunc(sinkpad, tmpparent, srcpad);
            RELEASE_PARENT(tmpparent);
        }

    link_check:
        OLS_PAD_LOCK(srcpad);
        OLS_PAD_LOCK(sinkpad);

        /* 释放锁后检查 pads 是否仍然链接 */
        if (OLS_PAD_PEER(srcpad) != sinkpad || OLS_PAD_PEER(sinkpad) != srcpad) {
            /* 并发链接操作，另一个操作先成功 */
            OLS_PAD_UNLOCK(sinkpad);
            OLS_PAD_UNLOCK(srcpad);
            return OLS_PAD_LINK_WAS_LINKED;
        }

        if (result != OLS_PAD_LINK_OK) {
            /* 链接失败，清除 peer 引用 */
            OLS_PAD_PEER(srcpad) = NULL;
            OLS_PAD_PEER(sinkpad) = NULL;
            OLS_PAD_UNLOCK(sinkpad);
            OLS_PAD_UNLOCK(srcpad);
            return result;
        }
    }

    OLS_PAD_UNLOCK(sinkpad);
    OLS_PAD_UNLOCK(srcpad);

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

/**
 * ols_pad_chain_data_unchecked:
 * @pad: 目标 sink pad
 * @type: probe 类型
 * @data: 要传递的数据 (buffer 或 buffer list)
 *
 * 不执行额外参数检查的 chain 函数，用于内部调用以获得更好的性能。
 *
 * Returns: #OlsFlowReturn 表示数据处理结果
 */
static inline OlsFlowReturn ols_pad_chain_data_unchecked(ols_pad_t *pad, OlsPadProbeType type, void *data) {
    OlsFlowReturn ret;
    ols_object_t *parent;

    OLS_PAD_LOCK(pad);

    if (OLS_PAD_IS_EOS(pad)) {
        blog(LOG_WARNING, "chaining, but pad was EOS");
        OLS_PAD_UNLOCK(pad);
        ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
        return OLS_FLOW_EOS;
    }

    ACQUIRE_PARENT(pad, parent, no_parent);
    OLS_PAD_UNLOCK(pad);

    /* 注意: 我们在未持有锁的情况下读取 chainfunc。
     * 函数在创建时分配，不会经常改变，所以这是安全的 */
    if (type & OLS_PAD_PROBE_TYPE_BUFFER) {
        ols_pad_chain_function chainfunc = OLS_PAD_CHAINFUNC(pad);
        if (chainfunc == NULL) {
            RELEASE_PARENT(parent);
            ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
            return OLS_FLOW_NOT_SUPPORTED;
        }
        ret = chainfunc(pad, parent, OLS_BUFFER_CAST(data));
    } else {
        ols_pad_chain_list_function chainlistfunc = OLS_PAD_CHAINLISTFUNC(pad);
        if (chainlistfunc == NULL) {
            RELEASE_PARENT(parent);
            ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
            return OLS_FLOW_NOT_SUPPORTED;
        }
        ret = chainlistfunc(pad, parent, OLS_BUFFER_LIST_CAST(data));
    }

    RELEASE_PARENT(parent);
    return ret;

no_parent:
    OLS_PAD_UNLOCK(pad);
    ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
    return OLS_FLOW_FLUSHING;
}

static OlsFlowReturn ols_pad_chain_list_default(ols_pad_t *pad, ols_object_t *parent, ols_buffer_list_t *list) {
    UNUSED_PARAMETER(parent);
    uint32_t i, len;
    ols_buffer_t *buffer;
    OlsFlowReturn ret;

    // OLS_LOG_OBJECT(pad, "chaining each buffer in list individually");

    len = ols_buffer_list_length(list);

    ret = OLS_FLOW_OK;
    for (i = 0; i < len; i++) {
        buffer = ols_buffer_list_get(list, i);
        ret = ols_pad_chain_data_unchecked(pad, OLS_PAD_PROBE_TYPE_BUFFER | OLS_PAD_PROBE_TYPE_PUSH, ols_buffer_ref(buffer));
        if (ret != OLS_FLOW_OK) break;
    }
    ols_buffer_list_unref(list);

    return ret;
}

/**
 * ols_pad_push_data:
 * @pad: 源 pad
 * @type: probe 类型
 * @data: 要推送的数据
 *
 * 将数据推送到 peer pad 的内部实现。
 *
 * Returns: #OlsFlowReturn 表示推送结果
 */
static OlsFlowReturn ols_pad_push_data(ols_pad_t *pad, OlsPadProbeType type, void *data) {
    ols_pad_t *peer;
    OlsFlowReturn ret;

    OLS_PAD_LOCK(pad);

    if (OLS_PAD_IS_EOS(pad)) {
        blog(LOG_ERROR, "pushing on pad %s, but pad was EOS", OLS_PAD_NAME(pad));
        OLS_PAD_UNLOCK(pad);
        ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
        return OLS_FLOW_EOS;
    }

    peer = OLS_PAD_PEER(pad);
    if (peer == NULL) {
        blog(LOG_ERROR, "pushing on pad %s, but it was not linked", OLS_PAD_NAME(pad));
        OLS_PAD_UNLOCK(pad);
        ols_mini_object_unref(OLS_MINI_OBJECT_CAST(data));
        return OLS_FLOW_NOT_LINKED;
    }

    /* 在释放锁之前获取 peer pad 的引用 */
    ols_mini_object_ref(OLS_MINI_OBJECT_CAST(peer));
    OLS_PAD_UNLOCK(pad);

    ret = ols_pad_chain_data_unchecked(peer, type, data);

    ols_mini_object_unref(OLS_MINI_OBJECT_CAST(peer));

    return ret;
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
    res = ols_pad_push_data(pad, OLS_PAD_PROBE_TYPE_BUFFER | OLS_PAD_PROBE_TYPE_PUSH, buffer);
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
    if (result) ols_mini_object_ref(OLS_MINI_OBJECT_CAST(pad));
    OLS_PAD_UNLOCK(pad);

    return result;
}

/**
 * ols_pad_send_event_unchecked:
 * @pad: 目标 pad
 * @event: 要发送的事件
 * @type: probe 类型 (当前未使用)
 *
 * 不执行额外检查的事件发送函数。
 *
 * Returns: #OlsFlowReturn 表示事件发送结果
 */
static OlsFlowReturn ols_pad_send_event_unchecked(ols_pad_t *pad, ols_event_t *event, OlsPadProbeType type) {
    OlsFlowReturn ret;
    ols_pad_event_function eventfunc;
    ols_object_t *parent;

    UNUSED_PARAMETER(type);

    OLS_PAD_LOCK(pad);

    eventfunc = OLS_PAD_EVENTFUNC(pad);
    if (eventfunc == NULL) {
        OLS_PAD_UNLOCK(pad);
        ols_event_unref(event);
        return OLS_FLOW_NOT_SUPPORTED;
    }

    ACQUIRE_PARENT(pad, parent, no_parent);
    OLS_PAD_UNLOCK(pad);

    ret = eventfunc(pad, parent, event) ? OLS_FLOW_OK : OLS_FLOW_ERROR;

    RELEASE_PARENT(parent);
    return ret;

no_parent:
    OLS_PAD_UNLOCK(pad);
    ols_event_unref(event);
    return OLS_FLOW_FLUSHING;
}

/**
 * ols_pad_push_event_unchecked:
 * @pad: 源 pad (调用时必须持有锁)
 * @event: 要推送的事件
 * @type: probe 类型
 *
 * 将事件推送到 peer pad 的内部实现。
 * 调用前必须持有 pad 锁。
 *
 * Returns: #OlsFlowReturn 表示事件推送结果
 */
static OlsFlowReturn ols_pad_push_event_unchecked(ols_pad_t *pad, ols_event_t *event, OlsPadProbeType type) {
    OlsFlowReturn ret;
    ols_pad_t *peerpad;

    peerpad = OLS_PAD_PEER(pad);
    if (peerpad == NULL) {
        ols_event_unref(event);
        return OLS_FLOW_NOT_LINKED;
    }

    /* 在释放锁之前获取 peer pad 的引用 */
    ols_mini_object_ref(OLS_MINI_OBJECT_CAST(peerpad));
    OLS_PAD_UNLOCK(pad);

    ret = ols_pad_send_event_unchecked(peerpad, event, type);

    ols_mini_object_unref(OLS_MINI_OBJECT_CAST(peerpad));

    OLS_PAD_LOCK(pad);
    return ret;
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
    OlsPadProbeType type;
    OlsFlowReturn ret;

    return_val_if_fail(pad != NULL, false);
    return_val_if_fail(event != NULL, false);

    /* 根据 pad 方向确定事件类型和方向检查 */
    if (OLS_PAD_IS_SRC(pad)) {
        if (!OLS_EVENT_IS_DOWNSTREAM(event)) {
            ols_event_unref(event);
            return false;
        }
        type = OLS_PAD_PROBE_TYPE_EVENT_DOWNSTREAM;
    } else if (OLS_PAD_IS_SINK(pad)) {
        if (!OLS_EVENT_IS_UPSTREAM(event)) {
            ols_event_unref(event);
            return false;
        }
        type = OLS_PAD_PROBE_TYPE_EVENT_UPSTREAM;
    } else {
        /* 未知方向的 pad */
        ols_event_unref(event);
        return false;
    }

    OLS_PAD_LOCK(pad);

    ret = ols_pad_push_event_unchecked(pad, event, type);
    /* probe 丢弃的事件不算错误 */
    OLS_PAD_UNLOCK(pad);

    return (ret == OLS_FLOW_OK || ret == OLS_FLOW_CUSTOM_SUCCESS || ret == OLS_FLOW_CUSTOM_SUCCESS_1);
}
