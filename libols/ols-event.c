#include "ols-event.h"
#include "util/bmem.h"
#include "util/debug.h"


static ols_event_t * ols_event_new();

static void ols_event_init (ols_event_t * ols_event);

/* creation/deletion */
static void
_ols_event_free (ols_event_t * ols_event)
{
  /* This can be used to get statistics about meta_txt sizes */

#ifdef DEBUG_REFCOUNT
  blog (LOG_INFO, "freeing ols_event_t %p", ols_event);
#endif

    bfree(ols_event);
}


/* creation/deletion */
static ols_event_t *_ols_event_copy (ols_event_t * ols_event){

#ifdef DEBUG_REFCOUNT
blog (LOG_INFO, "copy ols_event_t %p", ols_event);
#endif

    ols_event_t *copy;

    return_val_if_fail (ols_event != NULL, NULL);

    /* create a fresh new buffer */
    copy = ols_event_new();
    
    if(copy){
        copy->ev_type = ols_event->ev_type;
    }

    return copy;
}


void ols_event_init (ols_event_t * ols_event){
    ols_mini_object_init (OLS_MINI_OBJECT_CAST (ols_event), 0, 1, (ols_mini_object_copy_function)_ols_event_copy, NULL,
        (ols_mini_object_free_function) _ols_event_free);
}

ols_event_t * ols_event_new(){
    ols_event_t *ols_event = (ols_event_t *)bmalloc(sizeof(ols_event_t));
    if(ols_event) {
        ols_event_init(ols_event);
    }
    return ols_event;
}


ols_event_t *ols_event_new_eos(){

    ols_event_t *ols_event = ols_event_new();

    if(ols_event) {
        ols_event_init(ols_event);
        ols_event->ev_type = OLS_EVENT_EOS;
    }

    return ols_event;
}


ols_event_t *ols_event_new_stream_start(){
    ols_event_t *ols_event = ols_event_new();

    if(ols_event) {
        ols_event_init(ols_event);
        ols_event->ev_type = OLS_EVENT_STREAM_START;
    }

    return ols_event;
}
