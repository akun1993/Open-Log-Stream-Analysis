/******************************************************************************
    Copyright (C) 2014-2015 by Ruwen Hahn <palana@stunned.de>

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

#include "ols-internal.h"

/* Since ids are just sequential size_t integers, we don't really need a
 * hash function to get an even distribution across buckets.
 * (Realistically this should never wrap, who has 4.29 billion hotkeys?!)  */
#undef HASH_FUNCTION
#define HASH_FUNCTION(s, len, hashv) (hashv) = *s % UINT_MAX

/* Custom definitions to make adding/looking up size_t integers easier */
#define HASH_ADD_HKEY(head, idfield, add) \
	HASH_ADD(hh, head, idfield, sizeof(size_t), add)
#define HASH_FIND_HKEY(head, id, out) \
	HASH_FIND(hh, head, &(id), sizeof(size_t), out)

static inline bool lock(void)
{
	if (!ols)
		return false;

	pthread_mutex_lock(&ols->hotkeys.mutex);
	return true;
}

static inline void unlock(void)
{
	pthread_mutex_unlock(&ols->hotkeys.mutex);
}

ols_hotkey_id ols_hotkey_get_id(const ols_hotkey_t *key)
{
	return key->id;
}

const char *ols_hotkey_get_name(const ols_hotkey_t *key)
{
	return key->name;
}

const char *ols_hotkey_get_description(const ols_hotkey_t *key)
{
	return key->description;
}

ols_hotkey_registerer_t ols_hotkey_get_registerer_type(const ols_hotkey_t *key)
{
	return key->registerer_type;
}

void *ols_hotkey_get_registerer(const ols_hotkey_t *key)
{
	return key->registerer;
}

ols_hotkey_id ols_hotkey_get_pair_partner_id(const ols_hotkey_t *key)
{
	return key->pair_partner_id;
}

ols_key_combination_t
ols_hotkey_binding_get_key_combination(ols_hotkey_binding_t *binding)
{
	return binding->key;
}

ols_hotkey_id ols_hotkey_binding_get_hotkey_id(ols_hotkey_binding_t *binding)
{
	return binding->hotkey_id;
}

ols_hotkey_t *ols_hotkey_binding_get_hotkey(ols_hotkey_binding_t *binding)
{
	return binding->hotkey;
}

void ols_hotkey_set_name(ols_hotkey_id id, const char *name)
{
	ols_hotkey_t *hotkey;
	HASH_FIND_HKEY(ols->hotkeys.hotkeys, id, hotkey);
	if (!hotkey)
		return;

	bfree(hotkey->name);
	hotkey->name = bstrdup(name);
}

void ols_hotkey_set_description(ols_hotkey_id id, const char *desc)
{
	ols_hotkey_t *hotkey;
	HASH_FIND_HKEY(ols->hotkeys.hotkeys, id, hotkey);
	if (!hotkey)
		return;

	bfree(hotkey->description);
	hotkey->description = bstrdup(desc);
}

void ols_hotkey_pair_set_names(ols_hotkey_pair_id id, const char *name0,
			       const char *name1)
{
	ols_hotkey_pair_t *pair;

	HASH_FIND_HKEY(ols->hotkeys.hotkey_pairs, id, pair);
	if (!pair)
		return;

	ols_hotkey_set_name(pair->id[0], name0);
	ols_hotkey_set_name(pair->id[1], name1);
}

void ols_hotkey_pair_set_descriptions(ols_hotkey_pair_id id, const char *desc0,
				      const char *desc1)
{
	ols_hotkey_pair_t *pair;

	HASH_FIND_HKEY(ols->hotkeys.hotkey_pairs, id, pair);
	if (!pair)
		return;

	ols_hotkey_set_description(pair->id[0], desc0);
	ols_hotkey_set_description(pair->id[1], desc1);
}

static void hotkey_signal(const char *signal, ols_hotkey_t *hotkey)
{
	calldata_t data;
	calldata_init(&data);
	calldata_set_ptr(&data, "key", hotkey);

	signal_handler_signal(ols->hotkeys.signals, signal, &data);

	calldata_free(&data);
}

static inline void load_bindings(ols_hotkey_t *hotkey, ols_data_array_t *data);

static inline void context_add_hotkey(struct ols_context_data *context,
				      ols_hotkey_id id)
{
	da_push_back(context->hotkeys, &id);
}

static inline ols_hotkey_id
ols_hotkey_register_internal(ols_hotkey_registerer_t type, void *registerer,
			     struct ols_context_data *context, const char *name,
			     const char *description, ols_hotkey_func func,
			     void *data)
{
	if ((ols->hotkeys.next_id + 1) == OLS_INVALID_HOTKEY_ID)
		blog(LOG_WARNING, "ols-hotkey: Available hotkey ids exhausted");

	ols_hotkey_id result = ols->hotkeys.next_id++;
	ols_hotkey_t *hotkey = bzalloc(sizeof(ols_hotkey_t));

	hotkey->id = result;
	hotkey->name = bstrdup(name);
	hotkey->description = bstrdup(description);
	hotkey->func = func;
	hotkey->data = data;
	hotkey->registerer_type = type;
	hotkey->registerer = registerer;
	hotkey->pair_partner_id = OLS_INVALID_HOTKEY_PAIR_ID;

	HASH_ADD_HKEY(ols->hotkeys.hotkeys, id, hotkey);

	if (context) {
		ols_data_array_t *data =
			ols_data_get_array(context->hotkey_data, name);
		load_bindings(hotkey, data);
		ols_data_array_release(data);

		context_add_hotkey(context, result);
	}

	hotkey_signal("hotkey_register", hotkey);

	return result;
}

ols_hotkey_id ols_hotkey_register_frontend(const char *name,
					   const char *description,
					   ols_hotkey_func func, void *data)
{
	if (!lock())
		return OLS_INVALID_HOTKEY_ID;

	ols_hotkey_id id = ols_hotkey_register_internal(
		OLS_HOTKEY_REGISTERER_FRONTEND, NULL, NULL, name, description,
		func, data);

	unlock();
	return id;
}

ols_hotkey_id ols_hotkey_register_encoder(ols_encoder_t *encoder,
					  const char *name,
					  const char *description,
					  ols_hotkey_func func, void *data)
{
	if (!encoder || !lock())
		return OLS_INVALID_HOTKEY_ID;

	ols_hotkey_id id = ols_hotkey_register_internal(
		OLS_HOTKEY_REGISTERER_ENCODER,
		ols_encoder_get_weak_encoder(encoder), &encoder->context, name,
		description, func, data);

	unlock();
	return id;
}

ols_hotkey_id ols_hotkey_register_output(ols_output_t *output, const char *name,
					 const char *description,
					 ols_hotkey_func func, void *data)
{
	if (!output || !lock())
		return OLS_INVALID_HOTKEY_ID;

	ols_hotkey_id id = ols_hotkey_register_internal(
		OLS_HOTKEY_REGISTERER_OUTPUT,
		ols_output_get_weak_output(output), &output->context, name,
		description, func, data);

	unlock();
	return id;
}

ols_hotkey_id ols_hotkey_register_service(ols_service_t *service,
					  const char *name,
					  const char *description,
					  ols_hotkey_func func, void *data)
{
	if (!service || !lock())
		return OLS_INVALID_HOTKEY_ID;

	ols_hotkey_id id = ols_hotkey_register_internal(
		OLS_HOTKEY_REGISTERER_SERVICE,
		ols_service_get_weak_service(service), &service->context, name,
		description, func, data);

	unlock();
	return id;
}

ols_hotkey_id ols_hotkey_register_source(ols_source_t *source, const char *name,
					 const char *description,
					 ols_hotkey_func func, void *data)
{
	if (!source || source->context.private || !lock())
		return OLS_INVALID_HOTKEY_ID;

	ols_hotkey_id id = ols_hotkey_register_internal(
		OLS_HOTKEY_REGISTERER_SOURCE,
		ols_source_get_weak_source(source), &source->context, name,
		description, func, data);

	unlock();
	return id;
}

static ols_hotkey_pair_t *create_hotkey_pair(struct ols_context_data *context,
					     ols_hotkey_active_func func0,
					     ols_hotkey_active_func func1,
					     void *data0, void *data1)
{
	if ((ols->hotkeys.next_pair_id + 1) == OLS_INVALID_HOTKEY_PAIR_ID)
		blog(LOG_WARNING, "ols-hotkey: Available hotkey pair ids "
				  "exhausted");

	ols_hotkey_pair_t *pair = bzalloc(sizeof(ols_hotkey_pair_t));

	pair->pair_id = ols->hotkeys.next_pair_id++;
	pair->func[0] = func0;
	pair->func[1] = func1;
	pair->id[0] = OLS_INVALID_HOTKEY_ID;
	pair->id[1] = OLS_INVALID_HOTKEY_ID;
	pair->data[0] = data0;
	pair->data[1] = data1;

	HASH_ADD_HKEY(ols->hotkeys.hotkey_pairs, pair_id, pair);

	if (context)
		da_push_back(context->hotkey_pairs, &pair->pair_id);

	return pair;
}

static void ols_hotkey_pair_first_func(void *data, ols_hotkey_id id,
				       ols_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);

	ols_hotkey_pair_t *pair = data;
	if (pair->pressed1)
		return;

	if (pair->pressed0 && !pressed)
		pair->pressed0 = false;
	else if (pair->func[0](pair->data[0], pair->pair_id, hotkey, pressed))
		pair->pressed0 = pressed;
}

static void ols_hotkey_pair_second_func(void *data, ols_hotkey_id id,
					ols_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);

	ols_hotkey_pair_t *pair = data;
	if (pair->pressed0)
		return;

	if (pair->pressed1 && !pressed)
		pair->pressed1 = false;
	else if (pair->func[1](pair->data[1], pair->pair_id, hotkey, pressed))
		pair->pressed1 = pressed;
}

static ols_hotkey_pair_id register_hotkey_pair_internal(
	ols_hotkey_registerer_t type, void *registerer,
	void *(*weak_ref)(void *), struct ols_context_data *context,
	const char *name0, const char *description0, const char *name1,
	const char *description1, ols_hotkey_active_func func0,
	ols_hotkey_active_func func1, void *data0, void *data1)
{

	if (!lock())
		return OLS_INVALID_HOTKEY_PAIR_ID;

	ols_hotkey_pair_t *pair =
		create_hotkey_pair(context, func0, func1, data0, data1);

	pair->id[0] = ols_hotkey_register_internal(type, weak_ref(registerer),
						   context, name0, description0,
						   ols_hotkey_pair_first_func,
						   pair);

	pair->id[1] = ols_hotkey_register_internal(type, weak_ref(registerer),
						   context, name1, description1,
						   ols_hotkey_pair_second_func,
						   pair);

	ols_hotkey_t *hotkey_1, *hotkey_2;
	HASH_FIND_HKEY(ols->hotkeys.hotkeys, pair->id[0], hotkey_1);
	HASH_FIND_HKEY(ols->hotkeys.hotkeys, pair->id[1], hotkey_2);

	if (hotkey_1)
		hotkey_1->pair_partner_id = pair->id[1];
	if (hotkey_2)
		hotkey_2->pair_partner_id = pair->id[0];

	ols_hotkey_pair_id id = pair->pair_id;

	unlock();
	return id;
}

static inline void *ols_id_(void *id_)
{
	return id_;
}

ols_hotkey_pair_id ols_hotkey_pair_register_frontend(
	const char *name0, const char *description0, const char *name1,
	const char *description1, ols_hotkey_active_func func0,
	ols_hotkey_active_func func1, void *data0, void *data1)
{
	return register_hotkey_pair_internal(OLS_HOTKEY_REGISTERER_FRONTEND,
					     NULL, ols_id_, NULL, name0,
					     description0, name1, description1,
					     func0, func1, data0, data1);
}

static inline void *weak_encoder_ref(void *ref)
{
	return ols_encoder_get_weak_encoder(ref);
}

ols_hotkey_pair_id ols_hotkey_pair_register_encoder(
	ols_encoder_t *encoder, const char *name0, const char *description0,
	const char *name1, const char *description1,
	ols_hotkey_active_func func0, ols_hotkey_active_func func1, void *data0,
	void *data1)
{
	if (!encoder)
		return OLS_INVALID_HOTKEY_PAIR_ID;
	return register_hotkey_pair_internal(OLS_HOTKEY_REGISTERER_ENCODER,
					     encoder, weak_encoder_ref,
					     &encoder->context, name0,
					     description0, name1, description1,
					     func0, func1, data0, data1);
}

static inline void *weak_output_ref(void *ref)
{
	return ols_output_get_weak_output(ref);
}

ols_hotkey_pair_id ols_hotkey_pair_register_output(
	ols_output_t *output, const char *name0, const char *description0,
	const char *name1, const char *description1,
	ols_hotkey_active_func func0, ols_hotkey_active_func func1, void *data0,
	void *data1)
{
	if (!output)
		return OLS_INVALID_HOTKEY_PAIR_ID;
	return register_hotkey_pair_internal(OLS_HOTKEY_REGISTERER_OUTPUT,
					     output, weak_output_ref,
					     &output->context, name0,
					     description0, name1, description1,
					     func0, func1, data0, data1);
}

static inline void *weak_service_ref(void *ref)
{
	return ols_service_get_weak_service(ref);
}

ols_hotkey_pair_id ols_hotkey_pair_register_service(
	ols_service_t *service, const char *name0, const char *description0,
	const char *name1, const char *description1,
	ols_hotkey_active_func func0, ols_hotkey_active_func func1, void *data0,
	void *data1)
{
	if (!service)
		return OLS_INVALID_HOTKEY_PAIR_ID;
	return register_hotkey_pair_internal(OLS_HOTKEY_REGISTERER_SERVICE,
					     service, weak_service_ref,
					     &service->context, name0,
					     description0, name1, description1,
					     func0, func1, data0, data1);
}

static inline void *weak_source_ref(void *ref)
{
	return ols_source_get_weak_source(ref);
}

ols_hotkey_pair_id ols_hotkey_pair_register_source(
	ols_source_t *source, const char *name0, const char *description0,
	const char *name1, const char *description1,
	ols_hotkey_active_func func0, ols_hotkey_active_func func1, void *data0,
	void *data1)
{
	if (!source)
		return OLS_INVALID_HOTKEY_PAIR_ID;
	return register_hotkey_pair_internal(OLS_HOTKEY_REGISTERER_SOURCE,
					     source, weak_source_ref,
					     &source->context, name0,
					     description0, name1, description1,
					     func0, func1, data0, data1);
}

typedef bool (*ols_hotkey_binding_internal_enum_func)(
	void *data, size_t idx, ols_hotkey_binding_t *binding);

static inline void enum_bindings(ols_hotkey_binding_internal_enum_func func,
				 void *data)
{
	const size_t num = ols->hotkeys.bindings.num;
	ols_hotkey_binding_t *array = ols->hotkeys.bindings.array;
	for (size_t i = 0; i < num; i++) {
		if (!func(data, i, &array[i]))
			break;
	}
}

typedef bool (*ols_hotkey_internal_enum_func)(void *data, ols_hotkey_t *hotkey);

static inline void enum_context_hotkeys(struct ols_context_data *context,
					ols_hotkey_internal_enum_func func,
					void *data)
{
	const size_t num = context->hotkeys.num;
	const ols_hotkey_id *array = context->hotkeys.array;
	ols_hotkey_t *hotkey;

	for (size_t i = 0; i < num; i++) {
		HASH_FIND_HKEY(ols->hotkeys.hotkeys, array[i], hotkey);
		if (!hotkey)
			continue;
		if (!func(data, hotkey))
			break;
	}
}

static inline void load_modifier(uint32_t *modifiers, ols_data_t *data,
				 const char *name, uint32_t flag)
{
	if (ols_data_get_bool(data, name))
		*modifiers |= flag;
}

static inline void create_binding(ols_hotkey_t *hotkey,
				  ols_key_combination_t combo)
{
	ols_hotkey_binding_t *binding = da_push_back_new(ols->hotkeys.bindings);
	if (!binding)
		return;

	binding->key = combo;
	binding->hotkey_id = hotkey->id;
	binding->hotkey = hotkey;
}

static inline void load_binding(ols_hotkey_t *hotkey, ols_data_t *data)
{
	if (!hotkey || !data)
		return;

	ols_key_combination_t combo = {0};
	uint32_t *modifiers = &combo.modifiers;
	load_modifier(modifiers, data, "shift", INTERACT_SHIFT_KEY);
	load_modifier(modifiers, data, "control", INTERACT_CONTROL_KEY);
	load_modifier(modifiers, data, "alt", INTERACT_ALT_KEY);
	load_modifier(modifiers, data, "command", INTERACT_COMMAND_KEY);

	combo.key = ols_key_from_name(ols_data_get_string(data, "key"));
	if (!modifiers &&
	    (combo.key == OLS_KEY_NONE || combo.key >= OLS_KEY_LAST_VALUE))
		return;

	create_binding(hotkey, combo);
}

static inline void load_bindings(ols_hotkey_t *hotkey, ols_data_array_t *data)
{
	const size_t count = ols_data_array_count(data);
	for (size_t i = 0; i < count; i++) {
		ols_data_t *item = ols_data_array_item(data, i);
		load_binding(hotkey, item);
		ols_data_release(item);
	}

	if (count)
		hotkey_signal("hotkey_bindings_changed", hotkey);
}

static inline bool remove_bindings(ols_hotkey_id id);

void ols_hotkey_load_bindings(ols_hotkey_id id,
			      ols_key_combination_t *combinations, size_t num)
{
	if (!lock())
		return;

	ols_hotkey_t *hotkey;
	HASH_FIND_HKEY(ols->hotkeys.hotkeys, id, hotkey);
	if (hotkey) {
		bool changed = remove_bindings(id);
		for (size_t i = 0; i < num; i++)
			create_binding(hotkey, combinations[i]);

		if (num || changed)
			hotkey_signal("hotkey_bindings_changed", hotkey);
	}

	unlock();
}

void ols_hotkey_load(ols_hotkey_id id, ols_data_array_t *data)
{
	if (!lock())
		return;

	ols_hotkey_t *hotkey;
	HASH_FIND_HKEY(ols->hotkeys.hotkeys, id, hotkey);
	if (hotkey) {
		remove_bindings(id);
		load_bindings(hotkey, data);
	}
	unlock();
}

static inline bool enum_load_bindings(void *data, ols_hotkey_t *hotkey)
{
	ols_data_array_t *hotkey_data = ols_data_get_array(data, hotkey->name);
	if (!hotkey_data)
		return true;

	load_bindings(hotkey, hotkey_data);
	ols_data_array_release(hotkey_data);
	return true;
}

void ols_hotkeys_load_encoder(ols_encoder_t *encoder, ols_data_t *hotkeys)
{
	if (!encoder || !hotkeys)
		return;
	if (!lock())
		return;

	enum_context_hotkeys(&encoder->context, enum_load_bindings, hotkeys);
	unlock();
}

void ols_hotkeys_load_output(ols_output_t *output, ols_data_t *hotkeys)
{
	if (!output || !hotkeys)
		return;
	if (!lock())
		return;

	enum_context_hotkeys(&output->context, enum_load_bindings, hotkeys);
	unlock();
}

void ols_hotkeys_load_service(ols_service_t *service, ols_data_t *hotkeys)
{
	if (!service || !hotkeys)
		return;
	if (!lock())
		return;

	enum_context_hotkeys(&service->context, enum_load_bindings, hotkeys);
	unlock();
}

void ols_hotkeys_load_source(ols_source_t *source, ols_data_t *hotkeys)
{
	if (!source || !hotkeys)
		return;
	if (!lock())
		return;

	enum_context_hotkeys(&source->context, enum_load_bindings, hotkeys);
	unlock();
}

void ols_hotkey_pair_load(ols_hotkey_pair_id id, ols_data_array_t *data0,
			  ols_data_array_t *data1)
{
	if ((!data0 && !data1) || !lock())
		return;

	ols_hotkey_pair_t *pair;
	HASH_FIND_HKEY(ols->hotkeys.hotkey_pairs, id, pair);
	if (!pair)
		goto unlock;

	ols_hotkey_t *p1, *p2;
	HASH_FIND_HKEY(ols->hotkeys.hotkeys, pair->id[0], p1);
	HASH_FIND_HKEY(ols->hotkeys.hotkeys, pair->id[1], p2);

	if (p1) {
		remove_bindings(pair->id[0]);
		load_bindings(p1, data0);
	}
	if (p2) {
		remove_bindings(pair->id[1]);
		load_bindings(p2, data1);
	}

unlock:
	unlock();
}

static inline void save_modifier(uint32_t modifiers, ols_data_t *data,
				 const char *name, uint32_t flag)
{
	if ((modifiers & flag) == flag)
		ols_data_set_bool(data, name, true);
}

struct save_bindings_helper_t {
	ols_data_array_t *array;
	ols_hotkey_t *hotkey;
};

static inline bool save_bindings_helper(void *data, size_t idx,
					ols_hotkey_binding_t *binding)
{
	UNUSED_PARAMETER(idx);
	struct save_bindings_helper_t *h = data;

	if (h->hotkey->id != binding->hotkey_id)
		return true;

	ols_data_t *hotkey = ols_data_create();

	uint32_t modifiers = binding->key.modifiers;
	save_modifier(modifiers, hotkey, "shift", INTERACT_SHIFT_KEY);
	save_modifier(modifiers, hotkey, "control", INTERACT_CONTROL_KEY);
	save_modifier(modifiers, hotkey, "alt", INTERACT_ALT_KEY);
	save_modifier(modifiers, hotkey, "command", INTERACT_COMMAND_KEY);

	ols_data_set_string(hotkey, "key", ols_key_to_name(binding->key.key));

	ols_data_array_push_back(h->array, hotkey);

	ols_data_release(hotkey);

	return true;
}

static inline ols_data_array_t *save_hotkey(ols_hotkey_t *hotkey)
{
	ols_data_array_t *data = ols_data_array_create();

	struct save_bindings_helper_t arg = {data, hotkey};
	enum_bindings(save_bindings_helper, &arg);

	return data;
}

ols_data_array_t *ols_hotkey_save(ols_hotkey_id id)
{
	ols_data_array_t *result = NULL;

	if (!lock())
		return result;

	ols_hotkey_t *hotkey;
	HASH_FIND_HKEY(ols->hotkeys.hotkeys, id, hotkey);
	if (hotkey)
		result = save_hotkey(hotkey);

	unlock();

	return result;
}

void ols_hotkey_pair_save(ols_hotkey_pair_id id, ols_data_array_t **p_data0,
			  ols_data_array_t **p_data1)
{
	if ((!p_data0 && !p_data1) || !lock())
		return;

	ols_hotkey_pair_t *pair;
	HASH_FIND_HKEY(ols->hotkeys.hotkey_pairs, id, pair);
	if (!pair)
		goto unlock;

	ols_hotkey_t *hotkey;
	if (p_data0) {
		HASH_FIND_HKEY(ols->hotkeys.hotkeys, pair->id[0], hotkey);
		if (hotkey)
			*p_data0 = save_hotkey(hotkey);
	}
	if (p_data1) {
		HASH_FIND_HKEY(ols->hotkeys.hotkeys, pair->id[1], hotkey);
		if (hotkey)
			*p_data1 = save_hotkey(hotkey);
	}

unlock:
	unlock();
}

static inline bool enum_save_hotkey(void *data, ols_hotkey_t *hotkey)
{
	ols_data_array_t *hotkey_data = save_hotkey(hotkey);
	ols_data_set_array(data, hotkey->name, hotkey_data);
	ols_data_array_release(hotkey_data);
	return true;
}

static inline ols_data_t *save_context_hotkeys(struct ols_context_data *context)
{
	if (!context->hotkeys.num)
		return NULL;

	ols_data_t *result = ols_data_create();
	enum_context_hotkeys(context, enum_save_hotkey, result);
	return result;
}

ols_data_t *ols_hotkeys_save_encoder(ols_encoder_t *encoder)
{
	ols_data_t *result = NULL;

	if (!lock())
		return result;

	result = save_context_hotkeys(&encoder->context);
	unlock();

	return result;
}

ols_data_t *ols_hotkeys_save_output(ols_output_t *output)
{
	ols_data_t *result = NULL;

	if (!lock())
		return result;

	result = save_context_hotkeys(&output->context);
	unlock();

	return result;
}

ols_data_t *ols_hotkeys_save_service(ols_service_t *service)
{
	ols_data_t *result = NULL;

	if (!lock())
		return result;

	result = save_context_hotkeys(&service->context);
	unlock();

	return result;
}

ols_data_t *ols_hotkeys_save_source(ols_source_t *source)
{
	ols_data_t *result = NULL;

	if (!lock())
		return result;

	result = save_context_hotkeys(&source->context);
	unlock();

	return result;
}

struct binding_find_data {
	ols_hotkey_id id;
	size_t *idx;
	bool found;
};

static inline bool binding_finder(void *data, size_t idx,
				  ols_hotkey_binding_t *binding)
{
	struct binding_find_data *find = data;
	if (binding->hotkey_id != find->id)
		return true;

	*find->idx = idx;
	find->found = true;
	return false;
}

static inline bool find_binding(ols_hotkey_id id, size_t *idx)
{
	struct binding_find_data data = {id, idx, false};
	enum_bindings(binding_finder, &data);
	return data.found;
}

static inline void release_pressed_binding(ols_hotkey_binding_t *binding);

static inline bool remove_bindings(ols_hotkey_id id)
{
	bool removed = false;
	size_t idx;
	while (find_binding(id, &idx)) {
		ols_hotkey_binding_t *binding =
			&ols->hotkeys.bindings.array[idx];

		if (binding->pressed)
			release_pressed_binding(binding);

		da_erase(ols->hotkeys.bindings, idx);
		removed = true;
	}

	return removed;
}

static void release_registerer(ols_hotkey_t *hotkey)
{
	switch (hotkey->registerer_type) {
	case OLS_HOTKEY_REGISTERER_FRONTEND:
		break;

	case OLS_HOTKEY_REGISTERER_OUTPUT:
		ols_weak_output_release(hotkey->registerer);
		break;

	case OLS_HOTKEY_REGISTERER_SERVICE:
		ols_weak_service_release(hotkey->registerer);
		break;

	case OLS_HOTKEY_REGISTERER_SOURCE:
		ols_weak_source_release(hotkey->registerer);
		break;
	}

	hotkey->registerer = NULL;
}

static inline void unregister_hotkey(ols_hotkey_id id)
{
	if (id >= ols->hotkeys.next_id)
		return;

	ols_hotkey_t *hotkey;
	HASH_FIND_HKEY(ols->hotkeys.hotkeys, id, hotkey);
	if (!hotkey)
		return;

	HASH_DEL(ols->hotkeys.hotkeys, hotkey);

	hotkey_signal("hotkey_unregister", hotkey);

	release_registerer(hotkey);

	if (hotkey->registerer_type == OLS_HOTKEY_REGISTERER_SOURCE)
		ols_weak_source_release(hotkey->registerer);

	bfree(hotkey->name);
	bfree(hotkey->description);
	bfree(hotkey);

	remove_bindings(id);
}

static inline void unregister_hotkey_pair(ols_hotkey_pair_id id)
{
	if (id >= ols->hotkeys.next_pair_id)
		return;

	ols_hotkey_pair_t *pair;
	HASH_FIND_HKEY(ols->hotkeys.hotkey_pairs, id, pair);
	if (!pair)
		return;

	unregister_hotkey(pair->id[0]);
	unregister_hotkey(pair->id[1]);

	HASH_DEL(ols->hotkeys.hotkey_pairs, pair);
	bfree(pair);
}

void ols_hotkey_unregister(ols_hotkey_id id)
{
	if (!lock())
		return;

	unregister_hotkey(id);
	unlock();
}

void ols_hotkey_pair_unregister(ols_hotkey_pair_id id)
{
	if (!lock())
		return;

	unregister_hotkey_pair(id);
	unlock();
}

static void context_release_hotkeys(struct ols_context_data *context)
{
	if (!context->hotkeys.num)
		goto cleanup;

	for (size_t i = 0; i < context->hotkeys.num; i++)
		unregister_hotkey(context->hotkeys.array[i]);

cleanup:
	da_free(context->hotkeys);
}

static void context_release_hotkey_pairs(struct ols_context_data *context)
{
	if (!context->hotkey_pairs.num)
		goto cleanup;

	for (size_t i = 0; i < context->hotkey_pairs.num; i++)
		unregister_hotkey_pair(context->hotkey_pairs.array[i]);

cleanup:
	da_free(context->hotkey_pairs);
}

void ols_hotkeys_context_release(struct ols_context_data *context)
{
	if (!lock())
		return;

	context_release_hotkeys(context);
	context_release_hotkey_pairs(context);

	ols_data_release(context->hotkey_data);
	unlock();
}

void ols_hotkeys_free(void)
{
	ols_hotkey_t *hotkey, *tmp;
	HASH_ITER (hh, ols->hotkeys.hotkeys, hotkey, tmp) {
		HASH_DEL(ols->hotkeys.hotkeys, hotkey);
		bfree(hotkey->name);
		bfree(hotkey->description);
		release_registerer(hotkey);
		bfree(hotkey);
	}

	ols_hotkey_pair_t *pair, *tmp2;
	HASH_ITER (hh, ols->hotkeys.hotkey_pairs, pair, tmp2) {
		HASH_DEL(ols->hotkeys.hotkey_pairs, pair);
		bfree(pair);
	}

	da_free(ols->hotkeys.bindings);

	for (size_t i = 0; i < OLS_KEY_LAST_VALUE; i++) {
		if (ols->hotkeys.translations[i]) {
			bfree(ols->hotkeys.translations[i]);
			ols->hotkeys.translations[i] = NULL;
		}
	}
}

void ols_enum_hotkeys(ols_hotkey_enum_func func, void *data)
{
	if (!lock())
		return;

	ols_hotkey_t *hk, *tmp;
	HASH_ITER (hh, ols->hotkeys.hotkeys, hk, tmp) {
		if (!func(data, hk->id, hk))
			break;
	}

	unlock();
}

void ols_enum_hotkey_bindings(ols_hotkey_binding_enum_func func, void *data)
{
	if (!lock())
		return;

	enum_bindings(func, data);
	unlock();
}

static inline bool modifiers_match(ols_hotkey_binding_t *binding,
				   uint32_t modifiers_, bool strict_modifiers)
{
	uint32_t modifiers = binding->key.modifiers;
	if (!strict_modifiers)
		return (modifiers & modifiers_) == modifiers;
	else
		return modifiers == modifiers_;
}

static inline bool is_pressed(ols_key_t key)
{
	return ols_hotkeys_platform_is_pressed(ols->hotkeys.platform_context,
					       key);
}

static inline void press_released_binding(ols_hotkey_binding_t *binding)
{
	binding->pressed = true;

	ols_hotkey_t *hotkey = binding->hotkey;
	if (hotkey->pressed++)
		return;

	if (!ols->hotkeys.reroute_hotkeys)
		hotkey->func(hotkey->data, hotkey->id, hotkey, true);
	else if (ols->hotkeys.router_func)
		ols->hotkeys.router_func(ols->hotkeys.router_func_data,
					 hotkey->id, true);
}

static inline void release_pressed_binding(ols_hotkey_binding_t *binding)
{
	binding->pressed = false;

	ols_hotkey_t *hotkey = binding->hotkey;
	if (--hotkey->pressed)
		return;

	if (!ols->hotkeys.reroute_hotkeys)
		hotkey->func(hotkey->data, hotkey->id, hotkey, false);
	else if (ols->hotkeys.router_func)
		ols->hotkeys.router_func(ols->hotkeys.router_func_data,
					 hotkey->id, false);
}

static inline void handle_binding(ols_hotkey_binding_t *binding,
				  uint32_t modifiers, bool no_press,
				  bool strict_modifiers, bool *pressed)
{
	bool modifiers_match_ =
		modifiers_match(binding, modifiers, strict_modifiers);
	bool modifiers_only = binding->key.key == OLS_KEY_NONE;

	if (!strict_modifiers && !binding->key.modifiers)
		binding->modifiers_match = true;

	if (modifiers_only)
		pressed = &modifiers_only;

	if (!binding->key.modifiers && modifiers_only)
		goto reset;

	if ((!binding->modifiers_match && !modifiers_only) || !modifiers_match_)
		goto reset;

	if ((pressed && !*pressed) ||
	    (!pressed && !is_pressed(binding->key.key)))
		goto reset;

	if (binding->pressed || no_press)
		return;

	press_released_binding(binding);
	return;

reset:
	binding->modifiers_match = modifiers_match_;
	if (!binding->pressed)
		return;

	release_pressed_binding(binding);
}

struct ols_hotkey_internal_inject {
	ols_key_combination_t hotkey;
	bool pressed;
	bool strict_modifiers;
};

static inline bool inject_hotkey(void *data, size_t idx,
				 ols_hotkey_binding_t *binding)
{
	UNUSED_PARAMETER(idx);
	struct ols_hotkey_internal_inject *event = data;

	if (modifiers_match(binding, event->hotkey.modifiers,
			    event->strict_modifiers)) {
		bool pressed = binding->key.key == event->hotkey.key &&
			       event->pressed;
		if (binding->key.key == OLS_KEY_NONE)
			pressed = true;

		if (pressed) {
			binding->modifiers_match = true;
			if (!binding->pressed)
				press_released_binding(binding);
		}
	}

	return true;
}

void ols_hotkey_inject_event(ols_key_combination_t hotkey, bool pressed)
{
	if (!lock())
		return;

	struct ols_hotkey_internal_inject event = {
		{hotkey.modifiers, hotkey.key},
		pressed,
		ols->hotkeys.strict_modifiers,
	};
	enum_bindings(inject_hotkey, &event);
	unlock();
}

void ols_hotkey_enable_background_press(bool enable)
{
	if (!lock())
		return;

	ols->hotkeys.thread_disable_press = !enable;
	unlock();
}

void ols_hotkey_enable_strict_modifiers(bool enable)
{
	if (!lock())
		return;

	ols->hotkeys.strict_modifiers = enable;
	unlock();
}

struct ols_query_hotkeys_helper {
	uint32_t modifiers;
	bool no_press;
	bool strict_modifiers;
};

static inline bool query_hotkey(void *data, size_t idx,
				ols_hotkey_binding_t *binding)
{
	UNUSED_PARAMETER(idx);

	struct ols_query_hotkeys_helper *param =
		(struct ols_query_hotkeys_helper *)data;
	handle_binding(binding, param->modifiers, param->no_press,
		       param->strict_modifiers, NULL);

	return true;
}

static inline void query_hotkeys()
{
	uint32_t modifiers = 0;
	if (is_pressed(OLS_KEY_SHIFT))
		modifiers |= INTERACT_SHIFT_KEY;
	if (is_pressed(OLS_KEY_CONTROL))
		modifiers |= INTERACT_CONTROL_KEY;
	if (is_pressed(OLS_KEY_ALT))
		modifiers |= INTERACT_ALT_KEY;
	if (is_pressed(OLS_KEY_META))
		modifiers |= INTERACT_COMMAND_KEY;

	struct ols_query_hotkeys_helper param = {
		modifiers,
		ols->hotkeys.thread_disable_press,
		ols->hotkeys.strict_modifiers,
	};
	enum_bindings(query_hotkey, &param);
}

#define NBSP "\xC2\xA0"

void *ols_hotkey_thread(void *arg)
{
	UNUSED_PARAMETER(arg);

	os_set_thread_name("libols: hotkey thread");

	const char *hotkey_thread_name =
		profile_store_name(ols_get_profiler_name_store(),
				   "ols_hotkey_thread(%g" NBSP "ms)", 25.);
	profile_register_root(hotkey_thread_name, (uint64_t)25000000);

	while (os_event_timedwait(ols->hotkeys.stop_event, 25) == ETIMEDOUT) {
		if (!lock())
			continue;

		profile_start(hotkey_thread_name);
		query_hotkeys();
		profile_end(hotkey_thread_name);

		unlock();

		profile_reenable_thread();
	}
	return NULL;
}

void ols_hotkey_trigger_routed_callback(ols_hotkey_id id, bool pressed)
{
	if (!lock())
		return;

	if (!ols->hotkeys.reroute_hotkeys)
		goto unlock;

	ols_hotkey_t *hotkey;
	HASH_FIND_HKEY(ols->hotkeys.hotkeys, id, hotkey);
	if (!hotkey)
		goto unlock;

	hotkey->func(hotkey->data, id, hotkey, pressed);

unlock:
	unlock();
}

void ols_hotkey_set_callback_routing_func(ols_hotkey_callback_router_func func,
					  void *data)
{
	if (!lock())
		return;

	ols->hotkeys.router_func = func;
	ols->hotkeys.router_func_data = data;
	unlock();
}

void ols_hotkey_enable_callback_rerouting(bool enable)
{
	if (!lock())
		return;

	ols->hotkeys.reroute_hotkeys = enable;
	unlock();
}

static void ols_set_key_translation(ols_key_t key, const char *translation)
{
	bfree(ols->hotkeys.translations[key]);
	ols->hotkeys.translations[key] = NULL;

	if (translation)
		ols->hotkeys.translations[key] = bstrdup(translation);
}

void ols_hotkeys_set_translations_s(
	struct ols_hotkeys_translations *translations, size_t size)
{
#define ADD_TRANSLATION(key_name, var_name) \
	if (t.var_name)                     \
		ols_set_key_translation(key_name, t.var_name);

	struct ols_hotkeys_translations t = {0};
	struct dstr numpad = {0};
	struct dstr mouse = {0};
	struct dstr button = {0};

	if (!translations) {
		return;
	}

	memcpy(&t, translations, (size < sizeof(t)) ? size : sizeof(t));

	ADD_TRANSLATION(OLS_KEY_INSERT, insert);
	ADD_TRANSLATION(OLS_KEY_DELETE, del);
	ADD_TRANSLATION(OLS_KEY_HOME, home);
	ADD_TRANSLATION(OLS_KEY_END, end);
	ADD_TRANSLATION(OLS_KEY_PAGEUP, page_up);
	ADD_TRANSLATION(OLS_KEY_PAGEDOWN, page_down);
	ADD_TRANSLATION(OLS_KEY_NUMLOCK, num_lock);
	ADD_TRANSLATION(OLS_KEY_SCROLLLOCK, scroll_lock);
	ADD_TRANSLATION(OLS_KEY_CAPSLOCK, caps_lock);
	ADD_TRANSLATION(OLS_KEY_BACKSPACE, backspace);
	ADD_TRANSLATION(OLS_KEY_TAB, tab);
	ADD_TRANSLATION(OLS_KEY_PRINT, print);
	ADD_TRANSLATION(OLS_KEY_PAUSE, pause);
	ADD_TRANSLATION(OLS_KEY_SHIFT, shift);
	ADD_TRANSLATION(OLS_KEY_ALT, alt);
	ADD_TRANSLATION(OLS_KEY_CONTROL, control);
	ADD_TRANSLATION(OLS_KEY_META, meta);
	ADD_TRANSLATION(OLS_KEY_MENU, menu);
	ADD_TRANSLATION(OLS_KEY_SPACE, space);
	ADD_TRANSLATION(OLS_KEY_ESCAPE, escape);
#ifdef __APPLE__
	const char *numpad_str = t.apple_keypad_num;
	ADD_TRANSLATION(OLS_KEY_NUMSLASH, apple_keypad_divide);
	ADD_TRANSLATION(OLS_KEY_NUMASTERISK, apple_keypad_multiply);
	ADD_TRANSLATION(OLS_KEY_NUMMINUS, apple_keypad_minus);
	ADD_TRANSLATION(OLS_KEY_NUMPLUS, apple_keypad_plus);
	ADD_TRANSLATION(OLS_KEY_NUMPERIOD, apple_keypad_decimal);
	ADD_TRANSLATION(OLS_KEY_NUMEQUAL, apple_keypad_equal);
#else
	const char *numpad_str = t.numpad_num;
	ADD_TRANSLATION(OLS_KEY_NUMSLASH, numpad_divide);
	ADD_TRANSLATION(OLS_KEY_NUMASTERISK, numpad_multiply);
	ADD_TRANSLATION(OLS_KEY_NUMMINUS, numpad_minus);
	ADD_TRANSLATION(OLS_KEY_NUMPLUS, numpad_plus);
	ADD_TRANSLATION(OLS_KEY_NUMPERIOD, numpad_decimal);
#endif

	if (numpad_str) {
		dstr_copy(&numpad, numpad_str);
		dstr_depad(&numpad);

		if (dstr_find(&numpad, "%1") == NULL) {
			dstr_cat(&numpad, " %1");
		}

#define ADD_NUMPAD_NUM(idx)                \
	dstr_copy_dstr(&button, &numpad);  \
	dstr_replace(&button, "%1", #idx); \
	ols_set_key_translation(OLS_KEY_NUM##idx, button.array)

		ADD_NUMPAD_NUM(0);
		ADD_NUMPAD_NUM(1);
		ADD_NUMPAD_NUM(2);
		ADD_NUMPAD_NUM(3);
		ADD_NUMPAD_NUM(4);
		ADD_NUMPAD_NUM(5);
		ADD_NUMPAD_NUM(6);
		ADD_NUMPAD_NUM(7);
		ADD_NUMPAD_NUM(8);
		ADD_NUMPAD_NUM(9);
	}

	if (t.mouse_num) {
		dstr_copy(&mouse, t.mouse_num);
		dstr_depad(&mouse);

		if (dstr_find(&mouse, "%1") == NULL) {
			dstr_cat(&mouse, " %1");
		}

#define ADD_MOUSE_NUM(idx)                 \
	dstr_copy_dstr(&button, &mouse);   \
	dstr_replace(&button, "%1", #idx); \
	ols_set_key_translation(OLS_KEY_MOUSE##idx, button.array)

		ADD_MOUSE_NUM(1);
		ADD_MOUSE_NUM(2);
		ADD_MOUSE_NUM(3);
		ADD_MOUSE_NUM(4);
		ADD_MOUSE_NUM(5);
		ADD_MOUSE_NUM(6);
		ADD_MOUSE_NUM(7);
		ADD_MOUSE_NUM(8);
		ADD_MOUSE_NUM(9);
		ADD_MOUSE_NUM(10);
		ADD_MOUSE_NUM(11);
		ADD_MOUSE_NUM(12);
		ADD_MOUSE_NUM(13);
		ADD_MOUSE_NUM(14);
		ADD_MOUSE_NUM(15);
		ADD_MOUSE_NUM(16);
		ADD_MOUSE_NUM(17);
		ADD_MOUSE_NUM(18);
		ADD_MOUSE_NUM(19);
		ADD_MOUSE_NUM(20);
		ADD_MOUSE_NUM(21);
		ADD_MOUSE_NUM(22);
		ADD_MOUSE_NUM(23);
		ADD_MOUSE_NUM(24);
		ADD_MOUSE_NUM(25);
		ADD_MOUSE_NUM(26);
		ADD_MOUSE_NUM(27);
		ADD_MOUSE_NUM(28);
		ADD_MOUSE_NUM(29);
	}

	dstr_free(&numpad);
	dstr_free(&mouse);
	dstr_free(&button);
}

const char *ols_get_hotkey_translation(ols_key_t key, const char *def)
{
	if (key == OLS_KEY_NONE) {
		return NULL;
	}

	return ols->hotkeys.translations[key] ? ols->hotkeys.translations[key]
					      : def;
}

void ols_hotkey_update_atomic(ols_hotkey_atomic_update_func func, void *data)
{
	if (!lock())
		return;

	func(data);

	unlock();
}

void ols_hotkeys_set_audio_hotkeys_translations(const char *mute,
						const char *unmute,
						const char *push_to_mute,
						const char *push_to_talk)
{
#define SET_T(n)               \
	bfree(ols->hotkeys.n); \
	ols->hotkeys.n = bstrdup(n)
	SET_T(mute);
	SET_T(unmute);
	SET_T(push_to_mute);
	SET_T(push_to_talk);
#undef SET_T
}

void ols_hotkeys_set_sceneitem_hotkeys_translations(const char *show,
						    const char *hide)
{
#define SET_T(n)                           \
	bfree(ols->hotkeys.sceneitem_##n); \
	ols->hotkeys.sceneitem_##n = bstrdup(n)
	SET_T(show);
	SET_T(hide);
#undef SET_T
}
