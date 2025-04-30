#include "ols-event.h"
#include "util/bmem.h"

ols_event_t *ols_event_new_eos(){
    return (ols_event_t *)bmalloc(sizeof(ols_event_t));
}
