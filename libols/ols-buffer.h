
#pragma once
#include "ols-mini-object.h"
#include <stdint.h>

struct ols_buffer;
typedef struct ols_buffer ols_buffer_t;

#define OLS_BUFFER_CAST(obj) ((ols_buffer_t *)(obj))
#define OLS_BUFFER(obj) (OLS_BUFFER_CAST(obj))

/**
 * OLS_BUFFER_FLAGS:
 * @buf: a #OlsBuffer.
 *
 * Returns a flags word containing #OlsBufferFlags flags set on this buffer.
 */
#define OLS_BUFFER_FLAGS(buf) OLS_MINI_OBJECT_FLAGS(buf)
/**
 * OLS_BUFFER_FLAG_IS_SET:
 * @buf: a #OlsBuffer.
 * @flag: the #OlsBufferFlags flag to check.
 *
 * Gives the status of a specific flag on a buffer.
 */
#define OLS_BUFFER_FLAG_IS_SET(buf, flag) OLS_MINI_OBJECT_FLAG_IS_SET(buf, flag)
/**
 * OLS_BUFFER_FLAG_SET:
 * @buf: a #OlsBuffer.
 * @flag: the #OlsBufferFlags flag to set.
 *
 * Sets a buffer flag on a buffer.
 */
#define OLS_BUFFER_FLAG_SET(buf, flag) OLS_MINI_OBJECT_FLAG_SET(buf, flag)
/**
 * OLS_BUFFER_FLAG_UNSET:
 * @buf: a #OlsBuffer.
 * @flag: the #OlsBufferFlags flag to clear.
 *
 * Clears a buffer flag.
 */
#define OLS_BUFFER_FLAG_UNSET(buf, flag) OLS_MINI_OBJECT_FLAG_UNSET(buf, flag)

struct ols_buffer {
  ols_mini_object_t mini_object;
  char *buffer;
};

#ifndef OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS
/* refcounting */
static inline ols_buffer_t *ols_buffer_ref(ols_buffer_t *buf) {
  return (ols_buffer_t *)ols_mini_object_ref(OLS_MINI_OBJECT_CAST(buf));
}

static inline void ols_buffer_unref(ols_buffer_t *buf) {
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(buf));
}

/* copy buffer */

/**
 * OlsBufferCopyFlags:
 * @OLS_BUFFER_COPY_NONE: copy nothing
 * @OLS_BUFFER_COPY_FLAGS: flag indicating that buffer flags should be copied
 * @OLS_BUFFER_COPY_TIMESTAMPS: flag indicating that buffer pts, dts,
 *   duration, offset and offset_end should be copied
 * @OLS_BUFFER_COPY_MEMORY: flag indicating that buffer memory should be reffed
 *   and appended to already existing memory. Unless the memory is marked as
 *   NO_SHARE, no actual copy of the memory is made but it is simply reffed.
 *   Add @OLS_BUFFER_COPY_DEEP to force a real copy.
 * @OLS_BUFFER_COPY_MERGE: flag indicating that buffer memory should be
 *   merged
 * @OLS_BUFFER_COPY_META: flag indicating that buffer meta should be
 *   copied
 *
 * A set of flags that can be provided to the ols_buffer_copy_into()
 * function to specify which items should be copied.
 */
typedef enum {
  OLS_BUFFER_COPY_NONE = 0,
  OLS_BUFFER_COPY_FLAGS = (1 << 0),
  OLS_BUFFER_COPY_TIMESTAMPS = (1 << 1),
  OLS_BUFFER_COPY_META = (1 << 2),
  OLS_BUFFER_COPY_MEMORY = (1 << 3),
  OLS_BUFFER_COPY_MERGE = (1 << 4),
  OLS_BUFFER_COPY_DEEP = (1 << 5)
} OlsBufferCopyFlags;

/**
 * OLS_BUFFER_COPY_METADATA: (value 7) (type OlsBufferCopyFlags)
 *
 * Combination of all possible metadata fields that can be copied with
 * ols_buffer_copy_into().
 */
#define OLS_BUFFER_COPY_METADATA                                               \
  ((OlsBufferCopyFlags)(OLS_BUFFER_COPY_FLAGS | OLS_BUFFER_COPY_TIMESTAMPS |   \
                        OLS_BUFFER_COPY_META))

/**
 * OLS_BUFFER_COPY_ALL: (value 15) (type OlsBufferCopyFlags)
 *
 * Combination of all possible fields that can be copied with
 * ols_buffer_copy_into().
 */
#define OLS_BUFFER_COPY_ALL                                                    \
  ((OlsBufferCopyFlags)(OLS_BUFFER_COPY_METADATA | OLS_BUFFER_COPY_MEMORY))

static inline ols_buffer_t *ols_buffer_copy(const ols_buffer_t *buf) {
  return OLS_BUFFER(ols_mini_object_copy(OLS_MINI_OBJECT_CONST_CAST(buf)));
}
#else  /* OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS */

ols_buffer_t *ols_buffer_ref(ols_buffer_t *buf);

void ols_buffer_unref(ols_buffer_t *buf);

ols_buffer_t *ols_buffer_copy(const ols_buffer_t *buf);
#endif /* OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS */

ols_buffer_t *ols_buffer_copy_deep(const ols_buffer_t *buf);
