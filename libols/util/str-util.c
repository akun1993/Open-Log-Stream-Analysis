
#include "str-util.h"
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

bool str_is_utf8(const unsigned char *data, size_t length) {
  size_t i = 0;
  while (i < length) {
    // 初始字节数设为0
    size_t bytesToFollow = 0;

    unsigned char head = data[i];

    // 单字节字符
    if ((head & 0x80) == 0x00) {
      // 直接进入下一个字符的判断
      i++;
      continue;
    }

    // 多字节字符的头字节

    if ((head & 0xE0) == 0xC0)
      bytesToFollow = 1; // 两字节字符

    else if ((head & 0xF0) == 0xE0)
      bytesToFollow = 2; // 三字节字符

    else if ((head & 0xF8) == 0xF0)
      bytesToFollow = 3; // 四字节字符

    else
      return false; // 不合法的头字节
    // 验证后续字节
    for (size_t j = 1; j <= bytesToFollow; j++) {
      if (i + j >= length)
        return false; // 字节不足

      unsigned char follower = data[i + j];
      if ((follower & 0xC0) != 0x80)
        return false; // 后续字节不合法
    }
    // 验证通过，跳过这些字节
    i += bytesToFollow + 1;
  }
  return true; // 全部验证通过
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

  // 去除前导空格
  while (isspace((unsigned char)*str))
    str++;

  return str;
}

void str_rtrim(char *str, int len) {
  char *end;
  if (*str == 0) // 如果全是空格，则直接返回空字符串
    return;

  // 去除尾部空格，从字符串末尾开始向前搜索非空格字符
  end = str + len - 1;
  while (end > str && isspace((unsigned char)*end))
    end--;
  // 在字符串末尾加上字符串结束符，覆盖最后一个非空格字符后的所有字符
  *(end + 1) = 0;
}
