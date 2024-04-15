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

#include <inttypes.h>
#include <math.h>
#include "util/threading.h"
#include "util/platform.h"
#include "util/util_uint64.h"
#include "callback/calldata.h"

#include "ols.h"
#include "ols-internal.h"

#define get_weak(source) ((ols_weak_source_t *)source->context.control)

static bool filter_compatible(ols_source_t *source, ols_source_t *filter);

static inline bool data_valid(const struct ols_source *source, const char *f)
{
	return ols_source_valid(source, f) && source->context.data;
}

static inline bool deinterlacing_enabled(const struct ols_source *source)
{
	return source->deinterlace_mode != OLS_DEINTERLACE_MODE_DISABLE;
}

static inline bool destroying(const struct ols_source *source)
{
	return os_atomic_load_long(&source->destroying);
}

struct ols_source_info *get_source_info(const char *id)
{
	for (size_t i = 0; i < ols->source_types.num; i++) {
		struct ols_source_info *info = &ols->source_types.array[i];
		if (strcmp(info->id, id) == 0)
			return info;
	}

	return NULL;
}

struct ols_source_info *get_source_info2(const char *unversioned_id,
					 uint32_t ver)
{
	for (size_t i = 0; i < ols->source_types.num; i++) {
		struct ols_source_info *info = &ols->source_types.array[i];
		if (strcmp(info->unversioned_id, unversioned_id) == 0 &&
		    info->version == ver)
			return info;
	}

	return NULL;
}

static const char *source_signals[] = {
	"void destroy(ptr source)",
	"void remove(ptr source)",
	"void update(ptr source)",
	"void save(ptr source)",
	"void load(ptr source)",
	"void activate(ptr source)",
	"void deactivate(ptr source)",
	"void show(ptr source)",
	"void hide(ptr source)",
	"void enable(ptr source, bool enabled)",
	"void rename(ptr source, string new_name, string prev_name)",
	"void update_properties(ptr source)",
	"void update_flags(ptr source, int flags)",
	"void filter_add(ptr source, ptr filter)",
	"void filter_remove(ptr source, ptr filter)",
	"void reorder_filters(ptr source)",
	NULL,
};

bool ols_source_init_context(struct ols_source *source, ols_data_t *settings,
			     const char *name, const char *uuid,
			     ols_data_t *hotkey_data, bool private)
{
	if (!ols_context_data_init(&source->context, OLS_OBJ_TYPE_SOURCE,
				   settings, name, uuid, hotkey_data, private))
		return false;

	return signal_handler_add_array(source->context.signals,
					source_signals);
}

const char *ols_source_get_display_name(const char *id)
{
	const struct ols_source_info *info = get_source_info(id);
	return (info != NULL) ? info->get_name(info->type_data) : NULL;
}



extern char *find_libols_data_file(const char *file);

/* internal initialization */
static bool ols_source_init(struct ols_source *source)
{
	source->user_volume = 1.0f;
	source->volume = 1.0f;
	source->sync_offset = 0;
	source->balance = 0.5f;

	pthread_mutex_init_value(&source->filter_mutex);
	pthread_mutex_init_value(&source->async_mutex);
	pthread_mutex_init_value(&source->caption_cb_mutex);


	if (pthread_mutex_init_recursive(&source->filter_mutex) != 0)
		return false;
	if (pthread_mutex_init(&source->audio_buf_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&source->audio_actions_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&source->audio_cb_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&source->audio_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init_recursive(&source->async_mutex) != 0)
		return false;
	if (pthread_mutex_init(&source->caption_cb_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&source->media_actions_mutex, NULL) != 0)
		return false;

	if (is_audio_source(source) || is_composite_source(source))
		allocate_audio_output_buffer(source);
	if (source->info.audio_mix)
		allocate_audio_mix_buffer(source);

	if (source->info.type == OLS_SOURCE_TYPE_TRANSITION) {
		if (!ols_transition_init(source))
			return false;
	}

	ols_context_init_control(&source->context, source,
				 (ols_destroy_cb)ols_source_destroy);

	source->deinterlace_top_first = true;
	source->audio_mixers = 0xFF;

	source->private_settings = ols_data_create();
	return true;
}

static void ols_source_init_finalize(struct ols_source *source)
{
	if (is_audio_source(source)) {
		pthread_mutex_lock(&ols->data.audio_sources_mutex);

		source->next_audio_source = ols->data.first_audio_source;
		source->prev_next_audio_source = &ols->data.first_audio_source;
		if (ols->data.first_audio_source)
			ols->data.first_audio_source->prev_next_audio_source =
				&source->next_audio_source;
		ols->data.first_audio_source = source;

		pthread_mutex_unlock(&ols->data.audio_sources_mutex);
	}

	if (!source->context.private) {
		ols_context_data_insert_name(&source->context,
					     &ols->data.sources_mutex,
					     &ols->data.public_sources);
	}
	ols_context_data_insert_uuid(&source->context, &ols->data.sources_mutex,
				     &ols->data.sources);
}


static ols_source_t *
ols_source_create_internal(const char *id, const char *name, const char *uuid,
			   ols_data_t *settings, ols_data_t *hotkey_data,
			   bool private, uint32_t last_ols_ver)
{
	struct ols_source *source = bzalloc(sizeof(struct ols_source));

	const struct ols_source_info *info = get_source_info(id);
	if (!info) {
		blog(LOG_ERROR, "Source ID '%s' not found", id);

		source->info.id = bstrdup(id);
		source->owns_info_id = true;
		source->info.unversioned_id = bstrdup(source->info.id);
	} else {
		source->info = *info;

		/* Always mark filters as private so they aren't found by
		 * source enum/search functions.
		 *
		 * XXX: Fix design flaws with filters */
		if (info->type == OLS_SOURCE_TYPE_FILTER)
		private = true;
	}

	source->mute_unmute_key = OLS_INVALID_HOTKEY_PAIR_ID;
	source->push_to_mute_key = OLS_INVALID_HOTKEY_ID;
	source->push_to_talk_key = OLS_INVALID_HOTKEY_ID;
	source->last_ols_ver = last_ols_ver;

	if (!ols_source_init_context(source, settings, name, uuid, hotkey_data,
				     private))
		goto fail;

	if (info) {
		if (info->get_defaults) {
			info->get_defaults(source->context.settings);
		}
		if (info->get_defaults2) {
			info->get_defaults2(info->type_data,
					    source->context.settings);
		}
	}

	if (!ols_source_init(source))
		goto fail;

	if (!private)
		ols_source_init_audio_hotkeys(source);

	/* allow the source to be created even if creation fails so that the
	 * user's data doesn't become lost */
	if (info && info->create)
		source->context.data =
			info->create(source->context.settings, source);
	if ((!info || info->create) && !source->context.data)
		blog(LOG_ERROR, "Failed to create source '%s'!", name);

	blog(LOG_DEBUG, "%ssource '%s' (%s) created", private ? "private " : "",
	     name, id);

	source->flags = source->default_flags;
	source->enabled = true;

	ols_source_init_finalize(source);
	if (!private) {
		ols_source_dosignal(source, "source_create", NULL);
	}

	return source;

fail:
	blog(LOG_ERROR, "ols_source_create failed");
	ols_source_destroy(source);
	return NULL;
}

ols_source_t *ols_source_create(const char *id, const char *name,
				ols_data_t *settings, ols_data_t *hotkey_data)
{
	return ols_source_create_internal(id, name, NULL, settings, hotkey_data,
					  false, LIBOLS_API_VER);
}

ols_source_t *ols_source_create_private(const char *id, const char *name,
					ols_data_t *settings)
{
	return ols_source_create_internal(id, name, NULL, settings, NULL, true,
					  LIBOLS_API_VER);
}

ols_source_t *ols_source_create_set_last_ver(const char *id, const char *name,
					     const char *uuid,
					     ols_data_t *settings,
					     ols_data_t *hotkey_data,
					     uint32_t last_ols_ver,
					     bool is_private)
{
	return ols_source_create_internal(id, name, uuid, settings, hotkey_data,
					  is_private, last_ols_ver);
}

static char *get_new_filter_name(ols_source_t *dst, const char *name)
{
	struct dstr new_name = {0};
	int inc = 0;

	dstr_copy(&new_name, name);

	for (;;) {
		ols_source_t *existing_filter =
			ols_source_get_filter_by_name(dst, new_name.array);
		if (!existing_filter)
			break;

		ols_source_release(existing_filter);

		dstr_printf(&new_name, "%s %d", name, ++inc + 1);
	}

	return new_name.array;
}

static void duplicate_filters(ols_source_t *dst, ols_source_t *src,
			      bool private)
{
	DARRAY(ols_source_t *) filters;

	da_init(filters);

	pthread_mutex_lock(&src->filter_mutex);
	da_reserve(filters, src->filters.num);
	for (size_t i = 0; i < src->filters.num; i++) {
		ols_source_t *s = ols_source_get_ref(src->filters.array[i]);
		if (s)
			da_push_back(filters, &s);
	}
	pthread_mutex_unlock(&src->filter_mutex);

	for (size_t i = filters.num; i > 0; i--) {
		ols_source_t *src_filter = filters.array[i - 1];
		char *new_name =
			get_new_filter_name(dst, src_filter->context.name);
		bool enabled = ols_source_enabled(src_filter);

		ols_source_t *dst_filter =
			ols_source_duplicate(src_filter, new_name, private);
		ols_source_set_enabled(dst_filter, enabled);

		bfree(new_name);
		ols_source_filter_add(dst, dst_filter);
		ols_source_release(dst_filter);
		ols_source_release(src_filter);
	}

	da_free(filters);
}

void ols_source_copy_filters(ols_source_t *dst, ols_source_t *src)
{
	if (!ols_source_valid(dst, "ols_source_copy_filters"))
		return;
	if (!ols_source_valid(src, "ols_source_copy_filters"))
		return;

	duplicate_filters(dst, src, dst->context.private);
}

static void duplicate_filter(ols_source_t *dst, ols_source_t *filter)
{
	if (!filter_compatible(dst, filter))
		return;

	char *new_name = get_new_filter_name(dst, filter->context.name);
	bool enabled = ols_source_enabled(filter);

	ols_source_t *dst_filter = ols_source_duplicate(filter, new_name, true);
	ols_source_set_enabled(dst_filter, enabled);

	bfree(new_name);
	ols_source_filter_add(dst, dst_filter);
	ols_source_release(dst_filter);
}

void ols_source_copy_single_filter(ols_source_t *dst, ols_source_t *filter)
{
	if (!ols_source_valid(dst, "ols_source_copy_single_filter"))
		return;
	if (!ols_source_valid(filter, "ols_source_copy_single_filter"))
		return;

	duplicate_filter(dst, filter);
}

ols_source_t *ols_source_duplicate(ols_source_t *source, const char *new_name,
				   bool create_private)
{
	ols_source_t *new_source;
	ols_data_t *settings;

	if (!ols_source_valid(source, "ols_source_duplicate"))
		return NULL;


	if ((source->info.output_flags & OLS_SOURCE_DO_NOT_DUPLICATE) != 0) {
		return ols_source_get_ref(source);
	}

	settings = ols_data_create();
	ols_data_apply(settings, source->context.settings);

	new_source = create_private
			     ? ols_source_create_private(source->info.id,
							 new_name, settings)
			     : ols_source_create(source->info.id, new_name,
						 settings, NULL);

	new_source->flags = source->flags;

	ols_data_apply(new_source->private_settings, source->private_settings);

	if (source->info.type != OLS_SOURCE_TYPE_FILTER)
		duplicate_filters(new_source, source, create_private);

	ols_data_release(settings);
	return new_source;
}


static void ols_source_destroy_defer(struct ols_source *source);

void ols_source_destroy(struct ols_source *source)
{
	if (!ols_source_valid(source, "ols_source_destroy"))
		return;

	if (os_atomic_set_long(&source->destroying, true) == true) {
		blog(LOG_ERROR, "Double destroy just occurred. "
				"Something called addref on a source "
				"after it was already fully released, "
				"I guess.");
		return;
	}

	ols_context_data_remove_uuid(&source->context, &ols->data.sources);
	if (!source->context.private)
		ols_context_data_remove_name(&source->context,
					     &ols->data.public_sources);

	/* defer source destroy */
	os_task_queue_queue_task(ols->destruction_task_thread,
				 (os_task_t)ols_source_destroy_defer, source);
}

static void ols_source_destroy_defer(struct ols_source *source)
{
	size_t i;

	/* prevents the destruction of sources if destroy triggered inside of
	 * a video tick call */
	ols_context_wait(&source->context);

	ols_source_dosignal(source, "source_destroy", "destroy");

	if (source->context.data) {
		source->info.destroy(source->context.data);
		source->context.data = NULL;
	}

	blog(LOG_DEBUG, "%ssource '%s' destroyed",
	     source->context.private ? "private " : "", source->context.name);


	ols_data_release(source->private_settings);
	ols_context_data_free(&source->context);

	if (source->owns_info_id) {
		bfree((void *)source->info.id);
		bfree((void *)source->info.unversioned_id);
	}

	bfree(source);
}

void ols_source_addref(ols_source_t *source)
{
	if (!source)
		return;

	ols_ref_addref(&source->context.control->ref);
}

void ols_source_release(ols_source_t *source)
{
	if (!ols && source) {
		blog(LOG_WARNING, "Tried to release a source when the OLS "
				  "core is shut down!");
		return;
	}

	if (!source)
		return;

	ols_weak_source_t *control = get_weak(source);
	if (ols_ref_release(&control->ref)) {
		ols_source_destroy(source);
		ols_weak_source_release(control);
	}
}

void ols_weak_source_addref(ols_weak_source_t *weak)
{
	if (!weak)
		return;

	ols_weak_ref_addref(&weak->ref);
}

void ols_weak_source_release(ols_weak_source_t *weak)
{
	if (!weak)
		return;

	if (ols_weak_ref_release(&weak->ref))
		bfree(weak);
}

ols_source_t *ols_source_get_ref(ols_source_t *source)
{
	if (!source)
		return NULL;

	return ols_weak_source_get_source(get_weak(source));
}

ols_weak_source_t *ols_source_get_weak_source(ols_source_t *source)
{
	if (!source)
		return NULL;

	ols_weak_source_t *weak = get_weak(source);
	ols_weak_source_addref(weak);
	return weak;
}

ols_source_t *ols_weak_source_get_source(ols_weak_source_t *weak)
{
	if (!weak)
		return NULL;

	if (ols_weak_ref_get_ref(&weak->ref))
		return weak->source;

	return NULL;
}

bool ols_weak_source_expired(ols_weak_source_t *weak)
{
	return weak ? ols_weak_ref_expired(&weak->ref) : true;
}

bool ols_weak_source_references_source(ols_weak_source_t *weak,
				       ols_source_t *source)
{
	return weak && source && weak->source == source;
}

void ols_source_remove(ols_source_t *source)
{
	if (!ols_source_valid(source, "ols_source_remove"))
		return;

	if (!source->removed) {
		ols_source_t *s = ols_source_get_ref(source);
		if (s) {
			s->removed = true;
			ols_source_dosignal(s, "source_remove", "remove");
			ols_source_release(s);
		}
	}
}

bool ols_source_removed(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_removed") ? source->removed
							      : true;
}

static inline ols_data_t *get_defaults(const struct ols_source_info *info)
{
	ols_data_t *settings = ols_data_create();
	if (info->get_defaults2)
		info->get_defaults2(info->type_data, settings);
	else if (info->get_defaults)
		info->get_defaults(settings);
	return settings;
}

ols_data_t *ols_source_settings(const char *id)
{
	const struct ols_source_info *info = get_source_info(id);
	return (info) ? get_defaults(info) : NULL;
}

ols_data_t *ols_get_source_defaults(const char *id)
{
	const struct ols_source_info *info = get_source_info(id);
	return info ? get_defaults(info) : NULL;
}

ols_properties_t *ols_get_source_properties(const char *id)
{
	const struct ols_source_info *info = get_source_info(id);
	if (info && (info->get_properties || info->get_properties2)) {
		ols_data_t *defaults = get_defaults(info);
		ols_properties_t *props;

		if (info->get_properties2)
			props = info->get_properties2(NULL, info->type_data);
		else
			props = info->get_properties(NULL);

		ols_properties_apply_settings(props, defaults);
		ols_data_release(defaults);
		return props;
	}
	return NULL;
}


bool ols_is_source_configurable(const char *id)
{
	const struct ols_source_info *info = get_source_info(id);
	return info && (info->get_properties || info->get_properties2);
}

bool ols_source_configurable(const ols_source_t *source)
{
	return data_valid(source, "ols_source_configurable") &&
	       (source->info.get_properties || source->info.get_properties2);
}

ols_properties_t *ols_source_properties(const ols_source_t *source)
{
	if (!data_valid(source, "ols_source_properties"))
		return NULL;

	if (source->info.get_properties2) {
		ols_properties_t *props;
		props = source->info.get_properties2(source->context.data,
						     source->info.type_data);
		ols_properties_apply_settings(props, source->context.settings);
		return props;

	} else if (source->info.get_properties) {
		ols_properties_t *props;
		props = source->info.get_properties(source->context.data);
		ols_properties_apply_settings(props, source->context.settings);
		return props;
	}

	return NULL;
}

uint32_t ols_source_get_output_flags(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_get_output_flags")
		       ? source->info.output_flags
		       : 0;
}

uint32_t ols_get_source_output_flags(const char *id)
{
	const struct ols_source_info *info = get_source_info(id);
	return info ? info->output_flags : 0;
}

static void ols_source_deferred_update(ols_source_t *source)
{
	if (source->context.data && source->info.update) {
		long count = os_atomic_load_long(&source->defer_update_count);
		source->info.update(source->context.data,
				    source->context.settings);
		os_atomic_compare_swap_long(&source->defer_update_count, count,
					    0);
		ols_source_dosignal(source, "source_update", "update");
	}
}

void ols_source_update(ols_source_t *source, ols_data_t *settings)
{
	if (!ols_source_valid(source, "ols_source_update"))
		return;

	if (settings) {
		ols_data_apply(source->context.settings, settings);
	}

	if (source->context.data && source->info.update) {
		source->info.update(source->context.data,
				    source->context.settings);
		ols_source_dosignal(source, "source_update", "update");
	}
}

void ols_source_reset_settings(ols_source_t *source, ols_data_t *settings)
{
	if (!ols_source_valid(source, "ols_source_reset_settings"))
		return;

	ols_data_clear(source->context.settings);
	ols_source_update(source, settings);
}

void ols_source_update_properties(ols_source_t *source)
{
	if (!ols_source_valid(source, "ols_source_update_properties"))
		return;

	ols_source_dosignal(source, NULL, "update_properties");
}


static void activate_source(ols_source_t *source)
{
	if (source->context.data && source->info.activate)
		source->info.activate(source->context.data);
	ols_source_dosignal(source, "source_activate", "activate");
}

static void deactivate_source(ols_source_t *source)
{
	if (source->context.data && source->info.deactivate)
		source->info.deactivate(source->context.data);
	ols_source_dosignal(source, "source_deactivate", "deactivate");
}


static void activate_tree(ols_source_t *parent, ols_source_t *child,
			  void *param)
{
	os_atomic_inc_long(&child->activate_refs);

	UNUSED_PARAMETER(parent);
	UNUSED_PARAMETER(param);
}

static void deactivate_tree(ols_source_t *parent, ols_source_t *child,
			    void *param)
{
	os_atomic_dec_long(&child->activate_refs);

	UNUSED_PARAMETER(parent);
	UNUSED_PARAMETER(param);
}

static void show_tree(ols_source_t *parent, ols_source_t *child, void *param)
{
	os_atomic_inc_long(&child->show_refs);

	UNUSED_PARAMETER(parent);
	UNUSED_PARAMETER(param);
}

static void hide_tree(ols_source_t *parent, ols_source_t *child, void *param)
{
	os_atomic_dec_long(&child->show_refs);

	UNUSED_PARAMETER(parent);
	UNUSED_PARAMETER(param);
}

void ols_source_activate(ols_source_t *source, enum view_type type)
{
	if (!ols_source_valid(source, "ols_source_activate"))
		return;

	os_atomic_inc_long(&source->show_refs);
	ols_source_enum_active_tree(source, show_tree, NULL);

	if (type == MAIN_VIEW) {
		os_atomic_inc_long(&source->activate_refs);
		ols_source_enum_active_tree(source, activate_tree, NULL);
	}
}

void ols_source_deactivate(ols_source_t *source, enum view_type type)
{
	if (!ols_source_valid(source, "ols_source_deactivate"))
		return;

	if (os_atomic_load_long(&source->show_refs) > 0) {
		os_atomic_dec_long(&source->show_refs);
		ols_source_enum_active_tree(source, hide_tree, NULL);
	}

	if (type == MAIN_VIEW) {
		if (os_atomic_load_long(&source->activate_refs) > 0) {
			os_atomic_dec_long(&source->activate_refs);
			ols_source_enum_active_tree(source, deactivate_tree,
						    NULL);
		}
	}
}


static inline uint64_t uint64_diff(uint64_t ts1, uint64_t ts2)
{
	return (ts1 < ts2) ? (ts2 - ts1) : (ts1 - ts2);
}



ols_data_t *ols_source_get_settings(const ols_source_t *source)
{
	if (!ols_source_valid(source, "ols_source_get_settings"))
		return NULL;

	ols_data_addref(source->context.settings);
	return source->context.settings;
}


const char *ols_source_get_name(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_get_name")
		       ? source->context.name
		       : NULL;
}

const char *ols_source_get_uuid(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_get_uuid")
		       ? source->context.uuid
		       : NULL;
}

void ols_source_set_name(ols_source_t *source, const char *name)
{
	if (!ols_source_valid(source, "ols_source_set_name"))
		return;

	if (!name || !*name || !source->context.name ||
	    strcmp(name, source->context.name) != 0) {
		struct calldata data;
		char *prev_name = bstrdup(source->context.name);

		if (!source->context.private) {
			ols_context_data_setname_ht(&source->context, name,
						    &ols->data.public_sources);
		} else {
			ols_context_data_setname(&source->context, name);
		}

		calldata_init(&data);
		calldata_set_ptr(&data, "source", source);
		calldata_set_string(&data, "new_name", source->context.name);
		calldata_set_string(&data, "prev_name", prev_name);
		if (!source->context.private)
			signal_handler_signal(ols->signals, "source_rename",
					      &data);
		signal_handler_signal(source->context.signals, "rename", &data);
		calldata_free(&data);
		bfree(prev_name);
	}
}

enum ols_source_type ols_source_get_type(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_get_type")
		       ? source->info.type
		       : OLS_SOURCE_TYPE_INPUT;
}

const char *ols_source_get_id(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_get_id") ? source->info.id
							     : NULL;
}

const char *ols_source_get_unversioned_id(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_get_unversioned_id")
		       ? source->info.unversioned_id
		       : NULL;
}


signal_handler_t *ols_source_get_signal_handler(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_get_signal_handler")
		       ? source->context.signals
		       : NULL;
}

proc_handler_t *ols_source_get_proc_handler(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_get_proc_handler")
		       ? source->context.procs
		       : NULL;
}


void ols_source_save(ols_source_t *source)
{
	if (!data_valid(source, "ols_source_save"))
		return;

	ols_source_dosignal(source, "source_save", "save");

	if (source->info.save)
		source->info.save(source->context.data,
				  source->context.settings);
}

void ols_source_load(ols_source_t *source)
{
	if (!data_valid(source, "ols_source_load"))
		return;
	if (source->info.load)
		source->info.load(source->context.data,
				  source->context.settings);

	ols_source_dosignal(source, "source_load", "load");
}

bool ols_source_active(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_active")
		       ? source->activate_refs != 0
		       : false;
}

bool ols_source_showing(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_showing")
		       ? source->show_refs != 0
		       : false;
}

static inline void signal_flags_updated(ols_source_t *source)
{
	struct calldata data;
	uint8_t stack[128];

	calldata_init_fixed(&data, stack, sizeof(stack));
	calldata_set_ptr(&data, "source", source);
	calldata_set_int(&data, "flags", source->flags);

	signal_handler_signal(source->context.signals, "update_flags", &data);
}

void ols_source_set_flags(ols_source_t *source, uint32_t flags)
{
	if (!ols_source_valid(source, "ols_source_set_flags"))
		return;

	if (flags != source->flags) {
		source->flags = flags;
		signal_flags_updated(source);
	}
}

void ols_source_set_default_flags(ols_source_t *source, uint32_t flags)
{
	if (!ols_source_valid(source, "ols_source_set_default_flags"))
		return;

	source->default_flags = flags;
}

uint32_t ols_source_get_flags(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_get_flags") ? source->flags
								: 0;
}

ols_data_t *ols_source_get_private_settings(ols_source_t *source)
{
	if (!ols_ptr_valid(source, "ols_source_get_private_settings"))
		return NULL;

	ols_data_addref(source->private_settings);
	return source->private_settings;
}


uint32_t ols_source_get_last_ols_version(const ols_source_t *source)
{
	return ols_source_valid(source, "ols_source_get_last_ols_version")
		       ? source->last_ols_ver
		       : 0;
}

enum ols_icon_type ols_source_get_icon_type(const char *id)
{
	const struct ols_source_info *info = get_source_info(id);
	return (info) ? info->icon_type : OLS_ICON_TYPE_UNKNOWN;
}



