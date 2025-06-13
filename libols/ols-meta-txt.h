

#pragma once
#include "ols-mini-object.h"
#include "util/dstr.h"
#include "ols-ref.h"
#include "ols-meta.h"

#ifdef __cplusplus
    extern "C" {
#endif


typedef struct ols_txt_file ols_txt_file_t;

typedef struct ols_meta_txt ols_meta_txt_t;


struct ols_meta_txt {
  ols_meta_t       meta;

  ols_txt_file_t *ref;
};


struct ols_txt_file_t {
    ols_mini_object_t  obj;

    uint8_t *data; //content 
    size_t  size; //length of data

    struct dstr file; //
    int   line; 

};




#ifdef __cplusplus
}
#endif