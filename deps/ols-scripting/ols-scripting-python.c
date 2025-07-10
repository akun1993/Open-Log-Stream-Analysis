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

#include "ols-scripting-python.h"
#include "ols-scripting-config.h"
#include <util/base.h>
#include <util/darray.h>
#include <util/dstr.h>
#include <util/platform.h>

#include <ols.h>

/* ========================================================================= */

// #define DEBUG_PYTHON_STARTUP

static const char *startup_script = "\n\
import sys\n\
import os\n\
import olspython\n\
class stdout_logger(object):\n\
	def write(self, message):\n\
		olspython.script_log_no_endl(olspython.LOG_INFO, message)\n\
	def flush(self):\n\
		pass\n\
class stderr_logger(object):\n\
	def write(self, message):\n\
		olspython.script_log_no_endl(olspython.LOG_ERROR, message)\n\
	def flush(self):\n\
		pass\n\
os.environ['PYTHONUNBUFFERED'] = '1'\n\
sys.stdout = stdout_logger()\n\
sys.stderr = stderr_logger()\n";

#if RUNTIME_LINK
static wchar_t home_path[1024] = {0};
static python_version_t python_version = {0};
#endif

DARRAY(char *) python_paths;
static bool python_loaded = false;
static bool mutexes_loaded = false;

static PyObject *py_olspython = NULL;
struct ols_python_script *cur_python_script = NULL;
struct python_ols_callback *cur_python_cb = NULL;

#define lock_callback(cb)                                                      \
  lock_python();                                                               \
  struct ols_python_script *__last_script = cur_python_script;                 \
  struct python_ols_callback *__last_cb = cur_python_cb;                       \
  cur_python_script = (struct ols_python_script *)cb->base.script;             \
  cur_python_cb = cb
#define unlock_callback()                                                      \
  cur_python_cb = __last_cb;                                                   \
  cur_python_script = __last_script;                                           \
  unlock_python()

/* ========================================================================= */

void add_functions_to_py_module(PyObject *module, PyMethodDef *method_list) {
  PyObject *dict = PyModule_GetDict(module);
  PyObject *name = PyModule_GetNameObject(module);
  if (!dict || !name) {
    return;
  }
  for (PyMethodDef *ml = method_list; ml->ml_name != NULL; ml++) {
    PyObject *func = PyCFunction_NewEx(ml, module, name);
    if (!func) {
      continue;
    }
    PyDict_SetItemString(dict, ml->ml_name, func);
    Py_DECREF(func);
  }
  Py_DECREF(name);
}

/* -------------------------------------------- */

static PyObject *py_get_current_script_path(PyObject *self, PyObject *args) {
  PyObject *dir;

  UNUSED_PARAMETER(args);

  dir = PyDict_GetItemString(PyModule_GetDict(self), "__script_dir__");
  Py_XINCREF(dir);
  return dir;
}

static bool load_python_script(struct ols_python_script *data) {
  PyObject *py_file = NULL;
  PyObject *py_module = NULL;
  PyObject *py_success = NULL;
  PyObject *py_tick = NULL;
  PyObject *py_load = NULL;
  PyObject *py_defaults = NULL;
  bool success = false;
  int ret;

  cur_python_script = data;

  if (!data->module) {
    py_file = PyUnicode_FromString(data->name.array);
    py_module = PyImport_Import(py_file);
  } else {
    py_module = PyImport_ReloadModule(data->module);
  }

  if (!py_module) {
    py_error();
    goto fail;
  }

  Py_XINCREF(py_olspython);
  ret = PyModule_AddObject(py_module, "olspython", py_olspython);
  if (py_error() || ret != 0)
    goto fail;

  ret =
      PyModule_AddStringConstant(py_module, "__script_dir__", data->dir.array);
  if (py_error() || ret != 0)
    goto fail;

  data->parse = PyObject_GetAttrString(py_module, "parse_str");
  if (!data->parse)
    PyErr_Clear();

  static PyMethodDef global_funcs[] = {{"script_path",
                                        py_get_current_script_path, METH_NOARGS,
                                        "Gets the script path"},
                                       {0}};

  add_functions_to_py_module(py_module, global_funcs);

  if (data->module)
    Py_XDECREF(data->module);
  data->module = py_module;
  py_module = NULL;

  success = true;

fail:
  Py_XDECREF(py_load);
  Py_XDECREF(py_tick);
  Py_XDECREF(py_defaults);
  Py_XDECREF(py_success);
  Py_XDECREF(py_file);
  if (!success)
    Py_XDECREF(py_module);
  cur_python_script = NULL;
  return success;
}

static void unload_python_script(struct ols_python_script *data) {
  PyObject *py_module = data->module;
  PyObject *py_func = NULL;
  PyObject *py_ret = NULL;

  cur_python_script = data;

fail:
  Py_XDECREF(py_ret);
  Py_XDECREF(py_func);

  cur_python_script = NULL;
}

static void add_to_python_path(const char *path) {
  PyObject *py_path_str = NULL;
  PyObject *py_path = NULL;
  int ret;

  if (!path || !*path)
    return;

  for (size_t i = 0; i < python_paths.num; i++) {
    const char *python_path = python_paths.array[i];
    if (strcmp(path, python_path) == 0)
      return;
  }

  ret = PyRun_SimpleString("import sys");
  if (py_error() || ret != 0)
    goto fail;

  /* borrowed reference here */
  py_path = PySys_GetObject("path");
  if (py_error() || !py_path)
    goto fail;

  py_path_str = PyUnicode_FromString(path);
  ret = PyList_Append(py_path, py_path_str);
  if (py_error() || ret != 0)
    goto fail;

  char *new_path = bstrdup(path);
  da_push_back(python_paths, &new_path);

fail:
  Py_XDECREF(py_path_str);
}

struct dstr cur_py_log_chunk = {0};

static PyObject *py_script_log_internal(PyObject *self, PyObject *args,
                                        bool add_endl) {
  static bool calling_self = false;
  int log_level;
  const char *msg;

  UNUSED_PARAMETER(self);

  if (calling_self)
    return python_none();
  calling_self = true;

  /* ------------------- */

  if (!parse_args(args, "is", &log_level, &msg))
    goto fail;
  if (!msg || !*msg)
    goto fail;

  dstr_cat(&cur_py_log_chunk, msg);
  if (add_endl)
    dstr_cat(&cur_py_log_chunk, "\n");

  const char *start = cur_py_log_chunk.array;
  char *endl = strchr(start, '\n');

  while (endl) {
    *endl = 0;
    if (cur_python_script)
      script_log(&cur_python_script->base, log_level, "%s", start);
    else
      script_log(NULL, log_level, "%s", start);
    *endl = '\n';

    start = endl + 1;
    endl = strchr(start, '\n');
  }

  if (start) {
    size_t len = strlen(start);
    if (len)
      memmove(cur_py_log_chunk.array, start, len);
    dstr_resize(&cur_py_log_chunk, len);
  }

  /* ------------------- */

fail:
  calling_self = false;
  return python_none();
}

static PyObject *py_script_log_no_endl(PyObject *self, PyObject *args) {
  return py_script_log_internal(self, args, false);
}

static PyObject *py_script_log(PyObject *self, PyObject *args) {
  return py_script_log_internal(self, args, true);
}

/* -------------------------------------------- */

static void add_hook_functions(PyObject *module) {
  static PyMethodDef funcs[] = {
#define DEF_FUNC(n, c) {n, c, METH_VARARGS, NULL}

      DEF_FUNC("script_log_no_endl", py_script_log_no_endl),
      DEF_FUNC("script_log", py_script_log),

#undef DEF_FUNC
      {0}};

  add_functions_to_py_module(module, funcs);
}

/* -------------------------------------------- */

bool ols_python_script_load(ols_script_t *s) {
  struct ols_python_script *data = (struct ols_python_script *)s;
  if (python_loaded && !data->base.loaded) {
    lock_python();
    if (!data->module)
      add_to_python_path(data->dir.array);
    data->base.loaded = load_python_script(data);
    unlock_python();

    if (data->base.loaded) {
      blog(LOG_INFO, "[ols-scripting]: Loaded python script: %s",
           data->base.file.array);
    }
  }

  return data->base.loaded;
}

ols_script_t *ols_python_script_create(const char *path, ols_data_t *settings) {
  struct ols_python_script *data = bzalloc(sizeof(*data));

  data->base.type = OLS_SCRIPT_LANG_PYTHON;

  dstr_copy(&data->base.path, path);
  dstr_replace(&data->base.path, "\\", "/");
  path = data->base.path.array;

  const char *slash = path && *path ? strrchr(path, '/') : NULL;
  if (slash) {
    slash++;
    dstr_copy(&data->base.file, slash);
    dstr_left(&data->dir, &data->base.path, slash - path);
  } else {
    dstr_copy(&data->base.file, path);
  }

  path = data->base.file.array;
  dstr_copy_dstr(&data->name, &data->base.file);

  const char *ext = strstr(path, ".py");
  if (ext)
    dstr_resize(&data->name, ext - path);

  data->base.settings = ols_data_create();
  if (settings)
    ols_data_apply(data->base.settings, settings);

  if (!python_loaded)
    return (ols_script_t *)data;

  lock_python();
  add_to_python_path(data->dir.array);
  data->base.loaded = load_python_script(data);
  if (data->base.loaded) {
    blog(LOG_INFO, "[ols-scripting]: Loaded python script: %s",
         data->base.file.array);
    cur_python_script = data;
    // ols_python_script_update(&data->base, NULL);
    cur_python_script = NULL;
  }
  unlock_python();

  return (ols_script_t *)data;
}

struct ols_meta_result ols_python_parse_data(ols_script_t *s, ols_meta_txt_t * txt_info) {
  struct ols_meta_result result;
  memset(&result, 0, sizeof(result));

  struct ols_python_script *python_script = (struct ols_python_script *)s;
  if (!s->loaded || !python_loaded)
    return result;


  lock_python();

  // printf("data %s len %d\n", data, strlen(data));

  PyObject *args = Py_BuildValue("(liiiss)", txt_info->msec,txt_info->pid,txt_info->tid,txt_info->log_lv,txt_info->tag.array,(const char *)txt_info->buff + txt_info->data_offset);
  // printf("parse object %p args %p\n",python_script->parse,args);
  PyObject *pValue = PyObject_CallObject(python_script->parse, args);
  // printf("parse object %p args %p py ret
  // %p\n",python_script->parse,args,py_ret);

  // 检查是否有错误发生
  if (PyErr_Occurred()) {
    PyErr_Print();
  } else {

    if (PyList_Check(pValue)) {
      Py_ssize_t size;
      PyObject *item;

      size = PyList_Size(pValue);

      for (Py_ssize_t i = 0; i < size; ++i) {
        item = PyList_GetItem(pValue, i);
        PyIter_Check(pValue);

        if (PyUnicode_Check(item)) {

          Py_ssize_t len = 0;
          char *desc = NULL;
          PyObject *bytes = NULL;

          bytes = PyUnicode_AsUTF8String(item);
          PyBytes_AsStringAndSize(bytes, &desc, &len);

          char *info = bstrdup_n(desc, len);
          da_push_back(result.info, &info);

          Py_DECREF(bytes);

          // printf("PyList_Check %s\n", cstr);
        } else {

          // printf("PyList_Check %s\n", cstr);
        }
      }
    } else if (PyUnicode_Check(pValue)) {

      const char *cstr = PyUnicode_AsUTF8(pValue);
      // printf("PyUnicode_AsUTF8 %s\n", cstr);
    }
  }

  py_error();

  Py_XDECREF(pValue);
  Py_XDECREF(args);

  unlock_python();

  return result;
}

void ols_python_script_unload(ols_script_t *s) {
  struct ols_python_script *data = (struct ols_python_script *)s;

  if (!s->loaded || !python_loaded)
    return;

  /* ---------------------------- */
  /* mark callbacks as removed    */

  lock_python();

  /* XXX: scripts can potentially make callbacks when this happens, so
   * this probably still isn't ideal as we can't predict how the
   * processor or operating system is going to schedule things. a more
   * ideal method would be to reference count the script objects and
   * atomically share ownership with callbacks when they're called. */

  Py_XDECREF(data->parse);
  Py_XDECREF(data->tick);
  Py_XDECREF(data->save);

  data->tick = NULL;
  data->save = NULL;
  data->parse = NULL;

  /* ---------------------------- */
  /* unload                       */

  unload_python_script(data);
  unlock_python();

  s->loaded = false;

  blog(LOG_INFO, "[ols-scripting]: Unloaded python script: %s",
       data->base.file.array);
}

void ols_python_script_destroy(ols_script_t *s) {
  struct ols_python_script *data = (struct ols_python_script *)s;

  if (data) {
    if (python_loaded) {
      lock_python();
      Py_XDECREF(data->module);
      unlock_python();
    }

    dstr_free(&data->base.path);
    dstr_free(&data->base.file);
    dstr_free(&data->base.desc);
    ols_data_release(data->base.settings);
    dstr_free(&data->dir);
    dstr_free(&data->name);
    bfree(data);
  }
}

/* -------------------------------------------- */

/* -------------------------------------------- */

void ols_python_unload(void);

bool ols_scripting_python_runtime_linked(void) { return (bool)RUNTIME_LINK; }

void ols_scripting_python_version(char *version, size_t version_length) {
#if RUNTIME_LINK
  snprintf(version, version_length, "%d.%d", python_version.major,
           python_version.minor);
#else
  snprintf(version, version_length, "%d.%d", PY_MAJOR_VERSION,
           PY_MINOR_VERSION);
#endif
}

bool ols_scripting_python_loaded(void) { return python_loaded; }

void ols_python_load(void) {
  da_init(python_paths);

  mutexes_loaded = true;
}

static bool python_loaded_at_all = false;

bool ols_scripting_load_python(const char *python_path) {
  if (python_loaded)
    return true;

  /* Use external python on windows and mac */
#if RUNTIME_LINK
  if (!import_python(python_path, &python_version))
    return false;

  if (python_path && *python_path) {
#ifdef __APPLE__
    char temp[PATH_MAX];
    snprintf(temp, sizeof(temp), "%s/Python.framework/Versions/Current",
             python_path);
    os_utf8_to_wcs(temp, 0, home_path, PATH_MAX);
    Py_SetPythonHome(home_path);
#else

    os_utf8_to_wcs(python_path, 0, home_path, 1024);
    Py_SetPythonHome(home_path);
#endif
  }
#else
  UNUSED_PARAMETER(python_path);
#endif

  Py_Initialize();
  if (!Py_IsInitialized())
    return false;

#if RUNTIME_LINK
  if (python_version.major == 3 && python_version.minor < 7) {
    PyEval_InitThreads();
    if (!PyEval_ThreadsInitialized())
      return false;
  }
#elif PY_VERSION_HEX < 0x03070000
  PyEval_InitThreads();
  if (!PyEval_ThreadsInitialized())
    return false;
#endif

  /* ---------------------------------------------- */
  /* Must set arguments for guis to work            */

  wchar_t *argv[] = {L"", NULL};
  int argc = sizeof(argv) / sizeof(wchar_t *) - 1;

  PRAGMA_WARN_PUSH
  PRAGMA_WARN_DEPRECATION
  PySys_SetArgv(argc, argv);
  PRAGMA_WARN_POP

#ifdef __APPLE__
  PyRun_SimpleString("import sys");
  PyRun_SimpleString("sys.dont_write_bytecode = True");
#endif

#ifdef DEBUG_PYTHON_STARTUP
  /* ---------------------------------------------- */
  /* Debug logging to file if startup is failing    */

  PyRun_SimpleString("import os");
  PyRun_SimpleString("import sys");
  PyRun_SimpleString("os.environ['PYTHONUNBUFFERED'] = '1'");
  PyRun_SimpleString("sys.stdout = open('./stdOut.txt','w',1)");
  PyRun_SimpleString("sys.stderr = open('./stdErr.txt','w',1)");
  PyRun_SimpleString("print(sys.version)");
#endif

  /* ---------------------------------------------- */
  /* Load main interface module                     */

#ifdef __APPLE__
  struct dstr plugin_path;
  struct dstr resource_path;

  dstr_init_move_array(&plugin_path, os_get_executable_path_ptr(""));
  dstr_init_copy(&resource_path, plugin_path.array);
  dstr_cat(&plugin_path, "../PlugIns");
  dstr_cat(&resource_path, "../Resources");

  char *absolute_plugin_path = os_get_abs_path_ptr(plugin_path.array);
  char *absolute_resource_path = os_get_abs_path_ptr(resource_path.array);

  if (absolute_plugin_path != NULL) {
    add_to_python_path(absolute_plugin_path);
    bfree(absolute_plugin_path);
  }
  dstr_free(&plugin_path);

  if (absolute_resource_path != NULL) {
    add_to_python_path(absolute_resource_path);
    bfree(absolute_resource_path);
  }
  dstr_free(&resource_path);
#else
  char *absolute_script_path = os_get_abs_path_ptr(SCRIPT_DIR);
  printf("script dir %s\n", absolute_script_path);
  add_to_python_path(absolute_script_path);
  bfree(absolute_script_path);
#endif
  // bool success= true;
  py_olspython = PyImport_ImportModule("olspython");
  bool success = !py_error();
  if (!success) {
    warn("Error importing olspython.py', unloading ols-python");
    goto out;
  }

  python_loaded = PyRun_SimpleString(startup_script) == 0;
  py_error();

  add_hook_functions(py_olspython);
  py_error();

out:
  /* ---------------------------------------------- */
  /* Free data                                      */

  PyEval_ReleaseThread(PyGILState_GetThisThreadState());

  if (!success) {
    warn("Failed to load python plugin");
    ols_python_unload();
  }

  python_loaded_at_all = success;

  info("Load python plugin end");

  return python_loaded;
}

void ols_python_unload(void) {

  if (!python_loaded_at_all)
    return;

  if (python_loaded && Py_IsInitialized()) {
    PyGILState_Ensure();

    Py_XDECREF(py_olspython);
    Py_Finalize();
  }

  /* ---------------------- */

  for (size_t i = 0; i < python_paths.num; i++)
    bfree(python_paths.array[i]);
  da_free(python_paths);

  dstr_free(&cur_py_log_chunk);

  python_loaded_at_all = false;
}
