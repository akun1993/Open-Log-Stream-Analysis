#pragma once

#include <util/threading.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* ols shared context data */

struct ols_weak_ref {
    volatile long refs;
    volatile long weak_refs;
  };
  
  
  /* ------------------------------------------------------------------------- */
  /* ref-counting  */
  
  static inline void ols_ref_addref(struct ols_weak_ref *ref) {
    os_atomic_inc_long(&ref->refs);
  }
  
  static inline bool ols_ref_release(struct ols_weak_ref *ref) {
    return os_atomic_dec_long(&ref->refs) == -1;
  }
  
  static inline void ols_weak_ref_addref(struct ols_weak_ref *ref) {
    os_atomic_inc_long(&ref->weak_refs);
  }
  
  static inline bool ols_weak_ref_release(struct ols_weak_ref *ref) {
    return os_atomic_dec_long(&ref->weak_refs) == -1;
  }
  
  static inline bool ols_weak_ref_get_ref(struct ols_weak_ref *ref) {
    long owners = os_atomic_load_long(&ref->refs);
    while (owners > -1) {
      if (os_atomic_compare_exchange_long(&ref->refs, &owners, owners + 1)) {
        return true;
      }
    }
    return false;
  }
  
  static inline bool ols_weak_ref_expired(struct ols_weak_ref *ref) {
    long owners = os_atomic_load_long(&ref->refs);
    return owners < 0;
  }

#ifdef __cplusplus
}
#endif  
  