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

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>


#ifdef _WIN32
    #include <windows.h>
    #define PATH_SEPARATOR "\\"
    #define SEVENZIP_CMD "7z"
#else
    #include <unistd.h>
    #define PATH_SEPARATOR "/"
#endif

#define MAX_PATH_LENGTH 1024
#define MAX_CMD_LENGTH 2048


/**
 * Archive format types
 */
typedef enum {
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
} ArchiveFormat;


#ifndef S_ISREG
#define S_ISREG(mode) ((mode)&_S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode) ((mode)&_S_IFDIR)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(x) (0)
#endif

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


/**
 * Get filename (without path)
 */
const char* get_filename(const char *path) {
    const char *filename = strrchr(path, '/');
    #ifdef _WIN32
    const char *filename_win = strrchr(path, '\\');
    if (filename_win > filename) filename = filename_win;
    #endif
    return filename ? filename + 1 : path;
}


/**
 * Read file header, automatically identify file format (ignoring extension)
 */
ArchiveFormat check_real_filetype(const char* filePath) {
    FILE* fp = os_fopen(filePath, "rb");
    if (!fp) return FORMAT_UNKNOWN;

    char buf[300] = {0};
    os_fgets(fp, buf, sizeof(buf) );
    fclose(fp);

    // ZIP: 50 4B 03 04
    if (buf[0] == 0x50 && buf[1] == 0x4B && buf[2] == 0x03 && buf[3] == 0x04) return FORMAT_ZIP;
    // RAR: 52 61 72 21
    if (buf[0] == 0x52 && buf[1] == 0x61 && buf[2] == 0x72 && buf[3] == 0x21) return FORMAT_RAR;

    // 7Z: 37 7A BC AF 27 1C
    if (buf[0] == 0x37 && buf[1] == 0x7A && buf[2] == 0xBC && buf[3] == 0xAF) return FORMAT_7Z;
    // GZIP: 1F 8B
    if (buf[0] == 0x1F && buf[1] == 0x8B) return FORMAT_GZ;
    // BZIP2: 42 5A 68
    if (buf[0] == 0x42 && buf[1] == 0x5A && buf[2] == 0x68) return FORMAT_BZ2;
    // XZ: FD 37 7A 58 5A
    if (buf[0] == 0xFD && buf[1] == 0x37 && buf[2] == 0x7A && buf[3] == 0x58) return FORMAT_XZ;
    // TAR: ustar
    if (buf[257] == 'u' && buf[258] == 's' && buf[259] == 't' && buf[260] == 'a' && buf[261] == 'r') return FORMAT_TAR;


    return FORMAT_UNKNOWN;
}


/**
 * @brief Remove compression suffix while keeping the full directory path
 * @param full_path Original full path with filename and extension
 * @return New string: full path WITHOUT compression suffix
 * @note Supports .tar.gz, .tar.bz2, .tar.xz, .zip, .rar, .7z
 */
std::string remove_compress_suffix(const std::string& full_path)
{
    size_t len = full_path.size();

    // Match and remove double suffix: .tar.gz
    if (len >= 7 && full_path.substr(len - 7) == ".tar.gz") {
        return full_path.substr(0, len - 7);
    }

    // Match and remove double suffix: .tar.bz2
    if (len >= 8 && full_path.substr(len - 8) == ".tar.bz2") {
        return full_path.substr(0, len - 8);
    }

    // Match and remove double suffix: .tar.xz
    if (len >= 7 && full_path.substr(len - 7) == ".tar.xz") {
        return full_path.substr(0, len - 7);
    }

    // Remove normal single suffix (.zip, .rar, .7z, etc.)
    size_t last_dot = full_path.find_last_of('.');
    size_t last_sep = full_path.find_last_of("/\\");

    // Ensure dot is in filename, not in directory path
    if (last_dot != std::string::npos && (last_sep == std::string::npos || last_dot > last_sep)) {
        return full_path.substr(0, last_dot);
    }

    // No valid extension found, return original path
    return full_path;
}



/**
 * Detect compression format (enhanced version)
 */
ArchiveFormat detect_format(const char *filename) {
    const char *lower = filename;
    char lower_buf[MAX_PATH_LENGTH];
    
    // Convert to lowercase
    strncpy(lower_buf, filename, sizeof(lower_buf) - 1);
    lower_buf[sizeof(lower_buf) - 1] = '\0';
    for (char *p = lower_buf; *p; p++) {
        *p = tolower((unsigned char)*p);
    }
    lower = lower_buf;
    
    // Detect composite formats (high priority)
    if (strstr(lower, ".tar.gz") != NULL || 
        strstr(lower, ".tgz") != NULL) {
        return FORMAT_TAR_GZ;
    }
    if (strstr(lower, ".tar.xz") != NULL || 
        strstr(lower, ".txz") != NULL) {
        return FORMAT_TAR_XZ;
    }
    if (strstr(lower, ".tar.bz2") != NULL || 
        strstr(lower, ".tbz2") != NULL ||
        strstr(lower, ".tar.bz") != NULL) {
        return FORMAT_TAR_BZ2;
    }

    // Detect single format
    const char *ext = strrchr(lower, '.');
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

/**
 * Get format name
 */
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


/**
 * Get output directory name (based on archive name)
 */
void get_default_output_dir(const char *archive, char *output_dir, size_t size) {
    const char *filename = get_filename(archive);
    char base_name[MAX_PATH_LENGTH];
    
    strncpy(base_name, filename, sizeof(base_name) - 1);
    base_name[sizeof(base_name) - 1] = '\0';
     
    // Remove composite extension
    char *ext;
    if ((ext = strstr(base_name, ".tar.gz")) != NULL) *ext = '\0';
    else if ((ext = strstr(base_name, ".tar.xz")) != NULL) *ext = '\0';
    else if ((ext = strstr(base_name, ".tar.bz2")) != NULL) *ext = '\0';
    else if ((ext = strstr(base_name, ".tgz")) != NULL) *ext = '\0';
    else if ((ext = strstr(base_name, ".txz")) != NULL) *ext = '\0';
    else if ((ext = strstr(base_name, ".tbz2")) != NULL) *ext = '\0';
    else if ((ext = strrchr(base_name, '.')) != NULL) *ext = '\0';
    
    snprintf(output_dir, size, "%s", base_name);
}


// 递归遍历目录，查找以 prefix 开头的文件，返回它们所在的目录（去重）
void find_dirs_by_file_prefix( const std::string& rootDir,const std::string& filePrefix,std::set<std::string>& resultDirs) {

    os_dir_t* dir = os_opendir(rootDir.c_str());
    if (!dir) return;

    struct os_dirent* entry;

    while ((entry = os_readdir(dir)) != nullptr) {
        // 跳过 . 和 ..
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;

        // 拼接完整路径
        std::string fullPath = rootDir + PATH_SEPARATOR + entry->d_name;

        // 如果是目录，递归
        if (entry->directory) {
            find_dirs_by_file_prefix(fullPath, filePrefix, resultDirs);
        } else  {
            std::string fileName = entry->d_name;
            if (fileName.compare(0, filePrefix.length(), filePrefix) == 0) {
                resultDirs.insert(rootDir); // 只存目录，自动去重
            }
        }
    }

    os_closedir(dir);
}

// 对外接口：返回 vector<string>
std::vector<std::string> getDirsWithPrefixFile( const std::string& rootDir, const std::string& filePrefix) {
    std::set<std::string> dirSet;
    find_dirs_by_file_prefix(rootDir, filePrefix, dirSet);
    return std::vector<std::string>(dirSet.begin(), dirSet.end());
}



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



std::string get_file_dir(const std::string &file,std::string default_dir){

  size_t pos  = file.find_last_of(FILE_SEPARATOR);

  std::string dest_dir;
  
  if(pos != std::string::npos){
    dest_dir = file.substr(0,pos);
  } else {
    dest_dir = default_dir;
  }

  return dest_dir;
}



// PCRE2 regex match (generic)
int regexMatch(const char *pattern, const char *str) {
    int errorCode;
    PCRE2_SIZE errorOffset;

    pcre2_code *re = pcre2_compile(
        (PCRE2_SPTR)pattern,
        PCRE2_ZERO_TERMINATED,
        0,
        &errorCode,
        &errorOffset,
        NULL
    );

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
            if (regexMatch(regex, ent->d_name))
              matches.push_back(std::string(full));
        }
    }
    os_closedir(dir);
    return matches;
}

/**
 * Execute 7z decompression command
 */
int extract_with_7z(const char* file, const char* outDir) {

    struct dstr command = {0};
    dstr_printf(&command, "\"%s\" x \"%s\" -y -o\"%s\"", SEVENZIP_CMD, file, outDir);
  
    blog(LOG_INFO,"[%s]do decompress command %s", __FUNCTION__, command.array);

    os_process_pipe_t * pipe = os_process_pipe_create(command.array,"r");

    if(pipe){
      uint8_t  buff[1024] = {'\0'};
      while(os_process_pipe_read(pipe,buff,1024)){
        blog(LOG_INFO,"%s \n",buff);
      }
      os_process_pipe_destroy(pipe);
    }
    dstr_free(&command);

    return 0 ;
}

/**
 * Two-step decompression: archive → tar → final file (gz/bz2/xz)
 */
int extract_compressed_tar(const char* file, const char* outDir) {

   
    char  tar_path[1024] = {'\0'};

    auto system_command = [](const char* file, const char* outDir){

      struct dstr command = {0};

      dstr_printf(&command, "\"%s\" x \"%s\" -y -o\"%s\"", SEVENZIP_CMD, file, outDir);

      blog(LOG_INFO,"[%s]do decompress command %s", __FUNCTION__, command.array);

      os_process_pipe_t * pipe = os_process_pipe_create(command.array,"r");

      if(pipe){
        uint8_t  buff[1024] = {'\0'};
        while(os_process_pipe_read(pipe,buff,1024)){
          blog(LOG_INFO,"%s \n",buff);
        }
        os_process_pipe_destroy(pipe);
      }
      dstr_free(&command);
    };
    
    // Step 1: Extract tar
  
    system_command(file,outDir);

    // Generate tar path (remove .gz suffix, use original filename if not found)
    const char *filename = get_filename(file);
    const char* dot = strrchr(filename, '.');

    if (dot && (strcmp(dot, ".gz") == 0 || strcmp(dot, ".bz2") == 0 || strcmp(dot, ".xz") == 0)) {
      char *prefix = (char *)malloc(dot - filename + 1);
      strncpy(prefix, filename, dot - filename);
      prefix[dot - filename] = '\0';
      snprintf(tar_path, sizeof(tar_path), "%s\\%s", outDir, prefix);
      free(prefix);
    } else {
      snprintf(tar_path, sizeof(tar_path), "%s\\%s", outDir, file);
    }

    // 生成中间 tar 路径  
    //snprintf(tar_path, sizeof(tar_path), "%s\\%s", outDir, file);

    // 第二步：解压 tar
    system_command(tar_path,outDir);

    // 删除中间文件
    os_unlink(tar_path);
    return 0;
}


// template
typedef std::function<void (uint8_t *, size_t )> ReadCallback; 

bool do_command(const char *command ,uint8_t *buff, size_t buff_len,ReadCallback callback){

  os_process_pipe_t * pipe = os_process_pipe_create(command,"r");
  blog(LOG_INFO,"[%s]do  command %s", __FUNCTION__, command);


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


void  decompress_log_file(const std::string &file){

  struct dstr command = {0};

  ArchiveFormat extension = detect_format(file.c_str());

# ifdef _WIN32

  std::string dest_dir = get_file_dir(file,".");
  if(extension == ArchiveFormat::FORMAT_GZ || extension == ArchiveFormat::FORMAT_TAR_GZ){
    dstr_printf(&command,"7z x  %s -aoa -o%s",file.c_str(),dest_dir.c_str());
  } else {
    return ;
  }
  
  bool format_errno = false ;
  uint8_t buffer[1024];
  do_command(command.array,buffer,1024,[extension,&format_errno](uint8_t *buff, size_t buff_len){
      blog(LOG_INFO,"ext is %s read %s",format_name(extension),buff);
      if(extension == ArchiveFormat::FORMAT_TAR_GZ){
        if(strstr((const char *)buff,"Cannot open the file as [gzip]") != nullptr){
          format_errno = true;
      }
    }
  });

  if(extension == ArchiveFormat::FORMAT_TAR_GZ){


    size_t len = file.size();

    // Match and remove double suffix: .tar.gz
    if (len >= 7 && file.substr(len - 7) == ".tar.gz") {
        std::string file_prefix =  file.substr(0, len - 7);
        std::string new_file =  file_prefix + ".tar";

        if(!os_file_exists(new_file.c_str())) {
          new_file = file_prefix + ".gz";
        } 

        dstr_printf(&command,"7z x %s -aoa -o%s ",new_file.c_str(),dest_dir.c_str());
        do_command(command.array,buffer,1024,[](uint8_t *buff, size_t buff_len){
          
        });
    }

 
  }

# else

  if(extension == ArchiveFormat::FORMAT_TAR_GZ ){

    std::string dest_dir = get_file_dir(file,".");

    dstr_printf(&command,"tar -zvxf %s -C %s --overwrite",file.c_str(),dest_dir.c_str());
  } else if(extension == ArchiveFormat::FORMAT_GZ) {
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
      if(extension == ArchiveFormat::FORMAT_TAR_GZ){
        if(strstr((const char *)buff,"not in gzip format") != nullptr){
          format_errno = true;
      }
    }
  });

  if(format_errno){
    size_t pos  = file.find_last_of(FILE_SEPARATOR);
    std::string dest_dir;
    if(pos != std::string::npos){
      dest_dir = file.substr(0,pos);
      dstr_printf(&command,"cd %s; tar -xvf %s --overwrite | xargs gunzip ",dest_dir.c_str(),file.c_str());
      do_command(command.array,buffer,1024,[](uint8_t *buff, size_t buff_len){
  
      });
    }
  }
#endif
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
        update(settings);
  }

  inline ~TextSource() {}

  int fileSrcGetData(ols_buffer_t *buf);

  void loadFileText();
  bool fileSrcStart();
  /* unmap and close the file */
  bool fileSrcStop();

  bool isSupportedCompressedFile(const char * file);

  void decompressFile(const std::string &file, ArchiveFormat format,const  std::string &dest_dir);

  void loadMatchFilesInDir(const std::string &dest_dir, PCRE2_SPTR8 match_pattern, std::set<std::string> &files);

  bool openNextValidFile();

  inline void update(ols_data_t *settings);
};

static time_t get_modified_timestamp(const char *filename) {
  struct stat stats;
  if (os_stat(filename, &stats) != 0)
    return -1;
  return stats.st_mtime;
}



void TextSource::loadFileText() {
  //BPtr<char> file_text = os_quick_read_utf8_file(curr_filename_.c_str());
  // text = to_wide(GetMainString(file_text));

  // if (!text.empty() && text.back() != '\n')
  //   text.push_back('\n');
}

void TextSource::update(ols_data_t *settings) { 
 
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

int TextSource::fileSrcGetData(ols_buffer_t *buf) {

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
      if(openNextValidFile() ){
        continue;
      }  else {
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

void TextSource::loadMatchFilesInDir(const std::string &dest_dir,PCRE2_SPTR8 match_pattern,std::set<std::string> &files){
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
      //const char *ext;
  
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
        //blog(LOG_DEBUG,"Found match: '%.*s'\n", (int)(ovector[1] - ovector[0]),ent->d_name + ovector[0]);

        std::string file_path = dest_dir;
        file_path.append(1,FILE_SEPARATOR).append(ent->d_name);
        files.insert(file_path);
      }
    }
    pcre2_match_data_free(match_data);   /* Free resources */
    pcre2_code_free(re);        
  
    os_closedir(dir);
  } 
}

void TextSource::decompressFile(const std::string &file,ArchiveFormat format,const  std::string &dest_dir){

# ifdef _WIN32

    int res;
    if (format == ArchiveFormat::FORMAT_TAR_GZ || format == ArchiveFormat::FORMAT_TAR_XZ || format == ArchiveFormat::FORMAT_TAR_BZ2 ) {
        res = extract_compressed_tar(file.c_str(), dest_dir.c_str());
    } else {
        res = extract_with_7z(file.c_str(), dest_dir.c_str());
    }

    blog(LOG_INFO,"do decompress result  %d",res);


# else

  struct dstr command = {0};

  if(format == ArchiveFormat::FORMAT_ZIP){
    dstr_printf(&command,"unzip -o %s -d %s ",file.c_str(),dest_dir.c_str());
  } else  if(format == ArchiveFormat::FORMAT_TAR_GZ){
    dstr_printf(&command,"tar -zxf %s -C %s --overwrite",file.c_str(),dest_dir.c_str());
  } else if(format == ArchiveFormat::FORMAT_TAR_XZ){
    dstr_printf(&command,"tar -zJf %s -C %s --overwrite",file.c_str(),dest_dir.c_str());
  } else if(format == ArchiveFormat::FORMAT_TAR_BZ2){
    dstr_printf(&command,"tar -zjf %s -C %s --overwrite",file.c_str(),dest_dir.c_str());
  } else if(format == ArchiveFormat::FORMAT_GZ) {

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

  blog(LOG_INFO,"do decompress command %s",command.array);

  os_process_pipe_t * pipe = os_process_pipe_create(command.array,"r");

  if(pipe){
    uint8_t  buff[1024] = {'\0'};
    while(os_process_pipe_read(pipe,buff,1024)){
      blog(LOG_INFO,"%s \n",buff);
    }
    os_process_pipe_destroy(pipe);
  }

  dstr_free(&command);
#endif

}


bool TextSource::openNextValidFile(){

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
bool TextSource::fileSrcStart() {

  //blog(LOG_INFO,"%s",__FUNCTION__);

  struct stat stat_results;
  ArchiveFormat file_format;
  std::string dest_dir;
  std::set<std::string> files;

  if(base_file_.empty()){
    goto no_filename;
  }

  if (os_stat(base_file_.c_str(), &stat_results) < 0){
      goto no_stat;
  }

  if (!S_ISDIR(stat_results.st_mode)){

    file_format = detect_format(base_file_.c_str());

    if(file_format == ArchiveFormat::FORMAT_UNKNOWN ){ //treat as a regular file 
      blog(LOG_INFO,"Source file extension is NULL");
      //std::string file_path = base_file_;
      files_.push(base_file_);
    } else {
      dest_dir = remove_compress_suffix(base_file_); ;

      dest_dir.append(".ols");

      decompressFile(base_file_,file_format,dest_dir);

      if(!inner_dir_.empty())
        dest_dir.append(1,FILE_SEPARATOR).append(inner_dir_);

      blog(LOG_INFO," dest is %s\n",dest_dir.c_str());

    }
  } else {
    dest_dir = base_file_;

    if(!inner_dir_.empty())
        dest_dir.append(1,FILE_SEPARATOR).append(inner_dir_);

  }

  loadMatchFilesInDir(dest_dir,(PCRE2_SPTR8)file_wildcard_.c_str(),files);

  //decompress
  for(auto &file : files){
    decompress_log_file(file);
  }

  //reload after decompress
  files.clear();
  loadMatchFilesInDir(dest_dir,(PCRE2_SPTR8)file_wildcard_.c_str(),files);

  for(auto &file : files){
    ArchiveFormat ext = detect_format(file.c_str());
    if(ext == ArchiveFormat::FORMAT_UNKNOWN){
      files_.push(file);
      blog(LOG_INFO, "get file %s", file.c_str());
    } 
  }

  if(files_.empty()){
    return  false;
  }

  return openNextValidFile();

  /* ERROR */
  no_filename: {
    blog(LOG_ERROR, ("No file name specified for reading."));
  }
  return false;  

  no_stat:{
    blog(LOG_ERROR, ("Could not get info on \"%s\"."), base_file_.c_str());
  }
  return false;  
}

/* unmap and close the file */
bool TextSource::fileSrcStop() {
  /* close the file */
  if(curr_file_)
    fclose(curr_file_);
  /* zero out a lot of our state */
  curr_file_ = NULL;
  return true;
}

bool TextSource::isSupportedCompressedFile(const char * file){
 
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

  UNUSED_PARAMETER(data);

  ols_properties_t *props = ols_properties_create();

	ols_properties_add_color_alpha(props, "color", ols_module_text("ColorSource.Color"));

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
