%module olslua
%{
#define SWIG_FILE_WITH_INIT
#define DEPRECATED_START
#define DEPRECATED_END
#include <util/base.h>
#include "cstrcache.h"
#include "ols-scripting-config.h"


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
 * %newobject ols_module_get_config_path; */
%typemap(newfree) char * "bfree($1);";

%ignore blog;
%ignore blogva;
%ignore bcrash;
%ignore base_set_crash_handler;



%include "util/base.h"



