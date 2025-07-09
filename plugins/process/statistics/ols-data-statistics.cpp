
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

struct TagStatistics {

  uint32_t lineCnt;
  uint32_t bytes;
};

struct DataStatistics {
  ols_process_t *process_ = nullptr;

  /* --------------------------- */

  inline DataStatistics(ols_process_t *process, ols_data_t *settings)
      : process_(process) {
    ols_process_update(process_, settings);
  }

  inline ~DataStatistics() {}

  ols_pad_t *requestNewPad(const char *name, const char *caps);

  void Update(ols_data_t *settings);

  ols_pad_t *createRecvPad(const char *caps);
  ols_pad_t *createSendPad(const char *caps);

  void onDataBuff(ols_buffer_t *buffer);
};

/* ------------------------------------------------------------------------- */

/* clang-format off */
static OlsFlowReturn statistics_chainlist_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_list_t *buffer){

	blog(LOG_DEBUG, "statistics_chainlist_func");
	return OLS_FLOW_OK;
}

static OlsFlowReturn statistics_sink_chain_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_t *buffer){

	//blog(LOG_DEBUG, "statistics_sink_chain_func %s",ols_txt->data );
	return OLS_FLOW_OK;
}

static OlsPadLinkReturn statistics_sink_link_func(ols_pad_t *pad,ols_object_t *parent,ols_pad_t *peer){
	//blog(LOG_DEBUG, "statistics_sink_link_func");
	return OLS_PAD_LINK_OK;
}

static OlsPadLinkReturn statistics_src_link_func(ols_pad_t *pad,ols_object_t *parent,ols_pad_t *peer){
	blog(LOG_DEBUG, "stattistics_src_link_func");
	return OLS_PAD_LINK_OK;
}


ols_pad_t * DataStatistics::createRecvPad(const char *caps){

	ols_pad_t * sinkpad = ols_pad_new("statistics-sink",OLS_PAD_SINK);

	blog(LOG_DEBUG, "create recv pad success");

	ols_pad_set_link_function(sinkpad,statistics_sink_link_func);
	ols_pad_set_chain_function(sinkpad,statistics_sink_chain_func);

	ols_process_add_pad(process_, sinkpad);
	return sinkpad;
}

ols_pad_t * DataStatistics::createSendPad(const char *caps){
	ols_pad_t  * srcpad = ols_pad_new("statistics-src",OLS_PAD_SRC);

	ols_pad_set_link_function(srcpad,statistics_src_link_func);


	blog(LOG_DEBUG, "create send pad success");

	ols_process_add_pad(process_, srcpad);
	return srcpad;
}

ols_pad_t *DataStatistics::requestNewPad(const char *name, const char *caps)
{
	if(strcmp("sink",name) == 0){
		return  createRecvPad(caps);
	} else if(strcmp("src",name) == 0) {
		return   createSendPad(caps);
	}
	return NULL;
}


void DataStatistics::onDataBuff(ols_buffer_t *buffer){

	ols_txt_file_t * ols_txt = (ols_txt_file_t *) buffer->meta;
	

}

void DataStatistics::Update(ols_data_t *settings)
{
	UNUSED_PARAMETER(settings);
}



#define ols_data_get_uint32 (uint32_t) ols_data_get_int


OLS_DECLARE_MODULE()
OLS_MODULE_USE_DEFAULT_LOCALE("ols-data-statistics", "en-US")
MODULE_EXPORT const char *ols_module_description(void)
{
	return "data statistics";
}

static ols_properties_t *get_properties(void *data)
{
	DataStatistics *s = reinterpret_cast<DataStatistics *>(data);
	string path;

	ols_properties_t *props = ols_properties_create();
	ols_property_t *p;

	return props;
}


bool ols_module_load(void)
{
	ols_process_info si = {};
	si.id = "data_statistics";
	si.type = OLS_PROCESS_TYPE_INPUT;
	// si.output_flags = OLS_SOURCE_ | OLS_SOURCE_CUSTOM_DRAW |
	// 		  OLS_SOURCE_CAP_OLSOLETE | OLS_SOURCE_SRGB;
	si.get_properties = get_properties;
	si.icon_type = OLS_ICON_TYPE_TEXT;

	si.get_name = [](void *) {
		return ols_module_text("Data Statistics");
	};

	si.create = [](ols_data_t *settings, ols_process_t *process) {
		return (void *)new DataStatistics(process, settings);
	};

	si.destroy = [](void *data) {
		delete reinterpret_cast<DataStatistics *>(data);
	};

	si.request_new_pad = [](void *data, const char *name ,const char *caps) {
		return  reinterpret_cast<DataStatistics *>(data)->requestNewPad(name,caps) ;
	}; ; 

	si.get_defaults = [](ols_data_t *settings) {
		//defaults(settings, 1);
		UNUSED_PARAMETER(settings);
	};

	si.update = [](void *data, ols_data_t *settings) {
		reinterpret_cast<DataStatistics *>(data)->Update(settings);
	};

	ols_register_process(&si);


	return true;
}

void ols_module_unload(void)
{

}
