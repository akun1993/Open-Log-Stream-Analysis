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
#include "util/darray.h"
#include "ols-internal.h"
#include "ols-properties.h"

static inline void *get_property_data(struct ols_property *prop);

/* ------------------------------------------------------------------------- */

struct float_data {
	double min, max, step;
	enum ols_number_type type;
	char *suffix;
};

struct int_data {
	int min, max, step;
	enum ols_number_type type;
	char *suffix;
};

struct list_item {
	char *name;
	bool disabled;

	union {
		char *str;
		long long ll;
		double d;
		bool b;
	};
};

struct path_data {
	char *filter;
	char *default_path;
	enum ols_path_type type;
};

struct text_data {
	enum ols_text_type type;
	bool monospace;
	enum ols_text_info_type info_type;
	bool info_word_wrap;
};

struct list_data {
	DARRAY(struct list_item) items;
	enum ols_combo_type type;
	enum ols_combo_format format;
};

struct editable_list_data {
	enum ols_editable_list_type type;
	char *filter;
	char *default_path;
};

struct button_data {
	ols_property_clicked_t callback;
	enum ols_button_type type;
	char *url;
};

struct frame_rate_option {
	char *name;
	char *description;
};

struct frame_rate_range {
	struct media_frames_per_second min_time;
	struct media_frames_per_second max_time;
};

struct frame_rate_data {
	DARRAY(struct frame_rate_option) extra_options;
	DARRAY(struct frame_rate_range) ranges;
};

struct group_data {
	enum ols_group_type type;
	ols_properties_t *content;
};

static inline void path_data_free(struct path_data *data)
{
	bfree(data->default_path);
	if (data->type == OLS_PATH_FILE)
		bfree(data->filter);
}

static inline void editable_list_data_free(struct editable_list_data *data)
{
	bfree(data->default_path);
	bfree(data->filter);
}

static inline void list_item_free(struct list_data *data,
				  struct list_item *item)
{
	bfree(item->name);
	if (data->format == OLS_COMBO_FORMAT_STRING)
		bfree(item->str);
}

static inline void list_data_free(struct list_data *data)
{
	for (size_t i = 0; i < data->items.num; i++)
		list_item_free(data, data->items.array + i);

	da_free(data->items);
}

static inline void frame_rate_data_options_free(struct frame_rate_data *data)
{
	for (size_t i = 0; i < data->extra_options.num; i++) {
		struct frame_rate_option *opt = &data->extra_options.array[i];
		bfree(opt->name);
		bfree(opt->description);
	}

	da_resize(data->extra_options, 0);
}

static inline void frame_rate_data_ranges_free(struct frame_rate_data *data)
{
	da_resize(data->ranges, 0);
}

static inline void frame_rate_data_free(struct frame_rate_data *data)
{
	frame_rate_data_options_free(data);
	frame_rate_data_ranges_free(data);

	da_free(data->extra_options);
	da_free(data->ranges);
}

static inline void group_data_free(struct group_data *data)
{
	ols_properties_destroy(data->content);
}

static inline void int_data_free(struct int_data *data)
{
	if (data->suffix)
		bfree(data->suffix);
}

static inline void float_data_free(struct float_data *data)
{
	if (data->suffix)
		bfree(data->suffix);
}

static inline void button_data_free(struct button_data *data)
{
	if (data->url)
		bfree(data->url);
}

struct ols_properties;

struct ols_property {
	char *name;
	char *desc;
	char *long_desc;
	void *priv;
	enum ols_property_type type;
	bool visible;
	bool enabled;

	struct ols_properties *parent;

	ols_property_modified_t modified;
	ols_property_modified2_t modified2;

	UT_hash_handle hh;
};

struct ols_properties {
	void *param;
	void (*destroy)(void *param);
	uint32_t flags;
	uint32_t groups;

	struct ols_property *properties;
	struct ols_property *parent;
};

ols_properties_t *ols_properties_create(void)
{
	struct ols_properties *props;
	props = bzalloc(sizeof(struct ols_properties));
	return props;
}

void ols_properties_set_param(ols_properties_t *props, void *param,
			      void (*destroy)(void *param))
{
	if (!props)
		return;

	if (props->param && props->destroy)
		props->destroy(props->param);

	props->param = param;
	props->destroy = destroy;
}

void ols_properties_set_flags(ols_properties_t *props, uint32_t flags)
{
	if (props)
		props->flags = flags;
}

uint32_t ols_properties_get_flags(ols_properties_t *props)
{
	return props ? props->flags : 0;
}

void *ols_properties_get_param(ols_properties_t *props)
{
	return props ? props->param : NULL;
}

ols_properties_t *ols_properties_create_param(void *param,
					      void (*destroy)(void *param))
{
	struct ols_properties *props = ols_properties_create();
	ols_properties_set_param(props, param, destroy);
	return props;
}

static void ols_property_destroy(struct ols_property *property)
{
	if (property->type == OLS_PROPERTY_LIST)
		list_data_free(get_property_data(property));
	else if (property->type == OLS_PROPERTY_PATH)
		path_data_free(get_property_data(property));
	else if (property->type == OLS_PROPERTY_EDITABLE_LIST)
		editable_list_data_free(get_property_data(property));
	else if (property->type == OLS_PROPERTY_FRAME_RATE)
		frame_rate_data_free(get_property_data(property));
	else if (property->type == OLS_PROPERTY_GROUP)
		group_data_free(get_property_data(property));
	else if (property->type == OLS_PROPERTY_INT)
		int_data_free(get_property_data(property));
	else if (property->type == OLS_PROPERTY_FLOAT)
		float_data_free(get_property_data(property));
	else if (property->type == OLS_PROPERTY_BUTTON)
		button_data_free(get_property_data(property));

	bfree(property->name);
	bfree(property->desc);
	bfree(property->long_desc);
	bfree(property);
}

void ols_properties_destroy(ols_properties_t *props)
{
	if (props) {
		struct ols_property *p, *tmp;

		if (props->destroy && props->param)
			props->destroy(props->param);

		HASH_ITER (hh, props->properties, p, tmp) {
			HASH_DEL(props->properties, p);
			ols_property_destroy(p);
		}

		bfree(props);
	}
}

ols_property_t *ols_properties_first(ols_properties_t *props)
{
	return (props != NULL) ? props->properties : NULL;
}

ols_property_t *ols_properties_get(ols_properties_t *props, const char *name)
{
	struct ols_property *property, *tmp;

	if (!props)
		return NULL;

	HASH_FIND_STR(props->properties, name, property);
	if (property)
		return property;

	if (!props->groups)
		return NULL;

	/* Recursively check groups as well, if any */
	HASH_ITER (hh, props->properties, property, tmp) {
		if (property->type != OLS_PROPERTY_GROUP)
			continue;

		ols_properties_t *group = ols_property_group_content(property);
		ols_property_t *found = ols_properties_get(group, name);
		if (found)
			return found;
	}

	return NULL;
}

ols_properties_t *ols_properties_get_parent(ols_properties_t *props)
{
	return props->parent ? props->parent->parent : NULL;
}

void ols_properties_remove_by_name(ols_properties_t *props, const char *name)
{
	if (!props)
		return;

	struct ols_property *cur, *tmp;

	HASH_FIND_STR(props->properties, name, cur);

	if (cur) {
		HASH_DELETE(hh, props->properties, cur);

		if (cur->type == OLS_PROPERTY_GROUP)
			props->groups--;

		ols_property_destroy(cur);
		return;
	}

	if (!props->groups)
		return;

	HASH_ITER (hh, props->properties, cur, tmp) {
		if (cur->type != OLS_PROPERTY_GROUP)
			continue;

		ols_properties_remove_by_name(ols_property_group_content(cur),
					      name);
	}
}

typedef DARRAY(struct ols_property *) ols_property_da_t;

void ols_properties_apply_settings_internal(
	ols_properties_t *props, ols_property_da_t *properties_with_callback)
{
	struct ols_property *p = props->properties;

	while (p) {
		if (p->type == OLS_PROPERTY_GROUP) {
			ols_properties_apply_settings_internal(
				ols_property_group_content(p),
				properties_with_callback);
		}
		if (p->modified || p->modified2)
			da_push_back((*properties_with_callback), &p);

		p = p->hh.next;
	}
}

void ols_properties_apply_settings(ols_properties_t *props,
				   ols_data_t *settings)
{
	if (!props)
		return;

	ols_property_da_t properties_with_callback;
	da_init(properties_with_callback);

	ols_properties_apply_settings_internal(props,
					       &properties_with_callback);

	while (properties_with_callback.num > 0) {
		struct ols_property *p = *(struct ols_property **)da_end(
			properties_with_callback);
		if (p->modified)
			p->modified(props, p, settings);
		else if (p->modified2)
			p->modified2(p->priv, props, p, settings);
		da_pop_back(properties_with_callback);
	}

	da_free(properties_with_callback);
}

/* ------------------------------------------------------------------------- */

static inline size_t get_property_size(enum ols_property_type type)
{
	switch (type) {
	case OLS_PROPERTY_INVALID:
		return 0;
	case OLS_PROPERTY_BOOL:
		return 0;
	case OLS_PROPERTY_INT:
		return sizeof(struct int_data);
	case OLS_PROPERTY_FLOAT:
		return sizeof(struct float_data);
	case OLS_PROPERTY_TEXT:
		return sizeof(struct text_data);
	case OLS_PROPERTY_PATH:
		return sizeof(struct path_data);
	case OLS_PROPERTY_LIST:
		return sizeof(struct list_data);
	case OLS_PROPERTY_COLOR:
		return 0;
	case OLS_PROPERTY_BUTTON:
		return sizeof(struct button_data);
	case OLS_PROPERTY_FONT:
		return 0;
	case OLS_PROPERTY_EDITABLE_LIST:
		return sizeof(struct editable_list_data);
	case OLS_PROPERTY_FRAME_RATE:
		return sizeof(struct frame_rate_data);
	case OLS_PROPERTY_GROUP:
		return sizeof(struct group_data);
	case OLS_PROPERTY_COLOR_ALPHA:
		return 0;
	}

	return 0;
}

static inline struct ols_property *new_prop(struct ols_properties *props,
					    const char *name, const char *desc,
					    enum ols_property_type type)
{
	size_t data_size = get_property_size(type);
	struct ols_property *p;

	p = bzalloc(sizeof(struct ols_property) + data_size);
	p->parent = props;
	p->enabled = true;
	p->visible = true;
	p->type = type;
	p->name = bstrdup(name);
	p->desc = bstrdup(desc);

	HASH_ADD_STR(props->properties, name, p);

	return p;
}

static inline ols_properties_t *get_topmost_parent(ols_properties_t *props)
{
	ols_properties_t *parent = props;
	ols_properties_t *last_parent = parent;
	while (parent) {
		last_parent = parent;
		parent = ols_properties_get_parent(parent);
	}
	return last_parent;
}

static inline bool contains_prop(struct ols_properties *props, const char *name)
{
	struct ols_property *p, *tmp;
	HASH_FIND_STR(props->properties, name, p);

	if (p) {
		blog(LOG_WARNING, "Property '%s' exists", name);
		return true;
	}

	if (!props->groups)
		return false;

	HASH_ITER (hh, props->properties, p, tmp) {
		if (p->type != OLS_PROPERTY_GROUP)
			continue;
		if (contains_prop(ols_property_group_content(p), name))
			return true;
	}

	return false;
}

static inline bool has_prop(struct ols_properties *props, const char *name)
{
	return contains_prop(get_topmost_parent(props), name);
}

static inline void *get_property_data(struct ols_property *prop)
{
	return (uint8_t *)prop + sizeof(struct ols_property);
}

static inline void *get_type_data(struct ols_property *prop,
				  enum ols_property_type type)
{
	if (!prop || prop->type != type)
		return NULL;

	return get_property_data(prop);
}

ols_property_t *ols_properties_add_bool(ols_properties_t *props,
					const char *name, const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;
	return new_prop(props, name, desc, OLS_PROPERTY_BOOL);
}

static ols_property_t *add_int(ols_properties_t *props, const char *name,
			       const char *desc, int min, int max, int step,
			       enum ols_number_type type)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct ols_property *p = new_prop(props, name, desc, OLS_PROPERTY_INT);
	struct int_data *data = get_property_data(p);
	data->min = min;
	data->max = max;
	data->step = step;
	data->type = type;
	return p;
}

static ols_property_t *add_flt(ols_properties_t *props, const char *name,
			       const char *desc, double min, double max,
			       double step, enum ols_number_type type)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct ols_property *p =
		new_prop(props, name, desc, OLS_PROPERTY_FLOAT);
	struct float_data *data = get_property_data(p);
	data->min = min;
	data->max = max;
	data->step = step;
	data->type = type;
	return p;
}

ols_property_t *ols_properties_add_int(ols_properties_t *props,
				       const char *name, const char *desc,
				       int min, int max, int step)
{
	return add_int(props, name, desc, min, max, step, OLS_NUMBER_SCROLLER);
}

ols_property_t *ols_properties_add_float(ols_properties_t *props,
					 const char *name, const char *desc,
					 double min, double max, double step)
{
	return add_flt(props, name, desc, min, max, step, OLS_NUMBER_SCROLLER);
}

ols_property_t *ols_properties_add_int_slider(ols_properties_t *props,
					      const char *name,
					      const char *desc, int min,
					      int max, int step)
{
	return add_int(props, name, desc, min, max, step, OLS_NUMBER_SLIDER);
}

ols_property_t *ols_properties_add_float_slider(ols_properties_t *props,
						const char *name,
						const char *desc, double min,
						double max, double step)
{
	return add_flt(props, name, desc, min, max, step, OLS_NUMBER_SLIDER);
}

ols_property_t *ols_properties_add_text(ols_properties_t *props,
					const char *name, const char *desc,
					enum ols_text_type type)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct ols_property *p = new_prop(props, name, desc, OLS_PROPERTY_TEXT);
	struct text_data *data = get_property_data(p);
	data->type = type;
	data->info_type = OLS_TEXT_INFO_NORMAL;
	data->info_word_wrap = true;
	return p;
}

ols_property_t *ols_properties_add_path(ols_properties_t *props,
					const char *name, const char *desc,
					enum ols_path_type type,
					const char *filter,
					const char *default_path)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct ols_property *p = new_prop(props, name, desc, OLS_PROPERTY_PATH);
	struct path_data *data = get_property_data(p);
	data->type = type;
	data->default_path = bstrdup(default_path);

	if (data->type == OLS_PATH_FILE)
		data->filter = bstrdup(filter);

	return p;
}

ols_property_t *ols_properties_add_list(ols_properties_t *props,
					const char *name, const char *desc,
					enum ols_combo_type type,
					enum ols_combo_format format)
{
	if (!props || has_prop(props, name))
		return NULL;

	if (type == OLS_COMBO_TYPE_EDITABLE &&
	    format != OLS_COMBO_FORMAT_STRING) {
		blog(LOG_WARNING,
		     "List '%s', error: Editable combo boxes "
		     "must be of the 'string' type",
		     name);
		return NULL;
	}

	struct ols_property *p = new_prop(props, name, desc, OLS_PROPERTY_LIST);
	struct list_data *data = get_property_data(p);
	data->format = format;
	data->type = type;

	return p;
}

ols_property_t *ols_properties_add_color(ols_properties_t *props,
					 const char *name, const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;
	return new_prop(props, name, desc, OLS_PROPERTY_COLOR);
}

ols_property_t *ols_properties_add_color_alpha(ols_properties_t *props,
					       const char *name,
					       const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;
	return new_prop(props, name, desc, OLS_PROPERTY_COLOR_ALPHA);
}

ols_property_t *ols_properties_add_button(ols_properties_t *props,
					  const char *name, const char *text,
					  ols_property_clicked_t callback)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct ols_property *p =
		new_prop(props, name, text, OLS_PROPERTY_BUTTON);
	struct button_data *data = get_property_data(p);
	data->callback = callback;
	return p;
}

ols_property_t *ols_properties_add_button2(ols_properties_t *props,
					   const char *name, const char *text,
					   ols_property_clicked_t callback,
					   void *priv)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct ols_property *p =
		new_prop(props, name, text, OLS_PROPERTY_BUTTON);
	struct button_data *data = get_property_data(p);
	data->callback = callback;
	p->priv = priv;
	return p;
}

ols_property_t *ols_properties_add_font(ols_properties_t *props,
					const char *name, const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;
	return new_prop(props, name, desc, OLS_PROPERTY_FONT);
}

ols_property_t *
ols_properties_add_editable_list(ols_properties_t *props, const char *name,
				 const char *desc,
				 enum ols_editable_list_type type,
				 const char *filter, const char *default_path)
{
	if (!props || has_prop(props, name))
		return NULL;
	struct ols_property *p =
		new_prop(props, name, desc, OLS_PROPERTY_EDITABLE_LIST);

	struct editable_list_data *data = get_property_data(p);
	data->type = type;
	data->filter = bstrdup(filter);
	data->default_path = bstrdup(default_path);
	return p;
}

ols_property_t *ols_properties_add_frame_rate(ols_properties_t *props,
					      const char *name,
					      const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct ols_property *p =
		new_prop(props, name, desc, OLS_PROPERTY_FRAME_RATE);

	struct frame_rate_data *data = get_property_data(p);
	da_init(data->extra_options);
	da_init(data->ranges);
	return p;
}

static bool check_property_group_recursion(ols_properties_t *parent,
					   ols_properties_t *group)
{
	/* Scan the group for the parent. */
	ols_property_t *p, *tmp;

	HASH_ITER (hh, group->properties, p, tmp) {
		if (p->type != OLS_PROPERTY_GROUP)
			continue;

		ols_properties_t *cprops = ols_property_group_content(p);
		if (cprops == parent) {
			/* Contains find_props */
			return true;
		} else if (cprops == group) {
			/* Contains self, shouldn't be possible but
				 * lets verify anyway. */
			return true;
		}
		if (check_property_group_recursion(parent, cprops))
			return true;
	}

	return false;
}

static bool check_property_group_duplicates(ols_properties_t *parent,
					    ols_properties_t *group)
{
	ols_property_t *p, *tmp;

	HASH_ITER (hh, group->properties, p, tmp) {
		if (has_prop(parent, p->name))
			return true;
	}

	return false;
}

ols_property_t *ols_properties_add_group(ols_properties_t *props,
					 const char *name, const char *desc,
					 enum ols_group_type type,
					 ols_properties_t *group)
{
	if (!props || has_prop(props, name))
		return NULL;
	if (!group)
		return NULL;

	/* Prevent recursion. */
	if (props == group)
		return NULL;
	if (check_property_group_recursion(props, group))
		return NULL;

	/* Prevent duplicate properties */
	if (check_property_group_duplicates(props, group))
		return NULL;

	ols_property_t *p = new_prop(props, name, desc, OLS_PROPERTY_GROUP);
	props->groups++;
	group->parent = p;

	struct group_data *data = get_property_data(p);
	data->type = type;
	data->content = group;
	return p;
}

/* ------------------------------------------------------------------------- */

static inline bool is_combo(struct ols_property *p)
{
	return p->type == OLS_PROPERTY_LIST;
}

static inline struct list_data *get_list_data(struct ols_property *p)
{
	if (!p || !is_combo(p))
		return NULL;

	return get_property_data(p);
}

static inline struct list_data *get_list_fmt_data(struct ols_property *p,
						  enum ols_combo_format format)
{
	struct list_data *data = get_list_data(p);
	return (data && data->format == format) ? data : NULL;
}

/* ------------------------------------------------------------------------- */

bool ols_property_next(ols_property_t **p)
{
	if (!p || !*p)
		return false;

	*p = (*p)->hh.next;
	return *p != NULL;
}

void ols_property_set_modified_callback(ols_property_t *p,
					ols_property_modified_t modified)
{
	if (p)
		p->modified = modified;
}

void ols_property_set_modified_callback2(ols_property_t *p,
					 ols_property_modified2_t modified2,
					 void *priv)
{
	if (p) {
		p->modified2 = modified2;
		p->priv = priv;
	}
}

bool ols_property_modified(ols_property_t *p, ols_data_t *settings)
{
	if (p) {
		if (p->modified) {
			ols_properties_t *top = get_topmost_parent(p->parent);
			return p->modified(top, p, settings);
		} else if (p->modified2) {
			ols_properties_t *top = get_topmost_parent(p->parent);
			return p->modified2(p->priv, top, p, settings);
		}
	}
	return false;
}

bool ols_property_button_clicked(ols_property_t *p, void *obj)
{
	struct ols_context_data *context = obj;
	if (p) {
		struct button_data *data =
			get_type_data(p, OLS_PROPERTY_BUTTON);
		if (data && data->callback) {
			ols_properties_t *top = get_topmost_parent(p->parent);
			if (p->priv)
				return data->callback(top, p, p->priv);
			return data->callback(top, p,
					      (context ? context->data : NULL));
		}
	}

	return false;
}

void ols_property_set_visible(ols_property_t *p, bool visible)
{
	if (p)
		p->visible = visible;
}

void ols_property_set_enabled(ols_property_t *p, bool enabled)
{
	if (p)
		p->enabled = enabled;
}

void ols_property_set_description(ols_property_t *p, const char *description)
{
	if (p) {
		bfree(p->desc);
		p->desc = description && *description ? bstrdup(description)
						      : NULL;
	}
}

void ols_property_set_long_description(ols_property_t *p, const char *long_desc)
{
	if (p) {
		bfree(p->long_desc);
		p->long_desc = long_desc && *long_desc ? bstrdup(long_desc)
						       : NULL;
	}
}

const char *ols_property_name(ols_property_t *p)
{
	return p ? p->name : NULL;
}

const char *ols_property_description(ols_property_t *p)
{
	return p ? p->desc : NULL;
}

const char *ols_property_long_description(ols_property_t *p)
{
	return p ? p->long_desc : NULL;
}

enum ols_property_type ols_property_get_type(ols_property_t *p)
{
	return p ? p->type : OLS_PROPERTY_INVALID;
}

bool ols_property_enabled(ols_property_t *p)
{
	return p ? p->enabled : false;
}

bool ols_property_visible(ols_property_t *p)
{
	return p ? p->visible : false;
}

int ols_property_int_min(ols_property_t *p)
{
	struct int_data *data = get_type_data(p, OLS_PROPERTY_INT);
	return data ? data->min : 0;
}

int ols_property_int_max(ols_property_t *p)
{
	struct int_data *data = get_type_data(p, OLS_PROPERTY_INT);
	return data ? data->max : 0;
}

int ols_property_int_step(ols_property_t *p)
{
	struct int_data *data = get_type_data(p, OLS_PROPERTY_INT);
	return data ? data->step : 0;
}

enum ols_number_type ols_property_int_type(ols_property_t *p)
{
	struct int_data *data = get_type_data(p, OLS_PROPERTY_INT);
	return data ? data->type : OLS_NUMBER_SCROLLER;
}

const char *ols_property_int_suffix(ols_property_t *p)
{
	struct int_data *data = get_type_data(p, OLS_PROPERTY_INT);
	return data ? data->suffix : NULL;
}

double ols_property_float_min(ols_property_t *p)
{
	struct float_data *data = get_type_data(p, OLS_PROPERTY_FLOAT);
	return data ? data->min : 0;
}

double ols_property_float_max(ols_property_t *p)
{
	struct float_data *data = get_type_data(p, OLS_PROPERTY_FLOAT);
	return data ? data->max : 0;
}

double ols_property_float_step(ols_property_t *p)
{
	struct float_data *data = get_type_data(p, OLS_PROPERTY_FLOAT);
	return data ? data->step : 0;
}

const char *ols_property_float_suffix(ols_property_t *p)
{
	struct float_data *data = get_type_data(p, OLS_PROPERTY_FLOAT);
	return data ? data->suffix : NULL;
}

enum ols_number_type ols_property_float_type(ols_property_t *p)
{
	struct float_data *data = get_type_data(p, OLS_PROPERTY_FLOAT);
	return data ? data->type : OLS_NUMBER_SCROLLER;
}

enum ols_text_type ols_property_text_type(ols_property_t *p)
{
	struct text_data *data = get_type_data(p, OLS_PROPERTY_TEXT);
	return data ? data->type : OLS_TEXT_DEFAULT;
}

bool ols_property_text_monospace(ols_property_t *p)
{
	struct text_data *data = get_type_data(p, OLS_PROPERTY_TEXT);
	return data ? data->monospace : false;
}

enum ols_text_info_type ols_property_text_info_type(ols_property_t *p)
{
	struct text_data *data = get_type_data(p, OLS_PROPERTY_TEXT);
	return data ? data->info_type : OLS_TEXT_INFO_NORMAL;
}

bool ols_property_text_info_word_wrap(ols_property_t *p)
{
	struct text_data *data = get_type_data(p, OLS_PROPERTY_TEXT);
	return data ? data->info_word_wrap : true;
}

enum ols_path_type ols_property_path_type(ols_property_t *p)
{
	struct path_data *data = get_type_data(p, OLS_PROPERTY_PATH);
	return data ? data->type : OLS_PATH_DIRECTORY;
}

const char *ols_property_path_filter(ols_property_t *p)
{
	struct path_data *data = get_type_data(p, OLS_PROPERTY_PATH);
	return data ? data->filter : NULL;
}

const char *ols_property_path_default_path(ols_property_t *p)
{
	struct path_data *data = get_type_data(p, OLS_PROPERTY_PATH);
	return data ? data->default_path : NULL;
}

enum ols_combo_type ols_property_list_type(ols_property_t *p)
{
	struct list_data *data = get_list_data(p);
	return data ? data->type : OLS_COMBO_TYPE_INVALID;
}

enum ols_combo_format ols_property_list_format(ols_property_t *p)
{
	struct list_data *data = get_list_data(p);
	return data ? data->format : OLS_COMBO_FORMAT_INVALID;
}

void ols_property_int_set_limits(ols_property_t *p, int min, int max, int step)
{
	struct int_data *data = get_type_data(p, OLS_PROPERTY_INT);
	if (!data)
		return;

	data->min = min;
	data->max = max;
	data->step = step;
}

void ols_property_float_set_limits(ols_property_t *p, double min, double max,
				   double step)
{
	struct float_data *data = get_type_data(p, OLS_PROPERTY_FLOAT);
	if (!data)
		return;

	data->min = min;
	data->max = max;
	data->step = step;
}

void ols_property_int_set_suffix(ols_property_t *p, const char *suffix)
{
	struct int_data *data = get_type_data(p, OLS_PROPERTY_INT);
	if (!data)
		return;

	bfree(data->suffix);
	data->suffix = bstrdup(suffix);
}

void ols_property_float_set_suffix(ols_property_t *p, const char *suffix)
{
	struct float_data *data = get_type_data(p, OLS_PROPERTY_FLOAT);
	if (!data)
		return;

	bfree(data->suffix);
	data->suffix = bstrdup(suffix);
}

void ols_property_text_set_monospace(ols_property_t *p, bool monospace)
{
	struct text_data *data = get_type_data(p, OLS_PROPERTY_TEXT);
	if (!data)
		return;

	data->monospace = monospace;
}

void ols_property_text_set_info_type(ols_property_t *p,
				     enum ols_text_info_type type)
{
	struct text_data *data = get_type_data(p, OLS_PROPERTY_TEXT);
	if (!data)
		return;

	data->info_type = type;
}

void ols_property_text_set_info_word_wrap(ols_property_t *p, bool word_wrap)
{
	struct text_data *data = get_type_data(p, OLS_PROPERTY_TEXT);
	if (!data)
		return;

	data->info_word_wrap = word_wrap;
}

void ols_property_button_set_type(ols_property_t *p, enum ols_button_type type)
{
	struct button_data *data = get_type_data(p, OLS_PROPERTY_BUTTON);
	if (!data)
		return;

	data->type = type;
}

void ols_property_button_set_url(ols_property_t *p, char *url)
{
	struct button_data *data = get_type_data(p, OLS_PROPERTY_BUTTON);
	if (!data)
		return;

	data->url = bstrdup(url);
}

void ols_property_list_clear(ols_property_t *p)
{
	struct list_data *data = get_list_data(p);
	if (data)
		list_data_free(data);
}

static size_t add_item(struct list_data *data, const char *name,
		       const void *val)
{
	struct list_item item = {NULL};
	item.name = bstrdup(name);

	if (data->format == OLS_COMBO_FORMAT_INT)
		item.ll = *(const long long *)val;
	else if (data->format == OLS_COMBO_FORMAT_FLOAT)
		item.d = *(const double *)val;
	else if (data->format == OLS_COMBO_FORMAT_BOOL)
		item.b = *(const bool *)val;
	else
		item.str = bstrdup(val);

	return da_push_back(data->items, &item);
}

static void insert_item(struct list_data *data, size_t idx, const char *name,
			const void *val)
{
	struct list_item item = {NULL};
	item.name = bstrdup(name);

	if (data->format == OLS_COMBO_FORMAT_INT)
		item.ll = *(const long long *)val;
	else if (data->format == OLS_COMBO_FORMAT_FLOAT)
		item.d = *(const double *)val;
	else if (data->format == OLS_COMBO_FORMAT_BOOL)
		item.b = *(const bool *)val;
	else
		item.str = bstrdup(val);

	da_insert(data->items, idx, &item);
}

size_t ols_property_list_add_string(ols_property_t *p, const char *name,
				    const char *val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OLS_COMBO_FORMAT_STRING)
		return add_item(data, name, val);
	return 0;
}

size_t ols_property_list_add_int(ols_property_t *p, const char *name,
				 long long val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OLS_COMBO_FORMAT_INT)
		return add_item(data, name, &val);
	return 0;
}

size_t ols_property_list_add_float(ols_property_t *p, const char *name,
				   double val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OLS_COMBO_FORMAT_FLOAT)
		return add_item(data, name, &val);
	return 0;
}

size_t ols_property_list_add_bool(ols_property_t *p, const char *name, bool val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OLS_COMBO_FORMAT_BOOL)
		return add_item(data, name, &val);
	return 0;
}

void ols_property_list_insert_string(ols_property_t *p, size_t idx,
				     const char *name, const char *val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OLS_COMBO_FORMAT_STRING)
		insert_item(data, idx, name, val);
}

void ols_property_list_insert_int(ols_property_t *p, size_t idx,
				  const char *name, long long val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OLS_COMBO_FORMAT_INT)
		insert_item(data, idx, name, &val);
}

void ols_property_list_insert_float(ols_property_t *p, size_t idx,
				    const char *name, double val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OLS_COMBO_FORMAT_FLOAT)
		insert_item(data, idx, name, &val);
}

void ols_property_list_insert_bool(ols_property_t *p, size_t idx,
				   const char *name, bool val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OLS_COMBO_FORMAT_BOOL)
		insert_item(data, idx, name, &val);
}

void ols_property_list_item_remove(ols_property_t *p, size_t idx)
{
	struct list_data *data = get_list_data(p);
	if (data && idx < data->items.num) {
		list_item_free(data, data->items.array + idx);
		da_erase(data->items, idx);
	}
}

size_t ols_property_list_item_count(ols_property_t *p)
{
	struct list_data *data = get_list_data(p);
	return data ? data->items.num : 0;
}

bool ols_property_list_item_disabled(ols_property_t *p, size_t idx)
{
	struct list_data *data = get_list_data(p);
	return (data && idx < data->items.num) ? data->items.array[idx].disabled
					       : false;
}

void ols_property_list_item_disable(ols_property_t *p, size_t idx,
				    bool disabled)
{
	struct list_data *data = get_list_data(p);
	if (!data || idx >= data->items.num)
		return;
	data->items.array[idx].disabled = disabled;
}

const char *ols_property_list_item_name(ols_property_t *p, size_t idx)
{
	struct list_data *data = get_list_data(p);
	return (data && idx < data->items.num) ? data->items.array[idx].name
					       : NULL;
}

const char *ols_property_list_item_string(ols_property_t *p, size_t idx)
{
	struct list_data *data = get_list_fmt_data(p, OLS_COMBO_FORMAT_STRING);
	return (data && idx < data->items.num) ? data->items.array[idx].str
					       : NULL;
}

long long ols_property_list_item_int(ols_property_t *p, size_t idx)
{
	struct list_data *data = get_list_fmt_data(p, OLS_COMBO_FORMAT_INT);
	return (data && idx < data->items.num) ? data->items.array[idx].ll : 0;
}

double ols_property_list_item_float(ols_property_t *p, size_t idx)
{
	struct list_data *data = get_list_fmt_data(p, OLS_COMBO_FORMAT_FLOAT);
	return (data && idx < data->items.num) ? data->items.array[idx].d : 0.0;
}

bool ols_property_list_item_bool(ols_property_t *p, size_t idx)
{
	struct list_data *data = get_list_fmt_data(p, OLS_COMBO_FORMAT_BOOL);
	return (data && idx < data->items.num) ? data->items.array[idx].d
					       : false;
}

enum ols_editable_list_type ols_property_editable_list_type(ols_property_t *p)
{
	struct editable_list_data *data =
		get_type_data(p, OLS_PROPERTY_EDITABLE_LIST);
	return data ? data->type : OLS_EDITABLE_LIST_TYPE_STRINGS;
}

const char *ols_property_editable_list_filter(ols_property_t *p)
{
	struct editable_list_data *data =
		get_type_data(p, OLS_PROPERTY_EDITABLE_LIST);
	return data ? data->filter : NULL;
}

const char *ols_property_editable_list_default_path(ols_property_t *p)
{
	struct editable_list_data *data =
		get_type_data(p, OLS_PROPERTY_EDITABLE_LIST);
	return data ? data->default_path : NULL;
}

/* ------------------------------------------------------------------------- */
/* OLS_PROPERTY_FRAME_RATE */

void ols_property_frame_rate_clear(ols_property_t *p)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	if (!data)
		return;

	frame_rate_data_options_free(data);
	frame_rate_data_ranges_free(data);
}

void ols_property_frame_rate_options_clear(ols_property_t *p)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	if (!data)
		return;

	frame_rate_data_options_free(data);
}

void ols_property_frame_rate_fps_ranges_clear(ols_property_t *p)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	if (!data)
		return;

	frame_rate_data_ranges_free(data);
}

size_t ols_property_frame_rate_option_add(ols_property_t *p, const char *name,
					  const char *description)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	if (!data)
		return DARRAY_INVALID;

	struct frame_rate_option *opt = da_push_back_new(data->extra_options);

	opt->name = bstrdup(name);
	opt->description = bstrdup(description);

	return data->extra_options.num - 1;
}

size_t ols_property_frame_rate_fps_range_add(ols_property_t *p,
					     struct media_frames_per_second min,
					     struct media_frames_per_second max)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	if (!data)
		return DARRAY_INVALID;

	struct frame_rate_range *rng = da_push_back_new(data->ranges);

	rng->min_time = min;
	rng->max_time = max;

	return data->ranges.num - 1;
}

void ols_property_frame_rate_option_insert(ols_property_t *p, size_t idx,
					   const char *name,
					   const char *description)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	if (!data)
		return;

	struct frame_rate_option *opt = da_insert_new(data->extra_options, idx);

	opt->name = bstrdup(name);
	opt->description = bstrdup(description);
}

void ols_property_frame_rate_fps_range_insert(
	ols_property_t *p, size_t idx, struct media_frames_per_second min,
	struct media_frames_per_second max)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	if (!data)
		return;

	struct frame_rate_range *rng = da_insert_new(data->ranges, idx);

	rng->min_time = min;
	rng->max_time = max;
}

size_t ols_property_frame_rate_options_count(ols_property_t *p)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	return data ? data->extra_options.num : 0;
}

const char *ols_property_frame_rate_option_name(ols_property_t *p, size_t idx)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	return data && data->extra_options.num > idx
		       ? data->extra_options.array[idx].name
		       : NULL;
}

const char *ols_property_frame_rate_option_description(ols_property_t *p,
						       size_t idx)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	return data && data->extra_options.num > idx
		       ? data->extra_options.array[idx].description
		       : NULL;
}

size_t ols_property_frame_rate_fps_ranges_count(ols_property_t *p)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	return data ? data->ranges.num : 0;
}

struct media_frames_per_second
ols_property_frame_rate_fps_range_min(ols_property_t *p, size_t idx)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	return data && data->ranges.num > idx
		       ? data->ranges.array[idx].min_time
		       : (struct media_frames_per_second){0};
}
struct media_frames_per_second
ols_property_frame_rate_fps_range_max(ols_property_t *p, size_t idx)
{
	struct frame_rate_data *data =
		get_type_data(p, OLS_PROPERTY_FRAME_RATE);
	return data && data->ranges.num > idx
		       ? data->ranges.array[idx].max_time
		       : (struct media_frames_per_second){0};
}

enum ols_text_type ols_proprety_text_type(ols_property_t *p)
{
	return ols_property_text_type(p);
}

enum ols_group_type ols_property_group_type(ols_property_t *p)
{
	struct group_data *data = get_type_data(p, OLS_PROPERTY_GROUP);
	return data ? data->type : OLS_COMBO_INVALID;
}

ols_properties_t *ols_property_group_content(ols_property_t *p)
{
	struct group_data *data = get_type_data(p, OLS_PROPERTY_GROUP);
	return data ? data->content : NULL;
}

enum ols_button_type ols_property_button_type(ols_property_t *p)
{
	struct button_data *data = get_type_data(p, OLS_PROPERTY_BUTTON);
	return data ? data->type : OLS_BUTTON_DEFAULT;
}

const char *ols_property_button_url(ols_property_t *p)
{
	struct button_data *data = get_type_data(p, OLS_PROPERTY_BUTTON);
	return data ? data->url : "";
}
