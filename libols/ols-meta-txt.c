#include "ols-meta-txt.h"


static ols_txt_file_t *
_ols_txt_file_copy (const ols_txt_file_t * caps)
{
  ols_txt_file_t *new_txt_file;

  uint32_t i, n;

  g_return_val_if_fail (GST_IS_CAPS (caps), NULL);

  new_txt_file = ols_txt_file_new_empty ();

  GST_CAPS_FLAGS (newcaps) = GST_CAPS_FLAGS (caps);
  n = GST_CAPS_LEN (caps);

  GST_CAT_DEBUG_OBJECT (GST_CAT_PERFORMANCE, caps, "doing copy %p -> %p",
      caps, newcaps);

  return new_txt_file;
}

/* creation/deletion */
static void
_ols_txt_file_free (ols_txt_file_t * caps)
{
  GstStructure *structure;
  GstCapsFeatures *features;
  guint i, len;

  /* The refcount must be 0, but since we're only called by gst_caps_unref,
   * don't bother testing. */
  len = GST_CAPS_LEN (caps);
  /* This can be used to get statistics about caps sizes */
  /*GST_CAT_INFO (GST_CAT_CAPS, "caps size: %d", len); */
  for (i = 0; i < len; i++) {
    structure = gst_caps_get_structure_unchecked (caps, i);
    gst_structure_set_parent_refcount (structure, NULL);
    gst_structure_free (structure);
    features = gst_caps_get_features_unchecked (caps, i);
    if (features) {
      gst_caps_features_set_parent_refcount (features, NULL);
      gst_caps_features_free (features);
    }
  }
  g_array_free (GST_CAPS_ARRAY (caps), TRUE);

#ifdef DEBUG_REFCOUNT
  GST_CAT_TRACE (GST_CAT_CAPS, "freeing caps %p", caps);
#endif
  g_slice_free1 (sizeof (GstCapsImpl), caps);
}

static void
ols_txt_file_init (ols_txt_file_t * caps)
{
  ols_mini_object_init (OLS_MINI_OBJECT_CAST (caps), 0, _gst_caps_type,
      (OlsMiniObjectCopyFunction) _ols_txt_file_copy, NULL,
      (OlsMiniObjectFreeFunction) _ols_txt_file_free);

  /* the 32 has been determined by logging caps sizes in _gst_caps_free
   * but g_ptr_array uses 16 anyway if it expands once, so this does not help
   * in practice
   * GST_CAPS_ARRAY (caps) = g_ptr_array_sized_new (32);
   */
  GST_CAPS_ARRAY (caps) =
      g_array_new (FALSE, TRUE, sizeof (GstCapsArrayElement));
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

  txt_file = (ols_txt_file_t *) g_slice_new (GstCapsImpl);

  ols_txt_file_init (txt_file);

#ifdef DEBUG_REFCOUNT
  GST_CAT_TRACE (GST_CAT_CAPS, "created caps %p", caps);
#endif

  return txt_file;
}
