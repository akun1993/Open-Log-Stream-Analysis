
#pragma once
#include "ols-mini-object.h"
#include <stdint.h>

struct ols_event;
typedef struct ols_event ols_event_t;

/**
 * OlsEventTypeFlags:
 * @OLS_EVENT_TYPE_UPSTREAM:     Set if the event can travel upstream.
 * @OLS_EVENT_TYPE_DOWNSTREAM:   Set if the event can travel downstream.
 * @OLS_EVENT_TYPE_SERIALIZED:   Set if the event should be serialized with data
 *                               flow.
 *
 * #OlsEventTypeFlags indicate the aspects of the different #OlsEventType
 * values. You can get the type flags of a #OlsEventType with the
 * ols_event_type_get_flags() function.
 */
typedef enum {
  OLS_EVENT_TYPE_UPSTREAM = 1 << 0,
  OLS_EVENT_TYPE_DOWNSTREAM = 1 << 1,
  OLS_EVENT_TYPE_SERIALIZED = 1 << 2,
} OlsEventTypeFlags;

#define OLS_EVENT_NUM_SHIFT (8)

/**
 * OLS_EVENT_MAKE_TYPE:
 * @num: the event number to create
 * @flags: the event flags
 *
 * when making custom event types, use this macro with the num and
 * the given flags
 */
#define OLS_EVENT_MAKE_TYPE(num, flags)                                        \
  (((num) << OLS_EVENT_NUM_SHIFT) | (flags))

#define OLS_EVENT_CAST(obj) ((ols_event_t *)(obj))
#define OLS_EVENT(obj) (OLS_EVENT_CAST(obj))

/**
 * OLS_EVENT_TYPE:
 * @event: the event to query
 *
 * Get the #OlsEventType of the event.
 */
#define OLS_EVENT_TYPE(event) (OLS_EVENT_CAST(event)->ev_type)

/**
 * OLS_EVENT_IS_UPSTREAM:
 * @ev: the event to query
 *
 * Check if an event can travel upstream.
 */
#define OLS_EVENT_IS_UPSTREAM(ev)                                              \
  !!(OLS_EVENT_TYPE(ev) & OLS_EVENT_TYPE_UPSTREAM)
/**
 * OLS_EVENT_IS_DOWNSTREAM:
 * @ev: the event to query
 *
 * Check if an event can travel downstream.
 */
#define OLS_EVENT_IS_DOWNSTREAM(ev)                                            \
  !!(OLS_EVENT_TYPE(ev) & OLS_EVENT_TYPE_DOWNSTREAM)

/**
 * OLS_EVENT_TYPE_BOTH: (value 3) (type OlsEventTypeFlags)
 *
 * The same thing as #OLS_EVENT_TYPE_UPSTREAM | #OLS_EVENT_TYPE_DOWNSTREAM.
 */
#define OLS_EVENT_TYPE_BOTH                                                    \
  ((OlsEventTypeFlags)(OLS_EVENT_TYPE_UPSTREAM | OLS_EVENT_TYPE_DOWNSTREAM))

/**
 * OLS_EVENT_IS_SERIALIZED:
 * @ev: the event to query
 *
 * Check if an event is serialized with the data stream.
 */
#define OLS_EVENT_IS_SERIALIZED(ev)                                            \
  !!(OLS_EVENT_TYPE(ev) & OLS_EVENT_TYPE_SERIALIZED)
#define _FLAG(name) OLS_EVENT_TYPE_##name
/**
 * OLSEventType:
 * @OLS_EVENT_UNKNOWN: unknown event.
 * @OLS_EVENT_FLUSH_START: Start a flush operation. This event clears all data
 *                 from the pipeline and unblock all streaming threads.
 * @OLS_EVENT_FLUSH_STOP: Stop a flush operation. This event resets the
 *                 running-time of the pipeline.
 * @OLS_EVENT_STREAM_START: Event to mark the start of a new stream. Sent before
 * any other serialized event and only sent at the start of a new stream, not
 * after flushing seeks.
 * @OLS_EVENT_EOS: End-Of-Stream. No more data is to be expected to follow
 *                 without either a STREAM_START event, or a FLUSH_STOP and a
 * SEGMENT event.
 * @OLS_EVENT_GAP: Marks a gap in the datastream.

 * @OLS_EVENT_QOS: A quality message. Used to indicate to upstream elements
 *                 that the downstream elements should adjust their processing
 *                 rate.
 *
 * The custom event types can be used for private messages between elements
 * that can't be expressed using normal
 * OLSreamer buffer passing semantics. Custom events carry an arbitrary
 * #OLSStructure.
 * Specific custom events are distinguished by the name of the structure.
 */
/* NOTE: keep in sync with quark registration in olsevent.c */
typedef enum {
  OLS_EVENT_UNKNOWN = OLS_EVENT_MAKE_TYPE(0, 0),

  /* bidirectional events */
  OLS_EVENT_FLUSH_START = OLS_EVENT_MAKE_TYPE(10, _FLAG(BOTH)),

  OLS_EVENT_FLUSH_STOP =
      OLS_EVENT_MAKE_TYPE(20, _FLAG(BOTH) | _FLAG(SERIALIZED)),

  /* downstream serialized events */
  OLS_EVENT_STREAM_START =
      OLS_EVENT_MAKE_TYPE(30, _FLAG(DOWNSTREAM) | _FLAG(SERIALIZED)),

  OLS_EVENT_EOS =
      OLS_EVENT_MAKE_TYPE(40, _FLAG(DOWNSTREAM) | _FLAG(SERIALIZED)),

  // OLS_EVENT_TAG =
  //     OLS_EVENT_MAKE_TYPE(80, _FLAG(DOWNSTREAM) | _FLAG(SERIALIZED) |
  //                                 _FLAG(STICKY) | _FLAG(STICKY_MULTI)),
  // OLS_EVENT_BUFFERSIZE = OLS_EVENT_MAKE_TYPE(
  //     90, _FLAG(DOWNSTREAM) | _FLAG(SERIALIZED) | _FLAG(STICKY)),
  // OLS_EVENT_CAPS = OLS_EVENT_MAKE_TYPE(
  //     50, _FLAG(DOWNSTREAM) | _FLAG(SERIALIZED) | _FLAG(STICKY)),

  /* upstream events */

} ols_event_type;

#ifndef OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS
/* refcounting */
static inline ols_event_t *ols_event_ref(ols_event_t *event) {
  return (ols_event_t *)ols_mini_object_ref(OLS_MINI_OBJECT_CAST(event));
}

static inline void ols_event_unref(ols_event_t *event) {
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(event));
}

/* copy event */
static inline ols_event_t *ols_event_copy(const ols_event_t *event) {
  return OLS_EVENT_CAST(
      ols_mini_object_copy(OLS_MINI_OBJECT_CONST_CAST(event)));
}
#else  /* OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS */

ols_event_t *ols_event_ref(ols_event_t *event);

void ols_event_unref(ols_event_t *event);

ols_event_t *ols_event_copy(const ols_event_t *event);
#endif /* OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS */

struct ols_event {
  ols_mini_object_t mini_object;
  ols_event_type ev_type;
};
