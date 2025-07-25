#include "ols-meta-txt.h"
#include "util/base.h"
#include "util/bmem.h"

/* creation/deletion */
static void
_ols_meta_txt_free (ols_meta_txt_t * txt_file)
{
  /* This can be used to get statistics about meta_txt sizes */
  blog (LOG_INFO, "freeing ols_meta_txt %p", txt_file);
#ifdef DEBUG_REFCOUNT
  blog (LOG_INFO, "freeing ols_meta_txt %p", txt_file);
#endif

  if(txt_file->buff){
    bfree(txt_file->buff);
    txt_file->capacity = 0;
  }
  txt_file->len = 0;
  dstr_free(&txt_file->file);
  dstr_free(&txt_file->tag);
  bfree(txt_file);
}

static void
ols_meta_txt_init (ols_meta_txt_t * txt_file,size_t buff_size)
{
  ols_mini_object_init (OLS_MINI_OBJECT_CAST (txt_file), 0, 1, NULL, NULL,
      (ols_mini_object_free_function) _ols_meta_txt_free);

  dstr_init(&txt_file->file);
  dstr_init(&txt_file->tag);
      
  if(buff_size > 0){
    txt_file->buff = bzalloc(buff_size);
    txt_file->capacity = buff_size;
  }
}

/**
 * ols_meta_txt_new_empty:
 *
 * Creates a new #ols_meta_txt_t that is empty.  That is, the returned
 * #ols_meta_txt_t contains no data .
 *
 * Returns: (transfer full): the new #ols_meta_txt_t
 */
ols_meta_txt_t *
ols_meta_txt_new_empty (void)
{
  ols_meta_txt_t *txt_file;

  txt_file = (ols_meta_txt_t *) bzalloc (sizeof(ols_meta_txt_t));

  ols_meta_txt_init (txt_file,0);

#ifdef DEBUG_REFCOUNT
  blog (LOG_INFO, "created meta_txt %p", txt_file);
#endif

  return txt_file;
}

ols_meta_txt_t *ols_meta_txt_new_with_buffer (size_t buf_size){

  ols_meta_txt_t *txt_file;

  txt_file = (ols_meta_txt_t *) bzalloc (sizeof(ols_meta_txt_t));

  ols_meta_txt_init (txt_file,buf_size);

#ifdef DEBUG_REFCOUNT
  blog (LOG_INFO, "created meta_txt %p", txt_file);
#endif

  return txt_file;

}
