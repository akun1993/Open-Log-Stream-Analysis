

#pragma once
#include <cstdint>

struct OlsPad;
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
typedef OlsFlowReturn (*OlsPadChainFunction)(OlsPad *pad, OlsObject *parent,
                                             OlsBuffer *buffer);

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
typedef OlsFlowReturn (*OlsPadChainListFunction)(OlsPad *pad, OlsObject *parent,
                                                 OlsBufferList *list);

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
typedef bool (*OlsPadEventFunction)(OlsPad *pad, OlsObject *parent,
                                    OlsEvent *event);

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
typedef OlsFlowReturn (*OlsPadEventFullFunction)(OlsPad *pad, OlsObject *parent,
                                                 OlsEvent *event);

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
typedef OlsPadLinkReturn (*OlsPadLinkFunction)(OlsPad *pad, OlsObject *parent,
                                               OlsPad *peer);
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
typedef void (*OlsPadUnlinkFunction)(OlsPad *pad, OlsObject *parent);

/**
 * OlsPad:
 * @element_private: private data owned by the parent element
 * @padtemplate: padtemplate for this pad
 * @direction: the direction of the pad, cannot change after creating
 *             the pad.
 *
 * The #OlsPad structure. Use the functions to update the variables.
 */
struct _OlsPad {
  OLSObject object;

  /*< private >*/
  /* streaming rec_lock */
  GRecMutex stream_rec_lock;
  OLSTask *task;

  /* block cond, mutex is from the object */
  GCond block_cond;
  GHookList probes;

  /* pad link */
  OlsPad *peer;
  OlsPadLinkFunction linkfunc;
  gpointer linkdata;
  // GDestroyNotify linknotify;
  OlsPadUnlinkFunction unlinkfunc;
  gpointer unlinkdata;
  // GDestroyNotify unlinknotify;

  /* data transport functions */
  OlsPadChainFunction chainfunc;
  gpointer chaindata;
  // GDestroyNotify chainnotify;
  OlsPadChainListFunction chainlistfunc;
  gpointer chainlistdata;
  // GDestroyNotify chainlistnotify;

  GDestroyNotify getrangenotify;
  OlsPadEventFunction eventfunc;
  gpointer eventdata;
  // GDestroyNotify eventnotify;

  /* pad offset */
  gint64 offset;

  /* counts number of probes attached. */
  int32_t num_probes;
  int32_t num_blocked;

  OlsPadPrivate *priv;

  OlsFlowReturn last_flowret;
  OlsPadEventFullFunction eventfullfunc;
};
