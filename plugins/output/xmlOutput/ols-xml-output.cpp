
#include <algorithm>
#include <locale>
#include <memory>
#include <ols-module.h>
#include <string>
#include <sys/stat.h>
#include <util/platform.h>
#include <util/task.h>
#include <util/util.hpp>
#include "tinyxml2.h"

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
static OlsFlowReturn output_chainlist_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_list_t *buffer){
	blog(LOG_DEBUG, "script_chainlist_func");
	return OLS_FLOW_OK;
}

static OlsFlowReturn output_sink_chain_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_t *buffer){
	//blog(LOG_DEBUG, "script_chain_func");
	return OLS_FLOW_OK;
}

static OlsPadLinkReturn output_sink_link_func(ols_pad_t *pad,ols_object_t *parent,ols_pad_t *peer){
	//blog(LOG_DEBUG, "script_link_func");
	return OLS_PAD_LINK_OK;
}



struct XmlOutput  {
	ols_output_t *output_ = nullptr;
	// ols_pad_t     *srcpad_  = nullptr;
	// ols_pad_t     *sinkpad_  = nullptr;

	/* --------------------------- */

	inline XmlOutput(ols_output_t *process, ols_data_t *settings)
		: output_(process)
	{
		//ols_output_update(process_, settings);
		// srcpad_ = ols_pad_new("script-process-src",OLS_PAD_SRC);
		// sinkpad_ = ols_pad_new("script-process-sink",OLS_PAD_SINK);


		// ols_pad_set_link_function(sinkpad_,script_link_func);
		// ols_pad_set_chain_function(sinkpad_,script_chain_func);

		// blog(LOG_DEBUG, "ols_context_add_pad");
		// ols_process_add_pad(process_, sinkpad_);

		//ols_pad_set_chain_list_function(sinkpad,script_chainlist_func);
	}

	inline ~XmlOutput(){

	}

	ols_pad_t *requestNewPad(const char *name, const char *caps);

	void Update(ols_data_t *settings);

	ols_pad_t * createRecvPad(const char *caps);

};


ols_pad_t * XmlOutput::createRecvPad(const char *caps){

	ols_pad_t * sinkpad = ols_pad_new("xml-output-sink",OLS_PAD_SINK);

	blog(LOG_DEBUG, "create recv pad success");

	ols_pad_set_link_function(sinkpad,output_sink_link_func );
	ols_pad_set_chain_function(sinkpad,output_sink_chain_func);

	//ols_output_add_pad(output_, sinkpad);
	return sinkpad;
}



 ols_pad_t *XmlOutput::requestNewPad(const char *name, const char *caps)
{

	if(strcmp("sink",name) == 0){
		return  createRecvPad(caps);
	}

	return NULL;
}


void XmlOutput::Update(ols_data_t *settings)
{
	UNUSED_PARAMETER(settings);

	
// <?xml version="1.0"?>
// <Root>
//     <Element attribute="value">This is a text node</Element>
//     <SubElement subAttr="subValue">This is a sub text node</SubElement>
// </Root>

	   // 创建一个 XMLDocument 对象
    tinyxml2::XMLDocument doc;
 
    // 创建一个根节点 <Root>
    tinyxml2::XMLElement* root = doc.NewElement("Root");
    doc.InsertFirstChild(root);
 
    // 创建一个子节点 <Element> 并设置属性 attribute="value"
    tinyxml2::XMLElement* info = doc.NewElement("information");

	tinyxml2::XMLElement* soc_ver = doc.NewElement("soc_version");
	tinyxml2::XMLText* ver_text = doc.NewText("This is a text node");
	soc_ver->InsertEndChild(ver_text);

	info->InsertEndChild(soc_ver);

    root->InsertEndChild(info);
	
    // 添加文本到 <Element> 节点
    tinyxml2::XMLText* text = doc.NewText("This is a text node");
    element->InsertEndChild(text);
 
    // 创建另一个子节点 <SubElement> 并添加文本和属性
    tinyxml2::XMLElement* subElement = doc.NewElement("SubElement");

    root->InsertEndChild(subElement);
    tinyxml2::XMLText* subText = doc.NewText("This is a sub text node");
    subElement->InsertEndChild(subText);
 
    // 保存文件到磁盘
    tinyxml2::XMLError eResult = doc.SaveFile("example.xml");
    if (eResult != tinyxml2::XML_SUCCESS) {
        //std::cerr << "Error saving file: " << eResult << std::endl;
        //return -1;
    } else {
        //std::cout << "File saved successfully." << std::endl;
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
	ols_process_info si = {};
	si.id = "script_caller";
	si.type = OLS_PROCESS_TYPE_INPUT;
	// si.output_flags = OLS_SOURCE_ | OLS_SOURCE_CUSTOM_DRAW |
	// 		  OLS_SOURCE_CAP_OLSOLETE | OLS_SOURCE_SRGB;
	si.get_properties = get_properties;
	si.icon_type = OLS_ICON_TYPE_TEXT;

	si.get_name = [](void *) {
		return ols_module_text("TextFile");
	};

	si.create = [](ols_data_t *settings, ols_process_t *process) {
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

	ols_register_process(&si);


	return true;
}

void ols_module_unload(void)
{

}
