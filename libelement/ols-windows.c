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

#include "obs-internal.h"
#include "obs.h"
#include "util/dstr.h"
#include "util/platform.h"
#include "util/windows/win-registry.h"
#include "util/windows/win-version.h"

#include <iwscapi.h>
#include <windows.h>
#include <wscapi.h>

static uint32_t win_ver = 0;
static uint32_t win_build = 0;

const char *get_module_extension(void) { return ".dll"; }

static const char *module_bin[] = {"../../obs-plugins/64bit"};

static const char *module_data[] = {"../../data/obs-plugins/%module%"};

static const int module_patterns_size =
    sizeof(module_bin) / sizeof(module_bin[0]);

void add_default_module_paths(void) {
  for (int i = 0; i < module_patterns_size; i++)
    obs_add_module_path(module_bin[i], module_data[i]);
}

/* on windows, points to [base directory]/data/libobs */
char *find_libols_data_file(const char *file) {
  struct dstr path;
  dstr_init(&path);

  if (check_path(file, "../../data/libobs/", &path))
    return path.array;

  dstr_free(&path);
  return NULL;
}

static void log_processor_info(void) {
  HKEY key;
  wchar_t data[1024];
  char *str = NULL;
  DWORD size, speed;
  LSTATUS status;

  memset(data, 0, sizeof(data));

  status =
      RegOpenKeyW(HKEY_LOCAL_MACHINE,
                  L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", &key);
  if (status != ERROR_SUCCESS)
    return;

  size = sizeof(data);
  status = RegQueryValueExW(key, L"ProcessorNameString", NULL, NULL,
                            (LPBYTE)data, &size);
  if (status == ERROR_SUCCESS) {
    os_wcs_to_utf8_ptr(data, 0, &str);
    blog(LOG_INFO, "CPU Name: %s", str);
    bfree(str);
  }

  size = sizeof(speed);
  status = RegQueryValueExW(key, L"~MHz", NULL, NULL, (LPBYTE)&speed, &size);
  if (status == ERROR_SUCCESS)
    blog(LOG_INFO, "CPU Speed: %ldMHz", speed);

  RegCloseKey(key);
}

static void log_processor_cores(void) {
  blog(LOG_INFO, "Physical Cores: %d, Logical Cores: %d",
       os_get_physical_cores(), os_get_logical_cores());
}

static void log_emulation_status(void) {
  if (os_get_emulation_status()) {
    blog(LOG_WARNING, "Windows ARM64: Running with x64 emulation");
  }
}

static void log_available_memory(void) {
  MEMORYSTATUSEX ms;
  ms.dwLength = sizeof(ms);

  GlobalMemoryStatusEx(&ms);

#ifdef _WIN64
  const char *note = "";
#else
  const char *note = " (NOTE: 32bit programs cannot use more than 3gb)";
#endif

  blog(LOG_INFO, "Physical Memory: %luMB Total, %luMB Free%s",
       (DWORD)(ms.ullTotalPhys / 1048576), (DWORD)(ms.ullAvailPhys / 1048576),
       note);
}



extern const char *get_win_release_id();

static void log_windows_version(void) {
  struct win_version_info ver;
  get_win_ver(&ver);

  const char *release_id = get_win_release_id();

  bool b64 = is_64_bit_windows();
  const char *windows_bitness = b64 ? "64" : "32";

  bool arm64 = is_arm64_windows();
  const char *arm64_windows = arm64 ? "ARM " : "";

  blog(LOG_INFO,
       "Windows Version: %d.%d Build %d (release: %s; revision: %d; %s%s-bit)",
       ver.major, ver.minor, ver.build, release_id, ver.revis, arm64_windows,
       windows_bitness);
}

static void log_admin_status(void) {
  SID_IDENTIFIER_AUTHORITY auth = SECURITY_NT_AUTHORITY;
  PSID admin_group;
  BOOL success;

  success = AllocateAndInitializeSid(&auth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                     &admin_group);
  if (success) {
    if (!CheckTokenMembership(NULL, admin_group, &success))
      success = false;
    FreeSid(admin_group);
  }

  blog(LOG_INFO, "Running as administrator: %s", success ? "true" : "false");
}

#define WIN10_GAME_BAR_REG_KEY                                                 \
  L"Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"
#define WIN10_GAME_DVR_POLICY_REG_KEY                                          \
  L"SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR"
#define WIN10_GAME_DVR_REG_KEY L"System\\GameConfigStore"
#define WIN10_GAME_MODE_REG_KEY L"Software\\Microsoft\\GameBar"

static void log_gaming_features(void) {
  if (win_ver < 0xA00)
    return;

  struct reg_dword game_bar_enabled;
  struct reg_dword game_dvr_allowed;
  struct reg_dword game_dvr_enabled;
  struct reg_dword game_dvr_bg_recording;
  struct reg_dword game_mode_enabled;

  get_reg_dword(HKEY_CURRENT_USER, WIN10_GAME_BAR_REG_KEY, L"AppCaptureEnabled",
                &game_bar_enabled);
  get_reg_dword(HKEY_CURRENT_USER, WIN10_GAME_DVR_POLICY_REG_KEY,
                L"AllowGameDVR", &game_dvr_allowed);
  get_reg_dword(HKEY_CURRENT_USER, WIN10_GAME_DVR_REG_KEY, L"GameDVR_Enabled",
                &game_dvr_enabled);
  get_reg_dword(HKEY_CURRENT_USER, WIN10_GAME_BAR_REG_KEY,
                L"HistoricalCaptureEnabled", &game_dvr_bg_recording);
  get_reg_dword(HKEY_CURRENT_USER, WIN10_GAME_MODE_REG_KEY,
                L"AutoGameModeEnabled", &game_mode_enabled);

  if (game_mode_enabled.status != ERROR_SUCCESS) {
    get_reg_dword(HKEY_CURRENT_USER, WIN10_GAME_MODE_REG_KEY,
                  L"AllowAutoGameMode", &game_mode_enabled);
  }

  blog(LOG_INFO, "Windows 10/11 Gaming Features:");
  if (game_bar_enabled.status == ERROR_SUCCESS) {
    blog(LOG_INFO, "\tGame Bar: %s",
         (bool)game_bar_enabled.return_value ? "On" : "Off");
  }

  if (game_dvr_allowed.status == ERROR_SUCCESS) {
    blog(LOG_INFO, "\tGame DVR Allowed: %s",
         (bool)game_dvr_allowed.return_value ? "Yes" : "No");
  }

  if (game_dvr_enabled.status == ERROR_SUCCESS) {
    blog(LOG_INFO, "\tGame DVR: %s",
         (bool)game_dvr_enabled.return_value ? "On" : "Off");
  }

  if (game_dvr_bg_recording.status == ERROR_SUCCESS) {
    blog(LOG_INFO, "\tGame DVR Background Recording: %s",
         (bool)game_dvr_bg_recording.return_value ? "On" : "Off");
  }

  if (game_mode_enabled.status == ERROR_SUCCESS) {
    blog(LOG_INFO, "\tGame Mode: %s",
         (bool)game_mode_enabled.return_value ? "On" : "Off");
  } else if (win_build >= 19042) {
    // On by default in newer Windows 10 builds (no registry key set)
    blog(LOG_INFO, "\tGame Mode: Probably On (no reg key set)");
  }
}

static const char *get_str_for_state(int state) {
  switch (state) {
  case WSC_SECURITY_PRODUCT_STATE_ON:
    return "enabled";
  case WSC_SECURITY_PRODUCT_STATE_OFF:
    return "disabled";
  case WSC_SECURITY_PRODUCT_STATE_SNOOZED:
    return "temporarily disabled";
  case WSC_SECURITY_PRODUCT_STATE_EXPIRED:
    return "expired";
  default:
    return "unknown";
  }
}

static const char *get_str_for_type(int type) {
  switch (type) {
  case WSC_SECURITY_PROVIDER_ANTIVIRUS:
    return "AV";
  case WSC_SECURITY_PROVIDER_FIREWALL:
    return "FW";
  case WSC_SECURITY_PROVIDER_ANTISPYWARE:
    return "ASW";
  default:
    return "unknown";
  }
}

static void log_security_products_by_type(IWSCProductList *prod_list,
                                          int type) {
  HRESULT hr;
  LONG count = 0;
  IWscProduct *prod;
  BSTR name;
  WSC_SECURITY_PRODUCT_STATE prod_state;

  hr = prod_list->lpVtbl->Initialize(prod_list, type);

  if (FAILED(hr))
    return;

  hr = prod_list->lpVtbl->get_Count(prod_list, &count);
  if (FAILED(hr)) {
    prod_list->lpVtbl->Release(prod_list);
    return;
  }

  for (int i = 0; i < count; i++) {
    hr = prod_list->lpVtbl->get_Item(prod_list, i, &prod);
    if (FAILED(hr))
      continue;

    hr = prod->lpVtbl->get_ProductName(prod, &name);
    if (FAILED(hr))
      continue;

    hr = prod->lpVtbl->get_ProductState(prod, &prod_state);
    if (FAILED(hr)) {
      SysFreeString(name);
      continue;
    }

    char *product_name;
    os_wcs_to_utf8_ptr(name, 0, &product_name);

    blog(LOG_INFO, "\t%s: %s (%s)", product_name, get_str_for_state(prod_state),
         get_str_for_type(type));

    bfree(product_name);

    SysFreeString(name);
    prod->lpVtbl->Release(prod);
  }

  prod_list->lpVtbl->Release(prod_list);
}

static void log_security_products(void) {
  IWSCProductList *prod_list = NULL;
  HMODULE h_wsc;
  HRESULT hr;

  /* We load the DLL rather than import wcsapi.lib because the clsid /
   * iid only exists on Windows 8 or higher. */

  h_wsc = LoadLibraryW(L"wscapi.dll");
  if (!h_wsc)
    return;

  const CLSID *prod_list_clsid =
      (const CLSID *)GetProcAddress(h_wsc, "CLSID_WSCProductList");
  const IID *prod_list_iid =
      (const IID *)GetProcAddress(h_wsc, "IID_IWSCProductList");

  if (prod_list_clsid && prod_list_iid) {
    blog(LOG_INFO, "Sec. Software Status:");

    hr = CoCreateInstance(prod_list_clsid, NULL, CLSCTX_INPROC_SERVER,
                          prod_list_iid, &prod_list);
    if (!FAILED(hr)) {
      log_security_products_by_type(prod_list, WSC_SECURITY_PROVIDER_ANTIVIRUS);
    }

    hr = CoCreateInstance(prod_list_clsid, NULL, CLSCTX_INPROC_SERVER,
                          prod_list_iid, &prod_list);
    if (!FAILED(hr)) {
      log_security_products_by_type(prod_list, WSC_SECURITY_PROVIDER_FIREWALL);
    }

    hr = CoCreateInstance(prod_list_clsid, NULL, CLSCTX_INPROC_SERVER,
                          prod_list_iid, &prod_list);
    if (!FAILED(hr)) {
      log_security_products_by_type(prod_list,
                                    WSC_SECURITY_PROVIDER_ANTISPYWARE);
    }
  }

  FreeLibrary(h_wsc);
}

void log_system_info(void) {
  struct win_version_info ver;
  get_win_ver(&ver);

  win_ver = (ver.major << 8) | ver.minor;
  win_build = ver.build;

  log_processor_info();
  log_processor_cores();
  log_available_memory();
  log_windows_version();
  log_emulation_status();
  log_admin_status();
  log_gaming_features();
  log_security_products();

}


bool sym_initialize_called = false;

void reset_win32_symbol_paths(void) {
  static BOOL(WINAPI * sym_initialize_w)(HANDLE, const wchar_t *, BOOL);
  static BOOL(WINAPI * sym_set_search_path_w)(HANDLE, const wchar_t *);
  static bool funcs_initialized = false;
  static bool initialize_success = false;

  struct obs_module *module = obs->first_module;
  struct dstr path_str = {0};
  DARRAY(char *) paths;
  wchar_t *path_str_w = NULL;
  char *abspath;

  da_init(paths);

  if (!funcs_initialized) {
    HMODULE mod;
    funcs_initialized = true;

    mod = LoadLibraryW(L"DbgHelp");
    if (!mod)
      return;

    sym_initialize_w = (void *)GetProcAddress(mod, "SymInitializeW");
    sym_set_search_path_w = (void *)GetProcAddress(mod, "SymSetSearchPathW");
    if (!sym_initialize_w || !sym_set_search_path_w) {
      FreeLibrary(mod);
      return;
    }

    initialize_success = true;
    // Leaks 'mod' once.
  }

  if (!initialize_success)
    return;

  abspath = os_get_abs_path_ptr(".");
  if (abspath)
    da_push_back(paths, &abspath);

  while (module) {
    bool found = false;
    struct dstr path = {0};
    char *path_end;

    dstr_copy(&path, module->bin_path);
    dstr_replace(&path, "/", "\\");

    path_end = strrchr(path.array, '\\');
    if (!path_end) {
      module = module->next;
      dstr_free(&path);
      continue;
    }

    *path_end = 0;

    abspath = os_get_abs_path_ptr(path.array);
    if (abspath) {
      for (size_t i = 0; i < paths.num; i++) {
        const char *existing_path = paths.array[i];
        if (astrcmpi(abspath, existing_path) == 0) {
          found = true;
          break;
        }
      }

      if (!found) {
        da_push_back(paths, &abspath);
      } else {
        bfree(abspath);
      }
    }

    dstr_free(&path);

    module = module->next;
  }

  for (size_t i = 0; i < paths.num; i++) {
    const char *path = paths.array[i];
    if (path && *path) {
      if (i != 0)
        dstr_cat(&path_str, ";");
      dstr_cat(&path_str, paths.array[i]);
    }
  }

  if (path_str.array) {
    os_utf8_to_wcs_ptr(path_str.array, path_str.len, &path_str_w);
    if (path_str_w) {
      if (!sym_initialize_called) {
        sym_initialize_w(GetCurrentProcess(), path_str_w, false);
        sym_initialize_called = true;
      } else {
        sym_set_search_path_w(GetCurrentProcess(), path_str_w);
      }

      bfree(path_str_w);
    }
  }

  for (size_t i = 0; i < paths.num; i++)
    bfree(paths.array[i]);
  dstr_free(&path_str);
  da_free(paths);
}

extern void initialize_crash_handler(void);

void obs_init_win32_crash_handler(void) { initialize_crash_handler(); }

bool initialize_com(void) {
  const HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
  const bool success = SUCCEEDED(hr);
  if (!success)
    blog(LOG_ERROR, "CoInitializeEx failed: 0x%08X", hr);
  return success;
}

void uninitialize_com(void) { CoUninitialize(); }
