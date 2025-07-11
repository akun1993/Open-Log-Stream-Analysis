#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "base.h"

#ifdef DISABLE_CHECKS

#define return_if_fail(expr)			O_STMT_START{ (void)0; }O_STMT_END
#define return_val_if_fail(expr,val)		O_STMT_START{ (void)0; }O_STMT_END
#define return_if_reached()			O_STMT_START{ return; }O_STMT_END
#define return_val_if_reached(val)		O_STMT_START{ return (val); }O_STMT_END

#else /* !G_DISABLE_CHECKS */

#ifdef __GNUC__

#define return_if_fail(expr)		O_STMT_START{		\
     if (LIKELY(expr)) { } else       \
       {							\
	    blog (				\
		LOG_ERROR,					\
		"file %s: line %d (%s): assertion `%s' failed",	 \
		__FILE__,						\
		__LINE__,						\
		__PRETTY_FUNCTION__,			\
		#expr);							\
	 return;							\
       };        }O_STMT_END

#define return_val_if_fail(expr,val)	O_STMT_START{			\
     if (LIKELY(expr)) { } else						\
       {								\
        blog (						\
            LOG_ERROR,					\
		"file %s: line %d (%s): assertion `%s' failed",		\
		__FILE__,						\
		__LINE__,						\
		__PRETTY_FUNCTION__,					\
		#expr);							\
	 return (val);							\
       };				}O_STMT_END

#define return_if_reached()		O_STMT_START{			\
        blog (					\
	    LOG_ERROR,					\
	    "file %s: line %d (%s): should not be reached",		\
	    __FILE__,							\
	    __LINE__,							\
	    __PRETTY_FUNCTION__);					\
     return;				}O_STMT_END

#define return_val_if_reached(val)	O_STMT_START{			\
        blog (						\
	    LOG_ERROR,					\
	    "file %s: line %d (%s): should not be reached",		\
	    __FILE__,							\
	    __LINE__,							\
	    __PRETTY_FUNCTION__);					\
     return (val);			}O_STMT_END

#else /* !__GNUC__ */

#define return_if_fail(expr)		O_STMT_START{		\
     if (expr) { } else						\
       {							\
        blog (					\
            LOG_ERROR,				\
		"file %s: line %d: assertion `%s' failed",	\
		__FILE__,					\
		__LINE__,					\
		#expr);						\
	 return;						\
       };				}O_STMT_END

#define return_val_if_fail(expr, val)	O_STMT_START{		\
     if (expr) { } else						\
       {							\
        blog (					\
            LOG_ERROR,				\
		"file %s: line %d: assertion `%s' failed",	\
		__FILE__,					\
		__LINE__,					\
		#expr);						\
	 return (val);						\
       };				}O_STMT_END

#define return_if_reached()		O_STMT_START{		\
    blog (				\
	    LOG_ERROR,				\
	    "file %s: line %d: should not be reached",		\
	    __FILE__,						\
	    __LINE__);						\
     return;				}O_STMT_END

#define return_val_if_reached(val)	O_STMT_START{		\
    blog (					\
	    LOG_ERROR,				\
	    "file %s: line %d: should not be reached",		\
	    __FILE__,						\
	    __LINE__);						\
     return (val);			}O_STMT_END

#endif /* !__GNUC__ */

#endif

#endif