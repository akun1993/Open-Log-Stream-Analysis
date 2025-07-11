
#pragma once
#include "ols-meta-result.h"
#include "ols-mini-object.h"
#include "util/dstr.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ols_meta;

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

  struct ols_meta *meta;

  struct ols_meta_result *result;
};

/* refcounting */
static inline ols_buffer_t *ols_buffer_ref(ols_buffer_t *buf) {
  return (ols_buffer_t *)ols_mini_object_ref(OLS_MINI_OBJECT_CAST(buf));
}

static inline void ols_buffer_unref(ols_buffer_t *buf) {
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(buf));
}

/* lock / unlock */
static inline bool  ols_buffer_lock (ols_buffer_t *buf, OlsLockFlags flags){
  return ols_mini_object_lock(OLS_MINI_OBJECT_CAST(buf),flags);
}

static inline void  ols_buffer_unlock (ols_buffer_t *buf, OlsLockFlags flags){
  ols_mini_object_unlock(OLS_MINI_OBJECT_CAST(buf),flags);
}

/* copy buffer */

/**
 * OlsBufferCopyFlags:
 * @OLS_BUFFER_COPY_NONE: copy nothing
 * @OLS_BUFFER_COPY_FLAGS: flag indicating that buffer flags should be copied
 * @OLS_BUFFER_COPY_MERGE: flag indicating that buffer memory should be
 *   merged
 *
 * A set of flags that can be provided to the ols_buffer_copy_into()
 * function to specify which items should be copied.
 */
typedef enum {
  OLS_BUFFER_COPY_NONE = 0,
  OLS_BUFFER_COPY_FLAGS = (1 << 0),
  OLS_BUFFER_COPY_MEMORY = (1 << 1),
  OLS_BUFFER_COPY_DEEP = (1 << 2)
} OlsBufferCopyFlags;

static inline ols_buffer_t *ols_buffer_copy(const ols_buffer_t *buf) {
  return OLS_BUFFER(ols_mini_object_copy(OLS_MINI_OBJECT_CONST_CAST(buf)));
}

ols_buffer_t *ols_buffer_copy_deep(const ols_buffer_t *buf);

void ols_buffer_set_meta(ols_buffer_t *buf, struct ols_meta *meta);

#ifdef __cplusplus
}
#endif
