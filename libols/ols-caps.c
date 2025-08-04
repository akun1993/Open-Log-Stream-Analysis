#include "ols-caps.h"
#include "util/dstr.h"

static void _ols_caps_free(ols_caps_t *ols_caps);


static void ols_caps_init(ols_caps_t *ols_caps) {
  ols_mini_object_init(OLS_MINI_OBJECT_CAST(ols_caps), 0, 1,
                       (ols_mini_object_copy_function)NULL,
                       (ols_mini_object_dispose_function)NULL,
                       (ols_mini_object_free_function)_ols_caps_free);

}

static void ols_caps_add_cap(ols_caps_t *ols_caps, const char *cap_str) {
  if (!ols_caps || !cap_str) return;

  char *item = bstrdup(cap_str);
  da_push_back(ols_caps->caps, &item);
}


void  _ols_caps_free(ols_caps_t *ols_caps){

    if(!ols_caps) return;

    for (size_t i = 0; i < ols_caps->caps.num; i++)
        bfree(ols_caps->caps.array[i]);
    da_free(ols_caps->caps);
    bfree(ols_caps);
}

ols_caps_t *ols_caps_new(const char *caps_str){

    ols_caps_t *new_caps = (ols_caps_t *)bzalloc(sizeof(ols_caps_t));
    if(new_caps){

        ols_caps_init(new_caps);

        new_caps->flag = OLS_CAPS_DEFAULT;
        da_init(new_caps->caps);

        if(caps_str){
            char **cap_list = strlist_split(caps_str, ';', false);
            
            int idx = 0;
            while(cap_list[idx]){
                ols_caps_add_cap(new_caps,cap_list[idx]);
                idx++;
            }
           strlist_free(cap_list);
        }
    }

    return new_caps;
}

ols_caps_t  *ols_caps_new_any(){
    ols_caps_t *new_caps = (ols_caps_t *)bzalloc(sizeof(ols_caps_t));

    if(new_caps){
        ols_caps_init(new_caps);
        new_caps->flag = OLS_CAPS_ANY;
        da_init(new_caps->caps);
    }
    
    return new_caps;
}

const char * ols_caps_by_idx(ols_caps_t *ols_caps,size_t idx){
    if(!ols_caps || idx >= ols_caps->caps.num)
        return NULL;

    return ols_caps->caps.array[idx];
}

size_t ols_caps_count(ols_caps_t *ols_caps){
    if(!ols_caps )
        return 0;
    return ols_caps->caps.num;
}


