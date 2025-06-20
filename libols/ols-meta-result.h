

#pragma once
#include "ols-mini-object.h"
#include "ols-ref.h"
#include "util/darray.h"
#include "util/dstr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ols_meta_result ols_meta_result_t;

struct ols_meta_result {
  ols_mini_object_t obj;

  struct dstr tag;

  DARRAY(char *) info;

  struct ols_meta_result *next;
};

#ifdef __cplusplus
}
#endif