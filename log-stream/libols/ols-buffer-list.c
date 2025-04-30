#include "ols-buffer-list.h"

/**
 * ols_buffer_list_length:
 * @list: a #ols_buffer_list_t
 *
 * Returns the number of buffers in @list.
 *
 * Returns: the number of buffers in the buffer list
 */
uint32_t ols_buffer_list_length(ols_buffer_list_t *list) {
  // g_return_val_if_fail(OLS_IS_BUFFER_LIST(list), 0);

  return list->n_buffers;
}

/**
 * ols_buffer_list_get:
 * @list: a #ols_buffer_list_t
 * @idx: the index
 *
 * Gets the buffer at @idx.
 *
 * You must make sure that @idx does not exceed the number of
 * buffers available.
 *
 * Returns: (transfer none) (nullable): the buffer at @idx in @group
 *     or %NULL when there is no buffer. The buffer remains valid as
 *     long as @list is valid and buffer is not removed from the list.
 */
ols_buffer_t *ols_buffer_list_get(ols_buffer_list_t *list, uint32_t idx) {
  //   g_return_val_if_fail(OLS_IS_BUFFER_LIST(list), NULL);
  //   g_return_val_if_fail(idx < list->n_buffers, NULL);

  return list->buffers[idx];
}