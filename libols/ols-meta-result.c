#include "ols-meta-result.h"
#include "util/base.h"
#include "util/bmem.h"
#include "util/darray.h"


/* creation/deletion */
static void
_ols_meta_result_free (ols_meta_result_t * meta_result)
{

  /* This can be used to get statistics about caps sizes */
#ifdef DEBUG_REFCOUNT
  blog (LOG_INFO, "freeing meta result %p", txt_file);
#endif

  dstr_free(&meta_result->tag);
  bfree(meta_result);
}

static void ols_meta_result_init (ols_meta_result_t * meta_result)
{
  ols_mini_object_init (OLS_MINI_OBJECT_CAST (meta_result), 0, 1,NULL, NULL,
      (ols_mini_object_free_function) _ols_meta_result_free);

  dstr_init(&meta_result->tag);
  da_init(meta_result->info);

}

/**
 * ols_meta_txt_new_empty:
 *
 * Creates a new #ols_meta_result_t that is empty.  That is, the returned
 * #ols_meta_result_t contains no media formats.
 *
 * Returns: (transfer full): the new #ols_meta_result_t
 */
ols_meta_result_t * ols_meta_result_new (void)
{
   ols_meta_result_t *meta_result;

   meta_result = (ols_meta_result_t *) bzalloc (sizeof(ols_meta_result_t));

   ols_meta_result_init (meta_result);

#ifdef DEBUG_REFCOUNT
  blog (LOG_INFO, "created meta_result %p", meta_result);
#endif

  return meta_result;
}


