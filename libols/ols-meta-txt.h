

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

/* refcount */
/**
 * OLS_TXT_FILE_REFCOUNT:
 * @caps: a #GstCaps
 *
 * Get access to the reference count field of the caps
 */
#define OLS_TXT_FILE_REFCOUNT(ols_txt)                 OLS_MINI_OBJECT_REFCOUNT(ols_txt)
/**
 * OLS_TXT_FILE_REFCOUNT_VALUE:
 * @caps: a #GstCaps
 *
 * Get the reference count value of the caps.
 */
#define OLS_TXT_FILE_REFCOUNT_VALUE(ols_txt)           OLS_MINI_OBJECT_REFCOUNT_VALUE(ols_txt)


/* refcounting */
/**
 * gst_caps_ref:
 * @caps: the #GstCaps to reference
 *
 * Add a reference to a #GstCaps object.
 *
 * From this point on, until the caller calls gst_caps_unref() or
 * gst_caps_make_writable(), it is guaranteed that the caps object will not
 * change. This means its structures won't change, etc. To use a #GstCaps
 * object, you must always have a refcount on it -- either the one made
 * implicitly by e.g. gst_caps_new_simple(), or via taking one explicitly with
 * this function.
 *
 * Returns: the same #GstCaps object.
 */
static inline ols_txt_file_t *
ols_txt_file_ref (ols_txt_file_t * txt_file)
{
  return (ols_txt_file_t *) ols_mini_object_ref (OLS_MINI_OBJECT_CAST (txt_file));
}

/**
 * gst_caps_unref:
 * @caps: a #GstCaps.
 *
 * Unref a #GstCaps and and free all its structures and the
 * structures' values when the refcount reaches 0.
 */
static inline void
ols_txt_file_unref (ols_txt_file_t * txt_file)
{
  ols_mini_object_unref (OLS_MINI_OBJECT_CAST (txt_file));
}

/* copy caps */
/**
 * gst_caps_copy:
 * @caps: a #GstCaps.
 *
 * Creates a new #GstCaps as a copy of the old @caps. The new caps will have a
 * refcount of 1, owned by the caller. The structures are copied as well.
 *
 * Note that this function is the semantic equivalent of a gst_caps_ref()
 * followed by a gst_caps_make_writable(). If you only want to hold on to a
 * reference to the data, you should use gst_caps_ref().
 *
 * When you are finished with the caps, call gst_caps_unref() on it.
 *
 * Returns: the new #GstCaps
 */
static inline ols_txt_file_t *
ols_txt_file_copy (const ols_txt_file_t * txt_file)
{
  return OSL_TXTFILE (ols_mini_object_copy (OLS_MINI_OBJECT_CAST (txt_file)));
}




#ifdef __cplusplus
}
#endif