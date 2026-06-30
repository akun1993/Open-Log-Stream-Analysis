
#include "ols-module.h"
#include "ols-scripting.h"
#include "ols-data.h"
#include <jansson.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <util/platform.h>
#include <util/bmem.h>
#include <util/util.hpp>
#include "ols-context.h"
#include "ols-source.h"
#include "ols-process.h"
#include "ols-output.h"
#include "ols-archive.h"
#include <set>
#include <unordered_map>
#include <sys/stat.h>
#include <unistd.h>

int GetProgramDataPath(char *path, size_t size, const char *name) {
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
    if (s) plugins_path = s;

    s = getenv("OLS_PLUGINS_DATA_PATH");
    if (s) plugins_data_path = s;

    if (!plugins_path.empty() && !plugins_data_path.empty()) {
        std::string data_path_with_module_suffix;
        data_path_with_module_suffix += plugins_data_path;
        data_path_with_module_suffix += "/%module%";
        ols_add_module_path(plugins_path.c_str(), data_path_with_module_suffix.c_str());
    }

    char base_module_dir[512];
#if defined(_WIN32)
    int ret = GetProgramDataPath(base_module_dir, sizeof(base_module_dir), "ols-studio/plugins/%module%");
#elif defined(__APPLE__)
    int ret = GetConfigPath(base_module_dir, sizeof(base_module_dir), "ols-studio/plugins/%module%.plugin");
#else
    int ret = GetConfigPath(base_module_dir, sizeof(base_module_dir), "ols-studio/plugins/%module%");
#endif

    if (ret <= 0) return;

    std::string path = base_module_dir;
    printf("base module dir %s\n", path.c_str());
#if defined(__APPLE__)
    /* User Application Support Search Path */
    ols_add_module_path((path + "/Contents/MacOS").c_str(), (path + "/Contents/Resources").c_str());

#ifndef __aarch64__
    /* Legacy System Library Search Path */
    char system_legacy_module_dir[PATH_MAX];
    GetProgramDataPath(system_legacy_module_dir, sizeof(system_legacy_module_dir), "ols-studio/plugins/%module%");
    std::string path_system_legacy = system_legacy_module_dir;
    ols_add_module_path((path_system_legacy + "/bin").c_str(), (path_system_legacy + "/data").c_str());

    /* Legacy User Application Support Search Path */
    char user_legacy_module_dir[PATH_MAX];
    GetConfigPath(user_legacy_module_dir, sizeof(user_legacy_module_dir), "ols-studio/plugins/%module%");
    std::string path_user_legacy = user_legacy_module_dir;
    ols_add_module_path((path_user_legacy + "/bin").c_str(), (path_user_legacy + "/data").c_str());
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

static std::string json_escape(const std::string &value) {
    std::string escaped;
    escaped.reserve(value.size() * 2);

    for (char c : value) {
        switch (c) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += c;
                break;
        }
    }

    return escaped;
}

static ols_data_t *create_settings_from_json(json_t *settings_json) {
    if (!settings_json || !json_is_object(settings_json)) return nullptr;

    char *dump = json_dumps(settings_json, JSON_COMPACT);
    if (!dump) return nullptr;

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

static PipelineNodeType parse_node_type(const char *type) {
    if (!type) return NODE_UNKNOWN;

    if (strcmp(type, "source") == 0) return NODE_SOURCE;
    if (strcmp(type, "process") == 0) return NODE_PROCESS;
    if (strcmp(type, "output") == 0) return NODE_OUTPUT;

    return NODE_UNKNOWN;
}

static std::string make_default_pipeline_json(const std::string &base_file, const std::string &inner_dir, const std::string &script_path,
                                              const std::string &script_path_2) {
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

// ===== File-scope utility function definitions =====

/**
 * Check if a directory contains a file matching the wildcard pattern.
 * @param dir The directory path to search in.
 * @param wildcard The wildcard pattern (supports only '*' for prefix/suffix matching).
 * @return true if a matching file is found, false otherwise.
 */
static bool dir_has_wildcard(const std::string &dir, const std::string &wildcard) {
    // Open the directory
    os_dir_t *d = os_opendir(dir.c_str());
    if (!d) return false;
    struct os_dirent *ent;
    bool found = false;
    // Iterate through files in the directory
    while ((ent = os_readdir(d)) != nullptr) {
        // Skip current and parent directory entries
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        // Simple wildcard match: only supports '*'
        const char *name = ent->d_name;
        const char *wc = wildcard.c_str();
        // Only supports prefix*suffix pattern
        if (wildcard == name) {
            found = true;
            break;
        }
        const char *star = strchr(wc, '*');
        if (star) {
            std::string prefix(wc, star - wc);
            std::string suffix(star + 1);
            size_t nlen = strlen(name);
            // Check if filename matches prefix and suffix
            if (nlen >= prefix.size() + suffix.size() && strncmp(name, prefix.c_str(), prefix.size()) == 0 &&
                strcmp(name + nlen - suffix.size(), suffix.c_str()) == 0) {
                found = true;
                break;
            }
        }
    }
    // Close the directory
    os_closedir(d);
    return found;
}

/**
 * Recursively extract archives and search for files matching the wildcard pattern.
 * This function traverses directories up to 3 levels deep, extracting any archives
 * found along the way, until a file matching the wildcard pattern is found.
 * @param dir The starting directory path to search in.
 * @param wildcard The wildcard pattern to match files against.
 * @param depth Current recursion depth (starts at 1, max 3).
 * @param archive_to_dir Map tracking extracted archives to their destination directories.
 * @return true if a matching file is found, false otherwise.
 */
static bool extract_until_match(const std::string &dir, const std::string &wildcard, int depth,
                                std::unordered_map<std::string, std::string> &archive_to_dir) {
    // Limit recursion depth to prevent infinite loops
    if (depth > 3) return false;

    // Check if current directory already contains a matching file
    if (dir_has_wildcard(dir, wildcard)) return true;

    // Open the directory for traversal
    os_dir_t *d = os_opendir(dir.c_str());
    if (!d) return false;

    struct os_dirent *ent;
    std::vector<std::string> subdirs;

    // Iterate through directory entries
    while ((ent = os_readdir(d)) != nullptr) {
        // Skip current and parent directory entries
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;

        // Build full path for the current entry
        std::string full = dir + FILE_SEPARATOR + ent->d_name;

        if (ent->directory) {
            // Collect subdirectories for later recursive search
            subdirs.push_back(full);
        } else {
            // Check if the file is an archive (e.g., .zip, .tar.gz)
            ArchiveFormat fmt = detect_format(full.c_str());
            if (fmt != FORMAT_UNKNOWN) {
                // Determine the destination directory for extraction
                std::string dest_dir = remove_compress_suffix(full);

                // Only extract if not already processed (idempotent)
                if (archive_to_dir.find(full) == archive_to_dir.end()) {
                    // Decompress the archive to the destination directory
                    if (decompress_file(full, fmt, dest_dir)) {
                        archive_to_dir[full] = dest_dir;

                        // Recursively search the extracted contents
                        if (extract_until_match(dest_dir, wildcard, depth + 1, archive_to_dir)) {
                            os_closedir(d);
                            return true;
                        }
                    } else {
                        fprintf(stderr, "Failed to decompress: %s\n", full.c_str());
                    }
                }
            }
        }
    }
    os_closedir(d);

    // Recursively search subdirectories
    for (const auto &sub : subdirs) {
        if (extract_until_match(sub, wildcard, depth + 1, archive_to_dir)) return true;
    }
    return false;
}

static std::vector<PipelineNode> g_pipeline_nodes;
static std::atomic<bool> g_pipeline_done{false};
static std::atomic<int> g_output_count{0};
static std::atomic<int> g_output_stopped{0};
static std::vector<std::string> g_result_files;

/** Scan dir for .xml/.html files with mtime >= start_time. */
static std::vector<std::string> collect_result_files(const char *dir, time_t start_time) {
    std::vector<std::string> results;
    os_dir_t *d = os_opendir(dir);
    if (!d) return results;

    struct os_dirent *ent;
    while ((ent = os_readdir(d)) != nullptr) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        if (ent->directory) continue;

        const char *name = ent->d_name;
        size_t len = strlen(name);
        bool is_result = (len > 4 && strcmp(name + len - 4, ".xml") == 0) || (len > 5 && strcmp(name + len - 5, ".html") == 0);
        if (!is_result) continue;

        std::string full = std::string(dir) + "/" + name;
        struct stat st;
        if (stat(full.c_str(), &st) == 0 && st.st_mtime >= start_time) {
            results.push_back(full);
        }
    }
    os_closedir(d);
    return results;
}

/** Read entire stdin as a string. */
static std::string read_stdin() {
    std::string content;
    char buf[4096];
    while (fgets(buf, sizeof(buf), stdin)) {
        content += buf;
    }
    return content;
}

static void destroy_pipeline() {
    /* Deactivate sources first to stop data flow */
    for (auto &node : g_pipeline_nodes) {
        if (node.type == NODE_SOURCE && node.source && node.source->active) {
            ols_source_set_active(node.source, false);
        }
    }

    /* Release in reverse order: output -> process -> source */
    for (auto it = g_pipeline_nodes.rbegin(); it != g_pipeline_nodes.rend(); ++it) {
        switch (it->type) {
            case NODE_OUTPUT:
                if (it->output) ols_output_release(it->output);
                break;
            case NODE_PROCESS:
                if (it->process) ols_process_release(it->process);
                break;
            case NODE_SOURCE:
                if (it->source) ols_source_release(it->source);
                break;
            default:
                break;
        }
    }
    g_pipeline_nodes.clear();
    blog(LOG_INFO, "Pipeline destroyed and resources released");
}

static void on_output_stop(void *data, calldata_t *cd) {
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(cd);
    g_output_stopped.fetch_add(1);
    blog(LOG_INFO, "Output stop signal received (%d/%d)", g_output_stopped.load(), g_output_count.load());
    if (g_output_stopped.load() >= g_output_count.load()) {
        blog(LOG_INFO, "All outputs stopped, pipeline finished");
        g_pipeline_done = true;
    }
}

static void create_pipeline_from_json(const char *pipeline_json) {
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

    g_pipeline_nodes.clear();
    g_pipeline_nodes.reserve(json_array_size(nodes_json));

    // 1. Pre-extract all source and compressed files in their directories, up to 3 levels, prioritizing file_name_wildcard
    std::unordered_map<std::string, std::string> archive_to_dir;

    // Collect all base_file first
    size_t index;
    json_t *item;
    json_array_foreach(nodes_json, index, item) {
        if (!json_is_object(item)) continue;
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
                // Check if decompression succeeded before recording
                if (decompress_file(base_file, fmt, dest_dir)) {
                    archive_to_dir[base_file] = dest_dir;
                } else {
                    fprintf(stderr, "Failed to decompress: %s\n", base_file.c_str());
                    continue;
                }
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

    // 2. Create nodes, settings have been replaced with extraction directory
    json_array_foreach(nodes_json, index, item) {
        if (!json_is_object(item)) continue;

        const char *type = json_string_value(json_object_get(item, "type"));
        const char *id = json_string_value(json_object_get(item, "id"));
        const char *name = json_string_value(json_object_get(item, "name"));
        json_t *settings_json = json_object_get(item, "settings");
        json_t *active_json = json_object_get(item, "active");

        if (!type || !id || !name) continue;

        ols_data_t *settings = create_settings_from_json(settings_json);
        PipelineNode node;
        node.name = name;
        node.type = parse_node_type(type);
        node.context = nullptr;
        node.source = nullptr;
        node.process = nullptr;
        node.output = nullptr;

        bool create_success = false;
        switch (node.type) {
            case NODE_SOURCE:
                node.source = ols_source_create(id, name, settings);
                if (node.source) {
                    node.context = &node.source->context;
                    node.activate_requested = json_is_true(active_json);
                    create_success = true;
                }
                break;
            case NODE_PROCESS:
                node.process = ols_process_create(id, name, settings);
                if (node.process) {
                    node.context = &node.process->context;
                    create_success = true;
                }
                break;
            case NODE_OUTPUT:
                node.output = ols_output_create(id, name, settings);
                if (node.output) {
                    node.context = &node.output->context;
                    create_success = true;
                }
                break;
            default:
                fprintf(stderr, "Unknown node type '%s'\n", type);
                break;
        }

        if (settings) ols_data_release(settings);

        if (!create_success) {
            fprintf(stderr, "Failed to create node '%s' of type '%s'\n", name, type);
            continue;
        }

        g_pipeline_nodes.push_back(std::move(node));
    }

    std::unordered_map<std::string, ols_context_data *> context_map;
    for (auto &node : g_pipeline_nodes) {
        if (!node.name.empty() && node.context) context_map[node.name] = node.context;
    }

    if (links_json && json_is_array(links_json)) {
        json_array_foreach(links_json, index, item) {
            if (!json_is_object(item)) continue;

            const char *from = json_string_value(json_object_get(item, "from"));
            const char *to = json_string_value(json_object_get(item, "to"));
            if (!from || !to) continue;

            auto it_from = context_map.find(from);
            auto it_to = context_map.find(to);
            if (it_from == context_map.end() || it_to == context_map.end()) {
                fprintf(stderr, "Pipeline link missing node: %s -> %s\n", from, to);
                continue;
            }

            if (!ols_context_link(it_from->second, it_to->second)) {
                fprintf(stderr, "Failed to link %s -> %s\n", from, to);
            }
        }
    }

    /* Activate sources after creating nodes and links so activation happens
     * only once pipeline topology is established. */
    for (auto &node : g_pipeline_nodes) {
        if (node.type == NODE_SOURCE && node.source && node.activate_requested) {
            ols_source_set_active(node.source, true);
        }
    }

    /* Connect stop signal on outputs so we know when the pipeline finishes */
    for (auto &node : g_pipeline_nodes) {
        if (node.type == NODE_OUTPUT && node.output) {
            g_output_count.fetch_add(1);
            signal_handler_t *sh = ols_output_get_signal_handler(node.output);
            if (sh) {
                signal_handler_connect(sh, "stop", on_output_stop, nullptr);
            }
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
    file_path = "/home/V01/uidq8743/log-老国标数据上报失败-E22-358-2-V3.12.3.zip";
    // inner_dir = "TBoxLog/log";
    script_path = "script_python/exampleGbParse.py";
    script_path_2 = "script_python/script_others.py";
#endif

    std::string default_json = make_default_pipeline_json(file_path, inner_dir, script_path, script_path_2);

    const char *json_string = pipeline_json ? pipeline_json : default_json.c_str();
    create_pipeline_from_json(json_string);
}

int main(int argc, char **argv) {
    /* Set UTF-8 locale for subprocess filename handling */
    const char *cur_lc = getenv("LC_ALL");
    if (!cur_lc || !strstr(cur_lc, "UTF-8")) {
        setenv("LC_ALL", "zh_CN.UTF-8", 1);
        setenv("LANG", "zh_CN.UTF-8", 1);
    }

    /* Change working directory to the executable's directory */
    char *exe_dir = os_get_executable_path_ptr("");
    std::string work_dir;
    if (exe_dir) {
        if (os_chdir(exe_dir) == 0) {
            blog(LOG_INFO, "Changed working directory to %s", exe_dir);
            work_dir = exe_dir;
        }
        bfree(exe_dir);
    }
    if (work_dir.empty()) {
        char cwd[512];
        if (getcwd(cwd, sizeof(cwd))) work_dir = cwd;
    }

    if (!ols_startup("en-US", NULL, NULL)) {
        fprintf(stderr, "Couldn't create OLS\n");
        return 1;
    }
    struct ols_module_failure_info mfi;

    AddExtraModulePaths();
    ols_load_all_modules2(&mfi);
    ols_log_loaded_modules();
    ols_post_load_modules();

    ols_scripting_load();

    /* Record start time before pipeline runs */
    time_t start_time = time(NULL);

    const char *pipeline_json = nullptr;
    BPtr<char> file_data;
    std::string stdin_data;

    if (argc > 1) {
        /* Mode 1: JSON config from file argument */
        if (strcmp(argv[1], "-") == 0) {
            /* Read from stdin */
            stdin_data = read_stdin();
            if (!stdin_data.empty()) {
                pipeline_json = stdin_data.c_str();
                blog(LOG_INFO, "Loaded pipeline config from stdin (%zu bytes)", stdin_data.size());
            } else {
                fprintf(stderr, "Empty stdin input\n");
            }
        } else {
            file_data = os_quick_read_utf8_file(argv[1]);
            if (file_data) {
                pipeline_json = file_data;
                blog(LOG_INFO, "Loaded pipeline config from %s", argv[1]);
            } else {
                fprintf(stderr, "Failed to read pipeline config: %s\n", argv[1]);
            }
        }
    }

    create_pipeline(pipeline_json);

    /* Wait for pipeline to finish (EOS triggers output stop signal) */
    blog(LOG_INFO, "Pipeline running, waiting for completion...");
    int wait_count = 0;
    while (!g_pipeline_done) {
        os_sleep_ms(100);
        if (++wait_count >= 300) { /* 30 second timeout */
            blog(LOG_WARNING, "Pipeline timed out, forcing cleanup...");
            break;
        }
    }

    /* Wait briefly for async xml->html tasks to complete */
    os_sleep_ms(500);

    /* Collect generated result files */
    g_result_files = collect_result_files(work_dir.c_str(), start_time);

    destroy_pipeline();
    blog(LOG_INFO, "Pipeline completed.");

    ols_shutdown();
    ols_scripting_unload();

    /* Build result JSON string */
    std::string result_json = "{\"status\":\"completed\",\"result_files\":[";
    for (size_t i = 0; i < g_result_files.size(); i++) {
        if (i > 0) result_json += ",";
        result_json += "\"" + json_escape(g_result_files[i]) + "\"";
    }
    result_json += "]}";

    /* If OLS_RESULT_FILE env is set, write result JSON to that file;
     * otherwise print to stdout */
    const char *result_file = getenv("OLS_RESULT_FILE");
    if (result_file && result_file[0]) {
        FILE *fp = fopen(result_file, "w");
        if (fp) {
            fputs(result_json.c_str(), fp);
            fputc('\n', fp);
            fclose(fp);
        } else {
            fprintf(stderr, "Failed to write result file: %s\n", result_file);
            printf("%s\n", result_json.c_str());
        }
    } else {
        printf("%s\n", result_json.c_str());
    }

    return 0;
}
