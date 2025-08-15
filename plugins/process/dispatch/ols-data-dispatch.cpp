
#include "ols-meta-txt.h"
#include "ols-event.h"
#include <algorithm>
#include <locale>
#include <memory>
#include <ols-module.h>
#include <string>
#include <map>
#include <list>
#include <set>
#include <sys/stat.h>
#include <util/platform.h>
#include <util/str-util.h>
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

struct DataDispatch {
  ols_process_t *process_ = nullptr;

  int year_{1970};
  
  std::map<std::string, std::list<ols_pad_t *>> tag2Pad_;

  std::set<ols_pad_t *> tagAny_;

  /* --------------------------- */

  inline DataDispatch(ols_process_t *process, ols_data_t *settings)
      : process_(process) {
		update(settings);
  }

  inline ~DataDispatch() {}

  ols_pad_t *requestNewPad(const char *name, const char *caps);

  void update(ols_data_t *settings);

  ols_pad_t *createSinkPad(const char *caps);
  ols_pad_t *createSrcPad(const char *caps);

  void onDataBuff(ols_buffer_t *buffer);

  void onPeerLink(ols_pad_t *pad,ols_pad_t *peer);
};

/* ------------------------------------------------------------------------- */

/* clang-format off */
static OlsFlowReturn dispatch_chainlist_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_list_t *buffer){

	blog(LOG_DEBUG, "dispatch_chainlist_func");
	return OLS_FLOW_OK;
}

static OlsFlowReturn dispatch_sink_chain_func(ols_pad_t *pad,ols_object_t *parent,ols_buffer_t *buffer){

	DataDispatch *dispatch = reinterpret_cast<DataDispatch *>(parent->data);
	dispatch->onDataBuff(buffer);
	//blog(LOG_DEBUG, "dispatch_sink_chain_func %s",ols_txt->data );
	return OLS_FLOW_OK;
}

static OlsPadLinkReturn dispatch_sink_link_func(ols_pad_t *pad,ols_object_t *parent,ols_pad_t *peer){
	blog(LOG_DEBUG, "dispatch_sink_link_func");
	return OLS_PAD_LINK_OK;
}

static OlsPadLinkReturn dispatch_src_link_func(ols_pad_t *pad,ols_object_t *parent,ols_pad_t *peer){
	blog(LOG_DEBUG, "dispatch_src_link_func src %p peer %p",pad,peer);

	DataDispatch *dispatch = reinterpret_cast<DataDispatch *>(parent->data);
	dispatch->onPeerLink(pad,peer);

	return OLS_PAD_LINK_OK;
}

void  DataDispatch::onPeerLink(ols_pad_t *pad,ols_pad_t *peer){


	if(OLS_PAD_CAPS(peer)){

		//blog(LOG_DEBUG, "peer linked with caps");

		if(!CAPS_IS_ANY(OLS_PAD_CAPS(peer))){

			//blog(LOG_DEBUG, "peer linked with caps default");

			size_t count = ols_caps_count(OLS_PAD_CAPS(peer));
			for(size_t i = 0; i < count; ++i){
				std::string cap_tag = ols_caps_by_idx(OLS_PAD_CAPS(peer),i);
				blog(LOG_DEBUG, "DataDispatch::onPeerLink cap tag is %s",cap_tag.c_str());
				if(!cap_tag.empty()){
					tag2Pad_[cap_tag].push_back( pad);
				}
			}
		} else {
			blog(LOG_DEBUG, "peer linked with caps any");
			tagAny_.insert(pad);
		}
	} else {
		blog(LOG_ERROR, "no caps get on peer");
	}
}

ols_pad_t * DataDispatch::createSinkPad(const char *caps){

	ols_pad_t * sinkpad = ols_pad_new("dispatch-sink",OLS_PAD_SINK);

	blog(LOG_DEBUG, "create recv pad success");

	ols_pad_set_link_function(sinkpad,dispatch_sink_link_func);
	ols_pad_set_chain_function(sinkpad,dispatch_sink_chain_func);

	ols_process_add_pad(process_, sinkpad);
	return sinkpad;
}

ols_pad_t * DataDispatch::createSrcPad(const char *caps){
	ols_pad_t  * srcpad = ols_pad_new("dispatch-src",OLS_PAD_SRC);

	ols_pad_set_link_function(srcpad,dispatch_src_link_func);

	blog(LOG_DEBUG, "create src pad success %p",srcpad);

	ols_process_add_pad(process_, srcpad);
	return srcpad;
}

ols_pad_t *DataDispatch::requestNewPad(const char *name, const char *caps){

	if(strcmp("sink",name) == 0){
		return  createSinkPad(caps);
	} else if(strcmp("src",name) == 0) {
		return  createSrcPad(caps);
	}

	return NULL;
}


void DataDispatch::onDataBuff(ols_buffer_t *buffer){

	ols_meta_txt_t * ols_txt = (ols_meta_txt_t *) buffer->meta;

	size_t buff_len = ols_txt->len;
	size_t parse_len = 0;

	if(str_strncmp((const char *)ols_txt->buff,"**",2) == 0){
		//printf("data is %s \n",(const char *)ols_txt->buff);
		const char *result ;
		if((result = strstr((const char *)ols_txt->buff, "Log Start"))  != nullptr){
			while(*result != ':' && parse_len < buff_len ){
				++result;
				++parse_len;
			}
			++result;
			++parse_len;

			while(isspace(*result) && parse_len < buff_len){
				++result;
				++parse_len;
			}

			int year = 0;
			while (isdigit(*result) && parse_len < buff_len){
				year = year * 10 + (*result  - '0');
				++result;
				++parse_len;
			}

			if(year != 0 && year < 9999){ 
				year_ = year;
			}

		} else if((result = strstr((const char *)ols_txt->buff, "Parameters"))  != nullptr ) {

			//send flush event first
			ols_event_t *event_flush = ols_event_new_stream_flush();
			for (size_t i = 0; i < process_->context.srcpads.num; ++i) {
				ols_pad_t *pad = process_->context.srcpads.array[i];
				ols_pad_push_event(pad, event_flush);
			}	

			//begin of cold start 
			ols_event_t *event_start = ols_event_new_stream_start();
			for (size_t i = 0; i < process_->context.srcpads.num; ++i) {
				ols_pad_t *pad = process_->context.srcpads.array[i];
				ols_pad_push_event(pad, event_start);
			}				
		}
		ols_buffer_unref(buffer);
	} else {
		const char *p = (const char *)ols_txt->buff;
		
		//05-22 11:17:45.265  3006 16061 V DSVFSALib:
		if(isdigit(p[0]) &&  isdigit(p[1]) && p[2] == '-'){
		
			char time_buf[64] = {'\0'};
			sprintf(time_buf, "%d-", year_);
	
			str_strncat(time_buf,p,18);
			int64_t sec;
			int64_t msec;
			const char *err;
			if(parse("%Y-%m-%d %H:%M:%E*S", time_buf, & sec,
			   & msec,&err) ){
				//printf("%ld %ld \n",sec,fs);
			} else {
				//printf("%ld %ld \n",sec,fs);
				blog(LOG_ERROR,"parse time failed ,not a standard line \n");
				goto LOG_FORMAT_ERR;
			}

			ols_txt->msec = sec * 1000 + msec;
			
			p += 18;
			parse_len += 18;

			if(!isspace(*p)){
				goto LOG_FORMAT_ERR;
			}
	
			while(isspace(*p) && parse_len < buff_len){
				p++;
				parse_len++;
			}

			if(parse_len >= buff_len || !isdigit(*p)){
				goto LOG_FORMAT_ERR;
			}
			
			//parse process id
			int pid = 0;
			while(isdigit(*p) && parse_len < buff_len){
				
				pid = pid * 10 + (*p - '0');
				p++;
				parse_len++;
			}

			ols_txt->pid = pid;

			if(parse_len >= buff_len || !isspace(*p)){
				goto LOG_FORMAT_ERR;
			}			
	
			while(isspace(*p) && parse_len < buff_len){
				p++;
				parse_len++;
			}

			if(parse_len >= buff_len || !isdigit(*p)){
				goto LOG_FORMAT_ERR;
			}
	
			int tid = 0;
	
			while(isdigit(*p) && parse_len < buff_len){
				tid = tid * 10 + (*p - '0');
				p++;
				parse_len++;
			}
			//printf("tid is %d \n",tid);

			ols_txt->tid = tid;
	
			if(parse_len >= buff_len ||  !isspace(*p)){
				goto LOG_FORMAT_ERR;
			}

			while(isspace(*p) && parse_len < buff_len){
				p++;
				parse_len++;
			}
	
			if(parse_len >= buff_len || !isalpha(*p)){
				goto LOG_FORMAT_ERR;
			}

			ols_txt->log_lv = *p++;
			parse_len++;

			if(parse_len >= buff_len || !isspace(*p)){
				goto LOG_FORMAT_ERR;
			}
			
			//printf("log lv is %c \n",*p++);
			
			while(isspace(*p) && parse_len < buff_len){
				p++;
				parse_len++;
			}

			if(parse_len >= buff_len || !isalpha(*p)){
				goto LOG_FORMAT_ERR;
			}			

			const char *tag_beg = p;

			int tag_len = 0; //only support tag length less than 256
			while(*p != ':' && parse_len < buff_len && tag_len < 256){
				parse_len++;
				tag_len++;
				p++;
			}

			if(parse_len >= buff_len){
				goto LOG_FORMAT_ERR;
			}
			
			ols_txt->data_offset = (int32_t)parse_len + 1; //+ 1 for skip ':'

			if(*p == ':') --p;

			while(tag_len > 0 && isspace(*p)){
				--p;
				--tag_len;
			}

			dstr_ncopy(&ols_txt->tag,tag_beg,tag_len);

			if(tag2Pad_.count(ols_txt->tag.array) > 0){
				
				std::list<ols_pad_t *> &list_pad = tag2Pad_[ols_txt->tag.array];
				for (ols_pad_t * pad : list_pad) {
					ols_pad_push(pad, ols_buffer_ref(buffer));
				}
			}
			auto any_iter = tagAny_.begin();
			while(any_iter != tagAny_.end()){
				ols_pad_push(*any_iter,ols_buffer_ref(buffer) );
			}
    		//printf("tag is %s \n",tag);
		} else {
			goto LOG_FORMAT_ERR;
		}
	}

	return;

LOG_FORMAT_ERR:
	ols_buffer_unref(buffer);
}

void DataDispatch::update(ols_data_t *settings)
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
	//string path;

	ols_properties_t *props = ols_properties_create();
	//ols_property_t *p;

	return props;
}


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
		reinterpret_cast<DataDispatch *>(data)->update(settings);
	};

	ols_register_process(&si);


	return true;
}

void ols_module_unload(void)
{

}
