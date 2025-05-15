%module olslua
%{
#define SWIG_FILE_WITH_INIT
#define DEPRECATED_START
#define DEPRECATED_END
#include <ols.h>
#include <ols-source.h>
#include <ols-data.h>
#include <ols-properties.h>
#include <callback/calldata.h>
#include <callback/proc.h>
#include <callback/signal.h>
#include <util/bmem.h>
#include <util/base.h>
#include "cstrcache.h"
#include <util/platform.h>
#include <util/config-file.h>

#if defined(ENABLE_UI)
#include "obs-frontend-api.h"
#endif

%}

#define DEPRECATED_START
#define DEPRECATED_END
#define OLS_DEPRECATED
#define OLS_EXTERNAL_DEPRECATED
#define EXPORT

%rename(blog) wrap_blog;
%inline %{
static inline void wrap_blog(int log_level, const char *message)
{
        blog(log_level, "%s", message);
}
%}

%include "stdint.i"

/* Used to free when using %newobject functions.  E.G.:
 * %newobject obs_module_get_config_path; */
%typemap(newfree) char * "bfree($1);";

%ignore blog;
%ignore blogva;
%ignore bcrash;
%ignore base_set_crash_handler;
%ignore obs_source_info;
%ignore obs_register_source_s(const struct obs_source_info *info, size_t size);
%ignore obs_output_set_video(obs_output_t *output, video_t *video);
%ignore obs_output_video(const obs_output_t *output);
%ignore obs_add_tick_callback;
%ignore obs_remove_tick_callback;
%ignore obs_add_main_render_callback;
%ignore obs_remove_main_render_callback;
%ignore obs_enum_sources;
%ignore obs_source_enum_filters;
%ignore obs_properties_add_button;
%ignore obs_property_set_modified_callback;
%ignore signal_handler_connect;
%ignore signal_handler_disconnect;
%ignore signal_handler_connect_global;
%ignore signal_handler_disconnect_global;
%ignore signal_handler_remove_current;


%include "ols-data.h"
%include "ols-source.h"
%include "ols-properties.h"
%include "ols.h"
%include "callback/calldata.h"
%include "callback/proc.h"
%include "callback/signal.h"
%include "util/bmem.h"
%include "util/base.h"
%include "util/platform.h"
%include "util/config-file.h"

#if defined(ENABLE_UI)
%include "obs-frontend-api.h"
#endif
