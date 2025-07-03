#include "ols-meta-txt.h"


static ols_txt_file_t *
_ols_txt_file_copy (const ols_txt_file_t * txt_file)
{
  ols_txt_file_t *new_txt_file;

  uint32_t i, n;

  //g_return_val_if_fail (GST_IS_CAPS (caps), NULL);

  new_txt_file = ols_txt_file_new_empty ();
  new_txt_file->buff = txt_file->buff;
  new_txt_file->capacity = txt_file->capacity;
  new_txt_file->len = txt_file->len;

  new_txt_file->line = txt_file->line;
  new_txt_file->file = txt_file->file;


  new_txt_file->log_lv = txt_file->log_lv;
  new_txt_file->pid = txt_file->pid;
  new_txt_file->tid = txt_file->tid;
  new_txt_file->msec = txt_file->msec;
  new_txt_file->tag = txt_file->tag;

 // GST_CAPS_FLAGS (newcaps) = GST_CAPS_FLAGS (caps);
 // n = GST_CAPS_LEN (caps);

  blog (LOG_INFO, "doing copy %p -> %p",txt_file, new_txt_file);

  return new_txt_file;
}

/* creation/deletion */
static void
_ols_txt_file_free (ols_txt_file_t * txt_file)
{

  /* This can be used to get statistics about caps sizes */

  /*GST_CAT_INFO (GST_CAT_CAPS, "caps size: %d", len); */

#ifdef DEBUG_REFCOUNT
  GST_CAT_TRACE (GST_CAT_CAPS, "freeing caps %p", caps);
#endif
 // bfree (sizeof (GstCapsImpl), caps);
 if(txt_file->buff){
  bfree(txt_file->buff);
  txt_file->capacity = 0;
 }
 txt_file->len = 0;
 dstr_free(&txt_file->file);
 dstr_free(&txt_file->tag);
}

static void
ols_txt_file_init (ols_txt_file_t * txt_file,size_t buff_size)
{
  ols_mini_object_init (OLS_MINI_OBJECT_CAST (txt_file), 0, 1,
      (ols_mini_object_copy_function) _ols_txt_file_copy, NULL,
      (ols_mini_object_free_function) _ols_txt_file_free);

  dstr_init(&txt_file->file);
  dstr_init(&txt_file->tag);
      
    
  if(buff_size > 0){
    txt_file->buff = bzalloc(buff_size);
    txt_file->capacity = buff_size;

  }

}

/**
 * gst_caps_new_empty:
 *
 * Creates a new #GstCaps that is empty.  That is, the returned
 * #GstCaps contains no media formats.
 * The #GstCaps is guaranteed to be writable.
 * Caller is responsible for unreffing the returned caps.
 *
 * Returns: (transfer full): the new #GstCaps
 */
ols_txt_file_t *
ols_txt_file_new_empty (void)
{
  ols_txt_file_t *txt_file;

  txt_file = (ols_txt_file_t *) bzalloc (sizeof(ols_txt_file_t));

  ols_txt_file_init (txt_file,0);

#ifdef DEBUG_REFCOUNT
  GST_CAT_TRACE (GST_CAT_CAPS, "created caps %p", caps);
#endif

  return txt_file;
}

ols_txt_file_t *ols_txt_file_with_buffer (size_t buf_size){

  ols_txt_file_t *txt_file;

  txt_file = (ols_txt_file_t *) bzalloc (sizeof(ols_txt_file_t));

  ols_txt_file_init (txt_file,buf_size);

#ifdef DEBUG_REFCOUNT
  GST_CAT_TRACE (GST_CAT_CAPS, "created caps %p", caps);
#endif

  return txt_file;

}
