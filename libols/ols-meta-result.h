

#pragma once
#include "ols-meta.h"
#include "util/darray.h"
#include "util/dstr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ols_meta_result ols_meta_result_t;

typedef struct result_info_msg {
  char * key;
  char * val;
} result_info_msg_t;

#define OLS_META_RESULT_CAST(obj) ((ols_meta_result_t *)(obj))

struct ols_meta_result {
  ols_meta_t meta;
  //char      time[128];
  struct dstr tag;
  DARRAY(result_info_msg_t *) info;
};  


EXPORT ols_meta_result_t *ols_meta_result_new(void);

EXPORT void ols_meta_result_add_info(ols_meta_result_t * meta_result, const char *key, const char *val);

/* refcounting */
static inline ols_meta_result_t *ols_meta_result_ref(ols_meta_result_t *meta_result) {
  return (ols_meta_result_t *)ols_meta_ref(OLS_META_CAST(meta_result));
}

static inline void ols_meta_result_unref(ols_meta_result_t *meta_result) {
  ols_meta_unref(OLS_META_CAST(meta_result));
}


#ifdef __cplusplus
}
#endif