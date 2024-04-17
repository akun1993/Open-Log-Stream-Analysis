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

#pragma once
#include <stdbool.h> 
#include "util/c99defs.h"
#include "util/darray.h"
#include "util/deque.h"
#include "util/dstr.h"
#include "util/threading.h"
#include "util/platform.h"
#include "util/profiler.h"
#include "util/task.h"
#include "util/uthash.h"
#include "callback/signal.h"
#include "callback/proc.h"


#include "ols.h"

#include <olsversion.h>

/* Custom helpers for the UUID hash table */
#define HASH_FIND_UUID(head, uuid, out) \
	HASH_FIND(hh_uuid, head, uuid, UUID_STR_LENGTH, out)
#define HASH_ADD_UUID(head, uuid_field, add) \
	HASH_ADD(hh_uuid, head, uuid_field[0], UUID_STR_LENGTH, add)

struct tick_callback {
	void (*tick)(void *param, float seconds);
	void *param;
};


/* ------------------------------------------------------------------------- */
/* validity checks */

static inline bool ols_object_valid(const void *obj, const char *f,
				    const char *t)
{
	if (!obj) {
		blog(LOG_DEBUG, "%s: Null '%s' parameter", f, t);
		return false;
	}

	return true;
}

#define ols_ptr_valid(ptr, func) ols_object_valid(ptr, func, #ptr)
#define ols_source_valid ols_ptr_valid
#define ols_output_valid ols_ptr_valid
#define ols_service_valid ols_ptr_valid

/* ------------------------------------------------------------------------- */
/* modules */

struct ols_module {
	char *mod_name;
	const char *file;
	char *bin_path;
	char *data_path;
	void *module;
	bool loaded;

	bool (*load)(void);
	void (*unload)(void);
	void (*post_load)(void);
	void (*set_locale)(const char *locale);
	bool (*get_string)(const char *lookup_string,
			   const char **translated_string);
	void (*free_locale)(void);
	uint32_t (*ver)(void);
	void (*set_pointer)(ols_module_t *module);
	const char *(*name)(void);
	const char *(*description)(void);
	const char *(*author)(void);

	struct ols_module *next;
};

extern void free_module(struct ols_module *mod);

struct ols_module_path {
	char *bin;
	char *data;
};

static inline void free_module_path(struct ols_module_path *omp)
{
	if (omp) {
		bfree(omp->bin);
		bfree(omp->data);
	}
}

static inline bool check_path(const char *data, const char *path,
			      struct dstr *output)
{
	dstr_copy(output, path);
	dstr_cat(output, data);

	return os_file_exists(output->array);
}

/* ------------------------------------------------------------------------- */
/* hotkeys */

struct ols_hotkey {
	ols_hotkey_id id;
	char *name;
	char *description;

	ols_hotkey_func func;
	void *data;
	int pressed;

	ols_hotkey_registerer_t registerer_type;
	void *registerer;

	ols_hotkey_id pair_partner_id;

	UT_hash_handle hh;
};

struct ols_hotkey_pair {
	ols_hotkey_pair_id pair_id;
	ols_hotkey_id id[2];
	ols_hotkey_active_func func[2];
	bool pressed0;
	bool pressed1;
	void *data[2];

	UT_hash_handle hh;
};

typedef struct ols_hotkey_pair ols_hotkey_pair_t;

typedef struct ols_hotkeys_platform ols_hotkeys_platform_t;

void *ols_hotkey_thread(void *param);

struct ols_core_hotkeys;
bool ols_hotkeys_platform_init(struct ols_core_hotkeys *hotkeys);
void ols_hotkeys_platform_free(struct ols_core_hotkeys *hotkeys);
bool ols_hotkeys_platform_is_pressed(ols_hotkeys_platform_t *context,
				     ols_key_t key);

const char *ols_get_hotkey_translation(ols_key_t key, const char *def);

struct ols_context_data;
void ols_hotkeys_context_release(struct ols_context_data *context);

void ols_hotkeys_free(void);

struct ols_hotkey_binding {
	ols_key_combination_t key;
	bool pressed;
	bool modifiers_match;

	ols_hotkey_id hotkey_id;
	ols_hotkey_t *hotkey;
};

struct ols_hotkey_name_map_item;
void ols_hotkey_name_map_free(void);

/* ------------------------------------------------------------------------- */
/* views */

// struct ols_view {
// 	pthread_mutex_t channels_mutex;
// 	ols_source_t *channels[MAX_CHANNELS];
// };

// extern bool ols_view_init(struct ols_view *view);
// extern void ols_view_free(struct ols_view *view);

/* ------------------------------------------------------------------------- */
/* core */


struct ols_task_info {
	ols_task_t task;
	void *param;
};


/* user sources, output channels, and displays */
struct ols_core_data {
	/* Hash tables (uthash) */
	struct ols_source *sources;        /* Lookup by UUID (hh_uuid) */
	struct ols_source *public_sources; /* Lookup by name (hh) */

	/* Linked lists */
	struct ols_output *first_output;

	pthread_mutex_t sources_mutex;
	pthread_mutex_t outputs_mutex;
	DARRAY(struct tick_callback) tick_callbacks;

	//struct ols_view main_view;

	long long unnamed_index;

	ols_data_t *private_data;

	volatile bool valid;

	DARRAY(char *) protocols;
	DARRAY(ols_source_t *) sources_to_tick;
};

/* user hotkeys */
struct ols_core_hotkeys {
	pthread_mutex_t mutex;
	ols_hotkey_t *hotkeys;
	ols_hotkey_id next_id;
	ols_hotkey_pair_t *hotkey_pairs;
	ols_hotkey_pair_id next_pair_id;

	pthread_t hotkey_thread;
	bool hotkey_thread_initialized;
	os_event_t *stop_event;
	bool thread_disable_press;
	bool strict_modifiers;
	bool reroute_hotkeys;
	DARRAY(ols_hotkey_binding_t) bindings;

	ols_hotkey_callback_router_func router_func;
	void *router_func_data;

	ols_hotkeys_platform_t *platform_context;

	pthread_once_t name_map_init_token;
	struct ols_hotkey_name_map_item *name_map;

	signal_handler_t *signals;
};

typedef DARRAY(struct ols_source_info) ols_source_info_array_t;

struct ols_core {
	struct ols_module *first_module;
	DARRAY(struct ols_module_path) module_paths;
	DARRAY(char *) safe_modules;

	ols_source_info_array_t source_types;
	ols_source_info_array_t input_types;
	ols_source_info_array_t filter_types;
	DARRAY(struct ols_output_info) output_types;

	signal_handler_t *signals;
	proc_handler_t *procs;

	char *locale;
	char *module_config_path;
	bool name_store_owned;
	profiler_name_store_t *name_store;

	/* segmented into multiple sub-structures to keep things a bit more
	 * clean and organized */
	struct ols_core_data data;
	struct ols_core_hotkeys hotkeys;
	os_task_queue_t *destruction_task_thread;
	ols_task_handler_t ui_task_handler;
};

extern struct ols_core *ols;

/* ------------------------------------------------------------------------- */
/* ols shared context data */

struct ols_weak_ref {
	volatile long refs;
	volatile long weak_refs;
};

struct ols_weak_object {
	struct ols_weak_ref ref;
	struct ols_context_data *object;
};

typedef void (*ols_destroy_cb)(void *obj);

struct ols_context_data {
	char *name;
	const char *uuid;
	void *data;
	ols_data_t *settings;
	signal_handler_t *signals;
	proc_handler_t *procs;
	enum ols_obj_type type;

	struct ols_weak_object *control;
	ols_destroy_cb destroy;

	DARRAY(ols_hotkey_id) hotkeys;
	DARRAY(ols_hotkey_pair_id) hotkey_pairs;
	ols_data_t *hotkey_data;

	pthread_mutex_t *mutex;

	UT_hash_handle hh;
	UT_hash_handle hh_uuid;
	bool private;
};

extern bool ols_context_data_init(struct ols_context_data *context,
				  enum ols_obj_type type, ols_data_t *settings,
				  const char *name, const char *uuid,
				  ols_data_t *hotkey_data, bool private);
extern void ols_context_init_control(struct ols_context_data *context,
				     void *object, ols_destroy_cb destroy);
extern void ols_context_data_free(struct ols_context_data *context);

extern void ols_context_data_insert(struct ols_context_data *context,
				    pthread_mutex_t *mutex, void *first);
extern void ols_context_data_insert_name(struct ols_context_data *context,
					 pthread_mutex_t *mutex, void *first);
extern void ols_context_data_insert_uuid(struct ols_context_data *context,
					 pthread_mutex_t *mutex,
					 void *first_uuid);

extern void ols_context_data_remove(struct ols_context_data *context);
extern void ols_context_data_remove_name(struct ols_context_data *context,
					 void *phead);
extern void ols_context_data_remove_uuid(struct ols_context_data *context,
					 void *puuid_head);

extern void ols_context_wait(struct ols_context_data *context);

extern void ols_context_data_setname(struct ols_context_data *context,
				     const char *name);

extern void ols_context_data_setname_ht(struct ols_context_data *context,
					const char *name, void *phead);

/* ------------------------------------------------------------------------- */
/* ref-counting  */

static inline void ols_ref_addref(struct ols_weak_ref *ref)
{
	os_atomic_inc_long(&ref->refs);
}

static inline bool ols_ref_release(struct ols_weak_ref *ref)
{
	return os_atomic_dec_long(&ref->refs) == -1;
}

static inline void ols_weak_ref_addref(struct ols_weak_ref *ref)
{
	os_atomic_inc_long(&ref->weak_refs);
}

static inline bool ols_weak_ref_release(struct ols_weak_ref *ref)
{
	return os_atomic_dec_long(&ref->weak_refs) == -1;
}

static inline bool ols_weak_ref_get_ref(struct ols_weak_ref *ref)
{
	long owners = os_atomic_load_long(&ref->refs);
	while (owners > -1) {
		if (os_atomic_compare_exchange_long(&ref->refs, &owners,
						    owners + 1)) {
			return true;
		}
	}

	return false;
}

static inline bool ols_weak_ref_expired(struct ols_weak_ref *ref)
{
	long owners = os_atomic_load_long(&ref->refs);
	return owners < 0;
}

/* ------------------------------------------------------------------------- */
/* sources  */

struct ols_weak_source {
	struct ols_weak_ref ref;
	struct ols_source *source;
};


struct ols_source {
	struct ols_context_data context;
	struct ols_source_info info;

	/* general exposed flags that can be set for the source */
	uint32_t flags;
	uint32_t default_flags;
	uint32_t last_ols_ver;

	/* indicates ownership of the info.id buffer */
	bool owns_info_id;

	/* signals to call the source update in the video thread */
	long defer_update_count;

	/* ensures show/hide are only called once */
	volatile long show_refs;

	/* ensures activate/deactivate are only called once */
	volatile long activate_refs;

	/* source is in the process of being destroyed */
	volatile long destroying;

	/* used to indicate that the source has been removed and all
	 * references to it should be released (not exactly how I would prefer
	 * to handle things but it's the best option) */
	bool removed;

	/*  used to indicate if the source should show up when queried for user ui */
	bool temp_removed;

	bool active;

	/* used to temporarily disable sources if needed */
	bool enabled;

	uint64_t last_frame_ts;
	uint64_t last_sys_timestamp;

	/* private data */
	ols_data_t *private_settings;
};

extern struct ols_source_info *get_source_info(const char *id);


extern bool ols_source_init_context(struct ols_source *source,
				    ols_data_t *settings, const char *name,
				    const char *uuid, ols_data_t *hotkey_data,
				    bool private);


extern ols_source_t *
ols_source_create_set_last_ver(const char *id, const char *name,
			       const char *uuid, ols_data_t *settings,
			       ols_data_t *hotkey_data, uint32_t last_ols_ver,
			       bool is_private);
extern void ols_source_destroy(struct ols_source *source);

enum view_type {
	MAIN_VIEW,
	AUX_VIEW,
};

static inline void ols_source_dosignal(struct ols_source *source,
				       const char *signal_ols,
				       const char *signal_source)
{
	struct calldata data;
	uint8_t stack[128];

	calldata_init_fixed(&data, stack, sizeof(stack));
	calldata_set_ptr(&data, "source", source);
	if (signal_ols && !source->context.private)
		signal_handler_signal(ols->signals, signal_ols, &data);
	if (signal_source)
		signal_handler_signal(source->context.signals, signal_source,
				      &data);
}

extern void ols_source_activate(ols_source_t *source, enum view_type type);
extern void ols_source_deactivate(ols_source_t *source, enum view_type type);

/* ------------------------------------------------------------------------- */
/* outputs  */


struct ols_weak_output {
	struct ols_weak_ref ref;
	struct ols_output *output;
};

struct ols_output {
	struct ols_context_data context;
	struct ols_output_info info;
	/* indicates ownership of the info.id buffer */
	bool owns_info_id;
	volatile bool data_active;
	int stop_code;

	int reconnect_retry_sec;
	int reconnect_retry_max;
	int reconnect_retries;
	uint32_t reconnect_retry_cur_msec;
	float reconnect_retry_exp;
	pthread_t reconnect_thread;
	os_event_t *reconnect_stop_event;
	volatile bool reconnecting;
	volatile bool reconnect_thread_active;

	uint32_t starting_drawn_count;
	uint32_t starting_lagged_count;

	int total_frames;


	bool valid;


	char *last_error_message;
};

static inline void do_output_signal(struct ols_output *output,
				    const char *signal)
{
	struct calldata params = {0};
	calldata_set_ptr(&params, "output", output);
	signal_handler_signal(output->context.signals, signal, &params);
	calldata_free(&params);
}

extern void ols_output_cleanup_delay(ols_output_t *output);
extern bool ols_output_delay_start(ols_output_t *output);
extern void ols_output_delay_stop(ols_output_t *output);
extern bool ols_output_actual_start(ols_output_t *output);
extern void ols_output_actual_stop(ols_output_t *output, bool force,
				   uint64_t ts);

extern const struct ols_output_info *find_output(const char *id);

void ols_output_destroy(ols_output_t *output);


