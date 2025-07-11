#pragma once
#include "ols-mini-object.h"
#include "util/dstr.h"

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * GstMetaFlags:
 * @GST_META_FLAG_NONE: no flags
 * @GST_META_FLAG_READONLY: metadata should not be modified
 * @GST_META_FLAG_LOCKED: metadata should not be removed
 * @GST_META_FLAG_LAST: additional flags can be added starting from this flag.
 *
 * Extra metadata flags.
 */
typedef enum {
  OLS_META_FLAG_NONE        = 0,
  OLS_META_FLAG_READONLY    = (1 << 0),
  OLS_META_FLAG_LOCKED      = (1 << 1),

  OLS_META_FLAG_LAST        = (1 << 16)
} OlsMetaFlags;

typedef struct ols_meta ols_meta_t;

#define OLS_META_CAST(obj)        ((ols_meta_t*)(obj))
#define OSL_META(obj)             (OLS_META_CAST(obj))

struct ols_meta {
  ols_mini_object_t  obj;
  OlsMetaFlags       flags;
};

/* refcount */
/**
 * OLS_META_REFCOUNT:
 * @caps: a #GstCaps
 *
 * Get access to the reference count field of the caps
 */
#define OLS_META_REFCOUNT(ols_meta)                 OLS_MINI_OBJECT_REFCOUNT(OLS_MINI_OBJECT_CAST (ols_meta))
/**
 * OLS_META_REFCOUNT_VALUE:
 * @caps: a #GstCaps
 *
 * Get the reference count value of the caps.
 */
#define OLS_META_REFCOUNT_VALUE(ols_meta)           OLS_MINI_OBJECT_REFCOUNT_VALUE(OLS_MINI_OBJECT_CAST (ols_meta))


/* refcounting */
/**
 * ols_meta_ref:
 * @meta: the #ols_meta_t to reference
 *
 * Add a reference to a #ols_meta_t object.
 *
 * From this point on, until the caller calls ols_meta_unref() 
 *
 * Returns: the same #ols_meta_t object.
 */
static inline ols_meta_t *
ols_meta_ref (ols_meta_t * meta)
{
  return (ols_meta_t *) ols_mini_object_ref (OLS_MINI_OBJECT_CAST (meta));
}

/**
 * ols_meta_unref:
 * @meta: a #ols_meta_t.
 *
 * Unref a #ols_meta_t and and free all its structures and the
 * structures' values when the refcount reaches 0.
 */
static inline void
ols_meta_unref (ols_meta_t * meta)
{
  ols_mini_object_unref (OLS_MINI_OBJECT_CAST (meta));
}


/* lock / unlock */
static inline bool  ols_meta_lock (ols_meta_t *meta, OlsLockFlags flags){
  return ols_mini_object_lock(OLS_MINI_OBJECT_CAST(meta),flags);
}

static inline void  ols_meta_unlock (ols_meta_t *meta, OlsLockFlags flags){
  ols_mini_object_unlock(OLS_MINI_OBJECT_CAST(meta),flags);
}

/* copy meta */
/**
 * ols_meta_copy:
 * @meta: a #ols_meta_t.
 *
 * Creates a new #ols_meta_t as a copy of the old @meta. The new caps will have a
 * refcount of 1, owned by the caller. The structures are copied as well.
 *
 * Note that this function is the semantic equivalent of a ols_meta_ref()
 * . If you only want to hold on to a reference to the data, you should use ols_meta_ref().
 *
 * When you are finished with the caps, call ols_meta_unref() on it.
 *
 * Returns: the new #ols_meta_t
 */
static inline ols_meta_t *
ols_meta_copy (const ols_meta_t * meta)
{
  return OSL_META (ols_mini_object_copy (OLS_MINI_OBJECT_CAST (meta)));
}



#ifdef __cplusplus
}
#endif