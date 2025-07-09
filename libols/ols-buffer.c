#include "ols-buffer.h"
#include "ols-meta.h"

static void _ols_buffer_free(ols_buffer_t *buffer);
static ols_buffer_t *_ols_buffer_copy(const ols_buffer_t *buffer);
static bool _ols_buffer_dispose(ols_buffer_t *buffer);

static void ols_buffer_init(ols_buffer_t *buffer) {
  ols_mini_object_init(OLS_MINI_OBJECT_CAST(buffer), 0, 1,
                       (ols_mini_object_copy_function)_ols_buffer_copy,
                       (ols_mini_object_dispose_function)_ols_buffer_dispose,
                       (ols_mini_object_free_function)_ols_buffer_free);

}

static void _ols_buffer_free(ols_buffer_t *buffer) {

#ifdef USE_POISONING
  memset(buffer, 0xff, sizeof(GstBufferImpl));
#endif
  if(buffer->meta)
    ols_meta_unref(buffer->meta);
  
  if(buffer->result)
    ols_meta_result_unref(buffer->result);

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

  ols_buffer_t *newbuf = (ols_buffer_t *)bzalloc(sizeof(ols_buffer_t));
  // OLS_CAT_LOG(OLS_CAT_BUFFER, "new %p", newbuf);

  ols_buffer_init(newbuf);

  return OLS_BUFFER_CAST(newbuf);
}

/* the default dispose function revives the buffer and returns it to the
 * pool when there is a pool */
static bool _ols_buffer_dispose(ols_buffer_t *buffer) {

  UNUSED_PARAMETER(buffer);
  /* keep the buffer alive */
  // /* return the buffer to the pool */
  // GST_CAT_LOG(GST_CAT_BUFFER, "release %p to pool %p", buffer, pool);


  return true;
}

static ols_buffer_t *ols_buffer_copy_with_flags(const ols_buffer_t *buffer,
                                                OlsBufferCopyFlags flags) {
  UNUSED_PARAMETER(flags);
  ols_buffer_t *copy;

  //return_val_if_fail (buffer != NULL, NULL);

  /* create a fresh new buffer */
  copy = ols_buffer_new();
  
  if(buffer->meta){
    ols_meta_ref(buffer->meta);
    copy->meta = buffer->meta;
  }

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

void ols_buffer_set_meta(ols_buffer_t *buf,  struct ols_meta *meta){

  buf->meta = meta;
}


