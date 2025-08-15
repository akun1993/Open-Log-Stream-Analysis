
#pragma once
#include "ols-mini-object.h"
#include "util/darray.h"
#include "util/c99defs.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ols_caps ols_caps_t;

#define OLS_CAPS_CAST(obj) ((ols_caps_t *)(obj))
#define OLS_CAPS(obj) (OLS_CAPS_CAST(obj))

#define OLS_CAPS_FLAGS(obj) (OLS_CAPS(obj)->flag)

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



#define GST_CAPS_LEN(caps)   (OLS_CAPS(c)->caps.num)

/* same as ols_caps_is_any () */
#define CAPS_IS_ANY(caps)				\
  (!!(OLS_CAPS_FLAGS(caps) & OLS_CAPS_ANY))

/* same as ols_caps_is_empty () */
#define CAPS_IS_EMPTY(caps)				\
  (!CAPS_IS_ANY(caps) && GST_CAPS_LEN(caps) == 0)

struct ols_caps 
{
    ols_mini_object_t mini_object;
    OlsCapsFlags flag;
    DARRAY(char *) caps;
};

EXPORT ols_caps_t *ols_caps_new(const char *caps_str);

EXPORT ols_caps_t  *ols_caps_new_any();

EXPORT const char * ols_caps_by_idx(ols_caps_t *,size_t idx);

EXPORT size_t ols_caps_count(ols_caps_t *);

/* refcounting */
static inline ols_caps_t *ols_caps_ref(ols_caps_t *ols_caps) {
  return (ols_caps_t *)ols_mini_object_ref(OLS_MINI_OBJECT_CAST(ols_caps));
}

static inline void ols_caps_unref(ols_caps_t *ols_caps) {
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(ols_caps));
}


#ifdef __cplusplus
}
#endif