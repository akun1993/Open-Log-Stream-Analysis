#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
    extern "C" {
#endif

struct ols_mini_object;
typedef struct ols_mini_object ols_mini_object_t;

#define OLS_MINI_OBJECT_CAST(obj) ((ols_mini_object_t *)(obj))
#define OLS_MINI_OBJECT_CONST_CAST(obj) ((const ols_mini_object_t *)(obj))
#define OLS_MINI_OBJECT(obj) (OLS_MINI_OBJECT_CONST_CAST(obj))

typedef uint32_t miniType;

/**
 * OLS_MINI_OBJECT_FLAGS:
 * @obj: ols_mini_object_t to return flags for.
 *
 * This macro returns the entire set of flags for the mini-object.
 */
#define OLS_MINI_OBJECT_FLAGS(obj) (OLS_MINI_OBJECT_CAST(obj)->flags)
/**
 * OLS_MINI_OBJECT_FLAG_IS_SET:
 * @obj: ols_mini_object_t to check for flags.
 * @flag: Flag to check for
 *
 * This macro checks to see if the given flag is set.
 */
#define OLS_MINI_OBJECT_FLAG_IS_SET(obj, flag)                                 \
  !!(OLS_MINI_OBJECT_FLAGS(obj) & (flag))
/**
 * OLS_MINI_OBJECT_FLAG_SET:
 * @obj: MiniObject to set flag in.
 * @flag: Flag to set, can by any number of bits in uint32_t.
 *
 * This macro sets the given bits.
 */
#define OLS_MINI_OBJECT_FLAG_SET(obj, flag)                                    \
  (OLS_MINI_OBJECT_FLAGS(obj) |= (flag))
/**
 * OLS_MINI_OBJECT_FLAG_UNSET:
 * @obj: ols_mini_object_t to unset flag in.
 * @flag: Flag to set, must be a single bit in uint32_t.
 *
 * This macro unsets the given bits.
 */
#define OLS_MINI_OBJECT_FLAG_UNSET(obj, flag)                                  \
  (OLS_MINI_OBJECT_FLAGS(obj) &= ~(flag))


  /**
 * OLS_MINI_OBJECT_REFCOUNT:
 * @obj: a #ols_mini_object_t
 *
 * Get access to the reference count field of the mini-object.
 */
#define OLS_MINI_OBJECT_REFCOUNT(obj)           ((OLS_MINI_OBJECT_CAST(obj))->refcount)
/**
 * OLS_MINI_OBJECT_REFCOUNT_VALUE:
 * @obj: a #ols_mini_object_t
 *
 * Get the reference count value of the mini-object.
 */
#define OLS_MINI_OBJECT_REFCOUNT_VALUE(obj)     (os_atomic_load_long (&(OLS_MINI_OBJECT_CAST(obj))->refcount))


/**
 * OlsMiniObjectFlags:
 * @OLS_MINI_OBJECT_FLAG_LOCKABLE: the object can be locked and unlocked with
 * ols_mini_object_lock() and ols_mini_object_unlock().
 * @OLS_MINI_OBJECT_FLAG_LOCK_READONLY: the object is permanently locked in
 * READONLY mode. Only read locks can be performed on the object.
 * @OLS_MINI_OBJECT_FLAG_MAY_BE_LEAKED: the object is expected to stay alive
 * even after ols_deinit() has been called and so should be ignored by leak
 * detection tools. (Since: 1.10)
 * @OLS_MINI_OBJECT_FLAG_LAST: first flag that can be used by subclasses.
 *
 * Flags for the mini object
 */
typedef enum {
  OLS_MINI_OBJECT_FLAG_LOCKABLE = (1 << 0),
  OLS_MINI_OBJECT_FLAG_LOCK_READONLY = (1 << 1),
  OLS_MINI_OBJECT_FLAG_MAY_BE_LEAKED = (1 << 2),
  /* padding */
  OLS_MINI_OBJECT_FLAG_LAST = (1 << 4)
} OlsMiniObjectFlags;

/**
 * OlsLockFlags:
 * @OLS_LOCK_FLAG_READ: lock for read access
 * @OLS_LOCK_FLAG_WRITE: lock for write access
 * @OLS_LOCK_FLAG_EXCLUSIVE: lock for exclusive access
 * @OLS_LOCK_FLAG_LAST: first flag that can be used for custom purposes
 *
 * Flags used when locking miniobjects
 */
typedef enum {
  OLS_LOCK_FLAG_READ = (1 << 0),
  OLS_LOCK_FLAG_WRITE = (1 << 1),
  OLS_LOCK_FLAG_EXCLUSIVE = (1 << 2),
  OLS_LOCK_FLAG_LAST = (1 << 8)
} OlsLockFlags;
/**
 * ols_mini_object_copy_function:
 * @obj: MiniObject to copy
 *
 * Function prototype for methods to create copies of instances.
 *
 * Returns: reference to cloned instance.
 */
typedef ols_mini_object_t *(*ols_mini_object_copy_function)(
    const ols_mini_object_t *obj);
/**
 * ols_mini_object_dispose_function:
 * @obj: MiniObject to dispose
 *
 * Function prototype for when a miniobject has lost its last refcount.
 * Implementation of the mini object are allowed to revive the
 * passed object by doing a ols_mini_object_ref(). If the object is not
 * revived after the dispose function, the function should return %TRUE
 * and the memory associated with the object is freed.
 *
 * Returns: %TRUE if the object should be cleaned up.
 */
typedef bool (*ols_mini_object_dispose_function)(ols_mini_object_t *obj);
/**
 * ols_mini_object_free_function:
 * @obj: MiniObject to free
 *
 * Virtual function prototype for methods to free resources used by
 * mini-objects.
 */
typedef void (*ols_mini_object_free_function)(ols_mini_object_t *obj);

void ols_mini_object_init(ols_mini_object_t *mini_object, uint32_t flags,
                          miniType type,
                          ols_mini_object_copy_function copy_func,
                          ols_mini_object_dispose_function dispose_func,
                          ols_mini_object_free_function free_func);

ols_mini_object_t *ols_mini_object_ref(ols_mini_object_t *mini_object);
void ols_mini_object_unref(ols_mini_object_t *mini_object);


// void  ols_mini_object_weak_ref (ols_mini_object_t *object, void * data);
// void  ols_mini_object_weak_unref	(ols_mini_object_t *object,void * data);

/* locking */

bool   ols_mini_object_lock            (ols_mini_object_t *object, OlsLockFlags flags);

void  ols_mini_object_unlock          (ols_mini_object_t *object, OlsLockFlags flags);


/* copy */


ols_mini_object_t * ols_mini_object_copy(const ols_mini_object_t *mini_object) ;//G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT



// void            gst_mini_object_set_qdata       (GstMiniObject *object, GQuark quark,
//                                                  gpointer data, GDestroyNotify destroy);

// gpointer        gst_mini_object_get_qdata       (GstMiniObject *object, GQuark quark);


// gpointer        gst_mini_object_steal_qdata     (GstMiniObject *object, GQuark quark);


// bool        ols_mini_object_replace         (ols_mini_object_t **olddata, ols_mini_object_t *newdata);


// bool        ols_mini_object_take            (ols_mini_object_t **olddata, ols_mini_object_t *newdata);

// ols_mini_object_t * ols_mini_object_steal   (ols_mini_object_t **olddata) ;//G_GNUC_WARN_UNUSED_RESULT


struct ols_mini_object {
  miniType type;

  /*< public >*/ /* with COW */
  volatile long refcount;
  volatile long lockstate;
  uint32_t flags;

  ols_mini_object_copy_function copy;
  ols_mini_object_dispose_function dispose;
  ols_mini_object_free_function free;

  /* < private > */
  /* Used to keep track of parents, weak ref notifies and qdata */
  // uint32_t priv_uint;
  // void *priv_pointer;
};

#ifdef __cplusplus
}
#endif
