

#pragma once
#include "ols-mini-object.h"
#include "util/dstr.h"
#include "ols-ref.h"
#include "ols-meta.h"

#ifdef __cplusplus
extern "C" {
#endif


#define OLS_TXTFILE_CAST(obj)        ((ols_txt_file_t*)(obj))
#define OSL_TXTFILE(obj)             (OLS_TXTFILE_CAST(obj))

#define OSL_TXTFILE_BUFF(obj)             ((obj)->data)
#define OSL_TXTFILE_BUFF_SIZE(obj)             ((obj)->size)

typedef struct ols_txt_file ols_txt_file_t;


struct ols_txt_file {
  struct ols_meta;
  uint8_t *data; //content 
  size_t  size; //length of data

  struct dstr file; //
  int   line; 

};

typedef struct ols_result ols_result_t;

struct ols_result {
  ols_mini_object_t  obj;

  struct dstr tag;

  struct dstr info;

  struct ols_result *next;
};

EXPORT ols_txt_file_t *ols_txt_file_new_empty (void);
EXPORT ols_txt_file_t *ols_txt_file_with_buffer (size_t buf_size);

#ifdef __cplusplus
}
#endif