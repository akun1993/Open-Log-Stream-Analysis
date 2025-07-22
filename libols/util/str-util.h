#pragma once

#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

EXPORT int str_strncmp(const char *s1, const char *s2, size_t n);
EXPORT bool str_is_utf8(const unsigned char *data, size_t length);
EXPORT char *str_strncat(char *front, const char *back, size_t count);
EXPORT char *str_ltrim(char *str);
EXPORT void str_rtrim(char *str, int len);
EXPORT bool str_endwith(const char *str, const char *suffix);


#ifdef __cplusplus
}
#endif
