
#include <util/platform.h>
#include <util/util.hpp>
#include <util/task.h>
#include <ols-module.h>
#include <sys/stat.h>
#include <algorithm>
#include <string>
#include <memory>
#include <locale>

using namespace std;

#define warning(format, ...)                                           \
	blog(LOG_WARNING, "[%s] " format, ols_source_get_name(source), \
	     ##__VA_ARGS__)

#define warn_stat(call)                                                   \
	do {                                                              \
		if (stat != Ok)                                           \
			warning("%s: %s failed (%d)", __FUNCTION__, call, \
				(int)stat);                               \
	} while (false)

#ifndef clamp
#define clamp(val, min_val, max_val) \
	if (val < min_val)           \
		val = min_val;       \
	else if (val > max_val)      \
		val = max_val;
#endif



/* ------------------------------------------------------------------------- */

/* clang-format off */



/* clang-format on */

/* ------------------------------------------------------------------------- */

static inline wstring to_wide(const char *utf8)
{
	wstring text;

	size_t len = os_utf8_to_wcs(utf8, 0, nullptr, 0);
	text.resize(len);
	if (len)
		os_utf8_to_wcs(utf8, 0, &text[0], len + 1);

	return text;
}


struct TextSource {
	ols_source_t *source = nullptr;
	ols_pad_t     *srcpad  = nullptr;

	bool read_from_file = false;
	string file;
	time_t file_timestamp = 0;

	wstring text;


	/* --------------------------- */

	inline TextSource(ols_source_t *source_, ols_data_t *settings)
		: source(source_)
	{
		ols_source_update(source, settings);
		srcpad = ols_pad_new("text-src",OLS_PAD_SRC);
	}

	inline ~TextSource(){

	}

	int  GetData(ols_buffer_t *buf);


	void LoadFileText();


	inline void Update(ols_data_t *settings);


};

static time_t get_modified_timestamp(const char *filename)
{
	struct stat stats;
	if (os_stat(filename, &stats) != 0)
		return -1;
	return stats.st_mtime;
}


static void ols_base_src_loop(void *param){

	ols_pad_t *pad = (ols_pad_t *)(param);

	ret = ols_pad_push (pad, buf);

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

void TextSource::LoadFileText()
{
	BPtr<char> file_text = os_quick_read_utf8_file(file.c_str());
	//text = to_wide(GetMainString(file_text));

	if (!text.empty() && text.back() != '\n')
		text.push_back('\n');
}


void TextSource::Update(ols_data_t *settings)
{
	UNUSED_PARAMETER(settings);
}

int TextSource::GetData(ols_buffer_t *buf){

	//ols_pad_start_task (srcpad, (os_task_t ) ols_base_src_loop,srcpad);
	return 0;
}


#define obs_data_get_uint32 (uint32_t) obs_data_get_int


OLS_DECLARE_MODULE()
OLS_MODULE_USE_DEFAULT_LOCALE("ols-text", "en-US")
MODULE_EXPORT const char *ols_module_description(void)
{
	return "text source";
}

static ols_properties_t *get_properties(void *data)
{
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

	if (s && !s->file.empty()) {
		const char *slash;

		path = s->file;
		replace(path.begin(), path.end(), '\\', '/');
		slash = strrchr(path.c_str(), '/');
		if (slash)
			path.resize(slash - path.c_str() + 1);
	}



	return props;
}

static ols_pad_t *get_new_pad(void *data)
{
	TextSource *s = reinterpret_cast<TextSource *>(data);


	return s->srcpad;
}


// static void missing_file_callback(void *src, const char *new_path, void *data)
// {
// 	TextSource *s = reinterpret_cast<TextSource *>(src);

// 	ols_source_t *source = s->source;
// 	ols_data_t *settings = ols_source_get_settings(source);
// 	ols_data_set_string(settings, S_FILE, new_path);
// 	ols_source_update(source, settings);
// 	ols_data_release(settings);

// 	UNUSED_PARAMETER(data);
// }

bool obs_module_load(void)
{
	ols_source_info si = {};
	si.id = "text_file";
	si.type = OLS_SOURCE_TYPE_INPUT;
	// si.output_flags = OLS_SOURCE_ | OBS_SOURCE_CUSTOM_DRAW |
	// 		  OBS_SOURCE_CAP_OBSOLETE | OBS_SOURCE_SRGB;
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

	si.get_new_pad = get_new_pad; 

	si.get_defaults = [](ols_data_t *settings) {
		//defaults(settings, 1);
		UNUSED_PARAMETER(settings);
	};

	si.update = [](void *data, ols_data_t *settings) {
		reinterpret_cast<TextSource *>(data)->Update(settings);
	};


	si.get_data = [](void *data,ols_buffer_t *buf) {
		return reinterpret_cast<TextSource *>(data)->GetData( buf);
	};


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

void obs_module_unload(void)
{
	//GdiplusShutdown(gdip_token);
}
