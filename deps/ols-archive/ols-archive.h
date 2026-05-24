#pragma once

#include <string>

enum ArchiveFormat {
    FORMAT_UNKNOWN = 0,
    FORMAT_TAR_GZ,
    FORMAT_TAR_XZ,
    FORMAT_TAR_BZ2,
    FORMAT_TAR_ZST,
    FORMAT_TAR,
    FORMAT_GZ,
    FORMAT_XZ,
    FORMAT_BZ2,
    FORMAT_ZIP,
    FORMAT_7Z,
    FORMAT_RAR
};

ArchiveFormat detect_format(const char *filename);
const char* format_name(ArchiveFormat format);
std::string remove_compress_suffix(const std::string& full_path);
bool decompress_file(const std::string &file, ArchiveFormat format, const std::string &dest_dir);
bool decompress_log_file(const std::string &file);
