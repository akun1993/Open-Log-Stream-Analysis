
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
 * @OLS_EVENT_TYPE_STICKY:       Set if the event is sticky on the pads.
 * @OLS_EVENT_TYPE_STICKY_MULTI: Multiple sticky events can be on a pad, each
 *                               identified by the event name.
 *
 * #OlsEventTypeFlags indicate the aspects of the different #OlsEventType
 * values. You can get the type flags of a #OlsEventType with the
 * ols_event_type_get_flags() function.
 */
typedef enum {
  OLS_EVENT_TYPE_UPSTREAM = 1 << 0,
  OLS_EVENT_TYPE_DOWNSTREAM = 1 << 1,
  OLS_EVENT_TYPE_SERIALIZED = 1 << 2,
  OLS_EVENT_TYPE_STICKY = 1 << 3,
  OLS_EVENT_TYPE_STICKY_MULTI = 1 << 4
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
 * @OLS_EVENT_SELECT_STREAMS: A request to select one or more streams
 * (Since: 1.10)
 * @OLS_EVENT_STREAM_START: Event to mark the start of a new stream. Sent before
 * any other serialized event and only sent at the start of a new stream, not
 * after flushing seeks.
 * @OLS_EVENT_CAPS: #OLSCaps event. Notify the pad of a new media type.
 * @OLS_EVENT_SEGMENT: A new media segment follows in the dataflow. The
 *                 segment events contains information for clipping buffers and
 *                 converting buffer timestamps to running-time and
 *                 stream-time.
 * @OLS_EVENT_STREAM_COLLECTION: A new #OLSStreamCollection is available
 * (Since: 1.10)
 * @OLS_EVENT_TAG: A new set of metadata tags has been found in the stream.
 * @OLS_EVENT_BUFFERSIZE: Notification of buffering requirements. Currently not
 *                 used yet.
 * @OLS_EVENT_SINK_MESSAGE: An event that sinks turn into a message. Used to
 *                          send messages that should be emitted in sync with
 *                          rendering.
 * @OLS_EVENT_STREAM_GROUP_DONE: Indicates that there is no more data for
 *                 the stream group ID in the message. Sent before EOS
 *                 in some instances and should be handled mostly the same.
 * (Since: 1.10)
 * @OLS_EVENT_EOS: End-Of-Stream. No more data is to be expected to follow
 *                 without either a STREAM_START event, or a FLUSH_STOP and a
 * SEGMENT event.
 * @OLS_EVENT_SEGMENT_DONE: Marks the end of a segment playback.
 * @OLS_EVENT_GAP: Marks a gap in the datastream.
 * @OLS_EVENT_TOC: An event which indicates that a new table of contents (TOC)
 *                 was found or updated.
 * @OLS_EVENT_PROTECTION: An event which indicates that new or updated
 *                 encryption information has been found in the stream.
 * @OLS_EVENT_QOS: A quality message. Used to indicate to upstream elements
 *                 that the downstream elements should adjust their processing
 *                 rate.
 * @OLS_EVENT_SEEK: A request for a new playback position and rate.
 * @OLS_EVENT_NAVIGATION: Navigation events are usually used for communicating
 *                        user requests, such as mouse or keyboard movements,
 *                        to upstream elements.
 * @OLS_EVENT_LATENCY: Notification of new latency adjustment. Sinks will use
 *                     the latency information to adjust their synchronisation.
 * @OLS_EVENT_STEP: A request for stepping through the media. Sinks will usually
 *                  execute the step operation.
 * @OLS_EVENT_RECONFIGURE: A request for upstream renegotiating caps and
 * reconfiguring.
 * @OLS_EVENT_TOC_SELECT: A request for a new playback position based on TOC
 *                        entry's UID.
 * @OLS_EVENT_INSTANT_RATE_CHANGE: Notify downstream that a playback rate
 * override should be applied as soon as possible. (Since: 1.18)
 * @OLS_EVENT_INSTANT_RATE_SYNC_TIME: Sent by the pipeline to notify elements
 * that handle the instant-rate-change event about the running-time when the
 * rate multiplier should be applied (or was applied). (Since: 1.18)
 * @OLS_EVENT_CUSTOM_UPSTREAM: Upstream custom event
 * @OLS_EVENT_CUSTOM_DOWNSTREAM: Downstream custom event that travels in the
 *                        data flow.
 * @OLS_EVENT_CUSTOM_DOWNSTREAM_OOB: Custom out-of-band downstream event.
 * @OLS_EVENT_CUSTOM_DOWNSTREAM_STICKY: Custom sticky downstream event.
 * @OLS_EVENT_CUSTOM_BOTH: Custom upstream or downstream event.
 *                         In-band when travelling downstream.
 * @OLS_EVENT_CUSTOM_BOTH_OOB: Custom upstream or downstream out-of-band event.
 *
 * #OLSEventType lists the standard event types that can be sent in a pipeline.
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
  OLS_EVENT_STREAM_START = OLS_EVENT_MAKE_TYPE(
      40, _FLAG(DOWNSTREAM) | _FLAG(SERIALIZED) | _FLAG(STICKY)),
  OLS_EVENT_TAG =
      OLS_EVENT_MAKE_TYPE(80, _FLAG(DOWNSTREAM) | _FLAG(SERIALIZED) |
                                  _FLAG(STICKY) | _FLAG(STICKY_MULTI)),
  OLS_EVENT_BUFFERSIZE = OLS_EVENT_MAKE_TYPE(
      90, _FLAG(DOWNSTREAM) | _FLAG(SERIALIZED) | _FLAG(STICKY)),
  OLS_EVENT_CAPS = OLS_EVENT_MAKE_TYPE(
      50, _FLAG(DOWNSTREAM) | _FLAG(SERIALIZED) | _FLAG(STICKY)),
  OLS_EVENT_EOS = OLS_EVENT_MAKE_TYPE(
      110, _FLAG(DOWNSTREAM) | _FLAG(SERIALIZED) | _FLAG(STICKY)),

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

static inline void ols_clear_event(ols_event_t **event_ptr) {
  ols_clear_mini_object((ols_mini_object_t **)event_ptr);
}

/* copy event */
static inline ols_event_t *ols_event_copy(const ols_event_t *event) {
  return OLS_EVENT_CAST(
      ols_mini_object_copy(OLS_MINI_OBJECT_CONST_CAST(event)));
}
#else  /* OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS */

ols_event_t *ols_event_ref(ols_event_t *event);

void ols_event_unref(ols_event_t *event);

void ols_clear_event(ols_event_t **event_ptr);

ols_event_t *ols_event_copy(const ols_event_t *event);
#endif /* OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS */

struct ols_event {
  ols_mini_object_t mini_object;
  ols_event_type ev_type;
};
