
#include "ols-meta-txt.h"
#include "ols-scripting.h"
#include <algorithm>
#include <ctype.h>
#include <locale>
#include <memory>
#include <ols-module.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <util/platform.h>
#include <util/str-util.h>
#include <util/task.h>
#include <util/time-parse.h>
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

struct ScriptCallerProcess {
  ols_process_t *process_ = nullptr;
  ols_script_t *script_ = nullptr;
  std::string script_path_;

  /* --------------------------- */

  inline ScriptCallerProcess(ols_process_t *process, ols_data_t *settings)
      : process_(process) {
	update(settings);
	
	if(!script_path_.empty()){
		script_ = ols_script_create(script_path_.c_str(),NULL);
	}

  }

  inline ~ScriptCallerProcess() {}

  ols_pad_t *requestNewPad(const char *name, const char *caps);

  void update(ols_data_t *settings);

  ols_pad_t *createRecvPad(const char *caps);
  ols_pad_t *createSendPad(const char *caps);

  void onDataBuff(ols_buffer_t *buffer);
};

/* ------------------------------------------------------------------------- */

/* clang-format off */
static OlsFlowReturn script_chainlist_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_list_t *buffer){

	blog(LOG_DEBUG, "script_chainlist_func");
	return OLS_FLOW_OK;
}

static OlsFlowReturn script_sink_chain_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_t *buffer){

	ScriptCallerProcess *script_caller = reinterpret_cast<ScriptCallerProcess *>(parent->data);
	script_caller->onDataBuff(buffer);

	//blog(LOG_DEBUG, "script_chain_func %s",ols_txt->data );
	return OLS_FLOW_OK;
}

static OlsPadLinkReturn script_sink_link_func(ols_pad_t *pad,ols_object_t *parent,ols_pad_t *peer){
	//blog(LOG_DEBUG, "script_link_func");
	return OLS_PAD_LINK_OK;
}

static OlsPadLinkReturn script_src_link_func(ols_pad_t *pad,ols_object_t *parent,ols_pad_t *peer){
	blog(LOG_DEBUG, "script_link_func");
	return OLS_PAD_LINK_OK;
}

ols_pad_t * ScriptCallerProcess::createRecvPad(const char *caps){

	ols_pad_t * sinkpad = ols_pad_new("script-process-sink",OLS_PAD_SINK);

	blog(LOG_DEBUG, "create recv pad success");

	ols_pad_set_link_function(sinkpad,script_sink_link_func);
	ols_pad_set_chain_function(sinkpad,script_sink_chain_func);

	ols_process_add_pad(process_, sinkpad);
	return sinkpad;
}

ols_pad_t * ScriptCallerProcess::createSendPad(const char *caps){
	ols_pad_t  * srcpad = ols_pad_new("script-process-src",OLS_PAD_SRC);

	ols_pad_set_link_function(srcpad,script_src_link_func);


	blog(LOG_DEBUG, "create send pad success");

	ols_process_add_pad(process_, srcpad);
	return srcpad;
}

 ols_pad_t *ScriptCallerProcess::requestNewPad(const char *name, const char *caps)
{

	if(strcmp("sink",name) == 0){
		return  createRecvPad(caps);
	} else if(strcmp("src",name) == 0) {
		return   createSendPad(caps);
	}

	return NULL;
}


void ScriptCallerProcess::onDataBuff(ols_buffer_t *buffer){

	ols_meta_txt_t * ols_txt = (ols_meta_txt_t *) buffer->meta;
	//printf("data is %s len is %d \n",(const char *)ols_txt->buff,ols_txt->len);
	if(str_strncmp((const char *)ols_txt->buff,"****",4) == 0){
		//printf("data is %s \n",(const char *)ols_txt->buff);
	} else {
		const char *p = (const char *)ols_txt->buff;
		
		size_t buff_len = ols_txt->len;
        
		size_t parse_len = 0;

		//05-22 11:17:45.265  3006 16061 V DSVFSALib:
		if(isdigit(p[0]) &&  isdigit(p[1]) && p[2] == '-'){

			if(script_ ){

				ols_meta_result result = ols_scripting_prase(script_,ols_txt);
		
				buffer->result = (ols_meta_result *)bzalloc(sizeof(ols_meta_result));
				*buffer->result = result;

				dstr_copy_dstr(&buffer->result->tag , &ols_txt->tag);
		
				for (int i = 0; i < process_->context.numsrcpads; ++i) {
					ols_pad_t *pad = process_->context.srcpads.array[i];
					ols_pad_push(pad, buffer);
				}
				//ols_txt->dat
			}
			//printf("tag is %s \n",tag);
		}
	}

}

void ScriptCallerProcess::update(ols_data_t *settings)
{
	if (ols_data_get_string(settings, "script_file_path") != NULL ) {
		script_path_ = ols_data_get_string(settings, "script_file_path");
		
		blog(LOG_INFO, "set script path  %s",script_path_.c_str());
	}

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


bool ols_module_load(void)
{
	ols_process_info si = {};
	si.id = "script_caller";
	si.type = OLS_PROCESS_TYPE_INPUT;
	// si.output_flags = OLS_SOURCE_ | OLS_SOURCE_CUSTOM_DRAW |
	// 		  OLS_SOURCE_CAP_OLSOLETE | OLS_SOURCE_SRGB;
	si.get_properties = get_properties;
	si.icon_type = OLS_ICON_TYPE_TEXT;

	si.get_name = [](void *) {
		return ols_module_text("ScriptCaller");
	};

	si.create = [](ols_data_t *settings, ols_process_t *process) {
		return (void *)new ScriptCallerProcess(process, settings);
	};

	si.destroy = [](void *data) {
		delete reinterpret_cast<ScriptCallerProcess *>(data);
	};

	si.request_new_pad = [](void *data, const char *name ,const char *caps) {
		return  reinterpret_cast<ScriptCallerProcess *>(data)->requestNewPad(name,caps) ;
	}; ; 

	si.get_defaults = [](ols_data_t *settings) {
		//defaults(settings, 1);
		UNUSED_PARAMETER(settings);
	};

	si.update = [](void *data, ols_data_t *settings) {
		reinterpret_cast<ScriptCallerProcess *>(data)->update(settings);
	};

	ols_register_process(&si);


	return true;
}

void ols_module_unload(void)
{

}
