%module(threads="1") olspython
%nothread;
%{
#define SWIG_FILE_WITH_INIT
#define DEPRECATED_START
#define DEPRECATED_END

#include <util/base.h>
#include "ols-scripting-config.h"


/* Redefine SWIG_PYTHON_INITIALIZE_THREADS if:
 * - Python version is 3.7 or later because PyEval_InitThreads() became deprecated and unnecessary
 * - SWIG version is not 4.1 or later because SWIG_PYTHON_INITIALIZE_THREADS will be define correctly
 *   with Python 3.7 and later */
#if PY_VERSION_HEX >= 0x03070000 && SWIGVERSION < 0x040100
#undef SWIG_PYTHON_INITIALIZE_THREADS
#define SWIG_PYTHON_INITIALIZE_THREADS
#endif
%}

%feature("python:annotations", "c");
%feature("autodoc", "2");

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
#include "ols-scripting-config.h"

