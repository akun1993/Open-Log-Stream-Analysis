#pragma once
#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

EXPORT bool parse(const char * format, const char * input, int64_t* sec,
           int64_t* fs,const char **err) ;

#ifdef __cplusplus
}
#endif


