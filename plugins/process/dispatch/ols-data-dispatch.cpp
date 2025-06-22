
#include "ols-meta-txt.h"
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

struct DataDispatch {
  ols_process_t *process_ = nullptr;

  // ols_pad_t     *srcpad_  = nullptr;
  // ols_pad_t     *sinkpad_  = nullptr;

  /* --------------------------- */

  inline DataDispatch(ols_process_t *process, ols_data_t *settings)
      : process_(process) {
    ols_process_update(process_, settings);
	

    // srcpad_ = ols_pad_new("script-process-src",OLS_PAD_SRC);
    // sinkpad_ = ols_pad_new("script-process-sink",OLS_PAD_SINK);

    // ols_pad_set_link_function(sinkpad_,script_link_func);
    // ols_pad_set_chain_function(sinkpad_,script_chain_func);

    // blog(LOG_DEBUG, "ols_context_add_pad");
    // ols_process_add_pad(process_, sinkpad_);

    // ols_pad_set_chain_list_function(sinkpad,script_chainlist_func);
  }

  inline ~DataDispatch() {}

  ols_pad_t *requestNewPad(const char *name, const char *caps);

  void Update(ols_data_t *settings);

  ols_pad_t *createRecvPad(const char *caps);
  ols_pad_t *createSendPad(const char *caps);

  void onDataBuff(ols_buffer_t *buffer);
};

/* ------------------------------------------------------------------------- */

/* clang-format off */
static OlsFlowReturn dispatch_chainlist_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_list_t *buffer){

	

	blog(LOG_DEBUG, "dispatch_chainlist_func");
	return OLS_FLOW_OK;
}

static OlsFlowReturn dispatch_sink_chain_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_t *buffer){


	

	//blog(LOG_DEBUG, "dispatch_sink_chain_func %s",ols_txt->data );
	return OLS_FLOW_OK;
}

static OlsPadLinkReturn dispatch_sink_link_func(ols_pad_t *pad,ols_object_t *parent,ols_pad_t *peer){
	//blog(LOG_DEBUG, "dispatch_sink_link_func");
	return OLS_PAD_LINK_OK;
}

static OlsPadLinkReturn dispatch_src_link_func(ols_pad_t *pad,ols_object_t *parent,ols_pad_t *peer){
	blog(LOG_DEBUG, "dispatch_src_link_func");
	return OLS_PAD_LINK_OK;
}





ols_pad_t * DataDispatch::createRecvPad(const char *caps){

	ols_pad_t * sinkpad = ols_pad_new("dispatch-sink",OLS_PAD_SINK);

	blog(LOG_DEBUG, "create recv pad success");

	ols_pad_set_link_function(sinkpad,dispatch_sink_link_func);
	ols_pad_set_chain_function(sinkpad,dispatch_sink_chain_func);

	ols_process_add_pad(process_, sinkpad);
	return sinkpad;
}

ols_pad_t * DataDispatch::createSendPad(const char *caps){
	ols_pad_t  * srcpad = ols_pad_new("dispatch-src",OLS_PAD_SRC);

	ols_pad_set_link_function(srcpad,dispatch_src_link_func);


	blog(LOG_DEBUG, "create send pad success");

	ols_process_add_pad(process_, srcpad);
	return srcpad;
}

 ols_pad_t *DataDispatch::requestNewPad(const char *name, const char *caps)
{

	if(strcmp("sink",name) == 0){
		return  createRecvPad(caps);
	} else if(strcmp("src",name) == 0) {
		return   createSendPad(caps);
	}

	return NULL;
}


void DataDispatch::onDataBuff(ols_buffer_t *buffer){

	

}

void DataDispatch::Update(ols_data_t *settings)
{
	UNUSED_PARAMETER(settings);
}



#define ols_data_get_uint32 (uint32_t) ols_data_get_int


OLS_DECLARE_MODULE()
OLS_MODULE_USE_DEFAULT_LOCALE("ols-data-dispatch", "en-US")
MODULE_EXPORT const char *ols_module_description(void)
{
	return "Data dispatch";
}

static ols_properties_t *get_properties(void *data)
{
	DataDispatch *s = reinterpret_cast<DataDispatch *>(data);
	string path;

	ols_properties_t *props = ols_properties_create();
	ols_property_t *p;

	return props;
}

  // ols_script_t *script = ols_script_create("/home/zkun/OpenSource/Open-Log-Stream-Analysis/build/rundir/RelWithDebInfo/lib/ols-scripting/parse_log_2.py",NULL);

  // ols_scripting_prase(script,"parse this log in python",sizeof("parse this log in python") - 1);


bool ols_module_load(void)
{
	ols_process_info si = {};
	si.id = "data_dispatch";
	si.type = OLS_PROCESS_TYPE_INPUT;
	// si.output_flags = OLS_SOURCE_ | OLS_SOURCE_CUSTOM_DRAW |
	// 		  OLS_SOURCE_CAP_OLSOLETE | OLS_SOURCE_SRGB;
	si.get_properties = get_properties;
	si.icon_type = OLS_ICON_TYPE_TEXT;

	si.get_name = [](void *) {
		return ols_module_text("DataDispatch");
	};

	si.create = [](ols_data_t *settings, ols_process_t *process) {
		return (void *)new DataDispatch(process, settings);
	};

	si.destroy = [](void *data) {
		delete reinterpret_cast<DataDispatch *>(data);
	};

	si.request_new_pad = [](void *data, const char *name ,const char *caps) {
		return  reinterpret_cast<DataDispatch *>(data)->requestNewPad(name,caps) ;
	}; ; 

	si.get_defaults = [](ols_data_t *settings) {
		//defaults(settings, 1);
		UNUSED_PARAMETER(settings);
	};

	si.update = [](void *data, ols_data_t *settings) {
		reinterpret_cast<DataDispatch *>(data)->Update(settings);
	};

	ols_register_process(&si);


	return true;
}

void ols_module_unload(void)
{

}
