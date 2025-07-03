

#pragma once
#include "ols-meta-result.h"
#include "ols-meta.h"
#include "ols-mini-object.h"
#include "ols-ref.h"
#include "util/dstr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OLS_TXTFILE_CAST(obj) ((ols_txt_file_t *)(obj))
#define OSL_TXTFILE(obj) (OLS_TXTFILE_CAST(obj))

#define OSL_TXTFILE_BUFF(obj) ((obj)->buff)
#define OSL_TXTFILE_BUFF_CAPACITY(obj) ((obj)->capacity)

typedef struct ols_txt_file ols_txt_file_t;

struct ols_txt_file {
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
  uint8_t log_lv;
};

EXPORT ols_txt_file_t *ols_txt_file_new_empty(void);
EXPORT ols_txt_file_t *ols_txt_file_with_buffer(size_t buf_size);

#ifdef __cplusplus
}
#endif