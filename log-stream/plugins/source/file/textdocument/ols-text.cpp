
#include <algorithm>
#include <locale>
#include <memory>
#include <ols-module.h>
#include <string>
#include <sys/stat.h>
#include <util/base.h>
#include <util/platform.h>
#include <util/task.h>
#include <util/util.hpp>

using namespace std;

#define warning(format, ...)                                                   \
  blog(LOG_WARNING, "[%s] " format, ols_source_get_name(source), ##__VA_ARGS__)

#define warn_stat(call)                                                        \
  do {                                                                         \
    if (stat != Ok)                                                            \
      warning("%s: %s failed (%d)", __FUNCTION__, call, (int)stat);            \
  } while (false)

#ifndef clamp
#define clamp(val, min_val, max_val)                                           \
  if (val < min_val)                                                           \
    val = min_val;                                                             \
  else if (val > max_val)                                                      \
    val = max_val;
#endif

/* ------------------------------------------------------------------------- */

/* clang-format off */

/* clang-format on */

/* ------------------------------------------------------------------------- */

static inline wstring to_wide(const char *utf8) {
  wstring text;

  size_t len = os_utf8_to_wcs(utf8, 0, nullptr, 0);
  text.resize(len);
  if (len)
    os_utf8_to_wcs(utf8, 0, &text[0], len + 1);

  return text;
}

struct TextSource {
  ols_source_t *source_ = nullptr;

  bool read_from_file = false;
  string filename;
  string uri;
  int fd = -1;

  wstring text;

  /* --------------------------- */

  inline TextSource(ols_source_t *source, ols_data_t *settings)
      : source_(source) {
    ols_source_update(source, settings);
  }

  inline ~TextSource() {}

  int FileSrcGetData(ols_buffer_t *buf);

  void LoadFileText();
  bool FileSrcStart();
  /* unmap and close the file */
  bool FileSrcStop();

  inline void Update(ols_data_t *settings);
};

static time_t get_modified_timestamp(const char *filename) {
  struct stat stats;
  if (os_stat(filename, &stats) != 0)
    return -1;
  return stats.st_mtime;
}

// const char *TextSource::GetMainString(const char *str)
// {
// 	if (!str)
// 		return "";

// 	size_t len = strlen(str);
// 	if (!len)
// 		return str;

// 	const char *temp = str + len;

// 	while (temp != str) {
// 		temp--;

// 		if (temp[0] == '\n' && temp[1] != 0) {
// 			if (!--lines)
// 				break;
// 		}
// 	}

// 	return *temp == '\n' ? temp + 1 : temp;
// }

void TextSource::LoadFileText() {
  BPtr<char> file_text = os_quick_read_utf8_file(filename.c_str());
  // text = to_wide(GetMainString(file_text));

  if (!text.empty() && text.back() != '\n')
    text.push_back('\n');
}

void TextSource::Update(ols_data_t *settings) { UNUSED_PARAMETER(settings); }

int TextSource::FileSrcGetData(ols_buffer_t *buf) {

  blog(LOG_DEBUG, "TextSource::GetData");
  uint32_t to_read, bytes_read;
  int ret;

  uint8_t *data;

  data = info.data;

  bytes_read = 0;
  to_read = length;

  while (to_read > 0) {
    // blog(src, "Reading %d bytes at offset 0x%" G_GINT64_MODIFIER "x",
    //                to_read, offset + bytes_read);
    errno = 0;
    ret = os_fread_utf8(src->fd, data + bytes_read, to_read);
    if (UNLIKELY(ret < 0)) {
      if (errno == EAGAIN || errno == EINTR)
        continue;
      goto could_not_read;
    }

    /* files should eos if they read 0 and more was requested */
    if (UNLIKELY(ret == 0)) {
      /* .. but first we should return any remaining data */
      if (bytes_read > 0)
        break;
      goto eos;
    }

    to_read -= ret;
    bytes_read += ret;

    // src->read_position += ret;
  }

  if (bytes_read != length)
    gst_buffer_resize(buf, 0, bytes_read);

  return OLS_FLOW_OK;

could_not_read: {
  blog(LOG_ERROR, ("Can't read file"));

  gst_buffer_resize(buf, 0, 0);
  return OLS_FLOW_ERROR;
}
eos: {
  blog(LOG_DEBUG, "EOS");
  ols_buffer_resize(buf, 0, 0);
  return OLS_FLOW_EOS;
}
buffer_write_fail: {
  blog(LOG_ERROR, ("Can't write to buffer"));
  return OLS_FLOW_ERROR;
}
  return OLS_FLOW_OK;
}

/* open the file, necessary to go to READY state */
bool TextSource::FileSrcStart() {

  struct stat stat_results;

  if (filename.empty())
    goto no_filename;

  blog(LOG_INFO, "opening file %s", filename.c_str());

  /* open the file */
  fd = os_fopen(filename.c_str(), O_RDONLY | O_BINARY, 0);

  if (fd < 0)
    goto open_failed;

  /* check if it is a regular file, otherwise bail out */
  if (fstat(fd, &stat_results) < 0)
    goto no_stat;

  if (S_ISDIR(stat_results.st_mode))
    goto was_directory;

  if (S_ISSOCK(stat_results.st_mode))
    goto was_socket;

  return true;

  /* ERROR */
no_filename: {
  blog(LOG_ERROR, ("No file name specified for reading."));
  goto error_exit;
}

open_failed: {
  switch (errno) {
  case ENOENT:
    blog(LOG_ERROR, "No such file \"%s\"", filename.c_str());
    break;
  default:
    blog(LOG_ERROR, ("Could not open file \"%s\" for reading."),
         filename.c_str());
    break;
  }
  goto error_exit;
}
no_stat: {
  blog(LOG_ERROR, ("Could not get info on \"%s\"."), filename.c_str());
  goto error_close;
}

was_directory: {
  blog(LOG_ERROR, "\"%s\" is a directory.", filename.c_str());
  goto error_close;
}

was_socket: {
  blog(LOG_ERROR, ("File \"%s\" is a socket."), filename.c_str());
  goto error_close;
}

lseek_wonky: {
  blog(LOG_ERROR, "Could not seek back to zero after seek test in file \"%s\"",
       filename.c_str());
  goto error_close;
}

error_close:
  close(fd);
error_exit:
  return false;
}

/* unmap and close the file */
bool TextSource::FileSrcStop() {
  /* close the file */
  close(fd);
  /* zero out a lot of our state */
  fd = 0;
  return true;
}

#define obs_data_get_uint32 (uint32_t)obs_data_get_int

OLS_DECLARE_MODULE()

OLS_MODULE_USE_DEFAULT_LOCALE("ols-text", "en-US")
MODULE_EXPORT const char *ols_module_description(void) { return "text source"; }

static ols_properties_t *get_properties(void *data) {
  TextSource *s = reinterpret_cast<TextSource *>(data);
  string path;

  ols_properties_t *props = ols_properties_create();
  ols_property_t *p;

  // p = ols_properties_add_bool(props, S_USE_FILE, T_USE_FILE);
  // ols_property_set_modified_callback(p, use_file_changed);

  // string filter;
  // filter += T_FILTER_TEXT_FILES;
  // filter += " (*.txt);;";
  // filter += T_FILTER_ALL_FILES;
  // filter += " (*.*)";

  if (s && !s->filename.empty()) {
    const char *slash;

    path = s->filename;
    replace(path.begin(), path.end(), '\\', '/');
    slash = strrchr(path.c_str(), '/');
    if (slash)
      path.resize(slash - path.c_str() + 1);
  }

  return props;
}

// static ols_pad_t *get_new_pad(void *data) {
//   TextSource *s = reinterpret_cast<TextSource *>(data);

//   return s->srcpad;
// }

// static void missing_file_callback(void *src, const char *new_path, void
// *data)
// {
// 	TextSource *s = reinterpret_cast<TextSource *>(src);

// 	ols_source_t *source = s->source;
// 	ols_data_t *settings = ols_source_get_settings(source);
// 	ols_data_set_string(settings, S_FILE, new_path);
// 	ols_source_update(source, settings);
// 	ols_data_release(settings);

// 	UNUSED_PARAMETER(data);
// }

bool ols_module_load(void) {
  ols_source_info si = {};
  si.id = "text_file";
  si.type = OLS_SOURCE_TYPE_INPUT;
  // si.output_flags = OLS_SOURCE_ | OBS_SOURCE_CUSTOM_DRAW |
  // 		  OBS_SOURCE_CAP_OBSOLETE | OBS_SOURCE_SRGB;
  si.get_properties = get_properties;
  si.icon_type = OLS_ICON_TYPE_TEXT;

  si.get_name = [](void *) { return ols_module_text("TextFile"); };
  si.create = [](ols_data_t *settings, ols_source_t *source) {
    return (void *)new TextSource(source, settings);
  };
  si.destroy = [](void *data) { delete reinterpret_cast<TextSource *>(data); };

  si.get_new_pad = NULL;

  si.get_defaults = [](ols_data_t *settings) {
    // defaults(settings, 1);
    UNUSED_PARAMETER(settings);
  };

  si.update = [](void *data, ols_data_t *settings) {
    reinterpret_cast<TextSource *>(data)->Update(settings);
  };

  si.get_data = [](void *data, ols_buffer_t *buf) {
    return reinterpret_cast<TextSource *>(data)->FileSrcGetData(buf);
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
  // GdiplusShutdown(gdip_token);
}
