
#include "ols-meta-txt.h"
#include <algorithm>
#include <locale>
#include <memory>
#include <ols-module.h>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <sys/stat.h>
#include <util/base.h>
#include <util/platform.h>
#include <util/task.h>
#include <util/util.hpp>
#include <util/pipe.h>
#include <util/str-util.h>
#include <string.h>

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


#define UNKNOW_FILE_EXT "unknow"

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

static const char *supported_ext[] = {".zip",".tar.gz",".tar.xz", ".tar.bz2",".tar",".gz"};

const char * get_matched_extension(const char *file_name){

  for(size_t i = 0; i < sizeof(supported_ext)/sizeof(supported_ext[0]); ++i){

    if(str_endwith(file_name,supported_ext[i])){
      return supported_ext[i] + 1;
    }
  }
  return UNKNOW_FILE_EXT;
}

//声明一个模板  
typedef std::function<void (uint8_t *, size_t )> ReadCallback; 

bool do_command(const char *command ,uint8_t *buff, size_t buff_len,ReadCallback callback){

  os_process_pipe_t * pipe = os_process_pipe_create(command,"r");

  if(pipe){
    size_t len = 0; 
    while( (len = os_process_pipe_read_err(pipe,buff,buff_len)) != 0 ){
      callback(buff,len);
    }
    os_process_pipe_destroy(pipe);
    return true;
  }

  return false;
}

void  decompress_file(const std::string &file){
  struct dstr command = {0};

  std::string extension = get_matched_extension(file.c_str());

  if(extension == "tar.gz"){
    dstr_printf(&command,"tar -zvxf %s --overwrite",file.c_str());
  } else if(extension == "gz") {
    std::string dest_file = file;
    dest_file.erase(dest_file.size() - (sizeof(".gz") - 1));
    dstr_printf(&command,"gunzip -c %s > %s",file.c_str(),dest_file.c_str());
  } else {
    return ;
  }
  
  bool format_errno = false ;
  uint8_t buffer[1024];
  do_command(command.array,buffer,1024,[extension,&format_errno](uint8_t *buff, size_t buff_len){
      //blog(LOG_INFO,"ext is %s read %s",extension.c_str(),buff);
      if(extension == "tar.gz"){
        if(strstr((const char *)buff,"not in gzip format") != nullptr){
          format_errno = true;
      }
    }
  });

  if(format_errno){
    size_t pos  = file.find_last_of('/');
    std::string dest_dir;
    if(pos != std::string::npos){
      dest_dir = file.substr(0,pos);
      dstr_printf(&command,"cd %s; tar -xvf %s --overwrite | xargs gunzip ",dest_dir.c_str(),file.c_str());
      do_command(command.array,buffer,1024,[](uint8_t *buff, size_t buff_len){
  
      });
    }
  }

  dstr_free(&command);
}

struct TextSource {
  ols_source_t *source_ = nullptr;

  bool read_from_file_ = false;
  
  string base_file_;
  string base_type_hint_;
  string inner_dir_;
  std::string file_wildcard_;
  std::queue<std::string> files_;
  string  curr_filename_;
  FILE   *curr_file_ = nullptr;
  uint64_t line_cnt = 0;


  /* --------------------------- */

  inline TextSource(ols_source_t *source, ols_data_t *settings)
      : source_(source) {
      Update(settings);
  }

  inline ~TextSource() {}

  int FileSrcGetData(ols_buffer_t *buf);

  void LoadFileText();
  bool FileSrcStart();
  /* unmap and close the file */
  bool FileSrcStop();

  bool IsSupportedCompressedFile(const char * file);

  void DecompressFile(const std::string &file, std::string &ext_hint,const  std::string &dest_dir);

  void LoadMatchFilesInDir(const std::string &dest_dir, PCRE2_SPTR8 match_pattern, std::set<std::string> &files);

  bool OpenNextValidFile();

  inline void Update(ols_data_t *settings);
};

static time_t get_modified_timestamp(const char *filename) {
  struct stat stats;
  if (os_stat(filename, &stats) != 0)
    return -1;
  return stats.st_mtime;
}



void TextSource::LoadFileText() {
  //BPtr<char> file_text = os_quick_read_utf8_file(curr_filename_.c_str());
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

  ols_txt = ols_meta_txt_new_with_buffer(1024);

  if(!curr_file_){
    goto eos;
  }

  while(true){
    size = os_fgetline(curr_file_, (char *)OLS_META_TXT_BUFF(ols_txt),OLS_META_TXT_BUFF_CAPACITY(ols_txt));
    if (UNLIKELY(size == -1)) {
      if(OpenNextValidFile() ){
        continue;
      }  else {
        curr_file_ = NULL;
        goto eos;
      }
    } else {
      break;
    }
  }

  ols_txt->line = line_cnt;
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

void TextSource::LoadMatchFilesInDir(const std::string &dest_dir,PCRE2_SPTR8 match_pattern,std::set<std::string> &files){
  os_dir_t *dir = os_opendir(dest_dir.c_str());

  if (dir) {
  
    struct os_dirent *ent;
  
    /* Compile the pattern. */
    int error_number;
    PCRE2_SIZE error_offset;
    pcre2_code *re = pcre2_compile(
        match_pattern,               /* the pattern */
        PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
        0,                     /* default options */
        &error_number,         /* for error number */
        &error_offset,         /* for error offset */
        NULL);                 /* use default compile context */
    if (re == NULL) {
      blog(LOG_ERROR, "Invalid pattern: %s\n", match_pattern);
      os_closedir(dir);
      return ;
    }
  
    /* Match the pattern against the subject text. */
    pcre2_match_data *match_data =
        pcre2_match_data_create_from_pattern(re, NULL);
    

    for (;;) {
      const char *ext;
  
      ent = os_readdir(dir);
      if (!ent)
        break;
      if (ent->directory)
        continue;
  
      int rc = pcre2_match(
          re,                   /* the compiled pattern */
          ( PCRE2_SPTR8)ent->d_name,              /* the subject text */
          strlen(ent->d_name),      /* the length of the subject */
          0,                    /* start at offset 0 in the subject */
          0,                    /* default options */
          match_data,           /* block for storing the result */
          NULL);                /* use default match context */
  
      /* Print the match result. */
      if (rc == PCRE2_ERROR_NOMATCH) {
        blog(LOG_DEBUG,"No match : %s\n",ent->d_name);
      } else if (rc < 0) {
        blog(LOG_ERROR, "Matching error\n");
      } else {
        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
        blog(LOG_DEBUG,"Found match: '%.*s'\n", (int)(ovector[1] - ovector[0]),ent->d_name + ovector[0]);

        std::string file_path = dest_dir;
        file_path.append("/").append(ent->d_name);
        files.insert(file_path);
      }
    }
    pcre2_match_data_free(match_data);   /* Free resources */
    pcre2_code_free(re);        
  
    os_closedir(dir);
  } 
}


void TextSource::DecompressFile(const std::string &file, std::string &ext_hint,const  std::string &dest_dir){
  struct dstr command = {0};

  if(ext_hint == "zip"){
    dstr_printf(&command,"unzip -o %s -d %s ",file.c_str(),dest_dir.c_str());
  } else  if(ext_hint == "tar.gz"){
    dstr_printf(&command,"tar -zxf %s -C %s --overwrite",file.c_str(),dest_dir.c_str());
  } else if(ext_hint == "tar.xz"){
    dstr_printf(&command,"tar -zJf %s -C %s --overwrite",file.c_str(),dest_dir.c_str());
  } else if(ext_hint == "tar.bz2"){
    dstr_printf(&command,"tar -zjf %s -C %s --overwrite",file.c_str(),dest_dir.c_str());
  } else if(ext_hint == "gz") {

    if(!dest_dir.empty()){
      os_mkdir(dest_dir.c_str());

      size_t pos = file.find_last_of('/');

      std::string dest_file = dest_dir;
      if(pos != std::string::npos){
        dest_file += file.substr(pos);
      } else {
        dest_file += file;
      }
      
      if(str_endwith(dest_file.c_str(),".gz")){
        dest_file.erase(dest_file.size() - (sizeof(".gz") - 1));
      }

      dstr_printf(&command,"gunzip -c %s > %s",file.c_str(),dest_file.c_str());

    } else {
      dstr_printf(&command,"gzip -d %s ",file.c_str());
    }
  }

  os_process_pipe_t * pipe = os_process_pipe_create(command.array,"r");

  if(pipe){
    uint8_t  buff[1024] = {'\0'};
    while(os_process_pipe_read(pipe,buff,1024)){
      blog(LOG_INFO,"%s \n",buff);
    }
    os_process_pipe_destroy(pipe);
  }

  dstr_free(&command);
}


bool TextSource::OpenNextValidFile(){

  struct stat stat_results;

  if(curr_file_){
    fclose(curr_file_);
    curr_file_ = NULL;
  }

  if(files_.empty()){
    goto no_filename;
  }

  while(!files_.empty()) {

    curr_filename_ = files_.front();
    files_.pop();
  
    if (curr_filename_.empty()){
      blog(LOG_INFO, "file name is empty");
      continue;
    }
  
    if (os_stat(curr_filename_.c_str(), &stat_results) < 0){
      blog(LOG_ERROR, ("Could not get info on \"%s\"."), curr_filename_.c_str());
      continue;
    }
  
    if (S_ISDIR(stat_results.st_mode)){
      blog(LOG_ERROR, "\"%s\" is a directory.", curr_filename_.c_str());
      continue;
    }
    /* open the file */
    curr_file_ = os_fopen(curr_filename_.c_str(), "rb");
    if (curr_file_ == NULL){
      switch (errno) {
        case ENOENT:
          blog(LOG_ERROR, "No such file \"%s\"", curr_filename_.c_str());
          break;
        default:
          blog(LOG_ERROR, ("Could not open file \"%s\" for reading."),
          curr_filename_.c_str());
          break;
      }      
      continue;
    }
    blog(LOG_INFO, "Open file \"%s\" for reading.",curr_filename_.c_str());
    return true;
  } 

  return false;

    /* ERROR */
  no_filename: 
    blog(LOG_ERROR, ("No file name specified for reading."));
    return false;

}

/* open the file, necessary to go to READY state */
bool TextSource::FileSrcStart() {

  struct stat stat_results;
  std::string  file_ext;
  std::string dest_dir;
  std::set<std::string> files;
  size_t pos;

  if(base_file_.empty()){
    goto no_filename;
  }
  
  if (!S_ISDIR(stat_results.st_mode)){

    if(base_type_hint_.empty()){
      file_ext = get_matched_extension(base_file_.c_str());
    } else {
      file_ext = base_type_hint_;
    }

    if(file_ext == UNKNOW_FILE_EXT ){ //treat as a regular file 
      blog(LOG_INFO,"Source file extension is NULL");
      //std::string file_path = base_file_;
      files_.push(base_file_);
    } else {
      dest_dir = base_file_;
      pos = dest_dir.find(file_ext);

      if(pos != std::string::npos){
        dest_dir =  dest_dir.substr(0,pos).append("ols");
      } else {
        dest_dir.append(".ols");
      }      

      DecompressFile(base_file_,file_ext,dest_dir);

      if(!inner_dir_.empty())
        dest_dir.append("/").append(inner_dir_);

      blog(LOG_INFO," dest is %s\n",dest_dir.c_str());

    }
  } else {
    dest_dir = base_file_;

    if(!inner_dir_.empty())
        dest_dir.append("/").append(inner_dir_);

  }

  LoadMatchFilesInDir(dest_dir,(PCRE2_SPTR8)file_wildcard_.c_str(),files);

  //decompress
  for(auto &file : files){
    decompress_file(file);
  }

  //reload after decompress
  files.clear();
  LoadMatchFilesInDir(dest_dir,(PCRE2_SPTR8)file_wildcard_.c_str(),files);

  for(auto &file : files){
    std::string ext = get_matched_extension(file.c_str());
    if(ext == UNKNOW_FILE_EXT){
      files_.push(file);
      blog(LOG_INFO, "get file %s", file.c_str());
    } 
  }

  if(files_.empty()){
    return  false;
  }

  return OpenNextValidFile();


  /* ERROR */
  no_filename: {
    blog(LOG_ERROR, ("No file name specified for reading."));
  }
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

bool TextSource::IsSupportedCompressedFile(const char * file){
 
  bool flag = false ;

  for(size_t i = 0; i < sizeof(supported_ext)/sizeof(supported_ext[0]); ++i){

    if(str_endwith(file,supported_ext[i])){
      flag = true;
      break;
    }
  }

  return flag;
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
