

#pragma once
#include "ols-meta.h"
#include "ols-mini-object.h"
#include "util/dstr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OLS_META_TXT(obj) ((ols_meta_txt_t *)(obj))

#define OLS_META_TXT_BUFF(obj) ((obj)->buff)
#define OLS_META_TXT_BUFF_CAPACITY(obj) ((obj)->capacity)

typedef struct ols_meta_txt ols_meta_txt_t;

struct ols_meta_txt {
  struct   ols_meta meta;
  uint8_t *buff; // content
  size_t   capacity;   // length of buffer
  size_t   len;

  struct dstr file; //
  struct dstr tag;
  uint64_t msec;
  int32_t line;
  int32_t pid;
  int32_t tid;
  int32_t data_offset;
  uint8_t log_lv;
};

EXPORT ols_meta_txt_t *ols_meta_txt_new_empty(void);
EXPORT ols_meta_txt_t *ols_meta_txt_new_with_buffer(size_t buf_size);

/* refcounting */
static inline ols_meta_txt_t *ols_meta_txt_ref(ols_meta_txt_t *buf) {
  return (ols_meta_txt_t *)ols_meta_ref(OLS_META_CAST(buf));
}

static inline void ols_meta_txt_unref(ols_meta_txt_t *buf) {
  ols_meta_unref(OLS_META_CAST(buf));
}

/* copy meta txt */

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
  OLS_META_TXT_COPY_NONE = 0,
  OLS_META_TXT_COPY_FLAGS = (1 << 0),
  OLS_META_TXT_COPY_MEMORY = (1 << 1),
  OLS_META_TXT_COPY_DEEP = (1 << 2)
} OlsMetaTxtCopyFlags;

static inline ols_meta_txt_t *ols_meta_txt_copy(const ols_meta_txt_t *buf) {
  return OLS_META_TXT(ols_meta_copy(OLS_META_CAST(buf)));
}

//ols_meta_txt_t *ols_meta_txt_copy_deep(const ols_meta_txt_t *buf);

#ifdef __cplusplus
}
#endif