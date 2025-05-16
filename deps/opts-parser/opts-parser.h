#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ols_option {
	char *name;
	char *value;
};

struct ols_options {
	size_t count;
	struct ols_option *options;
	size_t ignored_word_count;
	char **ignored_words;
	char **input_words;
};

struct ols_options ols_parse_options(const char *options_string);
void ols_free_options(struct ols_options options);

#ifdef __cplusplus
}
#endif
