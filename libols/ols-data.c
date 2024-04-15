/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@olsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "util/bmem.h"
#include "util/threading.h"
#include "util/dstr.h"
#include "util/darray.h"
#include "util/platform.h"
#include "util/uthash.h"
#include "ols-data.h"

#include <jansson.h>

struct ols_data_item {
	volatile long ref;
	const char *name;
	struct ols_data *parent;
	UT_hash_handle hh;
	enum ols_data_type type;
	size_t name_len;
	size_t data_len;
	size_t data_size;
	size_t default_len;
	size_t default_size;
	size_t autoselect_size;
	size_t capacity;
};

struct ols_data {
	volatile long ref;
	char *json;
	struct ols_data_item *items;
};

struct ols_data_array {
	volatile long ref;
	DARRAY(ols_data_t *) objects;
};

struct ols_data_number {
	enum ols_data_number_type type;
	union {
		long long int_val;
		double double_val;
	};
};

/* ------------------------------------------------------------------------- */
/* Item structure, designed to be one allocation only */

static inline size_t get_align_size(size_t size)
{
	const size_t alignment = base_get_alignment();
	return (size + alignment - 1) & ~(alignment - 1);
}

/* ensures data after the name has alignment (in case of SSE) */
static inline size_t get_name_align_size(const char *name)
{
	size_t name_size = strlen(name) + 1;
	size_t alignment = base_get_alignment();
	size_t total_size;

	total_size = sizeof(struct ols_data_item) + (name_size + alignment - 1);
	total_size &= ~(alignment - 1);

	return total_size - sizeof(struct ols_data_item);
}

static inline char *get_item_name(struct ols_data_item *item)
{
	return (char *)item + sizeof(struct ols_data_item);
}

static inline void *get_data_ptr(ols_data_item_t *item)
{
	return (uint8_t *)get_item_name(item) + item->name_len;
}

static inline void *get_item_data(struct ols_data_item *item)
{
	if (!item->data_size && !item->default_size && !item->autoselect_size)
		return NULL;
	return get_data_ptr(item);
}

static inline void *get_default_data_ptr(ols_data_item_t *item)
{
	return (uint8_t *)get_data_ptr(item) + item->data_len;
}

static inline void *get_item_default_data(struct ols_data_item *item)
{
	return item->default_size ? get_default_data_ptr(item) : NULL;
}

static inline void *get_autoselect_data_ptr(ols_data_item_t *item)
{
	return (uint8_t *)get_default_data_ptr(item) + item->default_len;
}

static inline void *get_item_autoselect_data(struct ols_data_item *item)
{
	return item->autoselect_size ? get_autoselect_data_ptr(item) : NULL;
}

static inline size_t ols_data_item_total_size(struct ols_data_item *item)
{
	return sizeof(struct ols_data_item) + item->name_len + item->data_len +
	       item->default_len + item->autoselect_size;
}

static inline ols_data_t *get_item_obj(struct ols_data_item *item)
{
	if (!item)
		return NULL;

	ols_data_t **data = get_item_data(item);
	if (!data)
		return NULL;

	return *data;
}

static inline ols_data_t *get_item_default_obj(struct ols_data_item *item)
{
	if (!item || !item->default_size)
		return NULL;

	return *(ols_data_t **)get_default_data_ptr(item);
}

static inline ols_data_t *get_item_autoselect_obj(struct ols_data_item *item)
{
	if (!item || !item->autoselect_size)
		return NULL;

	return *(ols_data_t **)get_autoselect_data_ptr(item);
}

static inline ols_data_array_t *get_item_array(struct ols_data_item *item)
{
	ols_data_array_t **array;

	if (!item)
		return NULL;

	array = (ols_data_array_t **)get_item_data(item);
	return array ? *array : NULL;
}

static inline ols_data_array_t *
get_item_default_array(struct ols_data_item *item)
{
	if (!item || !item->default_size)
		return NULL;

	return *(ols_data_array_t **)get_default_data_ptr(item);
}

static inline ols_data_array_t *
get_item_autoselect_array(struct ols_data_item *item)
{
	if (!item || !item->autoselect_size)
		return NULL;

	return *(ols_data_array_t **)get_autoselect_data_ptr(item);
}

static inline void item_data_release(struct ols_data_item *item)
{
	if (!ols_data_item_has_user_value(item))
		return;

	if (item->type == OLS_DATA_OBJECT) {
		ols_data_t *obj = get_item_obj(item);
		ols_data_release(obj);

	} else if (item->type == OLS_DATA_ARRAY) {
		ols_data_array_t *array = get_item_array(item);
		ols_data_array_release(array);
	}
}

static inline void item_default_data_release(struct ols_data_item *item)
{
	if (item->type == OLS_DATA_OBJECT) {
		ols_data_t *obj = get_item_default_obj(item);
		ols_data_release(obj);

	} else if (item->type == OLS_DATA_ARRAY) {
		ols_data_array_t *array = get_item_default_array(item);
		ols_data_array_release(array);
	}
}

static inline void item_autoselect_data_release(struct ols_data_item *item)
{
	if (item->type == OLS_DATA_OBJECT) {
		ols_data_t *obj = get_item_autoselect_obj(item);
		ols_data_release(obj);

	} else if (item->type == OLS_DATA_ARRAY) {
		ols_data_array_t *array = get_item_autoselect_array(item);
		ols_data_array_release(array);
	}
}

static inline void item_data_addref(struct ols_data_item *item)
{
	if (item->type == OLS_DATA_OBJECT) {
		ols_data_t *obj = get_item_obj(item);
		ols_data_addref(obj);

	} else if (item->type == OLS_DATA_ARRAY) {
		ols_data_array_t *array = get_item_array(item);
		ols_data_array_addref(array);
	}
}

static inline void item_default_data_addref(struct ols_data_item *item)
{
	if (!item->data_size)
		return;

	if (item->type == OLS_DATA_OBJECT) {
		ols_data_t *obj = get_item_default_obj(item);
		ols_data_addref(obj);

	} else if (item->type == OLS_DATA_ARRAY) {
		ols_data_array_t *array = get_item_default_array(item);
		ols_data_array_addref(array);
	}
}

static inline void item_autoselect_data_addref(struct ols_data_item *item)
{
	if (item->type == OLS_DATA_OBJECT) {
		ols_data_t *obj = get_item_autoselect_obj(item);
		ols_data_addref(obj);

	} else if (item->type == OLS_DATA_ARRAY) {
		ols_data_array_t *array = get_item_autoselect_array(item);
		ols_data_array_addref(array);
	}
}

static struct ols_data_item *ols_data_item_create(const char *name,
						  const void *data, size_t size,
						  enum ols_data_type type,
						  bool default_data,
						  bool autoselect_data)
{
	struct ols_data_item *item;
	size_t name_size, total_size;

	if (!name || !data)
		return NULL;

	name_size = get_name_align_size(name);
	total_size = name_size + sizeof(struct ols_data_item) + size;

	item = bzalloc(total_size);

	item->capacity = total_size;
	item->type = type;
	item->name_len = name_size;
	item->ref = 1;

	if (default_data) {
		item->default_len = size;
		item->default_size = size;

	} else if (autoselect_data) {
		item->autoselect_size = size;

	} else {
		item->data_len = size;
		item->data_size = size;
	}

	char *name_ptr = get_item_name(item);
	item->name = name_ptr;

	strcpy(name_ptr, name);
	memcpy(get_item_data(item), data, size);

	item_data_addref(item);
	return item;
}

static inline void ols_data_item_detach(struct ols_data_item *item)
{
	if (item->parent) {
		HASH_DEL(item->parent->items, item);
		item->parent = NULL;
	}
}

static inline void ols_data_item_reattach(struct ols_data *parent,
					  struct ols_data_item *item)
{
	if (parent) {
		HASH_ADD_STR(parent->items, name, item);
		item->parent = parent;
	}
}

static struct ols_data_item *
ols_data_item_ensure_capacity(struct ols_data_item *item)
{
	size_t new_size = ols_data_item_total_size(item);
	struct ols_data_item *new_item;

	if (item->capacity >= new_size)
		return item;

	struct ols_data *parent = item->parent;
	ols_data_item_detach(item);

	new_item = brealloc(item, new_size);
	new_item->capacity = new_size;
	new_item->name = get_item_name(new_item);

	ols_data_item_reattach(parent, new_item);

	return new_item;
}

static inline void ols_data_item_destroy(struct ols_data_item *item)
{
	if (item->parent)
		HASH_DEL(item->parent->items, item);

	item_data_release(item);
	item_default_data_release(item);
	item_autoselect_data_release(item);
	ols_data_item_detach(item);
	bfree(item);
}

static inline void move_data(ols_data_item_t *old_item, void *old_data,
			     ols_data_item_t *item, void *data, size_t len)
{
	ptrdiff_t old_offset = (uint8_t *)old_data - (uint8_t *)old_item;
	ptrdiff_t new_offset = (uint8_t *)data - (uint8_t *)item;

	if (!old_data)
		return;

	memmove((uint8_t *)item + new_offset, (uint8_t *)item + old_offset,
		len);
}

static inline void ols_data_item_setdata(struct ols_data_item **p_item,
					 const void *data, size_t size,
					 enum ols_data_type type)
{
	if (!p_item || !*p_item)
		return;

	struct ols_data_item *item = *p_item;
	ptrdiff_t old_default_data_pos =
		(uint8_t *)get_default_data_ptr(item) - (uint8_t *)item;
	item_data_release(item);

	item->data_size = size;
	item->type = type;
	item->data_len = (item->default_size || item->autoselect_size)
				 ? get_align_size(size)
				 : size;
	item = ols_data_item_ensure_capacity(item);

	if (item->default_size || item->autoselect_size)
		memmove(get_default_data_ptr(item),
			(uint8_t *)item + old_default_data_pos,
			item->default_len + item->autoselect_size);

	if (size) {
		memcpy(get_item_data(item), data, size);
		item_data_addref(item);
	}

	*p_item = item;
}

static inline void ols_data_item_set_default_data(struct ols_data_item **p_item,
						  const void *data, size_t size,
						  enum ols_data_type type)
{
	if (!p_item || !*p_item)
		return;

	struct ols_data_item *item = *p_item;
	void *old_autoselect_data = get_autoselect_data_ptr(item);
	item_default_data_release(item);

	item->type = type;
	item->default_size = size;
	item->default_len = item->autoselect_size ? get_align_size(size) : size;
	item->data_len = item->data_size ? get_align_size(item->data_size) : 0;
	item = ols_data_item_ensure_capacity(item);

	if (item->autoselect_size)
		move_data(*p_item, old_autoselect_data, item,
			  get_autoselect_data_ptr(item), item->autoselect_size);

	if (size) {
		memcpy(get_item_default_data(item), data, size);
		item_default_data_addref(item);
	}

	*p_item = item;
}

static inline void
ols_data_item_set_autoselect_data(struct ols_data_item **p_item,
				  const void *data, size_t size,
				  enum ols_data_type type)
{
	if (!p_item || !*p_item)
		return;

	struct ols_data_item *item = *p_item;
	item_autoselect_data_release(item);

	item->autoselect_size = size;
	item->type = type;
	item->data_len = item->data_size ? get_align_size(item->data_size) : 0;
	item->default_len =
		item->default_size ? get_align_size(item->default_size) : 0;
	item = ols_data_item_ensure_capacity(item);

	if (size) {
		memcpy(get_item_autoselect_data(item), data, size);
		item_autoselect_data_addref(item);
	}

	*p_item = item;
}

/* ------------------------------------------------------------------------- */

static void ols_data_add_json_item(ols_data_t *data, const char *key,
				   json_t *json);

static inline void ols_data_add_json_object_data(ols_data_t *data, json_t *jobj)
{
	const char *item_key;
	json_t *jitem;

	json_object_foreach (jobj, item_key, jitem) {
		ols_data_add_json_item(data, item_key, jitem);
	}
}

static inline void ols_data_add_json_object(ols_data_t *data, const char *key,
					    json_t *jobj)
{
	ols_data_t *sub_obj = ols_data_create();

	ols_data_add_json_object_data(sub_obj, jobj);
	ols_data_set_obj(data, key, sub_obj);
	ols_data_release(sub_obj);
}

static void ols_data_add_json_array(ols_data_t *data, const char *key,
				    json_t *jarray)
{
	ols_data_array_t *array = ols_data_array_create();
	size_t idx;
	json_t *jitem;

	json_array_foreach (jarray, idx, jitem) {
		ols_data_t *item;

		if (!json_is_object(jitem))
			continue;

		item = ols_data_create();
		ols_data_add_json_object_data(item, jitem);
		ols_data_array_push_back(array, item);
		ols_data_release(item);
	}

	ols_data_set_array(data, key, array);
	ols_data_array_release(array);
}

static void ols_data_add_json_item(ols_data_t *data, const char *key,
				   json_t *json)
{
	if (json_is_object(json))
		ols_data_add_json_object(data, key, json);
	else if (json_is_array(json))
		ols_data_add_json_array(data, key, json);
	else if (json_is_string(json))
		ols_data_set_string(data, key, json_string_value(json));
	else if (json_is_integer(json))
		ols_data_set_int(data, key, json_integer_value(json));
	else if (json_is_real(json))
		ols_data_set_double(data, key, json_real_value(json));
	else if (json_is_true(json))
		ols_data_set_bool(data, key, true);
	else if (json_is_false(json))
		ols_data_set_bool(data, key, false);
}

/* ------------------------------------------------------------------------- */

static inline void set_json_string(json_t *json, const char *name,
				   ols_data_item_t *item)
{
	const char *val = ols_data_item_get_string(item);
	json_object_set_new(json, name, json_string(val));
}

static inline void set_json_number(json_t *json, const char *name,
				   ols_data_item_t *item)
{
	enum ols_data_number_type type = ols_data_item_numtype(item);

	if (type == OLS_DATA_NUM_INT) {
		long long val = ols_data_item_get_int(item);
		json_object_set_new(json, name, json_integer(val));
	} else {
		double val = ols_data_item_get_double(item);
		json_object_set_new(json, name, json_real(val));
	}
}

static inline void set_json_bool(json_t *json, const char *name,
				 ols_data_item_t *item)
{
	bool val = ols_data_item_get_bool(item);
	json_object_set_new(json, name, val ? json_true() : json_false());
}

static json_t *ols_data_to_json(ols_data_t *data);

static inline void set_json_obj(json_t *json, const char *name,
				ols_data_item_t *item)
{
	ols_data_t *obj = ols_data_item_get_obj(item);
	json_object_set_new(json, name, ols_data_to_json(obj));
	ols_data_release(obj);
}

static inline void set_json_array(json_t *json, const char *name,
				  ols_data_item_t *item)
{
	json_t *jarray = json_array();
	ols_data_array_t *array = ols_data_item_get_array(item);
	size_t count = ols_data_array_count(array);

	for (size_t idx = 0; idx < count; idx++) {
		ols_data_t *sub_item = ols_data_array_item(array, idx);
		json_t *jitem = ols_data_to_json(sub_item);
		json_array_append_new(jarray, jitem);
		ols_data_release(sub_item);
	}

	json_object_set_new(json, name, jarray);
	ols_data_array_release(array);
}

static json_t *ols_data_to_json(ols_data_t *data)
{
	json_t *json = json_object();

	ols_data_item_t *item = NULL;
	ols_data_item_t *temp = NULL;

	HASH_ITER (hh, data->items, item, temp) {
		enum ols_data_type type = ols_data_item_gettype(item);
		const char *name = get_item_name(item);

		if (!ols_data_item_has_user_value(item))
			continue;

		if (type == OLS_DATA_STRING)
			set_json_string(json, name, item);
		else if (type == OLS_DATA_NUMBER)
			set_json_number(json, name, item);
		else if (type == OLS_DATA_BOOLEAN)
			set_json_bool(json, name, item);
		else if (type == OLS_DATA_OBJECT)
			set_json_obj(json, name, item);
		else if (type == OLS_DATA_ARRAY)
			set_json_array(json, name, item);
	}

	return json;
}

/* ------------------------------------------------------------------------- */

ols_data_t *ols_data_create()
{
	struct ols_data *data = bzalloc(sizeof(struct ols_data));
	data->ref = 1;

	return data;
}

ols_data_t *ols_data_create_from_json(const char *json_string)
{
	ols_data_t *data = ols_data_create();

	json_error_t error;
	json_t *root = json_loads(json_string, JSON_REJECT_DUPLICATES, &error);

	if (root) {
		ols_data_add_json_object_data(data, root);
		json_decref(root);
	} else {
		blog(LOG_ERROR,
		     "ols-data.c: [ols_data_create_from_json] "
		     "Failed reading json string (%d): %s",
		     error.line, error.text);
		ols_data_release(data);
		data = NULL;
	}

	return data;
}

ols_data_t *ols_data_create_from_json_file(const char *json_file)
{
	char *file_data = os_quick_read_utf8_file(json_file);
	ols_data_t *data = NULL;

	if (file_data) {
		data = ols_data_create_from_json(file_data);
		bfree(file_data);
	}

	return data;
}

ols_data_t *ols_data_create_from_json_file_safe(const char *json_file,
						const char *backup_ext)
{
	ols_data_t *file_data = ols_data_create_from_json_file(json_file);
	if (!file_data && backup_ext && *backup_ext) {
		struct dstr backup_file = {0};

		dstr_copy(&backup_file, json_file);
		if (*backup_ext != '.')
			dstr_cat(&backup_file, ".");
		dstr_cat(&backup_file, backup_ext);

		if (os_file_exists(backup_file.array)) {
			blog(LOG_WARNING,
			     "ols-data.c: "
			     "[ols_data_create_from_json_file_safe] "
			     "attempting backup file");

			/* delete current file if corrupt to prevent it from
			 * being backed up again */
			os_rename(backup_file.array, json_file);

			file_data = ols_data_create_from_json_file(json_file);
		}

		dstr_free(&backup_file);
	}

	return file_data;
}

void ols_data_addref(ols_data_t *data)
{
	if (data)
		os_atomic_inc_long(&data->ref);
}

static inline void ols_data_destroy(struct ols_data *data)
{
	struct ols_data_item *item, *temp;

	HASH_ITER (hh, data->items, item, temp) {
		ols_data_item_detach(item);
		ols_data_item_release(&item);
	}

	/* NOTE: don't use bfree for json text, allocated by json */
	free(data->json);
	bfree(data);
}

void ols_data_release(ols_data_t *data)
{
	if (!data)
		return;

	if (os_atomic_dec_long(&data->ref) == 0)
		ols_data_destroy(data);
}

const char *ols_data_get_json(ols_data_t *data)
{
	if (!data)
		return NULL;

	/* NOTE: don't use libols bfree for json text */
	free(data->json);
	data->json = NULL;

	json_t *root = ols_data_to_json(data);
	data->json = json_dumps(root, JSON_PRESERVE_ORDER | JSON_COMPACT);
	json_decref(root);

	return data->json;
}

const char *ols_data_get_json_pretty(ols_data_t *data)
{
	if (!data)
		return NULL;

	/* NOTE: don't use libols bfree for json text */
	free(data->json);
	data->json = NULL;

	json_t *root = ols_data_to_json(data);
	data->json = json_dumps(root, JSON_PRESERVE_ORDER | JSON_INDENT(4));
	json_decref(root);

	return data->json;
}

const char *ols_data_get_last_json(ols_data_t *data)
{
	return data ? data->json : NULL;
}

bool ols_data_save_json(ols_data_t *data, const char *file)
{
	const char *json = ols_data_get_json(data);

	if (json && *json) {
		return os_quick_write_utf8_file(file, json, strlen(json),
						false);
	}

	return false;
}

bool ols_data_save_json_safe(ols_data_t *data, const char *file,
			     const char *temp_ext, const char *backup_ext)
{
	const char *json = ols_data_get_json(data);

	if (json && *json) {
		return os_quick_write_utf8_file_safe(
			file, json, strlen(json), false, temp_ext, backup_ext);
	}

	return false;
}

bool ols_data_save_json_pretty_safe(ols_data_t *data, const char *file,
				    const char *temp_ext,
				    const char *backup_ext)
{
	const char *json = ols_data_get_json_pretty(data);

	if (json && *json) {
		return os_quick_write_utf8_file_safe(
			file, json, strlen(json), false, temp_ext, backup_ext);
	}

	return false;
}

static void get_defaults_array_cb(ols_data_t *data, void *vp)
{
	ols_data_array_t *defs = (ols_data_array_t *)vp;
	ols_data_t *ols_defaults = ols_data_get_defaults(data);

	ols_data_array_push_back(defs, ols_defaults);

	ols_data_release(ols_defaults);
}

ols_data_t *ols_data_get_defaults(ols_data_t *data)
{
	ols_data_t *defaults = ols_data_create();

	if (!data)
		return defaults;

	struct ols_data_item *item, *temp;

	HASH_ITER (hh, data->items, item, temp) {
		const char *name = get_item_name(item);
		switch (item->type) {
		case OLS_DATA_NULL:
			break;

		case OLS_DATA_STRING: {
			const char *str =
				ols_data_get_default_string(data, name);
			ols_data_set_string(defaults, name, str);
			break;
		}

		case OLS_DATA_NUMBER: {
			switch (ols_data_item_numtype(item)) {
			case OLS_DATA_NUM_DOUBLE: {
				double val =
					ols_data_get_default_double(data, name);
				ols_data_set_double(defaults, name, val);
				break;
			}

			case OLS_DATA_NUM_INT: {
				long long val =
					ols_data_get_default_int(data, name);
				ols_data_set_int(defaults, name, val);
				break;
			}

			case OLS_DATA_NUM_INVALID:
				break;
			}
			break;
		}

		case OLS_DATA_BOOLEAN: {
			bool val = ols_data_get_default_bool(data, name);
			ols_data_set_bool(defaults, name, val);
			break;
		}

		case OLS_DATA_OBJECT: {
			ols_data_t *val = ols_data_get_default_obj(data, name);
			ols_data_t *defs = ols_data_get_defaults(val);

			ols_data_set_obj(defaults, name, defs);

			ols_data_release(defs);
			ols_data_release(val);
			break;
		}

		case OLS_DATA_ARRAY: {
			ols_data_array_t *arr =
				ols_data_get_default_array(data, name);
			ols_data_array_t *defs = ols_data_array_create();

			ols_data_array_enum(arr, get_defaults_array_cb,
					    (void *)defs);
			ols_data_set_array(defaults, name, defs);

			ols_data_array_release(defs);
			ols_data_array_release(arr);
			break;
		}
		}
	}

	return defaults;
}

static struct ols_data_item *get_item(struct ols_data *data, const char *name)
{
	if (!data)
		return NULL;

	struct ols_data_item *item;
	HASH_FIND_STR(data->items, name, item);
	return item;
}

static void set_item_data(struct ols_data *data, struct ols_data_item **item,
			  const char *name, const void *ptr, size_t size,
			  enum ols_data_type type, bool default_data,
			  bool autoselect_data)
{
	ols_data_item_t *new_item = NULL;

	if ((!item || !*item) && data) {
		new_item = ols_data_item_create(name, ptr, size, type,
						default_data, autoselect_data);
		new_item->parent = data;
		HASH_ADD_STR(data->items, name, new_item);

	} else if (default_data) {
		ols_data_item_set_default_data(item, ptr, size, type);
	} else if (autoselect_data) {
		ols_data_item_set_autoselect_data(item, ptr, size, type);
	} else {
		ols_data_item_setdata(item, ptr, size, type);
	}
}

static inline void set_item(struct ols_data *data, ols_data_item_t **item,
			    const char *name, const void *ptr, size_t size,
			    enum ols_data_type type)
{
	ols_data_item_t *actual_item = NULL;

	if (!data && !item)
		return;

	if (!item) {
		actual_item = get_item(data, name);
		item = &actual_item;
	}

	set_item_data(data, item, name, ptr, size, type, false, false);
}

static inline void set_item_def(struct ols_data *data, ols_data_item_t **item,
				const char *name, const void *ptr, size_t size,
				enum ols_data_type type)
{
	ols_data_item_t *actual_item = NULL;

	if (!data && !item)
		return;

	if (!item) {
		actual_item = get_item(data, name);
		item = &actual_item;
	}

	if (*item && (*item)->type != type)
		return;

	set_item_data(data, item, name, ptr, size, type, true, false);
}

static inline void set_item_auto(struct ols_data *data, ols_data_item_t **item,
				 const char *name, const void *ptr, size_t size,
				 enum ols_data_type type)
{
	ols_data_item_t *actual_item = NULL;

	if (!data && !item)
		return;

	if (!item) {
		actual_item = get_item(data, name);
		item = &actual_item;
	}

	set_item_data(data, item, name, ptr, size, type, false, true);
}

static void copy_obj(struct ols_data *data, const char *name,
		     struct ols_data *obj,
		     void (*callback)(ols_data_t *, const char *, ols_data_t *))
{
	if (obj) {
		ols_data_t *new_obj = ols_data_create();
		ols_data_apply(new_obj, obj);
		callback(data, name, new_obj);
		ols_data_release(new_obj);
	}
}

static void copy_array(struct ols_data *data, const char *name,
		       struct ols_data_array *array,
		       void (*callback)(ols_data_t *, const char *,
					ols_data_array_t *))
{
	if (array) {
		ols_data_array_t *new_array = ols_data_array_create();
		da_reserve(new_array->objects, array->objects.num);

		for (size_t i = 0; i < array->objects.num; i++) {
			ols_data_t *new_obj = ols_data_create();
			ols_data_t *obj = array->objects.array[i];

			ols_data_apply(new_obj, obj);
			ols_data_array_push_back(new_array, new_obj);

			ols_data_release(new_obj);
		}

		callback(data, name, new_array);
		ols_data_array_release(new_array);
	}
}

static inline void copy_item(struct ols_data *data, struct ols_data_item *item)
{
	const char *name = get_item_name(item);
	void *ptr = get_item_data(item);

	if (item->type == OLS_DATA_OBJECT) {
		ols_data_t **obj = item->data_size ? ptr : NULL;

		if (obj)
			copy_obj(data, name, *obj, ols_data_set_obj);

	} else if (item->type == OLS_DATA_ARRAY) {
		ols_data_array_t **array = item->data_size ? ptr : NULL;

		if (array)
			copy_array(data, name, *array, ols_data_set_array);

	} else {
		if (item->data_size)
			set_item(data, NULL, name, ptr, item->data_size,
				 item->type);
	}
}

void ols_data_apply(ols_data_t *target, ols_data_t *apply_data)
{
	if (!target || !apply_data || target == apply_data)
		return;

	struct ols_data_item *item, *temp;

	HASH_ITER (hh, apply_data->items, item, temp) {
		copy_item(target, item);
	}
}

void ols_data_erase(ols_data_t *data, const char *name)
{
	struct ols_data_item *item = get_item(data, name);

	if (item) {
		ols_data_item_detach(item);
		ols_data_item_release(&item);
	}
}

static inline void clear_item(struct ols_data_item *item)
{
	void *ptr = get_item_data(item);
	size_t size;

	if (item->data_len) {
		if (item->type == OLS_DATA_OBJECT) {
			ols_data_t **obj = item->data_size ? ptr : NULL;

			if (obj && *obj)
				ols_data_release(*obj);

		} else if (item->type == OLS_DATA_ARRAY) {
			ols_data_array_t **array = item->data_size ? ptr : NULL;

			if (array && *array)
				ols_data_array_release(*array);
		}

		size = item->default_len + item->autoselect_size;
		if (size)
			memmove(ptr, (uint8_t *)ptr + item->data_len, size);

		item->data_size = 0;
		item->data_len = 0;
	}
}

void ols_data_clear(ols_data_t *target)
{
	if (!target)
		return;

	struct ols_data_item *item, *temp;
	HASH_ITER (hh, target->items, item, temp) {
		clear_item(item);
	}
}

typedef void (*set_item_t)(ols_data_t *, ols_data_item_t **, const char *,
			   const void *, size_t, enum ols_data_type);

static inline void ols_set_string(ols_data_t *data, ols_data_item_t **item,
				  const char *name, const char *val,
				  set_item_t set_item_)
{
	if (!val)
		val = "";
	set_item_(data, item, name, val, strlen(val) + 1, OLS_DATA_STRING);
}

static inline void ols_set_int(ols_data_t *data, ols_data_item_t **item,
			       const char *name, long long val,
			       set_item_t set_item_)
{
	struct ols_data_number num;
	num.type = OLS_DATA_NUM_INT;
	num.int_val = val;
	set_item_(data, item, name, &num, sizeof(struct ols_data_number),
		  OLS_DATA_NUMBER);
}

static inline void ols_set_double(ols_data_t *data, ols_data_item_t **item,
				  const char *name, double val,
				  set_item_t set_item_)
{
	struct ols_data_number num;
	num.type = OLS_DATA_NUM_DOUBLE;
	num.double_val = val;
	set_item_(data, item, name, &num, sizeof(struct ols_data_number),
		  OLS_DATA_NUMBER);
}

static inline void ols_set_bool(ols_data_t *data, ols_data_item_t **item,
				const char *name, bool val,
				set_item_t set_item_)
{
	set_item_(data, item, name, &val, sizeof(bool), OLS_DATA_BOOLEAN);
}

static inline void ols_set_obj(ols_data_t *data, ols_data_item_t **item,
			       const char *name, ols_data_t *obj,
			       set_item_t set_item_)
{
	set_item_(data, item, name, &obj, sizeof(ols_data_t *),
		  OLS_DATA_OBJECT);
}

static inline void ols_set_array(ols_data_t *data, ols_data_item_t **item,
				 const char *name, ols_data_array_t *array,
				 set_item_t set_item_)
{
	set_item_(data, item, name, &array, sizeof(ols_data_t *),
		  OLS_DATA_ARRAY);
}

static inline void ols_take_obj(ols_data_t *data, ols_data_item_t **item,
				const char *name, ols_data_t *obj,
				set_item_t set_item_)
{
	ols_set_obj(data, item, name, obj, set_item_);
	ols_data_release(obj);
}

void ols_data_set_string(ols_data_t *data, const char *name, const char *val)
{
	ols_set_string(data, NULL, name, val, set_item);
}

void ols_data_set_int(ols_data_t *data, const char *name, long long val)
{
	ols_set_int(data, NULL, name, val, set_item);
}

void ols_data_set_double(ols_data_t *data, const char *name, double val)
{
	ols_set_double(data, NULL, name, val, set_item);
}

void ols_data_set_bool(ols_data_t *data, const char *name, bool val)
{
	ols_set_bool(data, NULL, name, val, set_item);
}

void ols_data_set_obj(ols_data_t *data, const char *name, ols_data_t *obj)
{
	ols_set_obj(data, NULL, name, obj, set_item);
}

void ols_data_set_array(ols_data_t *data, const char *name,
			ols_data_array_t *array)
{
	ols_set_array(data, NULL, name, array, set_item);
}

void ols_data_set_default_string(ols_data_t *data, const char *name,
				 const char *val)
{
	ols_set_string(data, NULL, name, val, set_item_def);
}

void ols_data_set_default_int(ols_data_t *data, const char *name, long long val)
{
	ols_set_int(data, NULL, name, val, set_item_def);
}

void ols_data_set_default_double(ols_data_t *data, const char *name, double val)
{
	ols_set_double(data, NULL, name, val, set_item_def);
}

void ols_data_set_default_bool(ols_data_t *data, const char *name, bool val)
{
	ols_set_bool(data, NULL, name, val, set_item_def);
}

void ols_data_set_default_obj(ols_data_t *data, const char *name,
			      ols_data_t *obj)
{
	ols_set_obj(data, NULL, name, obj, set_item_def);
}

void ols_data_set_default_array(ols_data_t *data, const char *name,
				ols_data_array_t *arr)
{
	ols_set_array(data, NULL, name, arr, set_item_def);
}

void ols_data_set_autoselect_string(ols_data_t *data, const char *name,
				    const char *val)
{
	ols_set_string(data, NULL, name, val, set_item_auto);
}

void ols_data_set_autoselect_int(ols_data_t *data, const char *name,
				 long long val)
{
	ols_set_int(data, NULL, name, val, set_item_auto);
}

void ols_data_set_autoselect_double(ols_data_t *data, const char *name,
				    double val)
{
	ols_set_double(data, NULL, name, val, set_item_auto);
}

void ols_data_set_autoselect_bool(ols_data_t *data, const char *name, bool val)
{
	ols_set_bool(data, NULL, name, val, set_item_auto);
}

void ols_data_set_autoselect_obj(ols_data_t *data, const char *name,
				 ols_data_t *obj)
{
	ols_set_obj(data, NULL, name, obj, set_item_auto);
}

void ols_data_set_autoselect_array(ols_data_t *data, const char *name,
				   ols_data_array_t *arr)
{
	ols_set_array(data, NULL, name, arr, set_item_auto);
}

const char *ols_data_get_string(ols_data_t *data, const char *name)
{
	return ols_data_item_get_string(get_item(data, name));
}

long long ols_data_get_int(ols_data_t *data, const char *name)
{
	return ols_data_item_get_int(get_item(data, name));
}

double ols_data_get_double(ols_data_t *data, const char *name)
{
	return ols_data_item_get_double(get_item(data, name));
}

bool ols_data_get_bool(ols_data_t *data, const char *name)
{
	return ols_data_item_get_bool(get_item(data, name));
}

ols_data_t *ols_data_get_obj(ols_data_t *data, const char *name)
{
	return ols_data_item_get_obj(get_item(data, name));
}

ols_data_array_t *ols_data_get_array(ols_data_t *data, const char *name)
{
	return ols_data_item_get_array(get_item(data, name));
}

const char *ols_data_get_default_string(ols_data_t *data, const char *name)
{
	return ols_data_item_get_default_string(get_item(data, name));
}

long long ols_data_get_default_int(ols_data_t *data, const char *name)
{
	return ols_data_item_get_default_int(get_item(data, name));
}

double ols_data_get_default_double(ols_data_t *data, const char *name)
{
	return ols_data_item_get_default_double(get_item(data, name));
}

bool ols_data_get_default_bool(ols_data_t *data, const char *name)
{
	return ols_data_item_get_default_bool(get_item(data, name));
}

ols_data_t *ols_data_get_default_obj(ols_data_t *data, const char *name)
{
	return ols_data_item_get_default_obj(get_item(data, name));
}

ols_data_array_t *ols_data_get_default_array(ols_data_t *data, const char *name)
{
	return ols_data_item_get_default_array(get_item(data, name));
}

const char *ols_data_get_autoselect_string(ols_data_t *data, const char *name)
{
	return ols_data_item_get_autoselect_string(get_item(data, name));
}

long long ols_data_get_autoselect_int(ols_data_t *data, const char *name)
{
	return ols_data_item_get_autoselect_int(get_item(data, name));
}

double ols_data_get_autoselect_double(ols_data_t *data, const char *name)
{
	return ols_data_item_get_autoselect_double(get_item(data, name));
}

bool ols_data_get_autoselect_bool(ols_data_t *data, const char *name)
{
	return ols_data_item_get_autoselect_bool(get_item(data, name));
}

ols_data_t *ols_data_get_autoselect_obj(ols_data_t *data, const char *name)
{
	return ols_data_item_get_autoselect_obj(get_item(data, name));
}

ols_data_array_t *ols_data_get_autoselect_array(ols_data_t *data,
						const char *name)
{
	return ols_data_item_get_autoselect_array(get_item(data, name));
}

ols_data_array_t *ols_data_array_create()
{
	struct ols_data_array *array = bzalloc(sizeof(struct ols_data_array));
	array->ref = 1;

	return array;
}

void ols_data_array_addref(ols_data_array_t *array)
{
	if (array)
		os_atomic_inc_long(&array->ref);
}

static inline void ols_data_array_destroy(ols_data_array_t *array)
{
	if (array) {
		for (size_t i = 0; i < array->objects.num; i++)
			ols_data_release(array->objects.array[i]);
		da_free(array->objects);
		bfree(array);
	}
}

void ols_data_array_release(ols_data_array_t *array)
{
	if (!array)
		return;

	if (os_atomic_dec_long(&array->ref) == 0)
		ols_data_array_destroy(array);
}

size_t ols_data_array_count(ols_data_array_t *array)
{
	return array ? array->objects.num : 0;
}

ols_data_t *ols_data_array_item(ols_data_array_t *array, size_t idx)
{
	ols_data_t *data;

	if (!array)
		return NULL;

	data = (idx < array->objects.num) ? array->objects.array[idx] : NULL;

	if (data)
		os_atomic_inc_long(&data->ref);
	return data;
}

size_t ols_data_array_push_back(ols_data_array_t *array, ols_data_t *obj)
{
	if (!array || !obj)
		return 0;

	os_atomic_inc_long(&obj->ref);
	return da_push_back(array->objects, &obj);
}

void ols_data_array_insert(ols_data_array_t *array, size_t idx, ols_data_t *obj)
{
	if (!array || !obj)
		return;

	os_atomic_inc_long(&obj->ref);
	da_insert(array->objects, idx, &obj);
}

void ols_data_array_push_back_array(ols_data_array_t *array,
				    ols_data_array_t *array2)
{
	if (!array || !array2)
		return;

	for (size_t i = 0; i < array2->objects.num; i++) {
		ols_data_t *obj = array2->objects.array[i];
		ols_data_addref(obj);
	}
	da_push_back_da(array->objects, array2->objects);
}

void ols_data_array_erase(ols_data_array_t *array, size_t idx)
{
	if (array) {
		ols_data_release(array->objects.array[idx]);
		da_erase(array->objects, idx);
	}
}

void ols_data_array_enum(ols_data_array_t *array,
			 void (*cb)(ols_data_t *data, void *param), void *param)
{
	if (array && cb) {
		for (size_t i = 0; i < array->objects.num; i++) {
			cb(array->objects.array[i], param);
		}
	}
}

/* ------------------------------------------------------------------------- */
/* Item status inspection */

bool ols_data_has_user_value(ols_data_t *data, const char *name)
{
	return data && ols_data_item_has_user_value(get_item(data, name));
}

bool ols_data_has_default_value(ols_data_t *data, const char *name)
{
	return data && ols_data_item_has_default_value(get_item(data, name));
}

bool ols_data_has_autoselect_value(ols_data_t *data, const char *name)
{
	return data && ols_data_item_has_autoselect_value(get_item(data, name));
}

bool ols_data_item_has_user_value(ols_data_item_t *item)
{
	return item && item->data_size;
}

bool ols_data_item_has_default_value(ols_data_item_t *item)
{
	return item && item->default_size;
}

bool ols_data_item_has_autoselect_value(ols_data_item_t *item)
{
	return item && item->autoselect_size;
}

/* ------------------------------------------------------------------------- */
/* Clearing data values */

void ols_data_unset_user_value(ols_data_t *data, const char *name)
{
	ols_data_item_unset_user_value(get_item(data, name));
}

void ols_data_unset_default_value(ols_data_t *data, const char *name)
{
	ols_data_item_unset_default_value(get_item(data, name));
}

void ols_data_unset_autoselect_value(ols_data_t *data, const char *name)
{
	ols_data_item_unset_autoselect_value(get_item(data, name));
}

void ols_data_item_unset_user_value(ols_data_item_t *item)
{
	if (!item || !item->data_size)
		return;

	void *old_non_user_data = get_default_data_ptr(item);

	item_data_release(item);
	item->data_size = 0;
	item->data_len = 0;

	if (item->default_size || item->autoselect_size)
		move_data(item, old_non_user_data, item,
			  get_default_data_ptr(item),
			  item->default_len + item->autoselect_size);
}

void ols_data_item_unset_default_value(ols_data_item_t *item)
{
	if (!item || !item->default_size)
		return;

	void *old_autoselect_data = get_autoselect_data_ptr(item);

	item_default_data_release(item);
	item->default_size = 0;
	item->default_len = 0;

	if (item->autoselect_size)
		move_data(item, old_autoselect_data, item,
			  get_autoselect_data_ptr(item), item->autoselect_size);
}

void ols_data_item_unset_autoselect_value(ols_data_item_t *item)
{
	if (!item || !item->autoselect_size)
		return;

	item_autoselect_data_release(item);
	item->autoselect_size = 0;
}

/* ------------------------------------------------------------------------- */
/* Item iteration */

ols_data_item_t *ols_data_first(ols_data_t *data)
{
	if (!data)
		return NULL;

	if (data->items)
		os_atomic_inc_long(&data->items->ref);
	return data->items;
}

ols_data_item_t *ols_data_item_byname(ols_data_t *data, const char *name)
{
	if (!data)
		return NULL;

	struct ols_data_item *item = get_item(data, name);
	if (item)
		os_atomic_inc_long(&item->ref);
	return item;
}

bool ols_data_item_next(ols_data_item_t **item)
{
	if (item && *item) {
		ols_data_item_t *next = (*item)->hh.next;
		ols_data_item_release(item);

		*item = next;

		if (next) {
			os_atomic_inc_long(&next->ref);
			return true;
		}
	}

	return false;
}

void ols_data_item_release(ols_data_item_t **item)
{
	if (item && *item) {
		long ref = os_atomic_dec_long(&(*item)->ref);
		if (!ref) {
			ols_data_item_destroy(*item);
			*item = NULL;
		}
	}
}

void ols_data_item_remove(ols_data_item_t **item)
{
	if (item && *item) {
		ols_data_item_detach(*item);
		ols_data_item_release(item);
	}
}

enum ols_data_type ols_data_item_gettype(ols_data_item_t *item)
{
	return item ? item->type : OLS_DATA_NULL;
}

enum ols_data_number_type ols_data_item_numtype(ols_data_item_t *item)
{
	struct ols_data_number *num;

	if (!item || item->type != OLS_DATA_NUMBER)
		return OLS_DATA_NUM_INVALID;

	num = get_item_data(item);
	if (!num)
		return OLS_DATA_NUM_INVALID;

	return num->type;
}

const char *ols_data_item_get_name(ols_data_item_t *item)
{
	if (!item)
		return NULL;

	return item->name;
}

void ols_data_item_set_string(ols_data_item_t **item, const char *val)
{
	ols_set_string(NULL, item, NULL, val, set_item);
}

void ols_data_item_set_int(ols_data_item_t **item, long long val)
{
	ols_set_int(NULL, item, NULL, val, set_item);
}

void ols_data_item_set_double(ols_data_item_t **item, double val)
{
	ols_set_double(NULL, item, NULL, val, set_item);
}

void ols_data_item_set_bool(ols_data_item_t **item, bool val)
{
	ols_set_bool(NULL, item, NULL, val, set_item);
}

void ols_data_item_set_obj(ols_data_item_t **item, ols_data_t *val)
{
	ols_set_obj(NULL, item, NULL, val, set_item);
}

void ols_data_item_set_array(ols_data_item_t **item, ols_data_array_t *val)
{
	ols_set_array(NULL, item, NULL, val, set_item);
}

void ols_data_item_set_default_string(ols_data_item_t **item, const char *val)
{
	ols_set_string(NULL, item, NULL, val, set_item_def);
}

void ols_data_item_set_default_int(ols_data_item_t **item, long long val)
{
	ols_set_int(NULL, item, NULL, val, set_item_def);
}

void ols_data_item_set_default_double(ols_data_item_t **item, double val)
{
	ols_set_double(NULL, item, NULL, val, set_item_def);
}

void ols_data_item_set_default_bool(ols_data_item_t **item, bool val)
{
	ols_set_bool(NULL, item, NULL, val, set_item_def);
}

void ols_data_item_set_default_obj(ols_data_item_t **item, ols_data_t *val)
{
	ols_set_obj(NULL, item, NULL, val, set_item_def);
}

void ols_data_item_set_default_array(ols_data_item_t **item,
				     ols_data_array_t *val)
{
	ols_set_array(NULL, item, NULL, val, set_item_def);
}

void ols_data_item_set_autoselect_string(ols_data_item_t **item,
					 const char *val)
{
	ols_set_string(NULL, item, NULL, val, set_item_auto);
}

void ols_data_item_set_autoselect_int(ols_data_item_t **item, long long val)
{
	ols_set_int(NULL, item, NULL, val, set_item_auto);
}

void ols_data_item_set_autoselect_double(ols_data_item_t **item, double val)
{
	ols_set_double(NULL, item, NULL, val, set_item_auto);
}

void ols_data_item_set_autoselect_bool(ols_data_item_t **item, bool val)
{
	ols_set_bool(NULL, item, NULL, val, set_item_auto);
}

void ols_data_item_set_autoselect_obj(ols_data_item_t **item, ols_data_t *val)
{
	ols_set_obj(NULL, item, NULL, val, set_item_auto);
}

void ols_data_item_set_autoselect_array(ols_data_item_t **item,
					ols_data_array_t *val)
{
	ols_set_array(NULL, item, NULL, val, set_item_auto);
}

static inline bool item_valid(struct ols_data_item *item,
			      enum ols_data_type type)
{
	return item && item->type == type;
}

typedef void *(*get_data_t)(ols_data_item_t *);

static inline const char *data_item_get_string(ols_data_item_t *item,
					       get_data_t get_data)
{
	const char *str;

	return item_valid(item, OLS_DATA_STRING) && (str = get_data(item)) ? str
									   : "";
}

static inline long long item_int(struct ols_data_item *item,
				 get_data_t get_data)
{
	struct ols_data_number *num;

	if (item && (num = get_data(item))) {
		return (num->type == OLS_DATA_NUM_INT)
			       ? num->int_val
			       : (long long)num->double_val;
	}

	return 0;
}

static inline long long data_item_get_int(ols_data_item_t *item,
					  get_data_t get_data)
{
	return item_int(item_valid(item, OLS_DATA_NUMBER) ? item : NULL,
			get_data);
}

static inline double item_double(struct ols_data_item *item,
				 get_data_t get_data)
{
	struct ols_data_number *num;

	if (item && (num = get_data(item))) {
		return (num->type == OLS_DATA_NUM_INT) ? (double)num->int_val
						       : num->double_val;
	}

	return 0.0;
}

static inline double data_item_get_double(ols_data_item_t *item,
					  get_data_t get_data)
{
	return item_double(item_valid(item, OLS_DATA_NUMBER) ? item : NULL,
			   get_data);
}

static inline bool data_item_get_bool(ols_data_item_t *item,
				      get_data_t get_data)
{
	bool *data;

	return item_valid(item, OLS_DATA_BOOLEAN) && (data = get_data(item))
		       ? *data
		       : false;
}

typedef ols_data_t *(*get_obj_t)(ols_data_item_t *);

static inline ols_data_t *data_item_get_obj(ols_data_item_t *item,
					    get_obj_t get_obj)
{
	ols_data_t *obj = item_valid(item, OLS_DATA_OBJECT) ? get_obj(item)
							    : NULL;

	if (obj)
		os_atomic_inc_long(&obj->ref);
	return obj;
}

typedef ols_data_array_t *(*get_array_t)(ols_data_item_t *);

static inline ols_data_array_t *data_item_get_array(ols_data_item_t *item,
						    get_array_t get_array)
{
	ols_data_array_t *array =
		item_valid(item, OLS_DATA_ARRAY) ? get_array(item) : NULL;

	if (array)
		os_atomic_inc_long(&array->ref);
	return array;
}

const char *ols_data_item_get_string(ols_data_item_t *item)
{
	return data_item_get_string(item, get_item_data);
}

long long ols_data_item_get_int(ols_data_item_t *item)
{
	return data_item_get_int(item, get_item_data);
}

double ols_data_item_get_double(ols_data_item_t *item)
{
	return data_item_get_double(item, get_item_data);
}

bool ols_data_item_get_bool(ols_data_item_t *item)
{
	return data_item_get_bool(item, get_item_data);
}

ols_data_t *ols_data_item_get_obj(ols_data_item_t *item)
{
	return data_item_get_obj(item, get_item_obj);
}

ols_data_array_t *ols_data_item_get_array(ols_data_item_t *item)
{
	return data_item_get_array(item, get_item_array);
}

const char *ols_data_item_get_default_string(ols_data_item_t *item)
{
	return data_item_get_string(item, get_item_default_data);
}

long long ols_data_item_get_default_int(ols_data_item_t *item)
{
	return data_item_get_int(item, get_item_default_data);
}

double ols_data_item_get_default_double(ols_data_item_t *item)
{
	return data_item_get_double(item, get_item_default_data);
}

bool ols_data_item_get_default_bool(ols_data_item_t *item)
{
	return data_item_get_bool(item, get_item_default_data);
}

ols_data_t *ols_data_item_get_default_obj(ols_data_item_t *item)
{
	return data_item_get_obj(item, get_item_default_obj);
}

ols_data_array_t *ols_data_item_get_default_array(ols_data_item_t *item)
{
	return data_item_get_array(item, get_item_default_array);
}

const char *ols_data_item_get_autoselect_string(ols_data_item_t *item)
{
	return data_item_get_string(item, get_item_autoselect_data);
}

long long ols_data_item_get_autoselect_int(ols_data_item_t *item)
{
	return data_item_get_int(item, get_item_autoselect_data);
}

double ols_data_item_get_autoselect_double(ols_data_item_t *item)
{
	return data_item_get_double(item, get_item_autoselect_data);
}

bool ols_data_item_get_autoselect_bool(ols_data_item_t *item)
{
	return data_item_get_bool(item, get_item_autoselect_data);
}

ols_data_t *ols_data_item_get_autoselect_obj(ols_data_item_t *item)
{
	return data_item_get_obj(item, get_item_autoselect_obj);
}

ols_data_array_t *ols_data_item_get_autoselect_array(ols_data_item_t *item)
{
	return data_item_get_array(item, get_item_autoselect_array);
}

