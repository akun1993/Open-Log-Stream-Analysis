/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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
#include "ols-data.h"

/**
 * @file
 * @brief libols header for the properties system used in libols
 *
 * @page properties Properties
 * @brief Platform and Toolkit independent settings implementation
 *
 * @section prop_overview_sec Overview
 *
 * libols uses a property system which lets for example sources specify
 * settings that can be displayed to the user by the UI.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Only update when the user presses OK or Apply */
#define OLS_PROPERTIES_DEFER_UPDATE (1 << 0)

enum ols_property_type {
	OLS_PROPERTY_INVALID,
	OLS_PROPERTY_BOOL,
	OLS_PROPERTY_INT,
	OLS_PROPERTY_FLOAT,
	OLS_PROPERTY_TEXT,
	OLS_PROPERTY_PATH,
	OLS_PROPERTY_LIST,
	OLS_PROPERTY_COLOR,
	OLS_PROPERTY_BUTTON,
	OLS_PROPERTY_FONT,
	OLS_PROPERTY_EDITABLE_LIST,
	OLS_PROPERTY_GROUP,
	OLS_PROPERTY_COLOR_ALPHA,
};

enum ols_combo_format {
	OLS_COMBO_FORMAT_INVALID,
	OLS_COMBO_FORMAT_INT,
	OLS_COMBO_FORMAT_FLOAT,
	OLS_COMBO_FORMAT_STRING,
	OLS_COMBO_FORMAT_BOOL,
};

enum ols_combo_type {
	OLS_COMBO_TYPE_INVALID,
	OLS_COMBO_TYPE_EDITABLE,
	OLS_COMBO_TYPE_LIST,
	OLS_COMBO_TYPE_RADIO,
};

enum ols_editable_list_type {
	OLS_EDITABLE_LIST_TYPE_STRINGS,
	OLS_EDITABLE_LIST_TYPE_FILES,
	OLS_EDITABLE_LIST_TYPE_FILES_AND_URLS,
};

enum ols_path_type {
	OLS_PATH_FILE,
	OLS_PATH_FILE_SAVE,
	OLS_PATH_DIRECTORY,
};

enum ols_text_type {
	OLS_TEXT_DEFAULT,
	OLS_TEXT_PASSWORD,
	OLS_TEXT_MULTILINE,
	OLS_TEXT_INFO,
};

enum ols_text_info_type {
	OLS_TEXT_INFO_NORMAL,
	OLS_TEXT_INFO_WARNING,
	OLS_TEXT_INFO_ERROR,
};

enum ols_number_type {
	OLS_NUMBER_SCROLLER,
	OLS_NUMBER_SLIDER,
};

enum ols_group_type {
	OLS_COMBO_INVALID,
	OLS_GROUP_NORMAL,
	OLS_GROUP_CHECKABLE,
};

enum ols_button_type {
	OLS_BUTTON_DEFAULT,
	OLS_BUTTON_URL,
};

#define OLS_FONT_BOLD (1 << 0)
#define OLS_FONT_ITALIC (1 << 1)
#define OLS_FONT_UNDERLINE (1 << 2)
#define OLS_FONT_STRIKEOUT (1 << 3)

struct ols_properties;
struct ols_property;
typedef struct ols_properties ols_properties_t;
typedef struct ols_property ols_property_t;

/* ------------------------------------------------------------------------- */

EXPORT ols_properties_t *ols_properties_create(void);
EXPORT ols_properties_t *
ols_properties_create_param(void *param, void (*destroy)(void *param));
EXPORT void ols_properties_destroy(ols_properties_t *props);

EXPORT void ols_properties_set_flags(ols_properties_t *props, uint32_t flags);
EXPORT uint32_t ols_properties_get_flags(ols_properties_t *props);

EXPORT void ols_properties_set_param(ols_properties_t *props, void *param,
				     void (*destroy)(void *param));
EXPORT void *ols_properties_get_param(ols_properties_t *props);

EXPORT ols_property_t *ols_properties_first(ols_properties_t *props);

EXPORT ols_property_t *ols_properties_get(ols_properties_t *props,
					  const char *property);

EXPORT ols_properties_t *ols_properties_get_parent(ols_properties_t *props);

/** Remove a property from a properties list.
 *
 * Removes a property from a properties list. Only valid in either
 * get_properties or modified_callback(2). modified_callback(2) must return
 * true so that all UI properties are rebuilt and returning false is undefined
 * behavior.
 *
 * @param props Properties to remove from.
 * @param property Name of the property to remove.
 */
EXPORT void ols_properties_remove_by_name(ols_properties_t *props,
					  const char *property);

/**
 * Applies settings to the properties by calling all the necessary
 * modification callbacks
 */
EXPORT void ols_properties_apply_settings(ols_properties_t *props,
					  ols_data_t *settings);

/* ------------------------------------------------------------------------- */

/**
 * Callback for when a button property is clicked.  If the properties
 * need to be refreshed due to changes to the property layout, return true,
 * otherwise return false.
 */
typedef bool (*ols_property_clicked_t)(ols_properties_t *props,
				       ols_property_t *property, void *data);

EXPORT ols_property_t *ols_properties_add_bool(ols_properties_t *props,
					       const char *name,
					       const char *description);

EXPORT ols_property_t *ols_properties_add_int(ols_properties_t *props,
					      const char *name,
					      const char *description, int min,
					      int max, int step);

EXPORT ols_property_t *ols_properties_add_float(ols_properties_t *props,
						const char *name,
						const char *description,
						double min, double max,
						double step);

EXPORT ols_property_t *ols_properties_add_int_slider(ols_properties_t *props,
						     const char *name,
						     const char *description,
						     int min, int max,
						     int step);

EXPORT ols_property_t *ols_properties_add_float_slider(ols_properties_t *props,
						       const char *name,
						       const char *description,
						       double min, double max,
						       double step);

EXPORT ols_property_t *ols_properties_add_text(ols_properties_t *props,
					       const char *name,
					       const char *description,
					       enum ols_text_type type);

/**
 * Adds a 'path' property.  Can be a directory or a file.
 *
 * If target is a file path, the filters should be this format, separated by
 * double semicolons, and extensions separated by space:
 *   "Example types 1 and 2 (*.ex1 *.ex2);;Example type 3 (*.ex3)"
 *
 * @param  props        Properties object
 * @param  name         Settings name
 * @param  description  Description (display name) of the property
 * @param  type         Type of path (directory or file)
 * @param  filter       If type is a file path, then describes the file filter
 *                      that the user can browse.  Items are separated via
 *                      double semicolons.  If multiple file types in a
 *                      filter, separate with space.
 */
EXPORT ols_property_t *
ols_properties_add_path(ols_properties_t *props, const char *name,
			const char *description, enum ols_path_type type,
			const char *filter, const char *default_path);

EXPORT ols_property_t *ols_properties_add_list(ols_properties_t *props,
					       const char *name,
					       const char *description,
					       enum ols_combo_type type,
					       enum ols_combo_format format);

EXPORT ols_property_t *ols_properties_add_color(ols_properties_t *props,
						const char *name,
						const char *description);

EXPORT ols_property_t *ols_properties_add_color_alpha(ols_properties_t *props,
						      const char *name,
						      const char *description);

EXPORT ols_property_t *
ols_properties_add_button(ols_properties_t *props, const char *name,
			  const char *text, ols_property_clicked_t callback);

EXPORT ols_property_t *
ols_properties_add_button2(ols_properties_t *props, const char *name,
			   const char *text, ols_property_clicked_t callback,
			   void *priv);

/**
 * Adds a font selection property.
 *
 * A font is an ols_data sub-object which contains the following items:
 *   face:   face name string
 *   style:  style name string
 *   size:   size integer
 *   flags:  font flags integer (OLS_FONT_* defined above)
 */
EXPORT ols_property_t *ols_properties_add_font(ols_properties_t *props,
					       const char *name,
					       const char *description);

EXPORT ols_property_t *
ols_properties_add_editable_list(ols_properties_t *props, const char *name,
				 const char *description,
				 enum ols_editable_list_type type,
				 const char *filter, const char *default_path);


EXPORT ols_property_t *ols_properties_add_group(ols_properties_t *props,
						const char *name,
						const char *description,
						enum ols_group_type type,
						ols_properties_t *group);

/* ------------------------------------------------------------------------- */

/**
 * Optional callback for when a property is modified.  If the properties
 * need to be refreshed due to changes to the property layout, return true,
 * otherwise return false.
 */
typedef bool (*ols_property_modified_t)(ols_properties_t *props,
					ols_property_t *property,
					ols_data_t *settings);
typedef bool (*ols_property_modified2_t)(void *priv, ols_properties_t *props,
					 ols_property_t *property,
					 ols_data_t *settings);

EXPORT void
ols_property_set_modified_callback(ols_property_t *p,
				   ols_property_modified_t modified);
EXPORT void ols_property_set_modified_callback2(
	ols_property_t *p, ols_property_modified2_t modified, void *priv);

EXPORT bool ols_property_modified(ols_property_t *p, ols_data_t *settings);
EXPORT bool ols_property_button_clicked(ols_property_t *p, void *obj);

EXPORT void ols_property_set_visible(ols_property_t *p, bool visible);
EXPORT void ols_property_set_enabled(ols_property_t *p, bool enabled);

EXPORT void ols_property_set_description(ols_property_t *p,
					 const char *description);
EXPORT void ols_property_set_long_description(ols_property_t *p,
					      const char *long_description);

EXPORT const char *ols_property_name(ols_property_t *p);
EXPORT const char *ols_property_description(ols_property_t *p);
EXPORT const char *ols_property_long_description(ols_property_t *p);
EXPORT enum ols_property_type ols_property_get_type(ols_property_t *p);
EXPORT bool ols_property_enabled(ols_property_t *p);
EXPORT bool ols_property_visible(ols_property_t *p);

EXPORT bool ols_property_next(ols_property_t **p);

EXPORT int ols_property_int_min(ols_property_t *p);
EXPORT int ols_property_int_max(ols_property_t *p);
EXPORT int ols_property_int_step(ols_property_t *p);
EXPORT enum ols_number_type ols_property_int_type(ols_property_t *p);
EXPORT const char *ols_property_int_suffix(ols_property_t *p);
EXPORT double ols_property_float_min(ols_property_t *p);
EXPORT double ols_property_float_max(ols_property_t *p);
EXPORT double ols_property_float_step(ols_property_t *p);
EXPORT enum ols_number_type ols_property_float_type(ols_property_t *p);
EXPORT const char *ols_property_float_suffix(ols_property_t *p);
EXPORT enum ols_text_type ols_property_text_type(ols_property_t *p);
EXPORT bool ols_property_text_monospace(ols_property_t *p);
EXPORT enum ols_text_info_type ols_property_text_info_type(ols_property_t *p);
EXPORT bool ols_property_text_info_word_wrap(ols_property_t *p);
EXPORT enum ols_path_type ols_property_path_type(ols_property_t *p);
EXPORT const char *ols_property_path_filter(ols_property_t *p);
EXPORT const char *ols_property_path_default_path(ols_property_t *p);
EXPORT enum ols_combo_type ols_property_list_type(ols_property_t *p);
EXPORT enum ols_combo_format ols_property_list_format(ols_property_t *p);

EXPORT void ols_property_int_set_limits(ols_property_t *p, int min, int max,
					int step);
EXPORT void ols_property_float_set_limits(ols_property_t *p, double min,
					  double max, double step);
EXPORT void ols_property_int_set_suffix(ols_property_t *p, const char *suffix);
EXPORT void ols_property_float_set_suffix(ols_property_t *p,
					  const char *suffix);
EXPORT void ols_property_text_set_monospace(ols_property_t *p, bool monospace);
EXPORT void ols_property_text_set_info_type(ols_property_t *p,
					    enum ols_text_info_type type);
EXPORT void ols_property_text_set_info_word_wrap(ols_property_t *p,
						 bool word_wrap);

EXPORT void ols_property_button_set_type(ols_property_t *p,
					 enum ols_button_type type);
EXPORT void ols_property_button_set_url(ols_property_t *p, char *url);

EXPORT void ols_property_list_clear(ols_property_t *p);

EXPORT size_t ols_property_list_add_string(ols_property_t *p, const char *name,
					   const char *val);
EXPORT size_t ols_property_list_add_int(ols_property_t *p, const char *name,
					long long val);
EXPORT size_t ols_property_list_add_float(ols_property_t *p, const char *name,
					  double val);
EXPORT size_t ols_property_list_add_bool(ols_property_t *p, const char *name,
					 bool val);

EXPORT void ols_property_list_insert_string(ols_property_t *p, size_t idx,
					    const char *name, const char *val);
EXPORT void ols_property_list_insert_int(ols_property_t *p, size_t idx,
					 const char *name, long long val);
EXPORT void ols_property_list_insert_float(ols_property_t *p, size_t idx,
					   const char *name, double val);
EXPORT void ols_property_list_insert_bool(ols_property_t *p, size_t idx,
					  const char *name, bool val);

EXPORT void ols_property_list_item_disable(ols_property_t *p, size_t idx,
					   bool disabled);
EXPORT bool ols_property_list_item_disabled(ols_property_t *p, size_t idx);

EXPORT void ols_property_list_item_remove(ols_property_t *p, size_t idx);

EXPORT size_t ols_property_list_item_count(ols_property_t *p);
EXPORT const char *ols_property_list_item_name(ols_property_t *p, size_t idx);
EXPORT const char *ols_property_list_item_string(ols_property_t *p, size_t idx);
EXPORT long long ols_property_list_item_int(ols_property_t *p, size_t idx);
EXPORT double ols_property_list_item_float(ols_property_t *p, size_t idx);
EXPORT bool ols_property_list_item_bool(ols_property_t *p, size_t idx);

EXPORT enum ols_editable_list_type
ols_property_editable_list_type(ols_property_t *p);
EXPORT const char *ols_property_editable_list_filter(ols_property_t *p);
EXPORT const char *ols_property_editable_list_default_path(ols_property_t *p);

EXPORT enum ols_group_type ols_property_group_type(ols_property_t *p);
EXPORT ols_properties_t *ols_property_group_content(ols_property_t *p);

EXPORT enum ols_button_type ols_property_button_type(ols_property_t *p);
EXPORT const char *ols_property_button_url(ols_property_t *p);


#ifdef __cplusplus
}
#endif
