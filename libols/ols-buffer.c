#include "ols-buffer.h"

static void _ols_buffer_free(ols_buffer_t *buffer);
static ols_buffer_t *_ols_buffer_copy(const ols_buffer_t *buffer);
static bool _ols_buffer_dispose(ols_buffer_t *buffer);

static void ols_buffer_init(ols_buffer_t *buffer) {
  ols_mini_object_init(OLS_MINI_OBJECT_CAST(buffer), 0, 1,
                       (ols_mini_object_copy_function)_ols_buffer_copy,
                       (ols_mini_object_dispose_function)_ols_buffer_dispose,
                       (ols_mini_object_free_function)_ols_buffer_free);

  // OLS_BUFFER(buffer)->pool = NULL;
  // S_BUFFER_MEM_LEN(buffer) = 0;
  dstr_init(&buffer->data);
}

static void _ols_buffer_free(ols_buffer_t *buffer) {

  // uint32_t i, len;

  // g_return_if_fail(buffer != NULL);

  // GST_CAT_LOG(GST_CAT_BUFFER, "finalize %p", buffer);

  /* free our memory */
  // len = GST_BUFFER_MEM_LEN(buffer);
  // for (i = 0; i < len; i++) {
  //   gst_memory_unlock(GST_BUFFER_MEM_PTR(buffer, i),
  //   GST_LOCK_FLAG_EXCLUSIVE); gst_mini_object_remove_parent(
  //       GST_MINI_OBJECT_CAST(GST_BUFFER_MEM_PTR(buffer, i)),
  //       GST_MINI_OBJECT_CAST(buffer));
  //   gst_memory_unref(GST_BUFFER_MEM_PTR(buffer, i));
  // }

  // /* free metadata */
  // for (walk = GST_BUFFER_META(buffer); walk; walk = next) {
  //   GstMeta *meta = &walk->meta;
  //   const GstMetaInfo *info = meta->info;

  //   /* call free_func if any */
  //   if (info->free_func)
  //     info->free_func(meta, buffer);

  //   next = walk->next;
  //   /* and free the slice */
  //   g_free(walk);
  // }

#ifdef USE_POISONING
  memset(buffer, 0xff, sizeof(GstBufferImpl));
#endif
  dstr_free(&buffer->data);
  bfree(buffer);
}

/**
 * ols_buffer_new:
 *
 * Creates a newly allocated buffer without any data.
 *
 * Returns: (transfer full): the new #OlsBuffer.
 */
ols_buffer_t *ols_buffer_new() {

  ols_buffer_t *newbuf = (ols_buffer_t *)bmalloc(sizeof(ols_buffer_t));
  // OLS_CAT_LOG(OLS_CAT_BUFFER, "new %p", newbuf);

  ols_buffer_init(newbuf);

  return OLS_BUFFER_CAST(newbuf);
}

/* the default dispose function revives the buffer and returns it to the
 * pool when there is a pool */
static bool _ols_buffer_dispose(ols_buffer_t *buffer) {

  UNUSED_PARAMETER(buffer);
  /* keep the buffer alive */
  // gst_buffer_ref(buffer);
  // /* return the buffer to the pool */
  // GST_CAT_LOG(GST_CAT_BUFFER, "release %p to pool %p", buffer, pool);
  // gst_buffer_pool_release_buffer(pool, buffer);

  return true;
}

static ols_buffer_t *ols_buffer_copy_with_flags(const ols_buffer_t *buffer,
                                                OlsBufferCopyFlags flags) {
  UNUSED_PARAMETER(flags);
  ols_buffer_t *copy;

  // g_return_val_if_fail (buffer != NULL, NULL);

  /* create a fresh new buffer */
  copy = ols_buffer_new();

  dstr_copy_dstr(&copy->data, &buffer->data);

  return copy;
}

static ols_buffer_t *_ols_buffer_copy(const ols_buffer_t *buffer) {
  return ols_buffer_copy_with_flags(buffer, OLS_BUFFER_COPY_MEMORY);
}

/**
 * ols_buffer_copy_deep:
 * @buf: a #OlsBuffer.
 *
 * Creates a copy of the given buffer. This will make a newly allocated
 * copy of the data the source buffer contains.
 *
 * Returns: (transfer full) (nullable): a new copy of @buf if the copy
 * succeeded, %NULL otherwise.
 */
ols_buffer_t *ols_buffer_copy_deep(const ols_buffer_t *buffer) {
  return ols_buffer_copy_with_flags(buffer, OLS_BUFFER_COPY_DEEP);
}
