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

#ifdef __cplusplus
extern "C" {
#endif

#define OLS_OUTPUT_SERVICE (1 << 3)
#define OLS_OUTPUT_CAN_PAUSE (1 << 5)


struct ols_output_info {
	/* required */
	const char *id;

	uint32_t flags;

	const char *(*get_name)(void *type_data);

	void *(*create)(ols_data_t *settings, ols_output_t *output);
	void (*destroy)(void *data);

	bool (*start)(void *data);
	void (*stop)(void *data, uint64_t ts);

	/* optional */
	void (*update)(void *data, ols_data_t *settings);

	void (*get_defaults)(ols_data_t *settings);

	ols_properties_t *(*get_properties)(void *data);

	uint64_t (*get_total_bytes)(void *data);

	int (*get_dropped_frames)(void *data);

	void *type_data;
	void (*free_type_data)(void *type_data);

	/* required if OLS_OUTPUT_SERVICE */
	const char *protocols;
};

EXPORT void ols_register_output_s(const struct ols_output_info *info,
				  size_t size);

#define ols_register_output(info) \
	ols_register_output_s(info, sizeof(struct ols_output_info))

#ifdef __cplusplus
}
#endif
