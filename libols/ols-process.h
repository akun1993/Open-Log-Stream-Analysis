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

#include "ols-ref.h"
#include "ols.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Process should not be fully duplicated
 *
 * When this is used, specifies that the process should not be fully duplicated,
 * and should prefer to duplicate via holding references rather than full
 * duplication.
 */
#define OLS_PROCESS_DO_NOT_DUPLICATE (1 << 0)

enum ols_process_type {
  OLS_PROCESS_TYPE_INPUT,
  OLS_PROCESS_TYPE_FILTER,
};


struct ols_process_info {
  /* required */
  const char *id;

  uint32_t flags;

  /**
   * Type of process.
   *
   */
  enum ols_process_type type;
  /** Source output flags */
  uint32_t output_flags;

  const char *(*get_name)(void *type_data);

  void *(*create)(ols_data_t *settings, ols_process_t *process);
  void (*destroy)(void *data);

  bool (*start)(void *data);
  void (*stop)(void *data, uint64_t ts);

  /** Called when the source has been activated in the main view */
  void (*activate)(void *data);
  /**
   * Called when the source has been deactivated from the main view
   * (no longer being played/displayed)
   */
  void (*deactivate)(void *data);

  /* optional */
  void (*update)(void *data, ols_data_t *settings);

  void (*get_defaults)(ols_data_t *settings);

  ols_properties_t *(*get_properties)(void *data);

  	/**
	 * Gets a  pad  of this process
	 *
	 * @return         a new pad 
	 */
	ols_pad_t *(*request_new_pad)(void *data,const char *name, const char *caps);

  uint64_t (*get_total_bytes)(void *data);

  int (*get_dropped_frames)(void *data);

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

  void *type_data;
  void (*free_type_data)(void *type_data);

  /** Icon type for the source */
  enum ols_icon_type icon_type;

  uint32_t version; /* increment if needed to specify a new version */
};

/* ------------------------------------------------------------------------- */
/* process  */

struct ols_weak_process {
  struct ols_weak_ref ref;
  struct ols_process *process;
};

struct ols_process {
  struct ols_context_data context;
  struct ols_process_info info;

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

  bool active;

  uint64_t last_frame_ts;
  uint64_t last_sys_timestamp;

  /* private data */
  /*< protected >*/
  ols_pad_t *sinkpad;
  ols_data_t *private_settings;
};

EXPORT void ols_register_process_s(const struct ols_process_info *info,
                                   size_t size);

#define ols_register_process(info)                                             \
  ols_register_process_s(info, sizeof(struct ols_process_info))

#ifdef __cplusplus
}
#endif
