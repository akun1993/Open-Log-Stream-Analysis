
#include "ols-module.h"
#include "ols-scripting.h"
#include "ols-data.h"
#include <jansson.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <util/platform.h>
#include <util/bmem.h>
#include "ols-context.h"
#include "ols-source.h"
#include "ols-process.h"
#include "ols-output.h"
#include "ols-archive.h"
#include <set>
#include <unordered_map>

int GetProgramDataPath(char *path, size_t size, const char *name)
{
	return os_get_program_data_path(path, size, name);
}

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

static std::string json_escape(const std::string &value)
{
  std::string escaped;
  escaped.reserve(value.size() * 2);

  for (char c : value) {
    switch (c) {
      case '\\': escaped += "\\\\"; break;
      case '"': escaped += "\\\""; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\t': escaped += "\\t"; break;
      default: escaped += c; break;
    }
  }

  return escaped;
}

static ols_data_t *create_settings_from_json(json_t *settings_json)
{
  if (!settings_json || !json_is_object(settings_json))
    return nullptr;

  char *dump = json_dumps(settings_json, JSON_COMPACT);
  if (!dump)
    return nullptr;

  ols_data_t *settings = ols_data_create_from_json(dump);
  free(dump);
  return settings;
}

enum PipelineNodeType {
  NODE_SOURCE,
  NODE_PROCESS,
  NODE_OUTPUT,
  NODE_UNKNOWN,
};

struct PipelineNode {
  std::string name;
  PipelineNodeType type;
  ols_context_data *context;
  ols_source_t *source;
  ols_process_t *process;
  ols_output_t *output;
  bool activate_requested;
};

// PipelineLink removed — links are applied directly when parsing node-level links.

static PipelineNodeType parse_node_type(const char *type)
{
  if (!type)
    return NODE_UNKNOWN;

  if (strcmp(type, "source") == 0)
    return NODE_SOURCE;
  if (strcmp(type, "process") == 0)
    return NODE_PROCESS;
  if (strcmp(type, "output") == 0)
    return NODE_OUTPUT;

  return NODE_UNKNOWN;
}

static std::string make_default_pipeline_json(const std::string &base_file,
                                              const std::string &inner_dir,
                                              const std::string &script_path,
                                              const std::string &script_path_2)
{
  std::string json = "{\"nodes\":[";
  json += "{\"type\":\"source\",\"id\":\"text_file\",\"name\":\"test_read\",\"settings\":{";
  json += "\"base_file\":\"" + json_escape(base_file) + "\",";
  json += "\"inner_dir\":\"" + json_escape(inner_dir) + "\",";
  json += "\"file_name_wildcard\":\"sv_user.log.*\"},\"active\":true}";
  json += ",{\"type\":\"process\",\"id\":\"data_dispatch\",\"name\":\"test_dispatch\",\"settings\":null}";
  json += ",{\"type\":\"process\",\"id\":\"script_caller\",\"name\":\"test_process\",\"settings\":{";
  json += "\"script_file_path\":\"" + json_escape(script_path) + "\",";
  json += "\"output_tag\":\"DSVVDCMAPP\",";
  json += "\"capacity\":\"DSVVDCMAPP;DSVTSPConnectSVC\"}}";
  json += ",{\"type\":\"process\",\"id\":\"script_caller\",\"name\":\"test_process_2\",\"settings\":{";
  json += "\"script_file_path\":\"" + json_escape(script_path_2) + "\",";
  json += "\"capacity\":\"DSVLocationAPP\"}}";
  json += ",{\"type\":\"output\",\"id\":\"xml_output\",\"name\":\"test_output\",\"settings\":null}]";
  json += ",\"links\":[";
  json += "{\"from\":\"test_read\",\"to\":\"test_dispatch\"},";
  json += "{\"from\":\"test_dispatch\",\"to\":\"test_process\"},";
  json += "{\"from\":\"test_dispatch\",\"to\":\"test_process_2\"},";
  json += "{\"from\":\"test_process\",\"to\":\"test_output\"},";
  json += "{\"from\":\"test_process_2\",\"to\":\"test_output\"}]";
  json += "}";
  return json;
}


// ===== 文件作用域工具函数定义 =====
static bool dir_has_wildcard(const std::string& dir, const std::string& wildcard) {
  os_dir_t* d = os_opendir(dir.c_str());
  if (!d) return false;
  struct os_dirent* ent;
  bool found = false;
  while ((ent = os_readdir(d)) != nullptr) {
    if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
      // Simple wildcard match: only supports '*'
    const char* name = ent->d_name;
    const char* wc = wildcard.c_str();
    // 只支持前缀*后缀
      if (wildcard == name) { found = true; break; }
    const char* star = strchr(wc, '*');
    if (star) {
      std::string prefix(wc, star-wc);
      std::string suffix(star+1);
      size_t nlen = strlen(name);
      if (nlen >= prefix.size() + suffix.size() &&
          strncmp(name, prefix.c_str(), prefix.size()) == 0 &&
          strcmp(name + nlen - suffix.size(), suffix.c_str()) == 0) {
        found = true; break;
      }
    }
  }
  os_closedir(d);
  return found;
}

static bool extract_until_match(const std::string& dir, const std::string& wildcard, int depth, std::unordered_map<std::string, std::string>& archive_to_dir) {
  if (depth > 3) return false;
  if (dir_has_wildcard(dir, wildcard)) return true;
  os_dir_t* d = os_opendir(dir.c_str());
  if (!d) return false;
  struct os_dirent* ent;
  std::vector<std::string> subdirs;
  while ((ent = os_readdir(d)) != nullptr) {
    if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
    std::string full = dir + FILE_SEPARATOR + ent->d_name;
    if (ent->directory) {
      subdirs.push_back(full);
    } else {
      ArchiveFormat fmt = detect_format(full.c_str());
      if (fmt != FORMAT_UNKNOWN) {
        std::string dest_dir = remove_compress_suffix(full);
        if (archive_to_dir.find(full) == archive_to_dir.end()) {
          decompress_file(full, fmt, dest_dir);
          archive_to_dir[full] = dest_dir;
          if (extract_until_match(dest_dir, wildcard, depth+1, archive_to_dir)) {
            os_closedir(d);
            return true;
          }
        }
      }
    }
  }
  os_closedir(d);
    // Recursively search subdirectories
    for (const auto& sub : subdirs) {
    if (extract_until_match(sub, wildcard, depth+1, archive_to_dir)) return true;
  }
  return false;
}

static void create_pipeline_from_json(const char *pipeline_json)
{
  printf("Creating test pipeline from JSON...\n");

  json_error_t error;
  json_t *root = json_loads(pipeline_json, JSON_REJECT_DUPLICATES, &error);
  if (!root || !json_is_object(root)) {
    fprintf(stderr, "Failed to parse pipeline JSON: %s\n", error.text);
    json_decref(root);
    return;
  }

  json_t *nodes_json = json_object_get(root, "nodes");
  json_t *links_json = json_object_get(root, "links");

  if (!nodes_json || !json_is_array(nodes_json)) {
    fprintf(stderr, "Pipeline JSON must contain an array named 'nodes'\n");
    json_decref(root);
    return;
  }

  std::vector<PipelineNode> nodes;
  nodes.reserve(json_array_size(nodes_json));

  // 1. Pre-extract all source and compressed files in their directories, up to 3 levels, prioritizing file_name_wildcard
  std::unordered_map<std::string, std::string> archive_to_dir;


  // Collect all base_file first
  size_t index;
  json_t *item;
  json_array_foreach(nodes_json, index, item) {
    if (!json_is_object(item))
      continue;
    const char *type = json_string_value(json_object_get(item, "type"));
    const char *id = json_string_value(json_object_get(item, "id"));
    if (!type || strcmp(type, "source") != 0 || !id || strcmp(id, "text_file") != 0) continue;
    json_t *settings_json = json_object_get(item, "settings");
    if (!settings_json) continue;
    json_t *base_file_json = json_object_get(settings_json, "base_file");
    json_t *wildcard_json = json_object_get(settings_json, "file_name_wildcard");
    if (!base_file_json || !json_is_string(base_file_json) || !wildcard_json || !json_is_string(wildcard_json)) continue;
    std::string base_file = json_string_value(base_file_json);
    std::string wildcard = json_string_value(wildcard_json);
    ArchiveFormat fmt = detect_format(base_file.c_str());
    if (fmt != FORMAT_UNKNOWN) {
      std::string dest_dir = remove_compress_suffix(base_file) + ".ols";
      // Idempotent extraction
      if (archive_to_dir.find(base_file) == archive_to_dir.end()) {
        decompress_file(base_file, fmt, dest_dir);
        archive_to_dir[base_file] = dest_dir;
      }
      // Replace base_file field with extracted directory
      json_object_set_new(settings_json, "base_file", json_string(dest_dir.c_str()));
      // Recursively extract, up to 3 levels, stop if wildcard is found
      extract_until_match(dest_dir, wildcard, 1, archive_to_dir);
    } else {
      // For non-archive files, directly search recursively
      extract_until_match(base_file, wildcard, 1, archive_to_dir);
    }
  }

  // 2. 创建节点，settings 已被替换为解压目录
  json_array_foreach(nodes_json, index, item) {
    if (!json_is_object(item))
      continue;

    const char *type = json_string_value(json_object_get(item, "type"));
    const char *id = json_string_value(json_object_get(item, "id"));
    const char *name = json_string_value(json_object_get(item, "name"));
    json_t *settings_json = json_object_get(item, "settings");
    json_t *active_json = json_object_get(item, "active");

    if (!type || !id || !name)
      continue;

    ols_data_t *settings = create_settings_from_json(settings_json);
    PipelineNode node;
    node.name = name;
    node.type = parse_node_type(type);
    node.context = nullptr;
    node.source = nullptr;
    node.process = nullptr;
    node.output = nullptr;

    switch (node.type) {
      case NODE_SOURCE:
        node.source = ols_source_create(id, name, settings);
        node.context = &node.source->context;
        node.activate_requested = json_is_true(active_json);
        break;
      case NODE_PROCESS:
        node.process = ols_process_create(id, name, settings);
        node.context = &node.process->context;
        break;
      case NODE_OUTPUT:
        node.output = ols_output_create(id, name, settings);
        node.context = &node.output->context;
        break;
      default:
        fprintf(stderr, "Unknown node type '%s'\n", type);
        if (settings)
          ols_data_release(settings);
        continue;
    }

    nodes.push_back(std::move(node));
  }

  std::unordered_map<std::string, ols_context_data *> context_map;
  for (auto &node : nodes) {
    if (!node.name.empty() && node.context)
      context_map[node.name] = node.context;
  }

  if (links_json && json_is_array(links_json)) {
    json_array_foreach(links_json, index, item) {
      if (!json_is_object(item))
        continue;

      const char *from = json_string_value(json_object_get(item, "from"));
      const char *to = json_string_value(json_object_get(item, "to"));
      if (!from || !to)
        continue;

      auto it_from = context_map.find(from);
      auto it_to = context_map.find(to);
      if (it_from == context_map.end() || it_to == context_map.end()) {
        fprintf(stderr, "Pipeline link missing node: %s -> %s\n", from,
                to);
        continue;
      }

      if (!ols_context_link(it_from->second, it_to->second)) {
        fprintf(stderr, "Failed to link %s -> %s\n", from, to);
      }
    }
  }

  /* Activate sources after creating nodes and links so activation happens
   * only once pipeline topology is established. */
  for (auto &node : nodes) {
    if (node.type == NODE_SOURCE && node.source && node.activate_requested) {
      ols_source_set_active(node.source, true);
    }
  }

  json_decref(root);
}

void create_pipeline(const char *pipeline_json) {
  std::string file_path;
  std::string inner_dir;
  std::string script_path;
  std::string script_path_2;

#if defined(_WIN32)
  file_path = "G:\\Test7z\\log.tar.gz";
  inner_dir = "app\\log";
  script_path = "script_python\\exampleGbParse.py";
  script_path_2 = script_path;
#else
  file_path = "/home/V01/uidq8743/TBoxLog_VIN123456_20241205_143824.zip";
  inner_dir = "TBoxLog/log";
  script_path = "script_python/exampleGbParse.py";
  script_path_2 = "script_python/script_others.py";
#endif

  std::string default_json = make_default_pipeline_json(file_path, inner_dir,
                                                        script_path, script_path_2);

  const char *json_string = pipeline_json ? pipeline_json : default_json.c_str();
  create_pipeline_from_json(json_string);
}

int main(int argc, char **argv) {

  /* Change working directory to the executable's directory */
  char *exe_dir = os_get_executable_path_ptr("");
  if (exe_dir) {
    if (os_chdir(exe_dir) == 0)
      printf("Changed working directory to %s\n", exe_dir);
    bfree(exe_dir);
  }

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

  create_pipeline(NULL);


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
