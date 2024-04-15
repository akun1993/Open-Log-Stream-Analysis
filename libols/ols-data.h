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

#include "util/c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * OLS data settings storage
 *
 *   This is used for retrieving or setting the data settings for things such
 * as sources, encoders, etc.  This is designed for JSON serialization.
 */

struct ols_data;
struct ols_data_item;
struct ols_data_array;
typedef struct ols_data ols_data_t;
typedef struct ols_data_item ols_data_item_t;
typedef struct ols_data_array ols_data_array_t;

enum ols_data_type {
	OLS_DATA_NULL,
	OLS_DATA_STRING,
	OLS_DATA_NUMBER,
	OLS_DATA_BOOLEAN,
	OLS_DATA_OBJECT,
	OLS_DATA_ARRAY
};

enum ols_data_number_type {
	OLS_DATA_NUM_INVALID,
	OLS_DATA_NUM_INT,
	OLS_DATA_NUM_DOUBLE
};

/* ------------------------------------------------------------------------- */
/* Main usage functions */

EXPORT ols_data_t *ols_data_create();
EXPORT ols_data_t *ols_data_create_from_json(const char *json_string);
EXPORT ols_data_t *ols_data_create_from_json_file(const char *json_file);
EXPORT ols_data_t *ols_data_create_from_json_file_safe(const char *json_file,
						       const char *backup_ext);
EXPORT void ols_data_addref(ols_data_t *data);
EXPORT void ols_data_release(ols_data_t *data);

EXPORT const char *ols_data_get_json(ols_data_t *data);
EXPORT const char *ols_data_get_json_pretty(ols_data_t *data);
EXPORT const char *ols_data_get_last_json(ols_data_t *data);
EXPORT bool ols_data_save_json(ols_data_t *data, const char *file);
EXPORT bool ols_data_save_json_safe(ols_data_t *data, const char *file,
				    const char *temp_ext,
				    const char *backup_ext);
EXPORT bool ols_data_save_json_pretty_safe(ols_data_t *data, const char *file,
					   const char *temp_ext,
					   const char *backup_ext);

EXPORT void ols_data_apply(ols_data_t *target, ols_data_t *apply_data);

EXPORT void ols_data_erase(ols_data_t *data, const char *name);
EXPORT void ols_data_clear(ols_data_t *data);

/* Set functions */
EXPORT void ols_data_set_string(ols_data_t *data, const char *name,
				const char *val);
EXPORT void ols_data_set_int(ols_data_t *data, const char *name, long long val);
EXPORT void ols_data_set_double(ols_data_t *data, const char *name, double val);
EXPORT void ols_data_set_bool(ols_data_t *data, const char *name, bool val);
EXPORT void ols_data_set_obj(ols_data_t *data, const char *name,
			     ols_data_t *obj);
EXPORT void ols_data_set_array(ols_data_t *data, const char *name,
			       ols_data_array_t *array);

/*
 * Creates an ols_data_t * filled with all default values.
 */
EXPORT ols_data_t *ols_data_get_defaults(ols_data_t *data);

/*
 * Default value functions.
 */
EXPORT void ols_data_set_default_string(ols_data_t *data, const char *name,
					const char *val);
EXPORT void ols_data_set_default_int(ols_data_t *data, const char *name,
				     long long val);
EXPORT void ols_data_set_default_double(ols_data_t *data, const char *name,
					double val);
EXPORT void ols_data_set_default_bool(ols_data_t *data, const char *name,
				      bool val);
EXPORT void ols_data_set_default_obj(ols_data_t *data, const char *name,
				     ols_data_t *obj);
EXPORT void ols_data_set_default_array(ols_data_t *data, const char *name,
				       ols_data_array_t *arr);

/*
 * Application overrides
 * Use these to communicate the actual values of settings in case the user
 * settings aren't appropriate
 */
EXPORT void ols_data_set_autoselect_string(ols_data_t *data, const char *name,
					   const char *val);
EXPORT void ols_data_set_autoselect_int(ols_data_t *data, const char *name,
					long long val);
EXPORT void ols_data_set_autoselect_double(ols_data_t *data, const char *name,
					   double val);
EXPORT void ols_data_set_autoselect_bool(ols_data_t *data, const char *name,
					 bool val);
EXPORT void ols_data_set_autoselect_obj(ols_data_t *data, const char *name,
					ols_data_t *obj);
EXPORT void ols_data_set_autoselect_array(ols_data_t *data, const char *name,
					  ols_data_array_t *arr);

/*
 * Get functions
 */
EXPORT const char *ols_data_get_string(ols_data_t *data, const char *name);
EXPORT long long ols_data_get_int(ols_data_t *data, const char *name);
EXPORT double ols_data_get_double(ols_data_t *data, const char *name);
EXPORT bool ols_data_get_bool(ols_data_t *data, const char *name);
EXPORT ols_data_t *ols_data_get_obj(ols_data_t *data, const char *name);
EXPORT ols_data_array_t *ols_data_get_array(ols_data_t *data, const char *name);

EXPORT const char *ols_data_get_default_string(ols_data_t *data,
					       const char *name);
EXPORT long long ols_data_get_default_int(ols_data_t *data, const char *name);
EXPORT double ols_data_get_default_double(ols_data_t *data, const char *name);
EXPORT bool ols_data_get_default_bool(ols_data_t *data, const char *name);
EXPORT ols_data_t *ols_data_get_default_obj(ols_data_t *data, const char *name);
EXPORT ols_data_array_t *ols_data_get_default_array(ols_data_t *data,
						    const char *name);

EXPORT const char *ols_data_get_autoselect_string(ols_data_t *data,
						  const char *name);
EXPORT long long ols_data_get_autoselect_int(ols_data_t *data,
					     const char *name);
EXPORT double ols_data_get_autoselect_double(ols_data_t *data,
					     const char *name);
EXPORT bool ols_data_get_autoselect_bool(ols_data_t *data, const char *name);
EXPORT ols_data_t *ols_data_get_autoselect_obj(ols_data_t *data,
					       const char *name);
EXPORT ols_data_array_t *ols_data_get_autoselect_array(ols_data_t *data,
						       const char *name);

/* Array functions */
EXPORT ols_data_array_t *ols_data_array_create();
EXPORT void ols_data_array_addref(ols_data_array_t *array);
EXPORT void ols_data_array_release(ols_data_array_t *array);

EXPORT size_t ols_data_array_count(ols_data_array_t *array);
EXPORT ols_data_t *ols_data_array_item(ols_data_array_t *array, size_t idx);
EXPORT size_t ols_data_array_push_back(ols_data_array_t *array,
				       ols_data_t *obj);
EXPORT void ols_data_array_insert(ols_data_array_t *array, size_t idx,
				  ols_data_t *obj);
EXPORT void ols_data_array_push_back_array(ols_data_array_t *array,
					   ols_data_array_t *array2);
EXPORT void ols_data_array_erase(ols_data_array_t *array, size_t idx);
EXPORT void ols_data_array_enum(ols_data_array_t *array,
				void (*cb)(ols_data_t *data, void *param),
				void *param);

/* ------------------------------------------------------------------------- */
/* Item status inspection */

EXPORT bool ols_data_has_user_value(ols_data_t *data, const char *name);
EXPORT bool ols_data_has_default_value(ols_data_t *data, const char *name);
EXPORT bool ols_data_has_autoselect_value(ols_data_t *data, const char *name);

EXPORT bool ols_data_item_has_user_value(ols_data_item_t *data);
EXPORT bool ols_data_item_has_default_value(ols_data_item_t *data);
EXPORT bool ols_data_item_has_autoselect_value(ols_data_item_t *data);

/* ------------------------------------------------------------------------- */
/* Clearing data values */

EXPORT void ols_data_unset_user_value(ols_data_t *data, const char *name);
EXPORT void ols_data_unset_default_value(ols_data_t *data, const char *name);
EXPORT void ols_data_unset_autoselect_value(ols_data_t *data, const char *name);

EXPORT void ols_data_item_unset_user_value(ols_data_item_t *data);
EXPORT void ols_data_item_unset_default_value(ols_data_item_t *data);
EXPORT void ols_data_item_unset_autoselect_value(ols_data_item_t *data);

/* ------------------------------------------------------------------------- */
/* Item iteration */

EXPORT ols_data_item_t *ols_data_first(ols_data_t *data);
EXPORT ols_data_item_t *ols_data_item_byname(ols_data_t *data,
					     const char *name);
EXPORT bool ols_data_item_next(ols_data_item_t **item);
EXPORT void ols_data_item_release(ols_data_item_t **item);
EXPORT void ols_data_item_remove(ols_data_item_t **item);

/* Gets Item type */
EXPORT enum ols_data_type ols_data_item_gettype(ols_data_item_t *item);
EXPORT enum ols_data_number_type ols_data_item_numtype(ols_data_item_t *item);
EXPORT const char *ols_data_item_get_name(ols_data_item_t *item);

/* Item set functions */
EXPORT void ols_data_item_set_string(ols_data_item_t **item, const char *val);
EXPORT void ols_data_item_set_int(ols_data_item_t **item, long long val);
EXPORT void ols_data_item_set_double(ols_data_item_t **item, double val);
EXPORT void ols_data_item_set_bool(ols_data_item_t **item, bool val);
EXPORT void ols_data_item_set_obj(ols_data_item_t **item, ols_data_t *val);
EXPORT void ols_data_item_set_array(ols_data_item_t **item,
				    ols_data_array_t *val);

EXPORT void ols_data_item_set_default_string(ols_data_item_t **item,
					     const char *val);
EXPORT void ols_data_item_set_default_int(ols_data_item_t **item,
					  long long val);
EXPORT void ols_data_item_set_default_double(ols_data_item_t **item,
					     double val);
EXPORT void ols_data_item_set_default_bool(ols_data_item_t **item, bool val);
EXPORT void ols_data_item_set_default_obj(ols_data_item_t **item,
					  ols_data_t *val);
EXPORT void ols_data_item_set_default_array(ols_data_item_t **item,
					    ols_data_array_t *val);

EXPORT void ols_data_item_set_autoselect_string(ols_data_item_t **item,
						const char *val);
EXPORT void ols_data_item_set_autoselect_int(ols_data_item_t **item,
					     long long val);
EXPORT void ols_data_item_set_autoselect_double(ols_data_item_t **item,
						double val);
EXPORT void ols_data_item_set_autoselect_bool(ols_data_item_t **item, bool val);
EXPORT void ols_data_item_set_autoselect_obj(ols_data_item_t **item,
					     ols_data_t *val);
EXPORT void ols_data_item_set_autoselect_array(ols_data_item_t **item,
					       ols_data_array_t *val);

/* Item get functions */
EXPORT const char *ols_data_item_get_string(ols_data_item_t *item);
EXPORT long long ols_data_item_get_int(ols_data_item_t *item);
EXPORT double ols_data_item_get_double(ols_data_item_t *item);
EXPORT bool ols_data_item_get_bool(ols_data_item_t *item);
EXPORT ols_data_t *ols_data_item_get_obj(ols_data_item_t *item);
EXPORT ols_data_array_t *ols_data_item_get_array(ols_data_item_t *item);

EXPORT const char *ols_data_item_get_default_string(ols_data_item_t *item);
EXPORT long long ols_data_item_get_default_int(ols_data_item_t *item);
EXPORT double ols_data_item_get_default_double(ols_data_item_t *item);
EXPORT bool ols_data_item_get_default_bool(ols_data_item_t *item);
EXPORT ols_data_t *ols_data_item_get_default_obj(ols_data_item_t *item);
EXPORT ols_data_array_t *ols_data_item_get_default_array(ols_data_item_t *item);

EXPORT const char *ols_data_item_get_autoselect_string(ols_data_item_t *item);
EXPORT long long ols_data_item_get_autoselect_int(ols_data_item_t *item);
EXPORT double ols_data_item_get_autoselect_double(ols_data_item_t *item);
EXPORT bool ols_data_item_get_autoselect_bool(ols_data_item_t *item);
EXPORT ols_data_t *ols_data_item_get_autoselect_obj(ols_data_item_t *item);
EXPORT ols_data_array_t *
ols_data_item_get_autoselect_array(ols_data_item_t *item);


static inline ols_data_t *ols_data_newref(ols_data_t *data)
{
	if (data)
		ols_data_addref(data);
	else
		data = ols_data_create();

	return data;
}

#ifdef __cplusplus
}
#endif
