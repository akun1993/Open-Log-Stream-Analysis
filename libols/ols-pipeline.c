#include "ols-pipeline.h"
#include <jansson.h>

ols_pipeline_t *ols_pipeline_create() {
  struct ols_pipeline *data = bzalloc(sizeof(struct ols_pipeline));

  return data;
}

ols_pipeline_t *ols_pipeline_new_from_json(const char *name,
                                           const char *pipe_json_str) {
  ols_pipeline_t *data = ols_pipeline_create();
  json_error_t error;
  json_t *root = json_loads(pipe_json_str, JSON_REJECT_DUPLICATES, &error);


  if (root && JSON_ARRAY == json_typeof(root)) {

    size_t size = json_array_size(root);

    printf("JSON Array of %lld element%s:\n", (long long)size, json_plural(size));
    for (int i = 0; i < size; i++) {
        json_t *element = json_array_get(root, i);

        if(JSON_OBJECT == json_typeof(element)){
            const char *key;
            json_t *value;

            size = json_object_size(element);

            printf("JSON Object of %lld pair%s:\n", (long long)size, json_plural(size));
            json_object_foreach(element, key, value) {
                
                if(strcmp("name",key)){

                } else if(strcmp("property",key)){

                } else if(strcmp("link",key)){
                    
                }

            }
        }
    }

    json_decref(root);

  } else {
    blog(LOG_ERROR,
         "ols-pipeline.c: [ols_pipeline_new] "
         "Failed reading json string (%d): %s",
         error.line, error.text);
    bfree(data);
    data = NULL;
  }

  return data;
}
