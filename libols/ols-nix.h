/******************************************************************************
    Copyright (C) 2020 by Georges Basile Stavracas Neto
<georges.stavracas@gmail.com>

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

#include "ols-internal.h"

struct ols_nix_hotkeys_vtable {
  bool (*init)(struct ols_core_hotkeys *hotkeys);

  void (*free)(struct ols_core_hotkeys *hotkeys);

  bool (*is_pressed)(ols_hotkeys_platform_t *context, ols_key_t key);

  void (*key_to_str)(ols_key_t key, struct dstr *dstr);

  ols_key_t (*key_from_virtual_key)(int sym);

  int (*key_to_virtual_key)(ols_key_t key);
};

#ifdef __cplusplus
}
#endif
