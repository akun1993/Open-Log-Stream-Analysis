/******************************************************************************
    Copyright (C) 2015 by Andrew Skinner <ols@theandyroid.com>
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

/* ---------------------------- */

#define SWIG_TYPE_TABLE olspython
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4115)
#pragma warning(disable : 4204)
#endif

#include "ols-scripting-python-import.h"

#include <structmember.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/* ---------------------------- */

#include "ols-scripting-callback.h"
#include "ols-scripting-internal.h"

#ifdef _WIN32
#define __func__ __FUNCTION__
#else
#include <dlfcn.h>
#endif

#include <callback/calldata.h>
#include <util/base.h>
#include <util/threading.h>

#define do_log(level, format, ...)                                             \
  blog(level, "[Python] " format, ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

/* ------------------------------------------------------------ */

struct python_ols_callback;

struct ols_python_script {
  ols_script_t base;

  struct dstr dir;
  struct dstr name;

  PyObject *module;

  PyObject *save;
  PyObject *update;
  PyObject *get_properties;

  struct script_callback *first_callback;

  PyObject *tick;
  struct ols_python_script *next_tick;
  struct ols_python_script **p_prev_next_tick;
};

/* ------------------------------------------------------------ */

struct python_ols_callback {
  struct script_callback base;

  PyObject *func;
};

static inline struct python_ols_callback *
add_python_ols_callback_extra(struct ols_python_script *script, PyObject *func,
                              size_t extra_size) {
  struct python_ols_callback *cb =
      add_script_callback(&script->first_callback, (ols_script_t *)script,
                          sizeof(*cb) + extra_size);

  Py_XINCREF(func);
  cb->func = func;
  return cb;
}

static inline struct python_ols_callback *
add_python_ols_callback(struct ols_python_script *script, PyObject *func) {
  return add_python_ols_callback_extra(script, func, 0);
}

static inline void *
python_ols_callback_extra_data(struct python_ols_callback *cb) {
  return (void *)&cb[1];
}

static inline struct ols_python_script *
python_ols_callback_script(struct python_ols_callback *cb) {
  return (struct ols_python_script *)cb->base.script;
}

static inline struct python_ols_callback *
find_next_python_ols_callback(struct ols_python_script *script,
                              struct python_ols_callback *cb, PyObject *func) {
  cb = cb ? (struct python_ols_callback *)cb->base.next
          : (struct python_ols_callback *)script->first_callback;

  while (cb) {
    if (cb->func == func)
      break;
    cb = (struct python_ols_callback *)cb->base.next;
  }

  return cb;
}

static inline struct python_ols_callback *
find_python_ols_callback(struct ols_python_script *script, PyObject *func) {
  return find_next_python_ols_callback(script, NULL, func);
}

static inline void remove_python_ols_callback(struct python_ols_callback *cb) {
  remove_script_callback(&cb->base);

  Py_XDECREF(cb->func);
  cb->func = NULL;
}

static inline void
just_free_python_ols_callback(struct python_ols_callback *cb) {
  just_free_script_callback(&cb->base);
}

static inline void free_python_ols_callback(struct python_ols_callback *cb) {
  free_script_callback(&cb->base);
}

/* ------------------------------------------------------------ */

static int parse_args_(PyObject *args, const char *func, const char *format,
                       ...) {
  char new_format[128];
  va_list va_args;
  int ret;

  snprintf(new_format, sizeof(new_format), "%s:%s", format, func);

  va_start(va_args, format);
  ret = PyArg_VaParse(args, new_format, va_args);
  va_end(va_args);

  return ret;
}

#define parse_args(args, format, ...)                                          \
  parse_args_(args, __FUNCTION__, format, ##__VA_ARGS__)

static inline bool py_error_(const char *func, int line) {
  if (PyErr_Occurred()) {
    warn("Python failure in %s:%d:", func, line);
    PyErr_Print();
    return true;
  }
  return false;
}

#define py_error() py_error_(__FUNCTION__, __LINE__)

#define lock_python() PyGILState_STATE gstate = PyGILState_Ensure()
#define relock_python() gstate = PyGILState_Ensure()
#define unlock_python() PyGILState_Release(gstate)

struct py_source;
typedef struct py_source py_source_t;

extern PyObject *py_libols;
extern struct python_ols_callback *cur_python_cb;
extern struct ols_python_script *cur_python_script;

extern void add_functions_to_py_module(PyObject *module,
                                       PyMethodDef *method_list);

/* ------------------------------------------------------------ */
/* Warning: the following functions expect python to be locked! */

static inline PyObject *python_none(void) {
  PyObject *none = Py_None;
  Py_INCREF(none);
  return none;
}
