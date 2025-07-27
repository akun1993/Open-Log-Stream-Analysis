
#include "ols-module.h"
#include "ols-scripting.h"
#include <stdio.h>
#include <string>
#include <util/platform.h>

#include "ols-context.h"
int GetConfigPath(char *path, size_t size, const char *name) {
#if ALLOW_PORTABLE_MODE
  if (portable_mode) {
    if (name && *name) {
      return snprintf(path, size, CONFIG_PATH "/%s", name);
    } else {
      return snprintf(path, size, CONFIG_PATH);
    }
  } else {
    return os_get_config_path(path, size, name);
  }
#else
  return os_get_config_path(path, size, name);
#endif
}

static void AddExtraModulePaths() {
  std::string plugins_path, plugins_data_path;
  char *s;

  s = getenv("OLS_PLUGINS_PATH");
  if (s)
    plugins_path = s;

  s = getenv("OLS_PLUGINS_DATA_PATH");
  if (s)
    plugins_data_path = s;

  if (!plugins_path.empty() && !plugins_data_path.empty()) {
    std::string data_path_with_module_suffix;
    data_path_with_module_suffix += plugins_data_path;
    data_path_with_module_suffix += "/%module%";
    ols_add_module_path(plugins_path.c_str(),
                        data_path_with_module_suffix.c_str());
  }

  char base_module_dir[512];
#if defined(_WIN32)
  int ret = GetProgramDataPath(base_module_dir, sizeof(base_module_dir),
                               "ols-studio/plugins/%module%");
#elif defined(__APPLE__)
  int ret = GetConfigPath(base_module_dir, sizeof(base_module_dir),
                          "ols-studio/plugins/%module%.plugin");
#else
  int ret = GetConfigPath(base_module_dir, sizeof(base_module_dir),
                          "ols-studio/plugins/%module%");
#endif

  if (ret <= 0)
    return;

  std::string path = base_module_dir;
  printf("base module dir %s\n", path.c_str());
#if defined(__APPLE__)
  /* User Application Support Search Path */
  ols_add_module_path((path + "/Contents/MacOS").c_str(),
                      (path + "/Contents/Resources").c_str());

#ifndef __aarch64__
  /* Legacy System Library Search Path */
  char system_legacy_module_dir[PATH_MAX];
  GetProgramDataPath(system_legacy_module_dir, sizeof(system_legacy_module_dir),
                     "ols-studio/plugins/%module%");
  std::string path_system_legacy = system_legacy_module_dir;
  ols_add_module_path((path_system_legacy + "/bin").c_str(),
                      (path_system_legacy + "/data").c_str());

  /* Legacy User Application Support Search Path */
  char user_legacy_module_dir[PATH_MAX];
  GetConfigPath(user_legacy_module_dir, sizeof(user_legacy_module_dir),
                "ols-studio/plugins/%module%");
  std::string path_user_legacy = user_legacy_module_dir;
  ols_add_module_path((path_user_legacy + "/bin").c_str(),
                      (path_user_legacy + "/data").c_str());
#endif
#else
#if ARCH_BITS == 64
  ols_add_module_path((path + "/bin/64bit").c_str(), (path + "/data").c_str());
  ols_add_module_path("../lib/ols-plugins", (path + "/data").c_str());
#else
  ols_add_module_path((path + "/bin/32bit").c_str(), (path + "/data").c_str());
#endif
#endif
}

int main(int argc, char **argv) {

  if (!ols_startup("en-US", NULL, NULL))
    printf("Couldn't create OLS");
  struct ols_module_failure_info mfi;

  AddExtraModulePaths();
  printf("---------------------------------\n");
  ols_load_all_modules2(&mfi);
  printf("---------------------------------\n");
  ols_log_loaded_modules();
  printf("---------------------------------\n");
  ols_post_load_modules();

  printf("load script start\n");
  ols_scripting_load();
  printf("load script end\n");

  ols_data_t * src_file_data = ols_data_create();

  ols_data_set_string(src_file_data,"base_file","/home/V01/uidq8743/TBoxLog_VIN123456_20241205_143824.zip");
  ols_data_set_string(src_file_data,"base_file_type_hint","zip");
  ols_data_set_string(src_file_data,"inner_dir","TBoxLog/log");

  ols_data_set_string(src_file_data,"file_name_wildcard","sv_dmesg.log.*");

  ols_source_t *source = ols_source_create("text_file", "test_read", src_file_data);

  ols_process_t *dispatch =
      ols_process_create("data_dispatch", "test_dispatch", nullptr);  


  ols_data_t * script_data = ols_data_create();

  ols_data_set_string(script_data,"script_file_path","/home/V01/uidq8743/OpenSource/Open-Log-Stream-Analysis/python_script/parse_log_2.py");
  ols_process_t *process =
      ols_process_create("script_caller", "test_process", script_data);

  ols_output_t *xml_output =
      ols_output_create("xml_output", "test_output", nullptr);

  ols_context_link(&source->context, &dispatch->context);

  ols_context_link(&dispatch->context, &process->context);

  ols_context_link(&process->context, &xml_output->context);

  // source->link->process2->link->output;

  printf("source create at address %p\n", source);

  ols_source_set_active(source, true);

  int c;
  printf("Enter characters, I shall repeat them.\n");
  printf("Press Ctrl+D (Unix/Linux) or Ctrl+Z (Windows) to exit.\n");
  while ((c = getchar()) != EOF) {
    putchar(c);
  }

  ols_scripting_unload();

  ols_shutdown();

  printf("\nProgram exited.\n");

  return 0;
}
