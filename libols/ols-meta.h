#pragma once
#include "ols-mini-object.h"
#include "util/dstr.h"
#include "ols-ref.h"


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

struct ols_meta;

typedef struct ols_meta ols_meta_t;
typedef struct ols_meta_info ols_meta_info_t;


struct ols_meta {

  OlsMetaFlags       flags;
  const ols_meta_info_t *info;
};


/**
 * GstMetaInitFunction:
 * @meta: a #GstMeta
 * @params: parameters passed to the init function
 * @buffer: a #GstBuffer
 *
 * Function called when @meta is initialized in @buffer.
 */
typedef bool (*ols_meta_init_func) (ols_meta_t *meta, void *params, ols_buffer_t *buffer);

/**
 * GstMetaFreeFunction:
 * @meta: a #GstMeta
 * @buffer: a #GstBuffer
 *
 * Function called when @meta is freed in @buffer.
 */
typedef void (*ols_meta_free_func)     (ols_meta_t *meta, ols_buffer_t *buffer);


/**
 * GstMetaInfo:
 * @api: tag identifying the metadata structure and api
 * @type: type identifying the implementor of the api
 * @size: size of the metadata
 * @init_func: function for initializing the metadata
 * @free_func: function for freeing the metadata
 * @transform_func: function for transforming the metadata
 *
 * The #GstMetaInfo provides information about a specific metadata
 * structure.
 */
struct _GstMetaInfo {

  //GType                      type;
  size_t                      size;

  ols_meta_init_func        init_func;
  ols_meta_free_func        free_func;

  /* No padding needed, GstMetaInfo is always allocated by GStreamer and is
   * not subclassable or stack-allocatable, so we can extend it as we please
   * just like interfaces */
};



#ifdef __cplusplus
}
#endif