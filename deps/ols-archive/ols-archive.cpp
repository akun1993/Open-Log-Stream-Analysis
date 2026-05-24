#include "ols-archive.h"
#include <functional>
#include <string>
#include <util/base.h>
#include <util/dstr.h>
#include <util/pipe.h>
#include <util/platform.h>
#include <util/str-util.h>
#include <util/task.h>
#include <string.h>

#define MAX_PATH_LENGTH 1024

#ifdef _WIN32
#include <windows.h>
#define SEVENZIP_CMD "7z"
#endif

static const char *get_filename(const char *path) {
    const char *filename = strrchr(path, '/');
#ifdef _WIN32
    const char *filename_win = strrchr(path, '\\');
    if (filename_win > filename) filename = filename_win;
#endif
    return filename ? filename + 1 : path;
}

static ArchiveFormat check_real_filetype(const char* filePath) {
    FILE* fp = os_fopen(filePath, "rb");
    if (!fp) return FORMAT_UNKNOWN;

    char buf[300] = {0};
    os_fgets(fp, buf, sizeof(buf));
    fclose(fp);

    if (buf[0] == 0x50 && buf[1] == 0x4B && buf[2] == 0x03 && buf[3] == 0x04) return FORMAT_ZIP;
    if (buf[0] == 0x52 && buf[1] == 0x61 && buf[2] == 0x72 && buf[3] == 0x21) return FORMAT_RAR;
    if (buf[0] == 0x37 && buf[1] == 0x7A && buf[2] == 0xBC && buf[3] == 0xAF) return FORMAT_7Z;
    if (buf[0] == 0x1F && buf[1] == 0x8B) return FORMAT_GZ;
    if (buf[0] == 0x42 && buf[1] == 0x5A && buf[2] == 0x68) return FORMAT_BZ2;
    if (buf[0] == 0xFD && buf[1] == 0x37 && buf[2] == 0x7A && buf[3] == 0x58) return FORMAT_XZ;
    if (buf[257] == 'u' && buf[258] == 's' && buf[259] == 't' && buf[260] == 'a' && buf[261] == 'r') return FORMAT_TAR;

    return FORMAT_UNKNOWN;
}

ArchiveFormat detect_format(const char *filename) {
    char lower_buf[MAX_PATH_LENGTH];
    strncpy(lower_buf, filename, sizeof(lower_buf) - 1);
    lower_buf[sizeof(lower_buf) - 1] = '\0';
    for (char *p = lower_buf; *p; ++p) {
        *p = (char)tolower((unsigned char)*p);
    }

    if (strstr(lower_buf, ".tar.gz") != NULL || strstr(lower_buf, ".tgz") != NULL) return FORMAT_TAR_GZ;
    if (strstr(lower_buf, ".tar.xz") != NULL || strstr(lower_buf, ".txz") != NULL) return FORMAT_TAR_XZ;
    if (strstr(lower_buf, ".tar.bz2") != NULL || strstr(lower_buf, ".tbz2") != NULL || strstr(lower_buf, ".tar.bz") != NULL) return FORMAT_TAR_BZ2;

    const char *ext = strrchr(lower_buf, '.');
    if (ext) {
        if (strcmp(ext, ".tar") == 0) return FORMAT_TAR;
        if (strcmp(ext, ".gz") == 0) return FORMAT_GZ;
        if (strcmp(ext, ".xz") == 0) return FORMAT_XZ;
        if (strcmp(ext, ".bz2") == 0) return FORMAT_BZ2;
        if (strcmp(ext, ".zip") == 0) return FORMAT_ZIP;
        if (strcmp(ext, ".7z") == 0) return FORMAT_7Z;
        if (strcmp(ext, ".rar") == 0) return FORMAT_RAR;
    }

    return check_real_filetype(filename);
}

const char* format_name(ArchiveFormat format) {
    switch (format) {
        case FORMAT_TAR_GZ:  return "tar.gz";
        case FORMAT_TAR_XZ:  return "tar.xz";
        case FORMAT_TAR_BZ2: return "tar.bz2";
        case FORMAT_TAR:     return "tar";
        case FORMAT_GZ:      return "gz";
        case FORMAT_XZ:      return "xz";
        case FORMAT_BZ2:     return "bz2";
        case FORMAT_ZIP:     return "zip";
        case FORMAT_7Z:      return "7z";
        case FORMAT_RAR:     return "rar";
        default:             return "unknown";
    }
}

std::string remove_compress_suffix(const std::string& full_path) {
    size_t len = full_path.size();
    if (len >= 7 && full_path.substr(len - 7) == ".tar.gz") {
        return full_path.substr(0, len - 7);
    }
    if (len >= 8 && full_path.substr(len - 8) == ".tar.bz2") {
        return full_path.substr(0, len - 8);
    }
    if (len >= 7 && full_path.substr(len - 7) == ".tar.xz") {
        return full_path.substr(0, len - 7);
    }
    size_t last_dot = full_path.find_last_of('.');
    size_t last_sep = full_path.find_last_of("/\\");
    if (last_dot != std::string::npos && (last_sep == std::string::npos || last_dot > last_sep)) {
        return full_path.substr(0, last_dot);
    }
    return full_path;
}

static bool run_command(const char *command, uint8_t *buff, size_t buff_len, std::function<void(uint8_t *, size_t)> callback) {
    os_process_pipe_t *pipe = os_process_pipe_create(command, "r");
    if (!pipe) return false;

    size_t len = 0;
    while ((len = os_process_pipe_read_err(pipe, buff, buff_len)) != 0) {
        callback(buff, len);
    }
    os_process_pipe_destroy(pipe);
    return true;
}

static bool extract_with_7z(const char* file, const char* outDir) {
    struct dstr command = {0};
    dstr_printf(&command, "\"%s\" x \"%s\" -y -o\"%s\"", SEVENZIP_CMD, file, outDir);
    blog(LOG_INFO, "[ols-archive] do decompress command %s", command.array);

    os_process_pipe_t *pipe = os_process_pipe_create(command.array, "r");
    if (pipe) {
        uint8_t buff[1024] = {'\0'};
        while (os_process_pipe_read(pipe, buff, sizeof(buff))) {
            blog(LOG_INFO, "%s", buff);
        }
        os_process_pipe_destroy(pipe);
    }

    dstr_free(&command);
    return true;
}

static bool extract_compressed_tar(const char* file, const char* outDir) {
    auto system_command = [outDir, file]() {
        struct dstr command = {0};
        dstr_printf(&command, "\"%s\" x \"%s\" -y -o\"%s\"", SEVENZIP_CMD, file, outDir);
        blog(LOG_INFO, "[ols-archive] do decompress command %s", command.array);

        os_process_pipe_t *pipe = os_process_pipe_create(command.array, "r");
        if (pipe) {
            uint8_t buff[1024] = {'\0'};
            while (os_process_pipe_read(pipe, buff, sizeof(buff))) {
                blog(LOG_INFO, "%s", buff);
            }
            os_process_pipe_destroy(pipe);
        }
        dstr_free(&command);
    };

    system_command();

    char tar_path[MAX_PATH_LENGTH] = {'\0'};
    const char *filename = get_filename(file);
    const char *dot = strrchr(filename, '.');
    if (dot && (strcmp(dot, ".gz") == 0 || strcmp(dot, ".bz2") == 0 || strcmp(dot, ".xz") == 0)) {
        size_t prefix_len = static_cast<size_t>(dot - filename);
        std::string prefix(filename, prefix_len);
        snprintf(tar_path, sizeof(tar_path), "%s\\%s", outDir, prefix.c_str());
    } else {
        snprintf(tar_path, sizeof(tar_path), "%s\\%s", outDir, filename);
    }

    system_command();
    os_unlink(tar_path);
    return true;
}

static std::string get_file_dir(const std::string &file, const char *default_dir) {
    size_t pos = file.find_last_of(FILE_SEPARATOR);
    if (pos != std::string::npos) {
        return file.substr(0, pos);
    }
    return std::string(default_dir);
}

bool decompress_file(const std::string &file, ArchiveFormat format, const std::string &dest_dir) {
    if (format == FORMAT_UNKNOWN) return false;

#ifdef _WIN32
    int res = 0;
    if (format == FORMAT_TAR_GZ || format == FORMAT_TAR_XZ || format == FORMAT_TAR_BZ2) {
        res = extract_compressed_tar(file.c_str(), dest_dir.c_str());
    } else {
        res = extract_with_7z(file.c_str(), dest_dir.c_str());
    }
    blog(LOG_INFO, "[ols-archive] decompress result %d", res);
    return res == 0 || res == 1;
#else
    struct dstr command = {0};
    if (format == FORMAT_ZIP) {
        dstr_printf(&command, "unzip -o %s -d %s", file.c_str(), dest_dir.c_str());
    } else if (format == FORMAT_TAR_GZ) {
        dstr_printf(&command, "tar -zvxf %s -C %s --overwrite", file.c_str(), dest_dir.c_str());
    } else if (format == FORMAT_TAR_XZ) {
        dstr_printf(&command, "tar -zJf %s -C %s --overwrite", file.c_str(), dest_dir.c_str());
    } else if (format == FORMAT_TAR_BZ2) {
        dstr_printf(&command, "tar -zjf %s -C %s --overwrite", file.c_str(), dest_dir.c_str());
    } else if (format == FORMAT_GZ) {
        if (!dest_dir.empty()) {
            os_mkdir(dest_dir.c_str());
            size_t pos = file.find_last_of('/');
            std::string dest_file = dest_dir;
            if (pos != std::string::npos) {
                dest_file += file.substr(pos);
            } else {
                dest_file += file;
            }
            if (str_endwith(dest_file.c_str(), ".gz")) {
                dest_file.erase(dest_file.size() - (sizeof(".gz") - 1));
            }
            dstr_printf(&command, "gunzip -c %s > %s", file.c_str(), dest_file.c_str());
        } else {
            dstr_printf(&command, "gzip -d %s", file.c_str());
        }
    } else {
        return false;
    }

    blog(LOG_INFO, "[ols-archive] do decompress command %s", command.array);
    os_process_pipe_t *pipe = os_process_pipe_create(command.array, "r");
    if (pipe) {
        uint8_t buff[1024] = {'\0'};
        while (os_process_pipe_read(pipe, buff, sizeof(buff))) {
            blog(LOG_INFO, "%s", buff);
        }
        os_process_pipe_destroy(pipe);
    }
    dstr_free(&command);
    return true;
#endif
}

bool decompress_log_file(const std::string &file) {
    ArchiveFormat extension = detect_format(file.c_str());
    if (extension == FORMAT_UNKNOWN) return false;

#ifdef _WIN32
    std::string dest_dir = get_file_dir(file, ".");
    if (extension == FORMAT_GZ || extension == FORMAT_TAR_GZ) {
        struct dstr command = {0};
        dstr_printf(&command, "7z x %s -aoa -o%s", file.c_str(), dest_dir.c_str());
        bool format_errno = false;
        uint8_t buffer[1024];
        run_command(command.array, buffer, sizeof(buffer), [extension, &format_errno](uint8_t *buff, size_t buff_len) {
            if (extension == FORMAT_TAR_GZ && strstr((const char *)buff, "Cannot open the file as [gzip]") != nullptr) {
                format_errno = true;
            }
        });
        if (extension == FORMAT_TAR_GZ) {
            size_t len = file.size();
            if (len >= 7 && file.substr(len - 7) == ".tar.gz") {
                std::string file_prefix = file.substr(0, len - 7);
                std::string new_file = file_prefix + ".tar";
                if (!os_file_exists(new_file.c_str())) {
                    new_file = file_prefix + ".gz";
                }
                struct dstr command2 = {0};
                dstr_printf(&command2, "7z x %s -aoa -o%s", new_file.c_str(), dest_dir.c_str());
                run_command(command2.array, buffer, sizeof(buffer), [](uint8_t *, size_t) {});
                dstr_free(&command2);
            }
        }
        dstr_free(&command);
        return true;
    }
    return false;
#else
    struct dstr command = {0};
    if (extension == FORMAT_TAR_GZ) {
        std::string dest_dir = get_file_dir(file, ".");
        dstr_printf(&command, "tar -zvxf %s -C %s --overwrite", file.c_str(), dest_dir.c_str());
    } else if (extension == FORMAT_GZ) {
        std::string dest_file = file;
        dest_file.erase(dest_file.size() - (sizeof(".gz") - 1));
        dstr_printf(&command, "gunzip -c %s > %s", file.c_str(), dest_file.c_str());
    } else {
        return false;
    }
    bool format_errno = false;
    uint8_t buffer[1024];
    run_command(command.array, buffer, sizeof(buffer), [extension, &format_errno](uint8_t *buff, size_t buff_len) {
        if (extension == FORMAT_TAR_GZ && strstr((const char *)buff, "not in gzip format") != nullptr) {
            format_errno = true;
        }
    });

    if (format_errno) {
        size_t pos = file.find_last_of(FILE_SEPARATOR);
        if (pos != std::string::npos) {
            std::string dest_dir = file.substr(0, pos);
            struct dstr fallback = {0};
            dstr_printf(&fallback, "cd %s; tar -xvf %s --overwrite | xargs gunzip", dest_dir.c_str(), file.c_str());
            run_command(fallback.array, buffer, sizeof(buffer), [](uint8_t *, size_t) {});
            dstr_free(&fallback);
        }
    }
    dstr_free(&command);
    return true;
#endif
}
