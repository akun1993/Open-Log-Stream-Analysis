#include "str-util.h"
#include <string.h>
#include <ctype.h>

int str_strncmp(const char *s1, const char *s2, size_t n) {
  unsigned char c1 = '\0';
  unsigned char c2 = '\0';
  if (n >= 4) {
    size_t n4 = n >> 2;
    do {
      c1 = (unsigned char)*s1++;
      c2 = (unsigned char)*s2++;
      if (c1 == '\0' || c1 != c2)
        return c1 - c2;
      c1 = (unsigned char)*s1++;
      c2 = (unsigned char)*s2++;
      if (c1 == '\0' || c1 != c2)
        return c1 - c2;
      c1 = (unsigned char)*s1++;
      c2 = (unsigned char)*s2++;
      if (c1 == '\0' || c1 != c2)
        return c1 - c2;
      c1 = (unsigned char)*s1++;
      c2 = (unsigned char)*s2++;
      if (c1 == '\0' || c1 != c2)
        return c1 - c2;
    } while (--n4 > 0);
    n &= 3;
  }
  while (n > 0) {
    c1 = (unsigned char)*s1++;
    c2 = (unsigned char)*s2++;
    if (c1 == '\0' || c1 != c2)
      return c1 - c2;
    n--;
  }
  return c1 - c2;
}


bool str_is_utf8(const unsigned char* utf8_str, size_t length)
{
    bool result;

    if (utf8_str == NULL)
    {
        /* Codes_SRS_UTF8_CHECKER_01_002: [ If utf8_checker_is_valid_utf8 is called with NULL utf8_str it shall return false. ]*/
        result = false;
    }
    else
    {
        size_t pos = 0;

        /* Codes_SRS_UTF8_CHECKER_01_003: [ If length is 0, utf8_checker_is_valid_utf8 shall consider utf8_str to be valid UTF-8 and return true. ]*/
        result = true;

        while ((result == true) &&
                (pos < length))
        {
            /* Codes_SRS_UTF8_CHECKER_01_001: [ utf8_checker_is_valid_utf8 shall verify that the sequence of chars pointed to by utf8_str represent UTF-8 encoded codepoints. ]*/
            if ((utf8_str[pos] >> 3) == 0x1E)
            {
                /* 4 bytes */
                /* Codes_SRS_UTF8_CHECKER_01_009: [ 000uuuuu zzzzyyyy yyxxxxxx 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx ]*/
                uint32_t code_point = (utf8_str[pos] & 0x07);

                pos++;
                if ((pos < length) &&
                    ((utf8_str[pos] >> 6) == 0x02))
                {
                    code_point <<= 6;
                    code_point += utf8_str[pos] & 0x3F;

                    pos++;
                    if ((pos < length) &&
                        ((utf8_str[pos] >> 6) == 0x02))
                    {
                        code_point <<= 6;
                        code_point += utf8_str[pos] & 0x3F;

                        pos++;
                        if ((pos < length) &&
                            ((utf8_str[pos] >> 6) == 0x02))
                        {
                            code_point <<= 6;
                            code_point += utf8_str[pos] & 0x3F;

                            if (code_point <= 0xFFFF)
                            {
                                result = false;
                            }
                            else
                            {
                                /* Codes_SRS_UTF8_CHECKER_01_005: [ On success it shall return true. ]*/
                                result = true;
                                pos++;
                            }
                        }
                        else
                        {
                            result = false;
                        }
                    }
                    else
                    {
                        result = false;
                    }
                }
                else
                {
                    result = false;
                }
            }
            else if ((utf8_str[pos] >> 4) == 0x0E)
            {
                /* 3 bytes */
                /* Codes_SRS_UTF8_CHECKER_01_008: [ zzzzyyyy yyxxxxxx 1110zzzz 10yyyyyy 10xxxxxx ]*/
                uint32_t code_point = (utf8_str[pos] & 0x0F);

                pos++;
                if ((pos < length) &&
                    ((utf8_str[pos] >> 6) == 0x02))
                {
                    code_point <<= 6;
                    code_point += utf8_str[pos] & 0x3F;

                    pos++;
                    if ((pos < length) &&
                        ((utf8_str[pos] >> 6) == 0x02))
                    {
                        code_point <<= 6;
                        code_point += utf8_str[pos] & 0x3F;

                        if (code_point <= 0x7FF)
                        {
                            result = false;
                        }
                        else
                        {
                            /* Codes_SRS_UTF8_CHECKER_01_005: [ On success it shall return true. ]*/
                            result = true;
                            pos++;
                        }
                    }
                    else
                    {
                        result = false;
                    }
                }
                else
                {
                    result = false;
                }
            }
            else if ((utf8_str[pos] >> 5) == 0x06)
            {
                /* 2 bytes */
                /* Codes_SRS_UTF8_CHECKER_01_007: [ 00000yyy yyxxxxxx 110yyyyy 10xxxxxx ]*/
                uint32_t code_point = (utf8_str[pos] & 0x1F);

                pos++;
                if ((pos < length) &&
                    ((utf8_str[pos] >> 6) == 0x02))
                {
                    code_point <<= 6;
                    code_point += utf8_str[pos] & 0x3F;

                    if (code_point <= 0x7F)
                    {
                        result = false;
                    }
                    else
                    {
                        /* Codes_SRS_UTF8_CHECKER_01_005: [ On success it shall return true. ]*/
                        result = true;
                        pos++;
                    }
                }
                else
                {
                    result = false;
                }
            }
            else if ((utf8_str[pos] >> 7) == 0x00)
            {
                /* 1 byte */
                /* Codes_SRS_UTF8_CHECKER_01_006: [ 00000000 0xxxxxxx 0xxxxxxx ]*/
                /* Codes_SRS_UTF8_CHECKER_01_005: [ On success it shall return true. ]*/
                result = true;
                pos++;
            }
            else
            {
                /* error */
                result = false;
            }
        }
    }

    return result;
}


char *str_strncat(char *front, const char *back, size_t count) {
  char *start = front;

  while (*front++)
    ;
  front--;

  while (count--)
    if ((*front++ = *back++) == 0)
      return (start);

  *front = '\0';
  return (start);
}

char *str_ltrim(char *str) {

  while (isspace((unsigned char)*str))
    str++;

  return str;
}

void str_rtrim(char *str, int len) {
  char *end;
  if (*str == 0) 
    return;

  end = str + len - 1;
  while (end > str && isspace((unsigned char)*end))
    end--;
  *(end + 1) = 0;
}


bool str_endwith(const char *str, const char *suffix) {
    size_t len_str = strlen(str);
    size_t len_suffix = strlen(suffix);
    if (len_suffix > len_str) return 0;
    return strcmp(str + len_str - len_suffix, suffix) == 0;
}
