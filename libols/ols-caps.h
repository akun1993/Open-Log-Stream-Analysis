
#pragma once
#include "util/darray.h"
#include "util/dstr.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ols_caps ols_caps_t;

/**
 * OlsCapsFlags:
 * @OLS_CAPS_DEFAULT: Caps mush has specific content
 * @OLS_CAPS_ANY: Caps has no specific content, but can contain
 *    anything.
 *
 * Extra flags for a caps.
 */

typedef enum {
  OLS_CAPS_DEFAULT = 0,
  OLS_CAPS_ANY = (1 << 0),
} OlsCapsFlags;

struct ols_caps 
{
    OlsCapsFlags flag;
    DARRAY(char *) caps;
};

ols_caps_t *ols_caps_new(const char *caps_str);

ols_caps_t  *ols_caps_new_any();

void  ols_caps_free(ols_caps_t *caps);

const char * ols_caps_by_idx(ols_caps_t *,size_t idx);

int ols_caps_count(ols_caps_t *);

#ifdef __cplusplus
}
#endif