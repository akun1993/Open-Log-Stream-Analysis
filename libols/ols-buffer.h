
#pragma once
#include "ols-mini-object.h"
#include <stdint.h>

struct ols_buffer;
typedef struct ols_buffer ols_buffer_t;

#define OLS_BUFFER_CAST(obj) ((ols_buffer_t *)(obj))
#define OLS_BUFFER(obj) (OLS_BUFFER_CAST(obj))

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

static inline void ols_clear_buffer(ols_buffer_t **buf_ptr) {
  ols_clear_mini_object((ols_mini_object_t **)buf_ptr);
}

/* copy buffer */
static inline ols_buffer_t *ols_buffer_copy(const ols_buffer_t *buf) {
  return OLS_BUFFER(ols_mini_object_copy(OLS_MINI_OBJECT_CONST_CAST(buf)));
}
#else  /* OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS */

ols_buffer_t *ols_buffer_ref(ols_buffer_t *buf);

void ols_buffer_unref(ols_buffer_t *buf);

void ols_clear_buffer(ols_buffer_t **buf_ptr);

ols_buffer_t *ols_buffer_copy(const ols_buffer_t *buf);
#endif /* OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS */

ols_buffer_t *ols_buffer_copy_deep(const ols_buffer_t *buf);
