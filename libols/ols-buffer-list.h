

#pragma once
#include "ols-buffer.h"
#include "ols-mini-object.h"

struct ols_buffer_list;

typedef struct ols_buffer_list ols_buffer_list_t;

#define OLS_BUFFER_LIST_CAST(obj) ((ols_buffer_list_t *)obj)
#define OLS_BUFFER_LIST(obj) (OLS_BUFFER_LIST_CAST(obj))

/**
 * OlsBufferListFunc:
 * @buffer: (out) (nullable): pointer to the buffer
 * @idx: the index of @buffer
 * @user_data: user data passed to ols_buffer_list_foreach()
 *
 * A function that will be called from ols_buffer_list_foreach(). The @buffer
 * field will point to a the reference of the buffer at @idx.
 *
 * When this function returns %TRUE, the next buffer will be
 * returned. When %FALSE is returned, ols_buffer_list_foreach() will return.
 *
 * When @buffer is set to %NULL, the item will be removed from the bufferlist.
 * When @buffer has been made writable, the new buffer reference can be assigned
 * to @buffer. This function is responsible for unreffing the old buffer when
 * removing or modifying.
 *
 * Returns: %FALSE when ols_buffer_list_foreach() should stop
 */
typedef bool (*OlsBufferListFunc)(ols_buffer_t **buffer, uint32_t idx,
                                  void *user_data);

#ifndef OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS
/* refcounting */
static inline ols_buffer_list_t *ols_buffer_list_ref(ols_buffer_list_t *list) {
  return OLS_BUFFER_LIST_CAST(ols_mini_object_ref(OLS_MINI_OBJECT_CAST(list)));
}

static inline void ols_buffer_list_unref(ols_buffer_list_t *list) {
  ols_mini_object_unref(OLS_MINI_OBJECT_CAST(list));
}

static inline void ols_clear_buffer_list(ols_buffer_list_t **list_ptr) {
  ols_clear_mini_object((ols_mini_object_t **)list_ptr);
}

/* copy */
static inline ols_buffer_list_t *
ols_buffer_list_copy(const ols_buffer_list_t *list) {
  return OLS_BUFFER_LIST_CAST(
      ols_mini_object_copy(OLS_MINI_OBJECT_CONST_CAST(list)));
}

static inline bool ols_buffer_list_replace(ols_buffer_list_t **old_list,
                                           ols_buffer_list_t *new_list) {
  return ols_mini_object_replace((ols_mini_object_t **)old_list,
                                 (ols_mini_object_t *)new_list);
}

static inline bool ols_buffer_list_take(ols_buffer_list_t **old_list,
                                        ols_buffer_list_t *new_list) {
  return ols_mini_object_take((ols_mini_object_t **)old_list,
                              (ols_mini_object_t *)new_list);
}
#else  /* OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS */

ols_buffer_list_t *ols_buffer_list_ref(ols_buffer_list_t *list);

void ols_buffer_list_unref(ols_buffer_list_t *list);

void ols_clear_buffer_list(ols_buffer_list_t **list_ptr);

ols_buffer_list_t *ols_buffer_list_copy(const ols_buffer_list_t *list);

bool ols_buffer_list_replace(ols_buffer_list_t **old_list,
                             ols_buffer_list_t *new_list);

bool ols_buffer_list_take(ols_buffer_list_t **old_list,
                          ols_buffer_list_t *new_list);
#endif /* OLS_DISABLE_MINIOBJECT_INLINE_FUNCTIONS */

/**
 * ols_buffer_list:
 *
 * Opaque list of grouped buffers.
 */
struct ols_buffer_list {
  // OlsMiniObject mini_object;

  ols_buffer_t **buffers;
  uint32_t n_buffers;
  uint32_t n_allocated;

  /* one-item array, in reality more items are pre-allocated
   * as part of the OlsBufferList structure, and that
   * pre-allocated array extends beyond the declared struct */
  ols_buffer_t *arr[1];
};

uint32_t ols_buffer_list_length(ols_buffer_list_t *list);
ols_buffer_t *ols_buffer_list_get(ols_buffer_list_t *list, uint32_t idx);
