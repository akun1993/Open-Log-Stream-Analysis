#pragma once


#include "callback/proc.h"
#include "callback/signal.h"
#include "ols-ref.h"
#include "ols-data.h"
#include "ols-pad.h"
#include "util/c99defs.h"
#include "util/darray.h"
#include "util/uthash.h"


#ifdef __cplusplus
extern "C" {
#endif



struct ols_weak_object {
    struct ols_weak_ref ref;
    struct ols_context_data *object;
};
  
typedef void (*ols_destroy_cb)(void *obj);

struct ols_context_data {
    char *name;
    const char *uuid;
    void *data;
    pthread_mutex_t mutex;
    ols_data_t *settings;
    signal_handler_t *signals;
    proc_handler_t *procs;
    enum ols_obj_type type;

    struct ols_weak_object *control;
    ols_destroy_cb destroy;

    /* element pads, these lists can only be iterated while holding
        * the LOCK or checking the cookie after each LOCK. */
    uint16_t numpads;
    DARRAY(ols_pad_t *)
    pads;
    uint16_t numsrcpads;
    DARRAY(ols_pad_t *)
    srcpads;
    uint16_t numsinkpads;
    DARRAY(ols_pad_t *)
    sinkpads;

    // for src link
    pthread_mutex_t *hh_mutex;
    UT_hash_handle hh;
    UT_hash_handle hh_uuid;
};

EXPORT bool ols_context_data_init(struct ols_context_data *context,
                                enum ols_obj_type type, ols_data_t *settings,
                                const char *name, const char *uuid);
                                
EXPORT void ols_context_init_control(struct ols_context_data *context,
                                    void *object, ols_destroy_cb destroy);
EXPORT void ols_context_data_free(struct ols_context_data *context);

EXPORT void ols_context_data_insert_name(struct ols_context_data *context,
                                        pthread_mutex_t *mutex, void *first);

EXPORT void ols_context_data_insert_uuid(struct ols_context_data *context,
                                        pthread_mutex_t *mutex,
                                        void *first_uuid);

EXPORT void ols_context_data_remove(struct ols_context_data *context);
EXPORT void ols_context_data_remove_name(struct ols_context_data *context,
                                        void *phead);
EXPORT void ols_context_data_remove_uuid(struct ols_context_data *context,
                                        void *puuid_head);

EXPORT void ols_context_wait(struct ols_context_data *context);

EXPORT void ols_context_data_setname_ht(struct ols_context_data *context,
                                        const char *name, void *phead);

EXPORT bool ols_context_add_pad(struct ols_context_data *context,
                                struct ols_pad *pad);

EXPORT bool ols_context_remove_pad(struct ols_context_data *context,
                                    struct ols_pad *pad);

EXPORT bool ols_context_link(struct ols_context_data *src,
                            struct ols_context_data *dest);

EXPORT void ols_context_unlink(struct ols_context_data *src,
                                struct ols_context_data *dest);

EXPORT bool ols_context_link_pads(struct ols_context_data *src,
                                const char *srcpadname,
                                struct ols_context_data *dest,
                                const char *destpadname);

EXPORT void ols_context_unlink_pads(struct ols_context_data *src,
                                    const char *srcpadname,
                                    struct ols_context_data *dest,
                                    const char *destpadname);
#ifdef __cplusplus
}
#endif
