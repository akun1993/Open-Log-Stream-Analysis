
#include "ols-meta-txt.h"
#include <algorithm>
#include <locale>
#include <memory>
#include <ols-module.h>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <util/base.h>
#include <util/platform.h>
#include <util/task.h>
#include <util/util.hpp>
#include <util/pipe.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

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

  bool read_from_file_ = false;
  
  string base_file_;
  string base_type_hint_;
  string inner_dir_;
  std::string file_wildcard_;
  std::vector<std::string> files_;
  string  curr_filename_;
  FILE   *curr_file_ = nullptr;
  uint64_t line_cnt = 0;


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
  BPtr<char> file_text = os_quick_read_utf8_file(curr_filename_.c_str());
  // text = to_wide(GetMainString(file_text));

  // if (!text.empty() && text.back() != '\n')
  //   text.push_back('\n');
}

void TextSource::Update(ols_data_t *settings) { 

	if (ols_data_get_string(settings, "base_file") != NULL ) {

    
    base_file_ = ols_data_get_string(settings, "base_file");
    
    blog(LOG_INFO, "base_file %s",ols_data_get_string(settings, "base_file"));
    // = 

    if (ols_data_get_string(settings, "base_file_type_hint") != NULL ) {

      base_type_hint_ = ols_data_get_string(settings, "base_file_type_hint");

      blog(LOG_INFO, "base_type_hint %s",ols_data_get_string(settings, "base_file_type_hint"));
    } 

    
    if (ols_data_get_string(settings, "inner_dir") != NULL ) {

      inner_dir_ = ols_data_get_string(settings, "inner_dir");

      blog(LOG_INFO, "inner_dir %s",ols_data_get_string(settings, "inner_dir"));
    } 

    if (ols_data_get_string(settings, "file_name_wildcard") != NULL ) {

      file_wildcard_ = ols_data_get_string(settings, "file_name_wildcard");

      blog(LOG_INFO, "file_wildcard  %s",ols_data_get_string(settings, "file_name_wildcard"));
    } 

	}


}

int TextSource::FileSrcGetData(ols_buffer_t *buf) {

  // blog(LOG_DEBUG, "TextSource::FileSrcGetData");
  errno = 0;
  ols_meta_txt_t *ols_txt;
  ssize_t size ;

  if(!curr_file_){
    goto eos;
  }

  ols_txt = ols_meta_txt_new_with_buffer(1024);

  size = os_fgetline(curr_file_, (char *)OLS_META_TXT_BUFF(ols_txt),OLS_META_TXT_BUFF_CAPACITY(ols_txt));
  if (UNLIKELY(size == -1)) {
    goto eos;
  }

  ols_txt->line = line_cnt;
  ols_txt->len = size;

  ols_buffer_set_meta(buf, OLS_META_CAST(ols_txt));

  ++line_cnt;

  // blog(LOG_DEBUG, "%s  line %ld", bytes, line_cnt);

  return OLS_FLOW_OK;

eos: {
  blog(LOG_DEBUG, "EOS");
  // ols_buffer_resize(buf, 0, 0);
  return OLS_FLOW_EOS;
}
}

/* open the file, necessary to go to READY state */
bool TextSource::FileSrcStart() {

  struct stat stat_results;
  std::string  file_ext;
  std::string dest_dir;
  size_t pos;

  if(base_file_.empty()){
    goto no_filename;
  }

  /* check if it is a dir */
  if (os_stat(base_file_.c_str(), &stat_results) < 0)
    goto no_stat;

  if (!S_ISDIR(stat_results.st_mode)){

    if(base_type_hint_.empty()){
      file_ext = os_get_path_extension(base_file_.c_str());
    } else {
      file_ext = base_type_hint_;
    }

    if(file_ext.empty()){ //treat as a regular file 

    } else {
      if(file_ext == "zip"){

        dest_dir = base_file_;
        pos = dest_dir.find_last_of('.');

        if(pos != std::string::npos){
          dest_dir = dest_dir.substr(pos).append(".ols");
        } else {
          dest_dir.append(".ols");
        }
        
        struct dstr command = {0};
        dstr_printf(&command,"unzip -d %s %s",dest_dir.c_str() ,base_file_.c_str());
        
        os_process_pipe_t * pipe = os_process_pipe_create(command.array,"r");

        if(pipe){
          uint8_t  buff[1024] = {'\0'};
          while(os_process_pipe_read(pipe,buff,1024)){
            blog(LOG_INFO,"%s",buff);
          }

          os_process_pipe_destroy(pipe);
        }
         

      } else  if(base_type_hint_ == "tar.gz"){

        os_process_pipe_t * pipe = os_process_pipe_create("tar -zxvf example.tar.gz -C destination_folder","r");

        
      } else if(base_type_hint_ == "gz"){

      } else {
          blog(LOG_ERROR,"Can not type : %s",base_type_hint_.c_str());
      }

      dest_dir.append("/").append(inner_dir_);

      printf("dest is %s\n",dest_dir.c_str());
      os_dir_t *dir = os_opendir(dest_dir.c_str());

      if (dir) {
  
        struct os_dirent *ent;

        for (;;) {
          const char *ext;

          ent = os_readdir(dir);
          if (!ent)
            break;
          if (ent->directory)
            continue;

          
          std::string file_path = dest_dir.append(ent->d_name);

          files_.push_back(file_path);
        }


        os_closedir(dir);
      } 
    }
  } else {
    base_file_.append("/").append(inner_dir_);
  }

  if (curr_filename_.empty())
    goto no_filename;
  

  if (os_stat(curr_filename_.c_str(), &stat_results) < 0)
    goto no_stat;

  if (S_ISDIR(stat_results.st_mode))
    goto was_directory;


  blog(LOG_INFO, "opening file %s", curr_filename_.c_str());

  /* open the file */
  curr_file_ = os_fopen(curr_filename_.c_str(), "rb");

  if (curr_file_ == NULL)
    goto open_failed;

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
    blog(LOG_ERROR, "No such file \"%s\"", curr_filename_.c_str());
    break;
  default:
    blog(LOG_ERROR, ("Could not open file \"%s\" for reading."),
    curr_filename_.c_str());
    break;
  }
  goto error_exit;
}
no_stat: {
  blog(LOG_ERROR, ("Could not get info on \"%s\"."), curr_filename_.c_str());
  goto error_close;
}

was_directory: {
  blog(LOG_ERROR, "\"%s\" is a directory.", curr_filename_.c_str());
  goto error_close;
}

was_socket: {
  blog(LOG_ERROR, ("File \"%s\" is a socket."), curr_filename_.c_str());
  goto error_close;
}

lseek_wonky: {
  blog(LOG_ERROR, "Could not seek back to zero after seek test in file \"%s\"",
    curr_filename_.c_str());
  goto error_close;
}

error_close:
  fclose(curr_file_);
error_exit:
  return false;
}

/* unmap and close the file */
bool TextSource::FileSrcStop() {
  /* close the file */
  if(curr_file_)
    fclose(curr_file_);
  /* zero out a lot of our state */
  curr_file_ = NULL;
  return true;
}

#define ols_data_get_uint32 (uint32_t)ols_data_get_int

OLS_DECLARE_MODULE()

OLS_MODULE_USE_DEFAULT_LOCALE("ols-text", "en-US")
MODULE_EXPORT const char *ols_module_description(void) { return "text source"; }

static ols_properties_t *get_properties(void *data) {
  TextSource *s = reinterpret_cast<TextSource *>(data);
  string path;

  ols_properties_t *props = ols_properties_create();
  ols_property_t *p;

  if (s && !s->curr_filename_.empty()) {
    const char *slash;

    path = s->curr_filename_;
    replace(path.begin(), path.end(), '\\', '/');
    slash = strrchr(path.c_str(), '/');
    if (slash)
      path.resize(slash - path.c_str() + 1);
  }

  return props;
}

// static ols_pad_t *request_new_pad(void *data) {
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
  // si.output_flags = OLS_SOURCE_ | OLS_SOURCE_CUSTOM_DRAW |
  // 		  OLS_SOURCE_CAP_OLSOLETE | OLS_SOURCE_SRGB;
  si.get_properties = get_properties;
  si.icon_type = OLS_ICON_TYPE_TEXT;

  si.get_name = [](void *) { return ols_module_text("TextFile"); };

  si.create = [](ols_data_t *settings, ols_source_t *source) {
    return (void *)new TextSource(source, settings);
  };
  
  si.destroy = [](void *data) { delete reinterpret_cast<TextSource *>(data); };

  si.request_new_pad = NULL;

  si.get_defaults = [](ols_data_t *settings) {
    // defaults(settings, 1);
    UNUSED_PARAMETER(settings);
  };

  si.activate = [](void *data) {
    reinterpret_cast<TextSource *>(data)->FileSrcStart();
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
