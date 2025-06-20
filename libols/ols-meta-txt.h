

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

#define OSL_TXTFILE_BUFF(obj) ((obj)->data)
#define OSL_TXTFILE_BUFF_SIZE(obj) ((obj)->size)

typedef struct ols_txt_file ols_txt_file_t;

struct ols_txt_file {
  struct ols_meta meta;
  uint8_t *data; // content
  size_t size;   // length of data

  struct dstr file; //
  int line;
};

EXPORT ols_txt_file_t *ols_txt_file_new_empty(void);
EXPORT ols_txt_file_t *ols_txt_file_with_buffer(size_t buf_size);

#ifdef __cplusplus
}
#endif