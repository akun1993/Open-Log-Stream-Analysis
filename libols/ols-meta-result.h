

#pragma once
#include "ols-meta.h"
#include "util/darray.h"
#include "util/dstr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OLS_META_RESULT_CAST(obj) ((ols_meta_result_t *)(obj))

typedef struct ols_meta_result ols_meta_result_t;

typedef struct result_info_msg {
  char * key;
  char * val;
} result_info_msg_t;


struct ols_meta_result {
  ols_meta_t meta;
  //char      time[128];
  struct dstr tag;
  DARRAY(result_info_msg_t *) info;
};  


EXPORT ols_meta_result_t *ols_meta_result_new(void);

/* refcounting */
static inline ols_meta_result_t *ols_meta_result_ref(ols_meta_result_t *buf) {
  return (ols_meta_result_t *)ols_meta_ref(OLS_META_CAST(buf));
}

static inline void ols_meta_result_unref(ols_meta_result_t *buf) {
  ols_meta_unref(OLS_META_CAST(buf));
}


#ifdef __cplusplus
}
#endif