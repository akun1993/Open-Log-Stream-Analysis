#include "ols-meta-txt.h"
#include <algorithm>
#include <locale>
#include <memory>
#include <ols-module.h>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <functional>
#include <sys/stat.h>
#include <util/base.h>
#include <util/platform.h>
#include <util/task.h>
#include <util/util.hpp>
#include <util/pipe.h>
#include <util/str-util.h>
#include <string.h>
#include "ols-archive.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

// 线程池支持
#include <util/taskpool.h>
#include <mutex>
#include <future>

#define MAX_PATH_LENGTH 1024
#define MAX_CMD_LENGTH 2048

#ifndef S_ISDIR
#define S_ISDIR(mode) ((mode) & _S_IFDIR)
#endif

using namespace std;

#define warning(format, ...) blog(LOG_WARNING, "[%s] " format, ols_source_get_name(source), ##__VA_ARGS__)

#define warn_stat(call)                                                               \
    do {                                                                              \
        if (stat != Ok) warning("%s: %s failed (%d)", __FUNCTION__, call, (int)stat); \
    } while (false)

#ifndef clamp
#define clamp(val, min_val, max_val) \
    if (val < min_val)               \
        val = min_val;               \
    else if (val > max_val)          \
        val = max_val;
#endif

#define UNKNOW_FILE_EXT "unknow"

// // Recursively traverse directory, find files starting with prefix, return their directories (deduplicated)
// void find_dirs_by_file_prefix(const std::string &rootDir, const std::string &filePrefix, std::set<std::string> &resultDirs) {
//     os_dir_t *dir = os_opendir(rootDir.c_str());
//     if (!dir) return;

//     struct os_dirent *entry;

//     while ((entry = os_readdir(dir)) != nullptr) {
//         // Skip . and ..
//         if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;

//         // Build full path
//         std::string fullPath = rootDir + FILE_SEPARATOR + entry->d_name;

//         // Recursively traverse if it's a directory
//         if (entry->directory) {
//             find_dirs_by_file_prefix(fullPath, filePrefix, resultDirs);
//         } else {
//             std::string fileName = entry->d_name;
//             if (fileName.compare(0, filePrefix.length(), filePrefix) == 0) {
//                 resultDirs.insert(rootDir);  // Only store directory, automatically deduplicated
//             }
//         }
//     }

//     os_closedir(dir);
// }

// // Public interface: return vector<string>
// std::vector<std::string> getDirsWithPrefixFile(const std::string &rootDir, const std::string &filePrefix) {
//     std::set<std::string> dirSet;
//     find_dirs_by_file_prefix(rootDir, filePrefix, dirSet);
//     return std::vector<std::string>(dirSet.begin(), dirSet.end());
// }

/* ------------------------------------------------------------------------- */

/* clang-format off */

/* clang-format on */

/* ------------------------------------------------------------------------- */

static inline wstring to_wide(const char *utf8) {
    wstring text;

    size_t len = os_utf8_to_wcs(utf8, 0, nullptr, 0);
    text.resize(len);
    if (len) os_utf8_to_wcs(utf8, 0, &text[0], len + 1);

    return text;
}

static const char *supported_ext[] = {".zip", ".tar.gz", ".tar.xz", ".tar.bz2", ".tar", ".gz"};

// PCRE2 regex match (generic)
int regexMatch(const char *pattern, const char *str) {
    int errorCode;
    PCRE2_SIZE errorOffset;

    pcre2_code *re = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, 0, &errorCode, &errorOffset, NULL);

    if (!re) return 0;

    pcre2_match_data *match = pcre2_match_data_create_from_pattern(re, NULL);
    int ret = pcre2_match(re, (PCRE2_SPTR)str, PCRE2_ZERO_TERMINATED, 0, 0, match, NULL);

    pcre2_match_data_free(match);
    pcre2_code_free(re);
    return ret >= 1;
}

// Recursively search folder, return all file paths matching regex
std::vector<std::string> searchFiles(const char *folder, const char *regex) {
    std::vector<std::string> matches;

    os_dir_t *dir = os_opendir(folder);
    if (!dir) return matches;

    struct os_dirent *ent;

    char full[1024];

    while ((ent = os_readdir(dir)) != NULL) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;

        snprintf(full, sizeof(full), "%s/%s", folder, ent->d_name);

        if (ent->directory) {
            searchFiles(full, regex);
        } else {
            if (regexMatch(regex, ent->d_name)) matches.push_back(std::string(full));
        }
    }
    os_closedir(dir);
    return matches;
}

struct TextSource {
    ols_source_t *source_ = nullptr;

    bool read_from_file_ = false;

    string base_file_;
    string base_type_hint_;
    string inner_dir_;
    std::string file_wildcard_;
    std::string filter_string_;  // 用于过滤文件的字符串
    std::queue<std::string> files_;
    string curr_filename_;
    FILE *curr_file_ = nullptr;
    uint64_t line_cnt = 0;

    /* --------------------------- */

    inline TextSource(ols_source_t *source, ols_data_t *settings) : source_(source) {
        update(settings);
    }

    inline ~TextSource() {
    }

    int fileSrcGetData(ols_buffer_t *buf);

    void loadFileText();
    bool fileSrcStart();
    /* unmap and close the file */
    bool fileSrcStop();

    bool isSupportedCompressedFile(const char *file);

    void decompressFile(const std::string &file, ArchiveFormat format, const std::string &dest_dir);

    void loadMatchFilesInDir(const std::string &dest_dir, PCRE2_SPTR8 match_pattern, std::set<std::string> &files);

    bool openNextValidFile();

    // 检查文件是否包含指定字符串
    bool fileContainsString(const std::string &filepath, const std::string &searchStr);

    // 过滤文件队列，只保留包含指定字符串的文件
    void filterFilesByString(const std::string &searchStr);

    inline void update(ols_data_t *settings);
};

static time_t get_modified_timestamp(const char *filename) {
    struct stat stats;
    if (os_stat(filename, &stats) != 0) return -1;
    return stats.st_mtime;
}

void TextSource::loadFileText() {
    // BPtr<char> file_text = os_quick_read_utf8_file(curr_filename_.c_str());
    //  text = to_wide(GetMainString(file_text));

    // if (!text.empty() && text.back() != '\n')
    //   text.push_back('\n');
}

void TextSource::update(ols_data_t *settings) {
    if (ols_data_get_string(settings, "base_file") != NULL) {
        base_file_ = ols_data_get_string(settings, "base_file");

        blog(LOG_INFO, "base_file %s", ols_data_get_string(settings, "base_file"));
        // =

        if (ols_data_get_string(settings, "base_file_type_hint") != NULL) {
            base_type_hint_ = ols_data_get_string(settings, "base_file_type_hint");

            blog(LOG_INFO, "base_type_hint %s", ols_data_get_string(settings, "base_file_type_hint"));
        }

        if (ols_data_get_string(settings, "inner_dir") != NULL) {
            inner_dir_ = ols_data_get_string(settings, "inner_dir");

            blog(LOG_INFO, "inner_dir %s", ols_data_get_string(settings, "inner_dir"));
        }

        if (ols_data_get_string(settings, "file_name_wildcard") != NULL) {
            file_wildcard_ = ols_data_get_string(settings, "file_name_wildcard");

            blog(LOG_INFO, "file_wildcard  %s", ols_data_get_string(settings, "file_name_wildcard"));
        }
    }
}

int TextSource::fileSrcGetData(ols_buffer_t *buf) {
    // blog(LOG_DEBUG, "TextSource::FileSrcGetData");
    errno = 0;
    ols_meta_txt_t *ols_txt;
    ssize_t size;

    ols_txt = ols_meta_txt_new_with_buffer(1024);

    if (!curr_file_) {
        goto eos;
    }

    while (true) {
        size = os_fgetline(curr_file_, (char *)OLS_META_TXT_BUFF(ols_txt), OLS_META_TXT_BUFF_CAPACITY(ols_txt));
        if (UNLIKELY(size == -1)) {
            if (openNextValidFile()) {
                continue;
            } else {
                curr_file_ = NULL;
                goto eos;
            }
        } else {
            break;
        }
    }

    ols_txt->line = (int32_t)line_cnt;
    ols_txt->len = size;

    ols_buffer_set_meta(buf, OLS_META_CAST(ols_txt));

    ++line_cnt;

    return OLS_FLOW_OK;

eos: {
    ols_meta_txt_unref(ols_txt);
    blog(LOG_DEBUG, "EOS");
    // ols_buffer_resize(buf, 0, 0);
    return OLS_FLOW_EOS;
}
}

void TextSource::loadMatchFilesInDir(const std::string &dest_dir, PCRE2_SPTR8 match_pattern, std::set<std::string> &files) {
    os_dir_t *dir = os_opendir(dest_dir.c_str());

    if (dir) {
        struct os_dirent *ent;
        /* Compile the pattern. */
        int error_number;
        PCRE2_SIZE error_offset;
        pcre2_code *re = pcre2_compile(match_pattern,         /* the pattern */
                                       PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
                                       0,                     /* default options */
                                       &error_number,         /* for error number */
                                       &error_offset,         /* for error offset */
                                       NULL);                 /* use default compile context */
        if (re == NULL) {
            blog(LOG_ERROR, "Invalid pattern: %s\n", match_pattern);
            os_closedir(dir);
            return;
        }

        /* Match the pattern against the subject text. */
        pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

        std::vector<std::string> subdirs;

        for (;;) {
            ent = os_readdir(dir);
            if (!ent) break;

            if (ent->directory) {
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                    std::string subdir = dest_dir;
                    subdir.append(1, FILE_SEPARATOR).append(ent->d_name);
                    subdirs.push_back(subdir);
                }
                continue;
            }

            int rc = pcre2_match(re,                       /* the compiled pattern */
                                 (PCRE2_SPTR8)ent->d_name, /* the subject text */
                                 strlen(ent->d_name),      /* the length of the subject */
                                 0,                        /* start at offset 0 in the subject */
                                 0,                        /* default options */
                                 match_data,               /* block for storing the result */
                                 NULL);                    /* use default match context */

            if (rc == PCRE2_ERROR_NOMATCH) {
                blog(LOG_DEBUG, "No match : %s\n", ent->d_name);
            } else if (rc < 0) {
                blog(LOG_ERROR, "Matching error\n");
            } else {
                std::string file_path = dest_dir;
                file_path.append(1, FILE_SEPARATOR).append(ent->d_name);
                files.insert(file_path);
            }
        }
        pcre2_match_data_free(match_data); /* Free resources */
        pcre2_code_free(re);

        os_closedir(dir);

        /* Recursively search subdirectories */
        for (const auto &subdir : subdirs) {
            loadMatchFilesInDir(subdir, match_pattern, files);
        }
    }
}

void TextSource::decompressFile(const std::string &file, ArchiveFormat format, const std::string &dest_dir) {
    decompress_file(file, format, dest_dir);
}

/**
 * 检查文件是否包含指定字符串（优化版本）
 * 使用更高效的搜索策略：先检查文件开头，再全文件搜索
 * @param filepath 文件路径
 * @param searchStr 要搜索的字符串
 * @return 如果文件包含该字符串返回true，否则返回false
 */
bool TextSource::fileContainsString(const std::string &filepath, const std::string &searchStr) {
    if (searchStr.empty()) {
        return true;
    }

    FILE *file = os_fopen(filepath.c_str(), "rb");
    if (!file) {
        blog(LOG_WARNING, "Cannot open file for filtering: %s", filepath.c_str());
        return false;
    }

    bool found = false;

    // 策略1: 先读取前64KB快速检查
    constexpr size_t PREVIEW_SIZE = 64 * 1024;
    char previewBuffer[PREVIEW_SIZE];
    size_t previewRead = fread(previewBuffer, 1, PREVIEW_SIZE, file);

    if (std::string(previewBuffer, previewRead).find(searchStr) != std::string::npos) {
        fclose(file);
        return true;
    }

    // 策略2: 如果文件较大，继续搜索剩余部分
    if (previewRead == PREVIEW_SIZE) {
        // 使用memmem进行高效的二进制搜索
        std::string overlap(searchStr.length() - 1, '\0');
        memcpy(&overlap[0], previewBuffer + previewRead - searchStr.length() + 1, searchStr.length() - 1);

        char buffer[64 * 1024];
        while (!found && fgets(buffer, sizeof(buffer), file) != nullptr) {
            std::string searchRegion = overlap + buffer;
            if (searchRegion.find(searchStr) != std::string::npos) {
                found = true;
            }
            // 更新重叠区域
            size_t bufLen = strlen(buffer);
            if (bufLen >= searchStr.length() - 1) {
                overlap.assign(buffer + bufLen - searchStr.length() + 1, searchStr.length() - 1);
            }
        }
    }

    fclose(file);
    return found;
}

/**
 * 过滤文件队列，使用线程池并行搜索
 * @param searchStr 要搜索的字符串，如果为空则不过滤
 */

// 用于线程池任务的结构体
struct FilterTaskData {
    TextSource *source;
    std::string filepath;
    std::string searchStr;
    std::atomic<bool> *result;
};

static void filterTaskFunc(void *param) {
    FilterTaskData *data = static_cast<FilterTaskData *>(param);
    bool found = data->source->fileContainsString(data->filepath, data->searchStr);
    data->result->store(found, std::memory_order_relaxed);
}

void TextSource::filterFilesByString(const std::string &searchStr) {
    if (searchStr.empty()) {
        blog(LOG_INFO, "Filter string is empty, skipping file filter");
        return;
    }

    blog(LOG_INFO, "Filtering files by string (parallel): '%s'", searchStr.c_str());

    // 将队列转换为vector便于并行处理
    std::vector<std::string> filesToCheck;
    while (!files_.empty()) {
        filesToCheck.push_back(files_.front());
        files_.pop();
    }

    int totalFiles = static_cast<int>(filesToCheck.size());
    if (totalFiles == 0) {
        return;
    }

    // 使用默认线程池并行检查每个文件
    os_taskpool_t *pool = os_taskpool_get_default();
    if (!pool) {
        blog(LOG_ERROR, "Failed to get thread pool");
        return;
    }

    // 准备任务数据和结果存储
    std::vector<std::unique_ptr<FilterTaskData>> taskDataList;
    std::vector<std::unique_ptr<std::atomic<bool>>> results;
    taskDataList.reserve(filesToCheck.size());
    results.reserve(filesToCheck.size());

    for (const auto &filepath : filesToCheck) {
        auto taskData = std::make_unique<FilterTaskData>();
        auto result = std::make_unique<std::atomic<bool>>(false);

        taskData->source = this;
        taskData->filepath = filepath;
        taskData->searchStr = searchStr;
        taskData->result = result.get();

        os_taskpool_queue_task(pool, filterTaskFunc, taskData.get());

        taskDataList.push_back(std::move(taskData));
        results.push_back(std::move(result));
    }

    // 等待所有任务完成
    os_taskpool_wait(pool);

    // 收集结果
    std::queue<std::string> filteredQueue;
    int matchedFiles = 0;

    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i]->load(std::memory_order_relaxed)) {
            filteredQueue.push(filesToCheck[i]);
            matchedFiles++;
            blog(LOG_INFO, "File matched filter: %s", filesToCheck[i].c_str());
        } else {
            blog(LOG_DEBUG, "File filtered out: %s", filesToCheck[i].c_str());
        }
    }

    files_ = filteredQueue;
    blog(LOG_INFO, "Filter complete: %d/%d files matched", matchedFiles, totalFiles);
}

bool TextSource::openNextValidFile() {
    struct stat stat_results;

    if (curr_file_) {
        fclose(curr_file_);
        curr_file_ = NULL;
    }

    if (files_.empty()) {
        goto no_filename;
    }

    while (!files_.empty()) {
        curr_filename_ = files_.front();
        files_.pop();

        if (curr_filename_.empty()) {
            blog(LOG_INFO, "file name is empty");
            continue;
        }

        if (os_stat(curr_filename_.c_str(), &stat_results) < 0) {
            blog(LOG_ERROR, ("Could not get info on \"%s\"."), curr_filename_.c_str());
            continue;
        }

        if (S_ISDIR(stat_results.st_mode)) {
            blog(LOG_ERROR, "\"%s\" is a directory.", curr_filename_.c_str());
            continue;
        }
        /* open the file */
        curr_file_ = os_fopen(curr_filename_.c_str(), "rb");
        if (curr_file_ == NULL) {
            switch (errno) {
                case ENOENT:
                    blog(LOG_ERROR, "No such file \"%s\"", curr_filename_.c_str());
                    break;
                default:
                    blog(LOG_ERROR, ("Could not open file \"%s\" for reading."), curr_filename_.c_str());
                    break;
            }
            continue;
        }
        blog(LOG_INFO, "Open file \"%s\" for reading.", curr_filename_.c_str());
        return true;
    }

    return false;

    /* ERROR */
no_filename:
    blog(LOG_ERROR, ("No file name specified for reading."));
    return false;
}

/* open the file, necessary to go to READY state */
bool TextSource::fileSrcStart() {
    struct stat stat_results;
    ArchiveFormat file_format;
    std::string dest_dir;
    std::set<std::string> files;

    if (base_file_.empty()) {
        blog(LOG_ERROR, "No file name specified for reading.");
        return false;
    }

    if (os_stat(base_file_.c_str(), &stat_results) < 0) {
        blog(LOG_ERROR, "Could not get info on \"%s\".", base_file_.c_str());
        return false;
    }

    if (!S_ISDIR(stat_results.st_mode)) {
        file_format = detect_format(base_file_.c_str());

        if (file_format == ArchiveFormat::FORMAT_UNKNOWN) {  // treat as a regular file
            blog(LOG_INFO, "Source file extension is NULL");
            files_.push(base_file_);
        } else {
            blog(LOG_INFO, "%s archive file must be decompress before read\n", base_file_.c_str());
        }
    } else {
        dest_dir = base_file_;

        if (!inner_dir_.empty()) dest_dir.append(1, FILE_SEPARATOR).append(inner_dir_);
    }

    loadMatchFilesInDir(dest_dir, (PCRE2_SPTR8)file_wildcard_.c_str(), files);

    // 并行解压 - 使用线程池同时处理多个压缩文件
    std::set<std::string> decompressed_targets;
    std::mutex decompress_mutex;  // 保护decompressed_targets的互斥锁
    std::vector<std::string> files_to_decompress;

    // 第一步：收集需要解压的文件（线程安全的预处理）
    for (auto &file : files) {
        std::string target = remove_compress_suffix(file);

        // Check if target file already exists in filesystem
        if (os_file_exists(target.c_str())) {
            blog(LOG_INFO, "Target file %s already exists, skip decompressing %s", target.c_str(), file.c_str());
            std::lock_guard<std::mutex> lock(decompress_mutex);
            decompressed_targets.insert(target);
            continue;
        }

        // Check if already processed in this batch
        {
            std::lock_guard<std::mutex> lock(decompress_mutex);
            if (decompressed_targets.find(target) != decompressed_targets.end()) {
                blog(LOG_INFO, "Skip decompressing %s, target already processed", file.c_str());
                continue;
            }
            decompressed_targets.insert(target);
        }

        files_to_decompress.push_back(file);
    }

    // 第二步：使用线程池并行解压
    if (!files_to_decompress.empty()) {
        blog(LOG_INFO, "Starting parallel decompression of %zu files", files_to_decompress.size());

        os_taskpool_t *pool = os_taskpool_get_default();
        if (pool) {
            // 提交所有解压任务到线程池
            for (const auto &file : files_to_decompress) {
                // 需要复制字符串，因为回调是异步的
                std::string *fileCopy = new std::string(file);
                os_taskpool_queue_task(
                    pool,
                    [](void *param) {
                        std::string *filePath = static_cast<std::string *>(param);
                        blog(LOG_DEBUG, "Decompressing file: %s", filePath->c_str());
                        decompress_log_file(*filePath);
                        blog(LOG_DEBUG, "Finished decompressing: %s", filePath->c_str());
                        delete filePath;
                    },
                    fileCopy);
            }

            // 等待所有解压任务完成
            os_taskpool_wait(pool);
        }

        blog(LOG_INFO, "Parallel decompression completed");
    }
    // reload after decompress
    files.clear();
    loadMatchFilesInDir(dest_dir, (PCRE2_SPTR8)file_wildcard_.c_str(), files);

    for (auto &file : files) {
        ArchiveFormat ext = detect_format(file.c_str());
        if (ext == ArchiveFormat::FORMAT_UNKNOWN) {
            files_.push(file);
            blog(LOG_INFO, "get file %s", file.c_str());
        }
    }

    if (files_.empty()) {
        return false;
    }

    // 根据filter_string_过滤文件
    // filterFilesByString(filter_string_);

    if (files_.empty()) {
        blog(LOG_WARNING, "No files matched the filter string: '%s'", filter_string_.c_str());
        return false;
    }

    return openNextValidFile();
}

/* unmap and close the file */
bool TextSource::fileSrcStop() {
    /* close the file */
    if (curr_file_) fclose(curr_file_);
    /* zero out a lot of our state */
    curr_file_ = NULL;
    return true;
}

bool TextSource::isSupportedCompressedFile(const char *file) {
    bool flag = false;

    for (size_t i = 0; i < sizeof(supported_ext) / sizeof(supported_ext[0]); ++i) {
        if (str_endwith(file, supported_ext[i])) {
            flag = true;
            break;
        }
    }

    return flag;
}

#define ols_data_get_uint32 (uint32_t)ols_data_get_int

OLS_DECLARE_MODULE()

OLS_MODULE_USE_DEFAULT_LOCALE("ols-text", "en-US")
MODULE_EXPORT const char *ols_module_description(void) {
    return "text source";
}

static ols_properties_t *get_properties(void *data) {
    UNUSED_PARAMETER(data);

    ols_properties_t *props = ols_properties_create();

    ols_properties_add_int(props, "base_file", ols_module_text("ColorSource.Width"), 0, 4096, 1);

    ols_properties_add_int(props, "base_file_type_hint", ols_module_text("ColorSource.Height"), 0, 4096, 1);

    ols_properties_add_int(props, "inner_dir", ols_module_text("ColorSource.Height"), 0, 4096, 1);

    ols_properties_add_int(props, "file_name_wildcard", ols_module_text("ColorSource.Height"), 0, 4096, 1);

    return props;
}

bool ols_module_load(void) {
    ols_source_info si = {};
    si.id = "text_file";
    si.type = OLS_SOURCE_TYPE_INPUT;
    // si.output_flags = OLS_SOURCE_
    si.get_properties = get_properties;
    si.icon_type = OLS_ICON_TYPE_TEXT;

    si.get_name = [](void *) {
        return ols_module_text("TextFile");
    };

    si.create = [](ols_data_t *settings, ols_source_t *source) {
        return (void *)new TextSource(source, settings);
    };

    si.destroy = [](void *data) {
        delete reinterpret_cast<TextSource *>(data);
    };

    si.request_new_pad = NULL;

    si.get_defaults = [](ols_data_t *settings) {
        // defaults(settings, 1);
        UNUSED_PARAMETER(settings);
    };

    si.activate = [](void *data) {
        reinterpret_cast<TextSource *>(data)->fileSrcStart();
    };

    si.update = [](void *data, ols_data_t *settings) {
        reinterpret_cast<TextSource *>(data)->update(settings);
    };

    si.get_data = [](void *data, ols_buffer_t *buf) {
        return reinterpret_cast<TextSource *>(data)->fileSrcGetData(buf);
    };

    si.version = 0;

    // si.missing_files = [](void *data) {
    // 	TextSource *s = reinterpret_cast<TextSource *>(data);
    // 	ols_missing_files_t *files = ols_missing_files_create();

    // 	ols_source_t *source = s->source;
    // 	ols_data_t *settings = ols_source_get_settings(source);

    // 	bool read = ols_data_get_bool(settings, S_USE_FILE);
    // 	const char *path = ols_data_get_string(settings, S_FILE);

    // 	if (read && strcmp(path, "") != 0) {
    // 		if (!os_file_exists(path)) {
    // 			ols_missing_file_t *file =
    // 				ols_missing_file_create(
    // 					path, missing_file_callback,
    // 					OLS_MISSING_FILE_SOURCE,
    // 					s->source, NULL);

    // 			ols_missing_files_add_file(files, file);
    // 		}
    // 	}

    // 	ols_data_release(settings);

    // 	return files;
    // };

    ols_register_source(&si);

    return true;
}

void ols_module_unload(void) {
}
