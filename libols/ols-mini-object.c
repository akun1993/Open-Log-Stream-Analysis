#include "ols-mini-object.h"
#include "util/threading.h"

#define SHARE_ONE (1 << 16)
#define SHARE_TWO (2 << 16)
#define SHARE_MASK (~(SHARE_ONE - 1))
#define IS_SHARED(state) (state >= SHARE_TWO)
#define LOCK_ONE (OLS_LOCK_FLAG_LAST)
#define FLAG_MASK (OLS_LOCK_FLAG_LAST - 1)
#define LOCK_MASK ((SHARE_ONE - 1) - FLAG_MASK)
#define LOCK_FLAG_MASK (SHARE_ONE - 1)

enum {
  PRIV_DATA_STATE_LOCKED = 0,
  PRIV_DATA_STATE_NO_PARENT = 1,
  PRIV_DATA_STATE_ONE_PARENT = 2,
  PRIV_DATA_STATE_PARENTS_OR_QDATA = 3,
};

/**
 * ols_mini_object_init: (skip)
 * @mini_object: a #OlsMiniObject
 * @flags: initial #OlsMiniObjectFlags
 * @type: the #GType of the mini-object to create
 * @copy_func: (allow-none): the copy function, or %NULL
 * @dispose_func: (allow-none): the dispose function, or %NULL
 * @free_func: (allow-none): the free function or %NULL
 *
 * Initializes a mini-object with the desired type and copy/dispose/free
 * functions.
 */
void ols_mini_object_init(ols_mini_object_t *mini_object, uint32_t flags,
                          miniType type,
                          ols_mini_object_copy_function copy_func,
                          ols_mini_object_dispose_function dispose_func,
                          ols_mini_object_free_function free_func) {
  mini_object->type = type;
  mini_object->refcount = 1;
  mini_object->lockstate = 0;
  mini_object->flags = flags;

  mini_object->copy = copy_func;
  mini_object->dispose = dispose_func;
  mini_object->free = free_func;

  //   g_atomic_int_set((int32_t *)&mini_object->priv_uint,
  //                    PRIV_DATA_STATE_NO_PARENT);
  //   mini_object->priv_pointer = NULL;

  // OLS_TRACER_MINI_OBJECT_CREATED(mini_object);
}

/**
 * ols_mini_object_copy: (skip)
 * @mini_object: the mini-object to copy
 *
 * Creates a copy of the mini-object.
 *
 * MT safe
 *
 * Returns: (transfer full) (nullable): the new mini-object if copying is
 * possible, %NULL otherwise.
 */
ols_mini_object_t *ols_mini_object_copy(const ols_mini_object_t *mini_object) {
  ols_mini_object_t *copy;

  // g_return_val_if_fail(mini_object != NULL, NULL);

  if (mini_object->copy)
    copy = mini_object->copy(mini_object);
  else
    copy = NULL;

  return copy;
}

/**
 * ols_mini_object_lock:
 * @object: the mini-object to lock
 * @flags: #OlsLockFlags
 *
 * Lock the mini-object with the specified access mode in @flags.
 *
 * Returns: %TRUE if @object could be locked.
 */
bool ols_mini_object_lock(ols_mini_object_t *object, OlsLockFlags flags) {
  uint32_t access_mode;
  long state, newstate;

  //   g_return_val_if_fail(object != NULL, false);
  //   g_return_val_if_fail(OLS_MINI_OBJECT_IS_LOCKABLE(object), false);

  if (object->flags & OLS_MINI_OBJECT_FLAG_LOCK_READONLY &&
      flags & OLS_LOCK_FLAG_WRITE)
    return false;

  do {
    access_mode = flags & FLAG_MASK;
    newstate = state = (uint32_t)os_atomic_load_long(&object->lockstate);

    // OLS_CAT_TRACE(OLS_CAT_LOCKING, "lock %p: state %08x, access_mode %u",
    //               object, state, access_mode);

    if (access_mode & OLS_LOCK_FLAG_EXCLUSIVE) {
      /* shared ref */
      newstate += SHARE_ONE;
      access_mode &= ~OLS_LOCK_FLAG_EXCLUSIVE;
    }

    /* shared counter > 1 and write access is not allowed */
    if (((state & OLS_LOCK_FLAG_WRITE) != 0 ||
         (access_mode & OLS_LOCK_FLAG_WRITE) != 0) &&
        IS_SHARED(newstate))
      goto lock_failed;

    if (access_mode) {
      if ((state & LOCK_FLAG_MASK) == 0) {
        /* nothing mapped, set access_mode */
        newstate |= access_mode;
      } else {
        /* access_mode must match */
        if ((state & access_mode) != access_mode)
          goto lock_failed;
      }
      /* increase refcount */
      newstate += LOCK_ONE;
    }
  } while (
      !os_atomic_compare_exchange_long(&object->lockstate, &state, newstate));

  return true;

lock_failed : {
  //   OLS_CAT_DEBUG(OLS_CAT_LOCKING, "lock failed %p: state %08x, access_mode
  //   %u",
  //                 object, state, access_mode);
  return false;
}
}
// static inline bool os_atomic_compare_exchange_long(volatile long *val,
// 						   long *old_ptr, long new_val)
/**
 * ols_mini_object_unlock:
 * @object: the mini-object to unlock
 * @flags: #OlsLockFlags
 *
 * Unlock the mini-object with the specified access mode in @flags.
 */
void ols_mini_object_unlock(ols_mini_object_t *object, OlsLockFlags flags) {
  uint32_t access_mode;
  long state, newstate;

  // g_return_if_fail(object != NULL);
  // g_return_if_fail(OLS_MINI_OBJECT_IS_LOCKABLE(object));

  do {
    access_mode = flags & FLAG_MASK;
    newstate = state = (uint32_t)os_atomic_load_long(&object->lockstate);

    // OLS_CAT_TRACE(OLS_CAT_LOCKING, "unlock %p: state %08x, access_mode %u",
    //               object, state, access_mode);

    if (access_mode & OLS_LOCK_FLAG_EXCLUSIVE) {
      /* shared counter */
      // g_return_if_fail(state >= SHARE_ONE);
      newstate -= SHARE_ONE;
      access_mode &= ~OLS_LOCK_FLAG_EXCLUSIVE;
    }

    if (access_mode) {
      // g_return_if_fail((state & access_mode) == access_mode);
      /* decrease the refcount */
      newstate -= LOCK_ONE;
      /* last refcount, unset access_mode */
      if ((newstate & LOCK_FLAG_MASK) == access_mode)
        newstate &= ~LOCK_FLAG_MASK;
    }
  } while (
      !os_atomic_compare_exchange_long(&object->lockstate, &state, newstate));
}

/**
 * ols_mini_object_ref: (skip)
 * @mini_object: the mini-object
 *
 * Increase the reference count of the mini-object.
 *
 * Note that the refcount affects the writability
 * of @mini-object, see ols_mini_object_is_writable(). It is
 * important to note that keeping additional references to
 * OlsMiniObject instances can potentially increase the number
 * of memcpy operations in a pipeline, especially if the miniobject
 * is a #OlsBuffer.
 *
 * Returns: (transfer full): the mini-object.
 */
ols_mini_object_t *ols_mini_object_ref(ols_mini_object_t *mini_object) {
  int32_t old_refcount, new_refcount;

  // g_return_val_if_fail(mini_object != NULL, NULL);
  /* we can't assert that the refcount > 0 since the _free functions
   * increments the refcount from 0 to 1 again to allow resurrecting
   * the object
   g_return_val_if_fail (mini_object->refcount > 0, NULL);
   */

  old_refcount = os_atomic_inc_long(&mini_object->refcount);
  new_refcount = old_refcount + 1;

  //   OLS_CAT_TRACE(OLS_CAT_REFCOUNTING, "%p ref %d->%d", mini_object,
  //   old_refcount,
  //                 new_refcount);

  //   OLS_TRACER_MINI_OBJECT_REFFED(mini_object, new_refcount);

  return mini_object;
}

static void free_priv_data(ols_mini_object_t *obj) {
  UNUSED_PARAMETER(obj);
  //   uint32_t i;
  //   int32_t priv_state = g_atomic_int_get((int32_t *)&obj->priv_uint);
  //   PrivData *priv_data;

  //   if (priv_state != PRIV_DATA_STATE_PARENTS_OR_QDATA) {
  //     if (priv_state == PRIV_DATA_STATE_LOCKED) {
  //       g_warning("%s: object finalizing but has locked private data
  //       (object:%p)",
  //                 G_STRFUNC, obj);
  //     } else if (priv_state == PRIV_DATA_STATE_ONE_PARENT) {
  //       g_warning(
  //           "%s: object finalizing but still has parent (object:%p,
  //           parent:%p)", G_STRFUNC, obj, obj->priv_pointer);
  //     }

  //     return;
  //   }

  //   priv_data = obj->priv_pointer;

  //   for (i = 0; i < priv_data->n_qdata; i++) {
  //     if (QDATA_QUARK(priv_data, i) == weak_ref_quark)
  //       QDATA_NOTIFY(priv_data, i)(QDATA_DATA(priv_data, i), obj);
  //     if (QDATA_DESTROY(priv_data, i))
  //       QDATA_DESTROY(priv_data, i)(QDATA_DATA(priv_data, i));
  //   }
  //   g_free(priv_data->qdata);

  //   if (priv_data->n_parents)
  //     g_warning("%s: object finalizing but still has %d parents (object:%p)",
  //               G_STRFUNC, priv_data->n_parents, obj);
  //   g_free(priv_data->parents);

  //   g_free(priv_data);
}

/**
 * ols_mini_object_unref: (skip)
 * @mini_object: the mini-object
 *
 * Decreases the reference count of the mini-object, possibly freeing
 * the mini-object.
 */
void ols_mini_object_unref(ols_mini_object_t *mini_object) {
  int32_t old_refcount, new_refcount;

  // g_return_if_fail(mini_object != NULL);
  // g_return_if_fail(OLS_MINI_OBJECT_REFCOUNT_VALUE(mini_object) > 0);

  old_refcount = os_atomic_dec_long(&mini_object->refcount);
  new_refcount = old_refcount - 1;

  // g_return_if_fail(old_refcount > 0);

  //   OLS_CAT_TRACE(OLS_CAT_REFCOUNTING, "%p unref %d->%d", mini_object,
  //                 old_refcount, new_refcount);

  //  OLS_TRACER_MINI_OBJECT_UNREFFED(mini_object, new_refcount);

  if (new_refcount == 0) {
    bool do_free;

    if (mini_object->dispose)
      do_free = mini_object->dispose(mini_object);
    else
      do_free = true;

    /* if the subclass recycled the object (and returned FALSE) we don't
     * want to free the instance anymore */
    if (do_free) {
      /* there should be no outstanding locks */
      if ((os_atomic_load_long(&mini_object->lockstate) & LOCK_MASK) < 4) {
        return;
      }

      free_priv_data(mini_object);

      // OLS_TRACER_MINI_OBJECT_DESTROYED(mini_object);
      if (mini_object->free)
        mini_object->free(mini_object);
    }
  }
}

/**
 * ols_mini_object_replace:
 * @olddata: (inout) (transfer full) (nullable): pointer to a pointer to a
 *     mini-object to be replaced
 * @newdata: (allow-none): pointer to new mini-object
 *
 * Atomically modifies a pointer to point to a new mini-object.
 * The reference count of @olddata is decreased and the reference count of
 * @newdata is increased.
 *
 * Either @newdata and the value pointed to by @olddata may be %NULL.
 *
 * Returns: %TRUE if @newdata was different from @olddata
 */
// bool ols_mini_object_replace (ols_mini_object_t ** olddata, ols_mini_object_t * newdata)
// {
//   ols_mini_object_t *olddata_val;

//   g_return_val_if_fail (olddata != NULL, false);

//   blog ( "replace %p (%d) with %p (%d)",
//       *olddata, *olddata ? (*olddata)->refcount : 0,
//       newdata, newdata ? newdata->refcount : 0);

//   olddata_val = g_atomic_pointer_get ((void *) olddata);

//   if (UNLIKELY (olddata_val == newdata))
//     return false;

//   if (newdata)
//     ols_mini_object_ref (newdata);

//   while (UNLIKELY (!g_atomic_pointer_compare_and_exchange ((void *) olddata, olddata_val, newdata))) {
//     olddata_val = g_atomic_pointer_get ((void *) olddata);
//     if (UNLIKELY (olddata_val == newdata))
//       break;
//   }

//   if (olddata_val)
//     ols_mini_object_unref (olddata_val);

//   return olddata_val != newdata;
// }

/**
 * gst_mini_object_steal: (skip)
 * @olddata: (inout) (transfer full): pointer to a pointer to a mini-object to
 *     be stolen
 *
 * Replace the current #GstMiniObject pointer to by @olddata with %NULL and
 * return the old value.
 *
 * Returns: (nullable): the #GstMiniObject at @oldata
 */
// ols_mini_object_t *
// ols_mini_object_steal (ols_mini_object_t ** olddata)
// {
//   ols_mini_object_t *olddata_val;

//   g_return_val_if_fail (olddata != NULL, NULL);

//   blog ( "steal %p (%d)",
//       *olddata, *olddata ? (*olddata)->refcount : 0);

//   do {
//     olddata_val = g_atomic_pointer_get ((void *) olddata);
//     if (olddata_val == NULL)
//       break;
//   } while (UNLIKELY (!g_atomic_pointer_compare_and_exchange ((void *)
//               olddata, olddata_val, NULL)));

//   return olddata_val;
// }

/**
 * gst_mini_object_take:
 * @olddata: (inout) (transfer full): pointer to a pointer to a mini-object to
 *     be replaced
 * @newdata: pointer to new mini-object
 *
 * Modifies a pointer to point to a new mini-object. The modification
 * is done atomically. This version is similar to gst_mini_object_replace()
 * except that it does not increase the refcount of @newdata and thus
 * takes ownership of @newdata.
 *
 * Either @newdata and the value pointed to by @olddata may be %NULL.
 *
 * Returns: %TRUE if @newdata was different from @olddata
 */
// bool
// ols_mini_object_take (ols_mini_object_t ** olddata, ols_mini_object_t * newdata)
// {
//   ols_mini_object_t *olddata_val;

//   g_return_val_if_fail (olddata != NULL, false);

//   blog ( "take %p (%d) with %p (%d)",
//       *olddata, *olddata ? (*olddata)->refcount : 0,
//       newdata, newdata ? newdata->refcount : 0);

//   do {
//     olddata_val = g_atomic_pointer_get ((void *) olddata);
//     if (UNLIKELY (olddata_val == newdata))
//       break;
//   } while (UNLIKELY (!g_atomic_pointer_compare_and_exchange ((void *)
//               olddata, olddata_val, newdata)));

//   if (olddata_val)
//     ols_mini_object_unref (olddata_val);

//   return olddata_val != newdata;
// }

