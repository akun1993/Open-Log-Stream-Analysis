#include "ols-buffer.h"

static void ols_buffer_init(OlsBufferImpl *buffer) {
  ols_mini_object_init(OLS_MINI_OBJECT_CAST(buffer), 0, _ols_buffer_type,
                       (ols_mini_object_copy_function)_ols_buffer_copy,
                       (ols_mini_object_dispose_function)_ols_buffer_dispose,
                       (ols_mini_object_free_function)_ols_buffer_free);

  // OLS_BUFFER(buffer)->pool = NULL;
  OLS_BUFFER_MEM_LEN(buffer) = 0;
}

/**
 * ols_buffer_new:
 *
 * Creates a newly allocated buffer without any data.
 *
 * Returns: (transfer full): the new #OlsBuffer.
 */
ols_buffer_t *ols_buffer_new(void) {
  OlsBufferImpl *newbuf;

  newbuf = g_new(OlsBufferImpl, 1);
  // OLS_CAT_LOG(OLS_CAT_BUFFER, "new %p", newbuf);

  ols_buffer_init(newbuf);

  return OLS_BUFFER_CAST(newbuf);
}

static ols_buffer_t *ols_buffer_copy_with_flags(const ols_buffer_t *buffer,
                                                OlsBufferCopyFlags flags) {
  ols_buffer_t *copy;

  // g_return_val_if_fail (buffer != NULL, NULL);

  /* create a fresh new buffer */
  copy = ols_buffer_new();

  /* copy what the 'flags' want from our parent */
  /* FIXME why we can't pass const to ols_buffer_copy_into() ? */
  if (!ols_buffer_copy_into(copy, (ols_buffer_t *)buffer, flags, 0, -1))
    ols_buffer_replace(&copy, NULL);

  if (copy)
    OLS_BUFFER_FLAG_UNSET(copy, OLS_BUFFER_FLAG_TAG_MEMORY);

  return copy;
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
 *
 * Since: 1.6
 */
ols_buffer_t *ols_buffer_copy_deep(const ols_buffer_t *buffer) {
  return ols_buffer_copy_with_flags(buffer,
                                    OLS_BUFFER_COPY_ALL | OLS_BUFFER_COPY_DEEP);
}
