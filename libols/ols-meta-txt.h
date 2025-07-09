

#pragma once
#include "ols-meta.h"
#include "ols-mini-object.h"
#include "util/dstr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OLS_TXTFILE_CAST(obj) ((ols_meta_txt_t *)(obj))
#define OSL_TXTFILE(obj) (OLS_TXTFILE_CAST(obj))

#define OSL_TXTFILE_BUFF(obj) ((obj)->buff)
#define OSL_TXTFILE_BUFF_CAPACITY(obj) ((obj)->capacity)

typedef struct ols_meta_txt ols_meta_txt_t;

struct ols_meta_txt {
  struct ols_meta meta;
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

#ifdef __cplusplus
}
#endif