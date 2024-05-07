#pragma once
#include <cstdint>

typedef struct _ols_mini_object ols_mini_object;

/**
 * GstMiniObjectCopyFunction:
 * @obj: MiniObject to copy
 *
 * Function prototype for methods to create copies of instances.
 *
 * Returns: reference to cloned instance.
 */
typedef ols_mini_object *(*ols_mini_object_copy_function)(
    const ols_mini_object *obj);
/**
 * GstMiniObjectDisposeFunction:
 * @obj: MiniObject to dispose
 *
 * Function prototype for when a miniobject has lost its last refcount.
 * Implementation of the mini object are allowed to revive the
 * passed object by doing a gst_mini_object_ref(). If the object is not
 * revived after the dispose function, the function should return %TRUE
 * and the memory associated with the object is freed.
 *
 * Returns: %TRUE if the object should be cleaned up.
 */
typedef bool (*ols_mini_object_dispose_function)(ols_mini_object *obj);
/**
 * GstMiniObjectFreeFunction:
 * @obj: MiniObject to free
 *
 * Virtual function prototype for methods to free resources used by
 * mini-objects.
 */
typedef void (*ols_mini_object_free_function)(ols_mini_object *obj);

struct _ols_mini_object {
  // GType   type;

  /*< public >*/ /* with COW */
  int32_t refcount;
  int32_t lockstate;
  uint32_t flags;

  ols_mini_object_copy_function copy;
  ols_mini_object_dispose_function dispose;
  ols_mini_object_free_function free;

  /* < private > */
  /* Used to keep track of parents, weak ref notifies and qdata */
  uint32_t priv_uint;
  void *priv_pointer;
};
