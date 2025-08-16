
#include "ols-meta-txt.h"
#include "tinyxml2.h"
#include <algorithm>
#include <locale>
#include <memory>
#include <ols-module.h>
#include <string>
#include <map>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <queue>
#include <mutex>
#include <time.h>
#include <sys/stat.h>
#include <util/platform.h>
#include <util/task.h>
#include <util/util.hpp>
#include <util/dstr.hpp>
#include <util/pipe.h>


using namespace std;

#define warning(format, ...)                                                   \
  blog(LOG_WARNING, "[%s] " format, ols_output_get_name(source), ##__VA_ARGS__)


// XML Struct 
//<protocol>
//  <root>
//		<system>
//			...
//		</system>
//		<applications>
//			<summary>
//			</summary>
//			<apps>
//				<app>
//					<name></name>
//					<verison></verison>
//					<compile_time></compile_time>
//					<analysis></analysis>
//				</app>
//			</apps>
//		</applications>
//  </root>
//</protocol>

struct XmlDocToHtmlTaskItem {

	tinyxml2::XMLDocument  *xmlDoc;
	XmlDocToHtmlTaskItem():xmlDoc(nullptr){
	}
	~XmlDocToHtmlTaskItem(){
		if(xmlDoc)
			delete xmlDoc;
	}
};

struct AppNode {
	tinyxml2::XMLElement* app_root_ = nullptr;
	tinyxml2::XMLElement* analysis_ = nullptr;
};

struct XmlOutput {
   ols_output_t *output_ = nullptr;
   tinyxml2::XMLDocument *xmldoc_ = nullptr;
   tinyxml2::XMLElement *root_ = nullptr;

   tinyxml2::XMLElement* system_ = nullptr;

   tinyxml2::XMLElement* applications_ = nullptr;

   tinyxml2::XMLElement* apps_ = nullptr;

   tinyxml2::XMLElement* summary_ = nullptr;

   std::map<std::string,AppNode> app_tags_;

   OlsEventType curr_event_ = OLS_EVENT_STREAM_START;

   os_task_queue_t * xml_2_html_task_;

   std::queue<XmlDocToHtmlTaskItem *> task_items_;
   std::mutex task_mtx_;

   int  idx_ {0};
   bool data_flag_{false};

	/* --------------------------- */
	inline XmlOutput(ols_output_t *output, ols_data_t *settings)
	: output_(output) {
		update( settings);

		xml_2_html_task_ = os_task_queue_create();

		initOutfile();
	}

  inline ~XmlOutput() {

	os_task_queue_destroy(xml_2_html_task_);
  }

  ols_pad_t *requestNewPad(const char *name, const char *caps);

  void update(ols_data_t *settings);

  ols_pad_t *createSinkPad(const char *caps);

  void onDataBuff(ols_buffer_t *buffer);

  void onEvent(ols_event_t *event);

  void initOutfile();

  void flushOutfile();

  static void xmlToHtmlTaskFunc(void *param);
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


static bool output_sink_event_func(ols_pad_t *pad, ols_object_t *parent,ols_event_t *event){

	XmlOutput *xml_output = reinterpret_cast<XmlOutput *>(parent->data);
	xml_output->onEvent(event);

	return true;
}


ols_pad_t * XmlOutput::createSinkPad(const char *caps){

	ols_pad_t * sinkpad = ols_pad_new("xml-output-sink",OLS_PAD_SINK);

	blog(LOG_DEBUG, "create recv pad success");

	ols_pad_set_link_function(sinkpad,output_sink_link_func );
	ols_pad_set_chain_function(sinkpad,output_sink_chain_func);

	ols_pad_set_event_function(sinkpad,output_sink_event_func);

	ols_output_add_pad(output_, sinkpad);
	return sinkpad;
}

static std::string formatTime(uint64_t msec){

	// 
	auto milliseconds = msec % 1000;

	std::chrono::system_clock::time_point tp(std::chrono::seconds(msec/1000));
	// 
	std::time_t tt = std::chrono::system_clock::to_time_t(tp);

	//
	auto time_tm = std::localtime(&tt);

	std::ostringstream oss;
	oss << std::put_time(time_tm, "%Y-%m-%d %H:%M:%S");
	oss << '.' << std::setw(3) << std::setfill('0') << milliseconds;
	oss << std::put_time(time_tm, " %z");

	return oss.str();
}

void XmlOutput::onDataBuff(ols_buffer_t *buffer){

	ols_meta_txt_t * meta_txt = (ols_meta_txt_t *) buffer->meta;
	ols_meta_result *meta_result = buffer->result;

	if(!meta_result->tag.array){
		blog(LOG_ERROR,"no tag in meta result");
		ols_buffer_unref(buffer);
		return;
	}

	std::string tag(meta_result->tag.array);

	auto &node = app_tags_[tag];

	if(node.app_root_ == nullptr){
		node.app_root_ = xmldoc_->NewElement("app");
		tinyxml2::XMLElement * name = xmldoc_->NewElement("name");
		name->SetText(meta_result->tag.array);
		node.app_root_->InsertFirstChild(name);

		node.analysis_ = xmldoc_->NewElement("analysis");
		node.app_root_->InsertEndChild(node.analysis_);

		apps_->InsertEndChild(node.app_root_);
	}

	for(size_t i = 0; i < meta_result->info.num; ++i){
	
		tinyxml2::XMLElement * item = xmldoc_->NewElement("item");
		{
			tinyxml2::XMLElement * time = xmldoc_->NewElement("time");

			time->SetText(formatTime(meta_txt->msec).c_str());

			item->InsertEndChild(time);

			tinyxml2::XMLElement * title = xmldoc_->NewElement("title");
			if(meta_result->info.array[i]->key){
				title->SetText(meta_result->info.array[i]->key);
			}
			item->InsertEndChild(title);

			tinyxml2::XMLElement * message = xmldoc_->NewElement("message");
			if(meta_result->info.array[i]->val){
				message->SetText(meta_result->info.array[i]->val);
			}
			item->InsertEndChild(message);

		}

		node.analysis_->InsertEndChild(item);
		//printf("data is %s \n",(const char *)meta_result->info.array[i]);
	}
	
	data_flag_ = true;
	ols_buffer_unref(buffer);

}

void XmlOutput::onEvent(ols_event_t *event){

	OlsEventType type = OLS_EVENT_TYPE(event);

	blog(LOG_INFO,"XmlOutput::onEvent receive event type %d , current %d",type,curr_event_);

	if(type == curr_event_){
		return;
	}
	curr_event_ = type;
	if(type == OLS_EVENT_EOS){
		flushOutfile();

	} else if(OLS_EVENT_TYPE(event) == OLS_EVENT_STREAM_START){
		initOutfile();

	} else if( OLS_EVENT_TYPE(event) == OLS_EVENT_STREAM_FLUSH  ){
		flushOutfile();

	}

}

void XmlOutput::xmlToHtmlTaskFunc(void *param){

	XmlOutput *xmlOut = (XmlOutput *)param;


	XmlDocToHtmlTaskItem * item = nullptr;
	
	{
		std::lock_guard<std::mutex> lck(xmlOut->task_mtx_);

		if(!xmlOut->task_items_.empty()){
			item = xmlOut->task_items_.front();
			
			xmlOut->task_items_.pop();
		}
	}

	if(item){

		std::string fileName("Tbox-analysis-report-");

		fileName.append(std::to_string(++xmlOut->idx_));
		fileName.append(1,'-');
		fileName.append(std::to_string(time(NULL)));
		
		std::string xmlFile = fileName + ".xml";
		item->xmlDoc->SaveFile(xmlFile.c_str() );

		DStr command ;
		dstr_printf(command,"xsltproc -o %s.html template.xslt %s.xml",fileName.c_str(),fileName.c_str());

		os_process_pipe_t * pipe = os_process_pipe_create(command->array,"r");

		if(pipe){
			size_t len = 0; 
			uint8_t buff[512];
			while( (len = os_process_pipe_read_err(pipe,buff,sizeof(buff))) != 0 ){
			     //ignore
			}
			os_process_pipe_destroy(pipe);
		} else {
			blog(LOG_INFO,"XmlOutput::xmlToHtmlTaskFunc create command pipe %s failed",command->array);
		}

	}

}

void XmlOutput::flushOutfile(){

	if(!data_flag_ ) {
		blog(LOG_INFO,"No  data is need to flush");
		return;
	}

	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_time = std::chrono::system_clock::to_time_t(now);


	XmlDocToHtmlTaskItem * task_data= new XmlDocToHtmlTaskItem();

	task_data->xmlDoc = xmldoc_;

	{
		std::lock_guard<std::mutex> lck(task_mtx_);
		task_items_.push(task_data);
	}

	os_task_queue_queue_task(xml_2_html_task_,xmlToHtmlTaskFunc,this);

	xmldoc_ = nullptr;

	root_ = nullptr;

	system_ = nullptr;
 
	applications_ = nullptr;
 
	apps_ = nullptr;
 
	summary_ = nullptr;
 
	app_tags_.clear();	

	data_flag_ = false;
}

ols_pad_t *XmlOutput::requestNewPad(const char *name, const char *caps){

	if(strcmp("sink",name) == 0){
		return  createSinkPad(caps);
	}

	return NULL;
}


void XmlOutput::initOutfile(){

	xmldoc_ = new tinyxml2::XMLDocument();

	tinyxml2::XMLDeclaration* decl = xmldoc_->NewDeclaration("xml version='1.0' encoding='UTF-8' standalone='yes'");
	xmldoc_->InsertFirstChild(decl);

	tinyxml2::XMLDeclaration* decl2 = xmldoc_->NewDeclaration("xml-stylesheet type=\"text/xsl\" href=\"Anything.xsl\"");
	xmldoc_->InsertAfterChild(decl,decl2);

	root_ = xmldoc_->NewElement("protocol");
	xmldoc_->InsertEndChild(root_);

	system_ = xmldoc_->NewElement("system");
	root_->InsertFirstChild(system_);

	applications_  = xmldoc_->NewElement("applications");

	summary_ = xmldoc_->NewElement("summary");
	applications_->InsertFirstChild(summary_);

	apps_ = xmldoc_->NewElement("apps");
	applications_->InsertFirstChild(apps_);

	root_->InsertEndChild(applications_);

	app_tags_.clear();
}

void XmlOutput::update(ols_data_t *settings){
	UNUSED_PARAMETER(settings);
}

#define ols_data_get_uint32 (uint32_t) ols_data_get_int

OLS_DECLARE_MODULE()
OLS_MODULE_USE_DEFAULT_LOCALE("ols-xml-output", "en-US")
MODULE_EXPORT const char *ols_module_description(void){
	return "xml output";
}

static ols_properties_t *get_properties(void *data){
	XmlOutput *s = reinterpret_cast<XmlOutput *>(data);
	//string path;

	ols_properties_t *props = ols_properties_create();
	//ols_property_t *p;

	return props;
}


bool ols_module_load(void){
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

	si.create = [](ols_data_t *settings, ols_output_t *output) {
		return (void *)new XmlOutput(output, settings);
	};

	si.destroy = [](void *data) {
		delete reinterpret_cast<XmlOutput *>(data);
	};

	si.request_new_pad = [](void *data, const char *name ,const char *caps) {
		return  reinterpret_cast<XmlOutput *>(data)->requestNewPad(name,caps) ;
	}; 

	si.get_defaults = [](ols_data_t *settings) {
		//defaults(settings, 1);
		UNUSED_PARAMETER(settings);
	};

	si.update = [](void *data, ols_data_t *settings) {
		reinterpret_cast<XmlOutput *>(data)->update(settings);
	};

	ols_register_output(&si);

	return true;
}

void ols_module_unload(void){

}
