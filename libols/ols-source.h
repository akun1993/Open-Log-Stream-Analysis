/******************************************************************************
    Copyright (C) 2023 by Kun Zhang <akun1993@163.com>

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
#include "ols-ref.h"
#include "ols.h"

/**
 * @file
 * @brief header for modules implementing sources.
 *
 * Sources are modules that either feed data to libols or modify it.
 */

#ifdef __cplusplus
extern "C" {
#endif

enum ols_source_type {
	OLS_SOURCE_TYPE_INPUT,
	OLS_SOURCE_TYPE_FILTER,
};


/**
 * Source should not be fully duplicated
 *
 * When this is used, specifies that the source should not be fully duplicated,
 * and should prefer to duplicate via holding references rather than full
 * duplication.
 */
#define OLS_SOURCE_DO_NOT_DUPLICATE (1 << 0)

/**
 * Source definition structure
 */
struct ols_source_info {
	/* ----------------------------------------------------------------- */
	/* Required implementation*/

	/** Unique string identifier for the source */
	const char *id;

	/**
	 * Type of source.
	 *
	 * OLS_SOURCE_TYPE_INPUT for input sources,
	 */
	enum ols_source_type type;

		/** Source output flags */
	uint32_t output_flags;

	/**
	 * Get the translated name of the source type
	 *
	 * @param  type_data  The type_data variable of this structure
	 * @return               The translated name of the source type
	 */
	const char *(*get_name)(void *type_data);

	/**
	 * Creates the source data for the source
	 *
	 * @param  settings  Settings to initialize the source with
	 * @param  source    Source that this data is associated with
	 * @return           The data associated with this source
	 */
	void *(*create)(ols_data_t *settings, ols_source_t *source);

	/**
	 * Destroys the private data for the source
	 *
	 * Async sources must not call ols_source_output_video after returning
	 * from destroy
	 */
	void (*destroy)(void *data);


		/**
	 * Destroys the private data for the source
	 *
	 * Async sources must not call ols_source_output_video after returning
	 * from destroy
	 */
	int (*get_data)(void *data,ols_buffer_t *buf);


	/* ----------------------------------------------------------------- */
	/* Optional implementation */

	/**
	 * Gets the default settings for this source
	 *
	 * @param[out]  settings  Data to assign default settings to           
	 */
	void (*get_defaults)(ols_data_t *settings);

	/**
	 * Gets the property information of this source
	 *
	 * @return         The properties data 
	 */
	ols_properties_t *(*get_properties)(void *data);


	/**
	 * Gets a  pad  of this source
	 *
	 * @return         a new pad 
	 */
	ols_pad_t *(*get_new_pad)(void *data);


	/**
	 * Updates the settings for this source
	 *
	 * @param data      Source data
	 * @param settings  New settings for this source
	 */
	void (*update)(void *data, ols_data_t *settings);



	/** Called when the source has been activated in the main view */
	void (*activate)(void *data);

	/**
	 * Called when the source has been deactivated from the main view
	 * (no longer being played/displayed)
	 */
	void (*deactivate)(void *data);

	/** Called when the source is disable */
	void (*disable)(void *data);

	/** Called when the source is enable */
	void (*enable)(void *data);

	/**
	 * Called when saving a source.  This is a separate function because
	 * sometimes a source needs to know when it is being saved so it
	 * doesn't always have to update the current settings until a certain
	 * point.
	 *
	 * @param  data      Source data
	 * @param  settings  Settings
	 */
	void (*save)(void *data, ols_data_t *settings);

	/**
	 * Called when loading a source from saved data.  This should be called
	 * after all the loading sources have actually been created because
	 * sometimes there are sources that depend on each other.
	 *
	 * @param  data      Source data
	 * @param  settings  Settings
	 */
	void (*load)(void *data, ols_data_t *settings);

	/**
	 * Private data associated with this entry
	 */


	void *type_data;
	/** Icon type for the source */
	enum ols_icon_type icon_type;
	/**
	 * If defined, called to free private data on shutdown
	 */
	void (*free_type_data)(void *type_data);


	uint32_t version; /* increment if needed to specify a new version */
};


/* ------------------------------------------------------------------------- */
/* sources  */

struct ols_weak_source {
	struct ols_weak_ref ref;
	struct ols_source *source;
  };
  
struct ols_source {
	struct ols_context_data context;
	struct ols_source_info info;
  
	ols_pad_t *srcpad;
  
	/* general exposed flags that can be set for the source */
	uint32_t flags;
	uint32_t default_flags;
	uint32_t last_ols_ver;
  
	/* indicates ownership of the info.id buffer */
	bool owns_info_id;
  
	/* signals to call the source update in the video thread */
	long defer_update_count;
  
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
  

EXPORT void ols_register_source_s(const struct ols_source_info *info,
				  size_t size);

/**
 * Registers a source definition to the current ols context.  This should be
 * used in ols_module_load.
 *
 * @param  info  Pointer to the source definition structure
 */
#define ols_register_source(info) \
	ols_register_source_s(info, sizeof(struct ols_source_info))

#ifdef __cplusplus
}
#endif
