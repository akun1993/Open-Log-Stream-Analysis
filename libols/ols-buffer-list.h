

#pragma once
#include "ols-buffer.h"
/**
 * GstBufferList:
 *
 * Opaque list of grouped buffers.
 */
struct ols_buffer_list {
  // GstMiniObject mini_object;

  ols_buffer_t **buffers;
  uint32_t n_buffers;
  uint32_t n_allocated;

  /* one-item array, in reality more items are pre-allocated
   * as part of the GstBufferList structure, and that
   * pre-allocated array extends beyond the declared struct */
  ols_buffer_t *arr[1];
};

typedef struct ols_buffer_list ols_buffer_list_t;