

#pragma once
#include "ols.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ols_pad;
typedef struct ols_pad ols_pad_t;

#define OLS_PAD_CAST(obj) ((ols_pad_t *)(obj))

/**
 * OLSPadDirection:
 * @OLS_PAD_UNKNOWN: direction is unknown.
 * @OLS_PAD_SRC: the pad is a source pad.
 * @OLS_PAD_SINK: the pad is a sink pad.
 *
 * The direction of a pad.
 */
typedef enum { OLS_PAD_UNKNOWN, OLS_PAD_SRC, OLS_PAD_SINK } OLSPadDirection;

/**
 * OLSPadMode:
 * @OLS_PAD_MODE_NONE: Pad will not handle dataflow
 * @OLS_PAD_MODE_PUSH: Pad handles dataflow in downstream push mode
 * @OLS_PAD_MODE_PULL: Pad handles dataflow in upstream pull mode
 *
 * The status of a OLSPad. After activating a pad, which usually happens when
 * the parent element goes from READY to PAUSED, the OLSPadMode defines if the
 * pad operates in push or pull mode.
 */
typedef enum {
  OLS_PAD_MODE_NONE,
  OLS_PAD_MODE_PUSH,
  OLS_PAD_MODE_PULL
} OLSPadMode;

/**
 * OLSPadProbeType:
 * @OLS_PAD_PROBE_TYPE_INVALID: invalid probe type
 * @OLS_PAD_PROBE_TYPE_IDLE: probe idle pads and block while the callback is
 * called
 * @OLS_PAD_PROBE_TYPE_BLOCK: probe and block pads
 * @OLS_PAD_PROBE_TYPE_BUFFER: probe buffers
 * @OLS_PAD_PROBE_TYPE_BUFFER_LIST: probe buffer lists
 * @OLS_PAD_PROBE_TYPE_EVENT_DOWNSTREAM: probe downstream events
 * @OLS_PAD_PROBE_TYPE_EVENT_UPSTREAM: probe upstream events
 * @OLS_PAD_PROBE_TYPE_EVENT_FLUSH: probe flush events. This probe has to be
 *     explicitly enabled and is not included in the
 *     @@OLS_PAD_PROBE_TYPE_EVENT_DOWNSTREAM or
 *     @@OLS_PAD_PROBE_TYPE_EVENT_UPSTREAM probe types.
 * @OLS_PAD_PROBE_TYPE_QUERY_DOWNSTREAM: probe downstream queries
 * @OLS_PAD_PROBE_TYPE_QUERY_UPSTREAM: probe upstream queries
 * @OLS_PAD_PROBE_TYPE_PUSH: probe push
 * @OLS_PAD_PROBE_TYPE_PULL: probe pull
 * @OLS_PAD_PROBE_TYPE_BLOCKING: probe and block at the next opportunity, at
 * data flow or when idle
 * @OLS_PAD_PROBE_TYPE_DATA_DOWNSTREAM: probe downstream data (buffers, buffer
 * lists, and events)
 * @OLS_PAD_PROBE_TYPE_DATA_UPSTREAM: probe upstream data (events)
 * @OLS_PAD_PROBE_TYPE_DATA_BOTH: probe upstream and downstream data (buffers,
 * buffer lists, and events)
 * @OLS_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM: probe and block downstream data
 * (buffers, buffer lists, and events)
 * @OLS_PAD_PROBE_TYPE_BLOCK_UPSTREAM: probe and block upstream data (events)
 * @OLS_PAD_PROBE_TYPE_EVENT_BOTH: probe upstream and downstream events
 * @OLS_PAD_PROBE_TYPE_QUERY_BOTH: probe upstream and downstream queries
 * @OLS_PAD_PROBE_TYPE_ALL_BOTH: probe upstream events and queries and
 * downstream buffers, buffer lists, events and queries
 * @OLS_PAD_PROBE_TYPE_SCHEDULING: probe push and pull
 *
 * The different probing types that can occur. When either one of
 * @OLS_PAD_PROBE_TYPE_IDLE or @OLS_PAD_PROBE_TYPE_BLOCK is used, the probe will
 * be a blocking probe.
 */
typedef enum {
  OLS_PAD_PROBE_TYPE_INVALID = 0,
  /* flags to control blocking */
  OLS_PAD_PROBE_TYPE_IDLE = (1 << 0),
  OLS_PAD_PROBE_TYPE_BLOCK = (1 << 1),
  /* flags to select datatypes */
  OLS_PAD_PROBE_TYPE_BUFFER = (1 << 4),
  OLS_PAD_PROBE_TYPE_BUFFER_LIST = (1 << 5),
  OLS_PAD_PROBE_TYPE_EVENT_DOWNSTREAM = (1 << 6),
  OLS_PAD_PROBE_TYPE_EVENT_UPSTREAM = (1 << 7),
  OLS_PAD_PROBE_TYPE_EVENT_FLUSH = (1 << 8),
  OLS_PAD_PROBE_TYPE_QUERY_DOWNSTREAM = (1 << 9),
  OLS_PAD_PROBE_TYPE_QUERY_UPSTREAM = (1 << 10),
  /* flags to select scheduling mode */
  OLS_PAD_PROBE_TYPE_PUSH = (1 << 12),
  OLS_PAD_PROBE_TYPE_PULL = (1 << 13),

  /* flag combinations */
  OLS_PAD_PROBE_TYPE_BLOCKING =
      OLS_PAD_PROBE_TYPE_IDLE | OLS_PAD_PROBE_TYPE_BLOCK,
  OLS_PAD_PROBE_TYPE_DATA_DOWNSTREAM = OLS_PAD_PROBE_TYPE_BUFFER |
                                       OLS_PAD_PROBE_TYPE_BUFFER_LIST |
                                       OLS_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
  OLS_PAD_PROBE_TYPE_DATA_UPSTREAM = OLS_PAD_PROBE_TYPE_EVENT_UPSTREAM,
  OLS_PAD_PROBE_TYPE_DATA_BOTH =
      OLS_PAD_PROBE_TYPE_DATA_DOWNSTREAM | OLS_PAD_PROBE_TYPE_DATA_UPSTREAM,
  OLS_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM =
      OLS_PAD_PROBE_TYPE_BLOCK | OLS_PAD_PROBE_TYPE_DATA_DOWNSTREAM,
  OLS_PAD_PROBE_TYPE_BLOCK_UPSTREAM =
      OLS_PAD_PROBE_TYPE_BLOCK | OLS_PAD_PROBE_TYPE_DATA_UPSTREAM,
  OLS_PAD_PROBE_TYPE_EVENT_BOTH =
      OLS_PAD_PROBE_TYPE_EVENT_DOWNSTREAM | OLS_PAD_PROBE_TYPE_EVENT_UPSTREAM,
  OLS_PAD_PROBE_TYPE_QUERY_BOTH =
      OLS_PAD_PROBE_TYPE_QUERY_DOWNSTREAM | OLS_PAD_PROBE_TYPE_QUERY_UPSTREAM,
  OLS_PAD_PROBE_TYPE_ALL_BOTH =
      OLS_PAD_PROBE_TYPE_DATA_BOTH | OLS_PAD_PROBE_TYPE_QUERY_BOTH,
  OLS_PAD_PROBE_TYPE_SCHEDULING =
      OLS_PAD_PROBE_TYPE_PUSH | OLS_PAD_PROBE_TYPE_PULL
} OlsPadProbeType;

/**
 * OLS_PAD_DIRECTION:
 * @pad: a #OLSPad
 *
 * Get the #OLSPadDirection of the given @pad. Accessor macro, use
 * ols_pad_get_direction() instead.
 */
#define OLS_PAD_DIRECTION(pad) (OLS_PAD_CAST(pad)->direction)

/**
 * OLS_PAD_PARENT:
 * @pad: a #OLSPad
 *
 * Get the #OLSPadDirection of the given @pad. Accessor macro, use
 * ols_pad_get_direction() instead.
 */
#define OLS_PAD_PARENT(pad) (OLS_PAD_CAST(pad)->parent)

/**
 * OLS_PAD_MODE:
 * @pad: a #OLSPad
 *
 * Get the #OLSPadMode of pad, which will be OLS_PAD_MODE_NONE if the pad
 * has not been activated yet, and otherwise either OLS_PAD_MODE_PUSH or
 * OLS_PAD_MODE_PULL depending on which mode the pad was activated in.
 */
#define OLS_PAD_MODE(pad) (OLS_PAD_CAST(pad)->mode)

/**
 * OLS_PAD_EVENTFUNC:
 * @pad: a #OlsPad
 *
 * Get the #OlsPadEventFunction from the given @pad, which
 * is the function that handles events on the pad. You can
 * use this to set your own event handling function on a pad
 * after you create it.  If your element derives from a base
 * class, use the base class's virtual functions instead.
 */
#define OLS_PAD_EVENTFUNC(pad) (OLS_PAD_CAST(pad)->eventfunc)

/**
 * OLSPadLinkReturn:
 * @OLS_PAD_LINK_OK		: link succeeded
 * @OLS_PAD_LINK_WRONG_HIERARCHY: pads have no common grandparent
 * @OLS_PAD_LINK_WAS_LINKED	: pad was already linked
 * @OLS_PAD_LINK_WRONG_DIRECTION: pads have wrong direction
 * @OLS_PAD_LINK_NOFORMAT	: pads do not have common format
 * @OLS_PAD_LINK_NOSCHED	: pads cannot cooperate in scheduling
 * @OLS_PAD_LINK_REFUSED	: refused for some reason
 *
 * Result values from ols_pad_link and friends.
 */
typedef enum {
  OLS_PAD_LINK_OK = 0,
  OLS_PAD_LINK_WRONG_HIERARCHY = -1,
  OLS_PAD_LINK_WAS_LINKED = -2,
  OLS_PAD_LINK_WRONG_DIRECTION = -3,
  OLS_PAD_LINK_NOFORMAT = -4,
  OLS_PAD_LINK_NOSCHED = -5,
  OLS_PAD_LINK_REFUSED = -6
} OlsPadLinkReturn;

/**
 * OlsPadFlags:
 * @OLS_PAD_FLAG_BLOCKED: is dataflow on a pad blocked
 * @OLS_PAD_FLAG_FLUSHING: is pad flushing
 * @OLS_PAD_FLAG_EOS: is pad in EOS state
 * @OLS_PAD_FLAG_BLOCKING: is pad currently blocking on a buffer or event
 * @OLS_PAD_FLAG_NEED_PARENT: ensure that there is a parent object before
 * calling into the pad callbacks.
 * @OLS_PAD_FLAG_NEED_RECONFIGURE: the pad should be reconfigured/renegotiated.
 *                            The flag has to be unset manually after
 *                            reconfiguration happened.
 * @OLS_PAD_FLAG_PENDING_EVENTS: the pad has pending events
 * @OLS_PAD_FLAG_FIXED_CAPS: the pad is using fixed caps. This means that
 *     once the caps are set on the pad, the default caps query function
 *     will only return those caps.
 * @OLS_PAD_FLAG_PROXY_CAPS: the default event and query handler will forward
 *                      all events and queries to the internally linked pads
 *                      instead of discarding them.
 * @OLS_PAD_FLAG_PROXY_ALLOCATION: the default query handler will forward
 *                      allocation queries to the internally linked pads
 *                      instead of discarding them.
 * @OLS_PAD_FLAG_PROXY_SCHEDULING: the default query handler will forward
 *                      scheduling queries to the internally linked pads
 *                      instead of discarding them.
 * @OLS_PAD_FLAG_ACCEPT_INTERSECT: the default accept-caps handler will check
 *                      it the caps intersect the query-caps result instead
 *                      of checking for a subset. This is interesting for
 *                      parsers that can accept incompletely specified caps.
 * @OLS_PAD_FLAG_ACCEPT_TEMPLATE: the default accept-caps handler will use
 *                      the template pad caps instead of query caps to
 *                      compare with the accept caps. Use this in combination
 *                      with %OLS_PAD_FLAG_ACCEPT_INTERSECT. (Since: 1.6)
 * @OLS_PAD_FLAG_LAST: offset to define more flags
 *
 * Pad state flags
 */

typedef enum {
  OLS_PAD_FLAG_BLOCKED = (1 << 0),
  OLS_PAD_FLAG_FLUSHING = (1 << 1),
  OLS_PAD_FLAG_EOS = (1 << 2),
  OLS_PAD_FLAG_BLOCKING = (1 << 3),
  OLS_PAD_FLAG_NEED_PARENT = (1 << 4),
  OLS_PAD_FLAG_NEED_RECONFIGURE = (1 << 5),
  OLS_PAD_FLAG_PENDING_EVENTS = (1 << 6),
  OLS_PAD_FLAG_FIXED_CAPS = (1 << 7),
  OLS_PAD_FLAG_PROXY_CAPS = (1 << 8),
  OLS_PAD_FLAG_PROXY_ALLOCATION = (1 << 9),
  OLS_PAD_FLAG_PROXY_SCHEDULING = (1 << 10),
  OLS_PAD_FLAG_ACCEPT_INTERSECT = (1 << 11),
  OLS_PAD_FLAG_ACCEPT_TEMPLATE = (1 << 12),
  /* padding */
  OLS_PAD_FLAG_LAST = (1 << 16)
} OlsPadFlags;

/**
 * OLS_PAD_LINK_FAILED:
 * @ret: the #OLSPadLinkReturn value
 *
 * Macro to test if the given #OLSPadLinkReturn value indicates a failed
 * link step.
 */
#define OLS_PAD_LINK_FAILED(ret) ((ret) < OLS_PAD_LINK_OK)

/**
 * OLS_PAD_LINK_SUCCESSFUL:
 * @ret: the #OLSPadLinkReturn value
 *
 * Macro to test if the given #OLSPadLinkReturn value indicates a successful
 * link step.
 */
#define OLS_PAD_LINK_SUCCESSFUL(ret) ((ret) >= OLS_PAD_LINK_OK)

/**
 * OLS_PAD_CHAINFUNC:
 * @pad: a #OLSPad
 *
 * Get the #OLSPadChainFunction from the given @pad.
 */
#define OLS_PAD_CHAINFUNC(pad) (OLS_PAD_CAST(pad)->chainfunc)

/**
 * OLS_PAD_CHAINLISTFUNC:
 * @pad: a #OLSPad
 *
 * Get the #OlsPadChainListFunction from the given @pad.
 */
#define OLS_PAD_CHAINLISTFUNC(pad) (OLS_PAD_CAST(pad)->chainlistfunc)

/**
 * OLS_PAD_PEER:
 * @pad: a #OlsPad
 *
 * Return the pad's peer member. This member is a pointer to the linked @pad.
 * No locking is performed in this function, use ols_pad_get_peer() instead.
 */
#define OLS_PAD_PEER(pad) (OLS_PAD_CAST(pad)->peer)

/**
 * OLS_PAD_LINKFUNC:
 * @pad: a #OLSPad
 *
 * Get the #OlsPadChainListFunction from the given @pad.
 */
#define OLS_PAD_LINKFUNC(pad) (OLS_PAD_CAST(pad)->linkfunc)

/**
 * OLS_PAD_UNLINKFUNC:
 * @pad: a #OLSPad
 *
 * Get the #OlsPadChainListFunction from the given @pad.
 */
#define OLS_PAD_UNLINKFUNC(pad) (OLS_PAD_CAST(pad)->unlinkfunc)

/**
 * OLS_OBJECT_FLAGS:
 * @obj: a #OlsObject
 *
 * This macro returns the entire set of flags for the object.
 */
#define OLS_PAD_FLAGS(obj) (OLS_PAD_CAST(obj)->flags)
/**
 * OLS_OBJECT_FLAG_IS_SET:
 * @obj: a #OlsObject
 * @flag: Flag to check for
 *
 * This macro checks to see if the given flag is set.
 */
#define OLS_PAD_FLAG_IS_SET(obj, flag) ((OLS_PAD_FLAGS(obj) & (flag)) == (flag))

/**
 * OLS_PAD_IS_EOS:
 * @pad: a #OlsPad
 *
 * Check if the @pad is in EOS state.
 */
#define OLS_PAD_IS_EOS(pad) (OLS_PAD_FLAG_IS_SET(pad, OLS_PAD_FLAG_EOS))

/**
 * OLS_PAD_IS_SRC:
 * @pad: a #OlsPad
 *
 * Returns: %TRUE if the pad is a source pad (i.e. produces data).
 */
#define OLS_PAD_IS_SRC(pad) (OLS_PAD_DIRECTION(pad) == OLS_PAD_SRC)
/**
 * OLS_PAD_IS_SINK:
 * @pad: a #OlsPad
 *
 * Returns: %TRUE if the pad is a sink pad (i.e. consumes data).
 */
#define OLS_PAD_IS_SINK(pad) (OLS_PAD_DIRECTION(pad) == OLS_PAD_SINK)

/**
 * OLSFlowReturn:
 * @OLS_FLOW_OK:		 Data passing was ok.
 * @OLS_FLOW_NOT_LINKED:	 Pad is not linked.
 * @OLS_FLOW_FLUSHING:	         Pad is flushing.
 * @OLS_FLOW_EOS:                Pad is EOS.
 * @OLS_FLOW_NOT_NEGOTIATED:	 Pad is not negotiated.
 * @OLS_FLOW_ERROR:		 Some (fatal) error occurred. Element generating
 *                               this error should post an error message using
 *                               OLS_ELEMENT_ERROR() with more details.
 * @OLS_FLOW_NOT_SUPPORTED:	 This operation is not supported.
 * @OLS_FLOW_CUSTOM_SUCCESS:	 Elements can use values starting from
 *                               this (and higher) to define custom success
 *                               codes.
 * @OLS_FLOW_CUSTOM_SUCCESS_1:	 Pre-defined custom success code (define your
 *                               custom success code to this to avoid compiler
 *                               warnings).
 * @OLS_FLOW_CUSTOM_SUCCESS_2:	 Pre-defined custom success code.
 * @OLS_FLOW_CUSTOM_ERROR:	 Elements can use values starting from
 *                               this (and lower) to define custom error codes.
 * @OLS_FLOW_CUSTOM_ERROR_1:	 Pre-defined custom error code (define your
 *                               custom error code to this to avoid compiler
 *                               warnings).
 * @OLS_FLOW_CUSTOM_ERROR_2:	 Pre-defined custom error code.
 *
 * The result of passing data to a pad.
 *
 * Note that the custom return values should not be exposed outside of the
 * element scope.
 */
typedef enum {
  /* custom success starts here */
  OLS_FLOW_CUSTOM_SUCCESS_2 = 102,
  OLS_FLOW_CUSTOM_SUCCESS_1 = 101,
  OLS_FLOW_CUSTOM_SUCCESS = 100,

  /* core predefined */
  OLS_FLOW_OK = 0,
  /* expected failures */
  OLS_FLOW_NOT_LINKED = -1,
  OLS_FLOW_FLUSHING = -2,
  /* error cases */
  OLS_FLOW_EOS = -3,
  OLS_FLOW_NOT_NEGOTIATED = -4,
  OLS_FLOW_ERROR = -5,
  OLS_FLOW_NOT_SUPPORTED = -6,

} OlsFlowReturn;

/* data passing */
/**
 * OlsPadChainFunction:
 * @pad: the sink #OlsPad that performed the chain.
 * @parent: (allow-none): the parent of @pad. If the #Ols_PAD_FLAG_NEED_PARENT
 *          flag is set, @parent is guaranteed to be not-%NULL and remain valid
 *          during the execution of this function.
 * @buffer: (transfer full): the #OlsBuffer that is chained, not %NULL.
 *
 * A function that will be called on sinkpads when chaining buffers.
 * The function typically processes the data contained in the buffer and
 * either consumes the data or passes it on to the internally linked pad(s).
 *
 * The implementer of this function receives a refcount to @buffer and should
 * ols_buffer_unref() when the buffer is no longer needed.
 *
 * When a chain function detects an error in the data stream, it must post an
 * error on the bus and return an appropriate #OlsFlowReturn value.
 *
 * Returns: #OLS_FLOW_OK for success
 */
typedef OlsFlowReturn (*ols_pad_chain_function)(ols_pad_t *pad,
                                                ols_object_t *parent,
                                                ols_buffer_t *buffer);

/**
 * OlsPadChainListFunction:
 * @pad: the sink #OlsPad that performed the chain.
 * @parent: (allow-none): the parent of @pad. If the #Ols_PAD_FLAG_NEED_PARENT
 *          flag is set, @parent is guaranteed to be not-%NULL and remain valid
 *          during the execution of this function.
 * @list: (transfer full): the #OlsBufferList that is chained, not %NULL.
 *
 * A function that will be called on sinkpads when chaining buffer lists.
 * The function typically processes the data contained in the buffer list and
 * either consumes the data or passes it on to the internally linked pad(s).
 *
 * The implementer of this function receives a refcount to @list and
 * should ols_buffer_list_unref() when the list is no longer needed.
 *
 * When a chainlist function detects an error in the data stream, it must
 * post an error on the bus and return an appropriate #OlsFlowReturn value.
 *
 * Returns: #Ols_FLOW_OK for success
 */
typedef OlsFlowReturn (*ols_pad_chain_list_function)(ols_pad_t *pad,
                                                     ols_object_t *parent,
                                                     ols_buffer_list_t *list);

/**
 * OlsPadEventFunction:
 * @pad: the #OlsPad to handle the event.
 * @parent: (allow-none): the parent of @pad. If the #Ols_PAD_FLAG_NEED_PARENT
 *          flag is set, @parent is guaranteed to be not-%NULL and remain valid
 *          during the execution of this function.
 * @event: (transfer full): the #OlsEvent to handle.
 *
 * Function signature to handle an event for the pad.
 *
 * Returns: %TRUE if the pad could handle the event.
 */
typedef bool (*ols_pad_event_function)(ols_pad_t *pad, ols_object_t *parent,
                                       ols_event_t *event);

/**
 * OlsPadEventFullFunction:
 * @pad: the #OlsPad to handle the event.
 * @parent: (allow-none): the parent of @pad. If the #Ols_PAD_FLAG_NEED_PARENT
 *          flag is set, @parent is guaranteed to be not-%NULL and remain valid
 *          during the execution of this function.
 * @event: (transfer full): the #OlsEvent to handle.
 *
 * Function signature to handle an event for the pad.
 *
 * This variant is for specific elements that will take into account the
 * last downstream flow return (from a pad push), in which case they can
 * return it.
 *
 * Returns: %Ols_FLOW_OK if the event was handled properly, or any other
 * #OlsFlowReturn dependent on downstream state.
 *
 * Since: 1.8
 */
typedef OlsFlowReturn (*ols_pad_event_full_function)(ols_pad_t *pad,
                                                     ols_object_t *parent,
                                                     ols_event_t *event);

/* linking */
/**
 * OlsPadLinkFunction:
 * @pad: the #OlsPad that is linked.
 * @parent: (allow-none): the parent of @pad. If the #Ols_PAD_FLAG_NEED_PARENT
 *          flag is set, @parent is guaranteed to be not-%NULL and remain valid
 *          during the execution of this function.
 * @peer: the peer #OlsPad of the link
 *
 * Function signature to handle a new link on the pad.
 *
 * Returns: the result of the link with the specified peer.
 */
typedef OlsPadLinkReturn (*ols_pad_link_function)(ols_pad_t *pad,
                                                  ols_object_t *parent,
                                                  ols_pad_t *peer);
/**
 * OlsPadUnlinkFunction:
 * @pad: the #OlsPad that is linked.
 * @parent: (allow-none): the parent of @pad. If the #Ols_PAD_FLAG_NEED_PARENT
 *          flag is set, @parent is guaranteed to be not-%NULL and remain valid
 *          during the execution of this function.
 *
 * Function signature to handle a unlinking the pad prom its peer.
 *
 * The pad's lock is already held when the unlink function is called, so most
 * pad functions cannot be called from within the callback.
 */
typedef void (*ols_pad_unlink_function)(ols_pad_t *pad, ols_object_t *parent);

OlsPadLinkReturn ols_pad_link_full(ols_pad_t *srcpad, ols_pad_t *sinkpad);

OlsFlowReturn ols_pad_push(ols_pad_t *pad, ols_buffer_t *buffer);

bool ols_pad_push_event(ols_pad_t *pad, ols_event_t *event);

void ols_pad_set_chain_function_full(ols_pad_t *pad,
                                     ols_pad_chain_function chain,
                                     void *user_data);

#define ols_pad_set_chain_function(p, f)                                       \
  ols_pad_set_chain_function_full((p), (f), NULL)
/**
 * OlsPad:
 * @element_private: private data owned by the parent element
 * @padtemplate: padtemplate for this pad
 * @direction: the direction of the pad, cannot change after creating
 *             the pad.
 *
 * The #OlsPad structure. Use the functions to update the variables.
 */

#define ols_pad_set_link_function(p, f)                                        \
  ols_pad_set_link_function_full((p), (f), NULL, NULL)
#define ols_pad_set_unlink_function(p, f)                                      \
  ols_pad_set_unlink_function_full((p), (f), NULL, NULL)

struct ols_pad {
  // ols_object_t context;
  ols_mini_object_t object;
  ols_object_t *parent;

  uint32_t flags;
  /*< private >*/

  pthread_mutex_t mutex;

  OLSPadDirection direction;

  pthread_mutex_t stream_rec_lock;

  /* block cond, mutex is from the object */
  pthread_cond_t block_cond;

  OLSPadMode mode;

  /* pad link */
  ols_pad_t *peer;
  ols_pad_link_function linkfunc;
  void *linkdata;

  ols_pad_unlink_function unlinkfunc;
  void *unlinkdata;

  /* data transport functions */
  ols_pad_chain_function chainfunc;
  void *chaindata;

  ols_pad_chain_list_function chainlistfunc;
  void *chainlistdata;

  ols_pad_event_function eventfunc;
  void *eventdata;

  OlsFlowReturn last_flowret;
  ols_pad_event_full_function eventfullfunc;
};

#ifdef __cplusplus
}
#endif
