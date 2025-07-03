
#include "tinyxml2.h"
#include <algorithm>
#include <locale>
#include <memory>
#include <ols-module.h>
#include <string>
#include <sys/stat.h>
#include <util/platform.h>
#include <util/task.h>
#include <util/util.hpp>
#include "ols-meta-txt.h"

using namespace std;

#define warning(format, ...)                                                   \
  blog(LOG_WARNING, "[%s] " format, ols_output_get_name(source), ##__VA_ARGS__)


  struct XmlOutput  {
	ols_output_t *output_ = nullptr;
	tinyxml2::XMLDocument xmldoc_;
	tinyxml2::XMLElement* root_ = nullptr;

	/* --------------------------- */

	inline XmlOutput(ols_output_t *process, ols_data_t *settings)
		: output_(process)
	{

	}

	inline ~XmlOutput(){

	}

	ols_pad_t *requestNewPad(const char *name, const char *caps);

	void Update(ols_data_t *settings);

	ols_pad_t * createRecvPad(const char *caps);

	void onDataBuff(ols_buffer_t *buffer);

	void initOutfile();
};

/* ------------------------------------------------------------------------- */

/* clang-format off */
static OlsFlowReturn output_chainlist_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_list_t *buffer){
	blog(LOG_DEBUG, "output_chainlist_func");
	return OLS_FLOW_OK;
}

static OlsFlowReturn output_sink_chain_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_t *buffer){
	//blog(LOG_DEBUG, "output_sink_chain_func");

	XmlOutput *xml_output = reinterpret_cast<XmlOutput *>(parent->data);
	xml_output->onDataBuff(buffer);

	return OLS_FLOW_OK;
}

static OlsPadLinkReturn output_sink_link_func(ols_pad_t *pad,ols_object_t *parent,ols_pad_t *peer){
	//blog(LOG_DEBUG, "output_sink_link_func");
	return OLS_PAD_LINK_OK;
}


ols_pad_t * XmlOutput::createRecvPad(const char *caps){

	ols_pad_t * sinkpad = ols_pad_new("xml-output-sink",OLS_PAD_SINK);

	blog(LOG_DEBUG, "create recv pad success");

	ols_pad_set_link_function(sinkpad,output_sink_link_func );
	ols_pad_set_chain_function(sinkpad,output_sink_chain_func);

	ols_output_add_pad(output_, sinkpad);
	return sinkpad;
}

void XmlOutput::onDataBuff(ols_buffer_t *buffer){

	ols_txt_file_t * meta_txt = (ols_txt_file_t *) buffer->meta;

	ols_meta_result *meta_result = buffer->result;

	//printf("tag is %s line = %d \n",meta_result->tag.array, meta_txt->line);
	for(size_t i = 0; i < meta_result->info.num; ++i){
		//printf("data is %s \n",(const char *)meta_result->info.array[i]);
	}
	
}

 ols_pad_t *XmlOutput::requestNewPad(const char *name, const char *caps)
{

	if(strcmp("sink",name) == 0){
		return  createRecvPad(caps);
	}

	return NULL;
}


void XmlOutput::initOutfile(){

	tinyxml2::XMLDeclaration* decl = xmldoc_.NewDeclaration("xml version='1.0' encoding='UTF-8' standalone='yes'");
	xmldoc_.InsertFirstChild(decl);

	tinyxml2::XMLDeclaration* decl2 = xmldoc_.NewDeclaration("xml-stylesheet type=\"text/xsl\" href=\"Anything.xsl\"");
	xmldoc_.InsertAfterChild(decl,decl2);

	root_ = xmldoc_.NewElement("protocol");
	xmldoc_.InsertEndChild(root_);

}

void XmlOutput::Update(ols_data_t *settings)
{
	UNUSED_PARAMETER(settings);

   
}



#define ols_data_get_uint32 (uint32_t) ols_data_get_int


OLS_DECLARE_MODULE()
OLS_MODULE_USE_DEFAULT_LOCALE("ols-xml-output", "en-US")
MODULE_EXPORT const char *ols_module_description(void)
{
	return "xml output";
}

static ols_properties_t *get_properties(void *data)
{
	XmlOutput *s = reinterpret_cast<XmlOutput *>(data);
	string path;

	ols_properties_t *props = ols_properties_create();
	ols_property_t *p;

	return props;
}

  // ols_script_t *script = ols_script_create("/home/zkun/OpenSource/Open-Log-Stream-Analysis/build/rundir/RelWithDebInfo/lib/ols-scripting/parse_log_2.py",NULL);

  // ols_scripting_prase(script,"parse this log in python",sizeof("parse this log in python") - 1);


bool ols_module_load(void)
{
	ols_output_info si = {};
	si.id = "xml_output";
	//si.type = OLS_PROCESS_TYPE_INPUT;
	// si.output_flags = OLS_SOURCE_ | OLS_SOURCE_CUSTOM_DRAW |
	// 		  OLS_SOURCE_CAP_OLSOLETE | OLS_SOURCE_SRGB;
	si.get_properties = get_properties;
	//si.icon_type = OLS_ICON_TYPE_TEXT;

	si.get_name = [](void *) {
		return ols_module_text("XmlOutput");
	};

	si.start = [](void *data){
		return true;
	};

	si.stop = [](void *data, uint64_t ts){
		return ;
	};


	si.create = [](ols_data_t *settings, ols_output_t *process) {
		return (void *)new XmlOutput(process, settings);
	};

	si.destroy = [](void *data) {
		delete reinterpret_cast<XmlOutput *>(data);
	};

	si.request_new_pad = [](void *data, const char *name ,const char *caps) {
		return  reinterpret_cast<XmlOutput *>(data)->requestNewPad(name,caps) ;
	}; ; 

	si.get_defaults = [](ols_data_t *settings) {
		//defaults(settings, 1);
		UNUSED_PARAMETER(settings);
	};

	si.update = [](void *data, ols_data_t *settings) {
		reinterpret_cast<XmlOutput *>(data)->Update(settings);
	};

	ols_register_output(&si);


	return true;
}

void ols_module_unload(void)
{

}
