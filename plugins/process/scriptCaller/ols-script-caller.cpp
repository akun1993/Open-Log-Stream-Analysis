
#include <algorithm>
#include <locale>
#include <memory>
#include <ols-module.h>
#include <string>
#include <sys/stat.h>
#include <util/platform.h>
#include <util/task.h>
#include <util/util.hpp>

using namespace std;

#define warning(format, ...)                                                   \
  blog(LOG_WARNING, "[%s] " format, ols_process_get_name(source), ##__VA_ARGS__)

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
static OlsFlowReturn script_chainlist_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_list_t *buffer){

}

static OlsFlowReturn script_chain_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_t *buffer){

}

static OlsPadLinkReturn script_link_func(ols_pad_t *pad,ols_object_t *parent,ols_pad_t *peer){

}


struct ScriptCallerProcess {
	ols_process_t *process = nullptr;
	ols_pad_t     *srcpad  = nullptr;
	ols_pad_t     *sinkpad  = nullptr;

	/* --------------------------- */

	inline ScriptCallerProcess(ols_process_t *process_, ols_data_t *settings)
		: process(process_)
	{
		ols_process_update(process_, settings);
		srcpad = ols_pad_new("script-process-src",OLS_PAD_SRC);
		sinkpad = ols_pad_new("script-process-sink",OLS_PAD_SINK);


		ols_pad_set_link_function(sinkpad,script_link_func);
		ols_pad_set_chain_function(sinkpad,script_chain_func);
		//ols_pad_set_chain_list_function(sinkpad,script_chainlist_func);
	}

	inline ~ScriptCallerProcess(){

	}

	void Update(ols_data_t *settings);

};




void ScriptCallerProcess::Update(ols_data_t *settings)
{
	UNUSED_PARAMETER(settings);
}



#define ols_data_get_uint32 (uint32_t) ols_data_get_int


OLS_DECLARE_MODULE()
OLS_MODULE_USE_DEFAULT_LOCALE("ols-script-caller", "en-US")
MODULE_EXPORT const char *ols_module_description(void)
{
	return "script caller";
}

static ols_properties_t *get_properties(void *data)
{
	ScriptCallerProcess *s = reinterpret_cast<ScriptCallerProcess *>(data);
	string path;

	ols_properties_t *props = ols_properties_create();
	ols_property_t *p;


	return props;
}

static ols_pad_t *get_new_pad(void *data)
{
	ScriptCallerProcess *s = reinterpret_cast<ScriptCallerProcess *>(data);


	return s->srcpad;
}


bool ols_module_load(void)
{
	ols_process_info si = {};
	si.id = "script caller";
	si.type = OLS_PROCESS_TYPE_INPUT;
	// si.output_flags = OLS_SOURCE_ | OLS_SOURCE_CUSTOM_DRAW |
	// 		  OLS_SOURCE_CAP_OLSOLETE | OLS_SOURCE_SRGB;
	si.get_properties = get_properties;
	si.icon_type = OLS_ICON_TYPE_TEXT;

	si.get_name = [](void *) {
		return ols_module_text("TextFile");
	};

	si.create = [](ols_data_t *settings, ols_process_t *process) {
		return (void *)new ScriptCallerProcess(process, settings);
	};

	si.destroy = [](void *data) {
		delete reinterpret_cast<ScriptCallerProcess *>(data);
	};

	//si.get_new_pad = get_new_pad; 

	si.get_defaults = [](ols_data_t *settings) {
		//defaults(settings, 1);
		UNUSED_PARAMETER(settings);
	};

	si.update = [](void *data, ols_data_t *settings) {
		reinterpret_cast<ScriptCallerProcess *>(data)->Update(settings);
	};

	ols_register_process(&si);


	return true;
}

void ols_module_unload(void)
{

}
