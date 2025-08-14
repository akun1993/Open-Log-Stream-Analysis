// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   https://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
#include "time-parse.h"

#if 0
#if !defined(HAS_STRPTIME)
#if !defined(_MSC_VER) && !defined(__MINGW32__) && !defined(__VXWORKS__)
#define HAS_STRPTIME 1 // Assume everyone else has strptime().
#endif
#endif
#endif

#if defined(HAS_STRPTIME) && HAS_STRPTIME
#if !defined(_XOPEN_SOURCE) && !defined(__FreeBSD__) && !defined(__OpenBSD__)
#define _XOPEN_SOURCE 500 // Exposes definitions for SUSv2 (UNIX 98).
#endif
#endif

// Include time.h directly since, by C++ standards, ctime doesn't have to
// declare strptime.
#include <ctype.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if 0
#if !HAS_STRPTIME
// Build a strptime() using C++11's std::get_time().
char *strptime(const char *s, const char *fmt, std::tm *tm) {
  std::istringstream input(s);
  input >> std::get_time(tm, fmt);
  if (input.fail())
    return nullptr;
  return const_cast<char *>(s) +
         (input.eof() ? strlen(s) : static_cast<std::size_t>(input.tellg()));
}
#endif
#endif

#if 0
// Convert a cctz::weekday to a tm_wday value (0-6, Sunday = 0).
int ToTmWday(weekday wd) {
  switch (wd) {
    case weekday::sunday:
      return 0;
    case weekday::monday:
      return 1;
    case weekday::tuesday:
      return 2;
    case weekday::wednesday:
      return 3;
    case weekday::thursday:
      return 4;
    case weekday::friday:
      return 5;
    case weekday::saturday:
      return 6;
  }
  return 0; /*NOTREACHED*/
}

// Convert a tm_wday value (0-6, Sunday = 0) to a cctz::weekday.
weekday FromTmWday(int tm_wday) {
  switch (tm_wday) {
    case 0:
      return weekday::sunday;
    case 1:
      return weekday::monday;
    case 2:
      return weekday::tuesday;
    case 3:
      return weekday::wednesday;
    case 4:
      return weekday::thursday;
    case 5:
      return weekday::friday;
    case 6:
      return weekday::saturday;
  }
  return weekday::sunday; /*NOTREACHED*/
}

#endif

#if 0

std::tm ToTM(const time_zone::absolute_lookup& al) {
  std::tm tm{};
  tm.tm_sec = al.cs.second();
  tm.tm_min = al.cs.minute();
  tm.tm_hour = al.cs.hour();
  tm.tm_mday = al.cs.day();
  tm.tm_mon = al.cs.month() - 1;

  // Saturate tm.tm_year is cases of over/underflow.
  if (al.cs.year() < std::numeric_limits<int>::min() + 1900) {
    tm.tm_year = std::numeric_limits<int>::min();
  } else if (al.cs.year() - 1900 > std::numeric_limits<int>::max()) {
    tm.tm_year = std::numeric_limits<int>::max();
  } else {
    tm.tm_year = static_cast<int>(al.cs.year() - 1900);
  }

  tm.tm_wday = ToTmWday(get_weekday(al.cs));
  tm.tm_yday = get_yearday(al.cs) - 1;
  tm.tm_isdst = al.is_dst ? 1 : 0;
  return tm;
}

// Returns the week of the year [0:53] given a civil day and the day on
// which weeks are defined to start.
int ToWeek(const civil_day& cd, weekday week_start) {
  const civil_day d(cd.year() % 400, cd.month(), cd.day());
  return static_cast<int>((d - prev_weekday(civil_year(d), week_start)) / 7);
}

#endif

char require_32_bit_integer_at_least[sizeof(int) >= sizeof(int32_t) ? 1 : -1];
int getJulianDayNumber(int year, int month, int day) {
  (void)require_32_bit_integer_at_least; // no warning please
  int a = (14 - month) / 12;
  int y = year + 4800 - a;
  int m = month + 12 * a - 3;
  return day + (153 * m + 2) / 5 + y * 365 + y / 4 - y / 100 + y / 400 - 32045;
}

const int kJulianDayOf1970_01_01 = 2440588;

const char kDigits[] = "0123456789";

// Formats a 64-bit integer in the given field width.  Note that it is up
// to the caller of Format64() [and Format02d()/FormatOffset()] to ensure
// that there is sufficient space before ep to hold the conversion.
char *Format64(char *ep, int width, int64_t v) {
  bool neg = false;
  if (v < 0) {
    --width;
    neg = true;
    if (v == LONG_MIN) {
      // Avoid negating minimum value.
      int64_t last_digit = -(v % 10);
      v /= 10;
      if (last_digit < 0) {
        ++v;
        last_digit += 10;
      }
      --width;
      *--ep = kDigits[last_digit];
    }
    v = -v;
  }
  do {
    --width;
    *--ep = kDigits[v % 10];
  } while (v /= 10);
  while (--width >= 0)
    *--ep = '0'; // zero pad
  if (neg)
    *--ep = '-';
  return ep;
}

// Formats [0 .. 99] as %02d.
char *Format02d(char *ep, int v) {
  *--ep = kDigits[v % 10];
  *--ep = kDigits[(v / 10) % 10];
  return ep;
}

// Formats a UTC offset, like +00:00.
char *FormatOffset(char *ep, int offset, const char *mode) {
  // TODO: Follow the RFC3339 "Unknown Local Offset Convention" and
  // generate a "negative zero" when we're formatting a zero offset
  // as the result of a failed load_time_zone().
  char sign = '+';
  if (offset < 0) {
    offset = -offset; // bounded by 24h so no overflow
    sign = '-';
  }
  const int seconds = offset % 60;
  const int minutes = (offset /= 60) % 60;
  const int hours = offset /= 60;
  const char sep = mode[0];
  const bool ext = (sep != '\0' && mode[1] == '*');
  const bool ccc = (ext && mode[2] == ':');
  if (ext && (!ccc || seconds != 0)) {
    ep = Format02d(ep, seconds);
    *--ep = sep;
  } else {
    // If we're not rendering seconds, sub-minute negative offsets
    // should get a positive sign (e.g., offset=-10s => "+00:00").
    if (hours == 0 && minutes == 0)
      sign = '+';
  }
  if (!ccc || minutes != 0 || seconds != 0) {
    ep = Format02d(ep, minutes);
    if (sep != '\0')
      *--ep = sep;
  }
  ep = Format02d(ep, hours);
  *--ep = sign;
  return ep;
}

#if 0
// Formats a std::tm using strftime(3).
void FormatTM(std::string* out, const std::string& fmt, const std::tm& tm) {
  // strftime(3) returns the number of characters placed in the output
  // array (which may be 0 characters).  It also returns 0 to indicate
  // an error, like the array wasn't large enough.  To accommodate this,
  // the following code grows the buffer size from 2x the format string
  // length up to 32x.
  for (std::size_t i = 2; i != 32; i *= 2) {
    std::size_t buf_size = fmt.size() * i;
    std::vector<char> buf(buf_size);
    if (std::size_t len = strftime(&buf[0], buf_size, fmt.c_str(), &tm)) {
      out->append(&buf[0], len);
      return;
    }
  }
}
#endif

// Used for %E#S/%E#f specifiers and for data values in parse().

const char *ParseInt(const char *dp, int width, int min, int max, int *vp) {
  if (dp != NULL) {
    const int kmin = INT_MIN;
    bool erange = false;
    bool neg = false;
    int value = 0;
    if (*dp == '-') {
      neg = true;
      if (width <= 0 || --width != 0) {
        ++dp;
      } else {
        dp = NULL; // width was 1
      }
    }
    const char *bp;
    if (bp = dp) {
      const char *cp;
      while (cp = strchr(kDigits, *dp)) {
        int d = (int)(cp - kDigits);
        if (d >= 10)
          break;
        if (value < kmin / 10) {
          erange = true;
          break;
        }
        value *= 10;
        if (value < kmin + d) {
          erange = true;
          break;
        }
        value -= d;
        dp += 1;
        if (width > 0 && --width == 0)
          break;
      }
      if (dp != bp && !erange && (neg || value != kmin)) {
        if (!neg || value != 0) {
          if (!neg)
            value = -value; // make positive
          if (min <= value && value <= max) {
            *vp = value;
          } else {
            dp = NULL;
          }
        } else {
          dp = NULL;
        }
      } else {
        dp = NULL;
      }
    }
  }
  return dp;
}

const char *ParseInt64(const char *dp, int width, int64_t min, int64_t max,
                       int64_t *vp) {
  if (dp != NULL) {
    const int64_t kmin = LONG_MIN;
    bool erange = false;
    bool neg = false;
    int64_t value = 0;
    if (*dp == '-') {
      neg = true;
      if (width <= 0 || --width != 0) {
        ++dp;
      } else {
        dp = NULL; // width was 1
      }
    }
    const char *bp;
    if (bp = dp) {
      const char *cp;
      while (cp = strchr(kDigits, *dp)) {
        int d = (int)(cp - kDigits);
        if (d >= 10)
          break;
        if (value < kmin / 10) {
          erange = true;
          break;
        }
        value *= 10;
        if (value < kmin + d) {
          erange = true;
          break;
        }
        value -= d;
        dp += 1;
        if (width > 0 && --width == 0)
          break;
      }
      if (dp != bp && !erange && (neg || value != kmin)) {
        if (!neg || value != 0) {
          if (!neg)
            value = -value; // make positive
          if (min <= value && value <= max) {
            *vp = value;
          } else {
            dp = NULL;
          }
        } else {
          dp = NULL;
        }
      } else {
        dp = NULL;
      }
    }
  }
  return dp;
}

// The number of base-10 digits that can be represented by a signed 64-bit
// integer.  That is, 10^kDigits10_64 <= 2^63 - 1 < 10^(kDigits10_64 + 1).
#define kDigits10_64 (18)

// 10^n for everything that can be represented by a signed 64-bit integer.
static const int64_t kExp10[kDigits10_64 + 1] = {
    1,
    10,
    100,
    1000,
    10000,
    100000,
    1000000,
    10000000,
    100000000,
    1000000000,
    10000000000,
    100000000000,
    1000000000000,
    10000000000000,
    100000000000000,
    1000000000000000,
    10000000000000000,
    100000000000000000,
    1000000000000000000,
};

// Uses strftime(3) to format the given Time.  The following extended format
// specifiers are also supported:
//
//   - %Ez  - RFC3339-compatible numeric UTC offset (+hh:mm or -hh:mm)
//   - %E*z - Full-resolution numeric UTC offset (+hh:mm:ss or -hh:mm:ss)
//   - %E#S - Seconds with # digits of fractional precision
//   - %E*S - Seconds with full fractional precision (a literal '*')
//   - %E4Y - Four-character years (-999 ... -001, 0000, 0001 ... 9999)
//   - %ET  - The RFC3339 "date-time" separator "T"
//
// The standard specifiers from RFC3339_* (%Y, %m, %d, %H, %M, and %S) are
// handled internally for performance reasons.  strftime(3) is slow due to
// a POSIX requirement to respect changes to ${TZ}.
//
// The TZ/GNU %s extension is handled internally because strftime() has
// to use mktime() to generate it, and that assumes the local time zone.
//
// We also handle the %z and %Z specifiers to accommodate platforms that do
// not support the tm_gmtoff and tm_zone extensions to std::tm.
//
// Requires that zero() <= fs < seconds(1).

#if 0
std::string format(const std::string& format, const time_point<seconds>& tp,
                   const detail::femtoseconds& fs, const time_zone& tz) {
  std::string result;
  result.reserve(format.size());  // A reasonable guess for the result size.
  const time_zone::absolute_lookup al = tz.lookup(tp);
  const std::tm tm = ToTM(al);

  // Scratch buffer for internal conversions.
  char buf[3 + kDigits10_64];  // enough for longest conversion
  char* const ep = buf + sizeof(buf);
  char* bp;  // works back from ep

  // Maintain three, disjoint subsequences that span format.
  //   [format.begin() ... pending) : already formatted into result
  //   [pending ... cur) : formatting pending, but no special cases
  //   [cur ... format.end()) : unexamined
  // Initially, everything is in the unexamined part.
  const char* pending = format.c_str();  // NUL terminated
  const char* cur = pending;
  const char* end = pending + format.length();

  while (cur != end) {  // while something is unexamined
    // Moves cur to the next percent sign.
    const char* start = cur;
    while (cur != end && *cur != '%') ++cur;

    // If the new pending text is all ordinary, copy it out.
    if (cur != start && pending == start) {
      result.append(pending, static_cast<std::size_t>(cur - pending));
      pending = start = cur;
    }

    // Span the sequential percent signs.
    const char* percent = cur;
    while (cur != end && *cur == '%') ++cur;

    // If the new pending text is all percents, copy out one
    // percent for every matched pair, then skip those pairs.
    if (cur != start && pending == start) {
      std::size_t escaped = static_cast<std::size_t>(cur - pending) / 2;
      result.append(pending, escaped);
      pending += escaped * 2;
      // Also copy out a single trailing percent.
      if (pending != cur && cur == end) {
        result.push_back(*pending++);
      }
    }

    // Loop unless we have an unescaped percent.
    if (cur == end || (cur - percent) % 2 == 0) continue;

    // Simple specifiers that we handle ourselves.
    if (strchr("YmdeUuWwHMSzZs%", *cur)) {
      if (cur - 1 != pending) {
        FormatTM(&result, std::string(pending, cur - 1), tm);
      }
      switch (*cur) {
        case 'Y':
          // This avoids the tm.tm_year overflow problem for %Y, however
          // tm.tm_year will still be used by other specifiers like %D.
          bp = Format64(ep, 0, al.cs.year());
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case 'm':
          bp = Format02d(ep, al.cs.month());
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case 'd':
        case 'e':
          bp = Format02d(ep, al.cs.day());
          if (*cur == 'e' && *bp == '0') *bp = ' ';  // for Windows
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case 'U':
          bp = Format02d(ep, ToWeek(civil_day(al.cs), weekday::sunday));
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case 'u':
          bp = Format64(ep, 0, tm.tm_wday ? tm.tm_wday : 7);
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case 'W':
          bp = Format02d(ep, ToWeek(civil_day(al.cs), weekday::monday));
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case 'w':
          bp = Format64(ep, 0, tm.tm_wday);
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case 'H':
          bp = Format02d(ep, al.cs.hour());
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case 'M':
          bp = Format02d(ep, al.cs.minute());
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case 'S':
          bp = Format02d(ep, al.cs.second());
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case 'z':
          bp = FormatOffset(ep, al.offset, "");
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case 'Z':
          result.append(al.abbr);
          break;
        case 's':
          bp = Format64(ep, 0, ToUnixSeconds(tp));
          result.append(bp, static_cast<std::size_t>(ep - bp));
          break;
        case '%':
          result.push_back('%');
          break;
      }
      pending = ++cur;
      continue;
    }

    // More complex specifiers that we handle ourselves.
    if (*cur == ':' && cur + 1 != end) {
      if (*(cur + 1) == 'z') {
        // Formats %:z.
        if (cur - 1 != pending) {
          FormatTM(&result, std::string(pending, cur - 1), tm);
        }
        bp = FormatOffset(ep, al.offset, ":");
        result.append(bp, static_cast<std::size_t>(ep - bp));
        pending = cur += 2;
        continue;
      }
      if (*(cur + 1) == ':' && cur + 2 != end) {
        if (*(cur + 2) == 'z') {
          // Formats %::z.
          if (cur - 1 != pending) {
            FormatTM(&result, std::string(pending, cur - 1), tm);
          }
          bp = FormatOffset(ep, al.offset, ":*");
          result.append(bp, static_cast<std::size_t>(ep - bp));
          pending = cur += 3;
          continue;
        }
        if (*(cur + 2) == ':' && cur + 3 != end) {
          if (*(cur + 3) == 'z') {
            // Formats %:::z.
            if (cur - 1 != pending) {
              FormatTM(&result, std::string(pending, cur - 1), tm);
            }
            bp = FormatOffset(ep, al.offset, ":*:");
            result.append(bp, static_cast<std::size_t>(ep - bp));
            pending = cur += 4;
            continue;
          }
        }
      }
    }

    // Loop if there is no E modifier.
    if (*cur != 'E' || ++cur == end) continue;

    // Format our extensions.
    if (*cur == 'T') {
      // Formats %ET.
      if (cur - 2 != pending) {
        FormatTM(&result, std::string(pending, cur - 2), tm);
      }
      result.append("T");
      pending = ++cur;
    } else if (*cur == 'z') {
      // Formats %Ez.
      if (cur - 2 != pending) {
        FormatTM(&result, std::string(pending, cur - 2), tm);
      }
      bp = FormatOffset(ep, al.offset, ":");
      result.append(bp, static_cast<std::size_t>(ep - bp));
      pending = ++cur;
    } else if (*cur == '*' && cur + 1 != end && *(cur + 1) == 'z') {
      // Formats %E*z.
      if (cur - 2 != pending) {
        FormatTM(&result, std::string(pending, cur - 2), tm);
      }
      bp = FormatOffset(ep, al.offset, ":*");
      result.append(bp, static_cast<std::size_t>(ep - bp));
      pending = cur += 2;
    } else if (*cur == '*' && cur + 1 != end &&
               (*(cur + 1) == 'S' || *(cur + 1) == 'f')) {
      // Formats %E*S or %E*F.
      if (cur - 2 != pending) {
        FormatTM(&result, std::string(pending, cur - 2), tm);
      }
      char* cp = ep;
      bp = Format64(cp, 15, fs.count());
      while (cp != bp && cp[-1] == '0') --cp;
      switch (*(cur + 1)) {
        case 'S':
          if (cp != bp) *--bp = '.';
          bp = Format02d(bp, al.cs.second());
          break;
        case 'f':
          if (cp == bp) *--bp = '0';
          break;
      }
      result.append(bp, static_cast<std::size_t>(cp - bp));
      pending = cur += 2;
    } else if (*cur == '4' && cur + 1 != end && *(cur + 1) == 'Y') {
      // Formats %E4Y.
      if (cur - 2 != pending) {
        FormatTM(&result, std::string(pending, cur - 2), tm);
      }
      bp = Format64(ep, 4, al.cs.year());
      result.append(bp, static_cast<std::size_t>(ep - bp));
      pending = cur += 2;
    } else if (std::isdigit(*cur)) {
      // Possibly found %E#S or %E#f.
      int n = 0;
      if (const char* np = ParseInt(cur, 0, 0, 1024, &n)) {
        if (*np == 'S' || *np == 'f') {
          // Formats %E#S or %E#f.
          if (cur - 2 != pending) {
            FormatTM(&result, std::string(pending, cur - 2), tm);
          }
          bp = ep;
          if (n > 0) {
            if (n > kDigits10_64) n = kDigits10_64;
            bp = Format64(bp, n,
                          (n > 15) ? fs.count() * kExp10[n - 15]
                                   : fs.count() / kExp10[15 - n]);
            if (*np == 'S') *--bp = '.';
          }
          if (*np == 'S') bp = Format02d(bp, al.cs.second());
          result.append(bp, static_cast<std::size_t>(ep - bp));
          pending = cur = ++np;
        }
      }
    }
  }

  // Formats any remaining data.
  if (end != pending) {
    FormatTM(&result, std::string(pending, end), tm);
  }

  return result;
}
#endif

const char *ParseOffset(const char *dp, const char *mode, int *offset) {
  if (dp != NULL) {
    const char first = *dp++;
    if (first == '+' || first == '-') {
      char sep = mode[0];
      int hours = 0;
      int minutes = 0;
      int seconds = 0;
      const char *ap = ParseInt(dp, 2, 0, 23, &hours);
      if (ap != NULL && ap - dp == 2) {
        dp = ap;
        if (sep != '\0' && *ap == sep)
          ++ap;
        const char *bp = ParseInt(ap, 2, 0, 59, &minutes);
        if (bp != NULL && bp - ap == 2) {
          dp = bp;
          if (sep != '\0' && *bp == sep)
            ++bp;
          const char *cp = ParseInt(bp, 2, 0, 59, &seconds);
          if (cp != NULL && cp - bp == 2)
            dp = cp;
        }
        *offset = ((hours * 60 + minutes) * 60) + seconds;
        if (first == '-')
          *offset = -*offset;
      } else {
        dp = NULL;
      }
    } else if (first == 'Z' || first == 'z') { // Zulu
      *offset = 0;
    } else {
      dp = NULL;
    }
  }
  return dp;
}

const char *ParseZone(const char *dp, char *zone) {
  // zone->clear();
  int idx = 0;
  if (dp != NULL) {
    while (*dp != '\0' && !isspace(*dp)) {
      zone[idx++] = *dp++;
    }
    if (idx == 0)
      dp = NULL;
  }
  return dp;
}

const char *ParseSubSeconds(const char *dp, int64_t *subseconds) {
  if (dp != NULL) {
    int64_t v = 0;
    int64_t exp = 0;
    const char *const bp = dp;
    const char *cp;
    while (cp = strchr(kDigits, *dp)) {
      int d = (int)(cp - kDigits);
      if (d >= 10)
        break;
      if (exp < 15) {
        exp += 1;
        v *= 10;
        v += d;
      }
      ++dp;
    }
    if (dp != bp) {
      // v *= kExp10[15 - exp];
      // printf("subsec %ld \n",v);
      *subseconds = v;
    } else {
      dp = NULL;
    }
  }
  return dp;
}

#if 0
// Parses a string into a std::tm using strptime(3).
const char* ParseTM(const char* dp, const char* fmt, std::tm* tm) {
  if (dp != NULL) {
    dp = strptime(dp, fmt, tm);
  }
  return dp;
}
#endif

// Sets year, tm_mon and tm_mday given the year, week_num, and tm_wday,
// and the day on which weeks are defined to start.  Returns false if year
// would need to move outside its bounds.
#if 0
bool FromWeek(int week_num, weekday week_start, year_t* year, std::tm* tm) {
  const civil_year y(*year % 400);
  civil_day cd = prev_weekday(y, week_start);  // week 0
  cd = next_weekday(cd - 1, FromTmWday(tm->tm_wday)) + (week_num * 7);
  if (const year_t shift = cd.year() - y.year()) {
    if (shift > 0) {
      if (*year > std::numeric_limits<year_t>::max() - shift) return false;
    } else {
      if (*year < std::numeric_limits<year_t>::min() - shift) return false;
    }
    *year += shift;
  }
  tm->tm_mon = cd.month() - 1;
  tm->tm_mday = cd.day();
  return true;
}
#endif

// Uses strptime(3) to parse the given input.  Supports the same extended
// format specifiers as format(), although %E#S and %E*S are treated
// identically (and similarly for %E#f and %E*f).  %Ez and %E*z also accept
// the same inputs. %ET accepts either 'T' or 't'.
//
// The standard specifiers from RFC3339_* (%Y, %m, %d, %H, %M, and %S) are
// handled internally so that we can normally avoid strptime() altogether
// (which is particularly helpful when the native implementation is broken).
//
// The TZ/GNU %s extension is handled internally because strptime() has to
// use localtime_r() to generate it, and that assumes the local time zone.
//
// We also handle the %z specifier to accommodate platforms that do not
// support the tm_gmtoff extension to std::tm.  %Z is parsed but ignored.
bool parse(const char *format, const char *input, int64_t *sec, int64_t *fs,
           const char **err) {
  // The unparsed input.
  const char *data = input; // NUL terminated

  // Skips leading whitespace.
  while (isspace(*data))
    ++data;

  // uint16_t year, month, day, hour, min, sec;
  // uint32_t rdn, sod, nsec;

  const int kyearmax = 9999;
  const int kyearmin = -999;

  // Sets default values for unspecified fields.
  bool saw_year = false;
  int year = 1970;

  int tm_year = 1970 - 1900;
  int tm_mon = 1 - 1; // Jan
  int tm_mday = 1;
  int tm_hour = 0;
  int tm_min = 0;
  int tm_sec = 0;
  int tm_wday = 4; // Thu
  int tm_yday = 0;
  int tm_isdst = 0;

  int64_t subseconds = 0;
  bool saw_offset = false;

  int offset = 0; // No offset from passed tz.
  char zone[64] = "UTC";

  const char *fmt = format; // NUL terminated
  bool twelve_hour = false;
  bool afternoon = false;
  // int week_num = -1;
  // weekday week_start = weekday::sunday;

  bool saw_percent_s = false;
  int64_t percent_s = 0;

  // Steps through format, one specifier at a time.
  while (data != NULL && *fmt != '\0') {
    if (isspace(*fmt)) {
      while (isspace(*data))
        ++data;
      while (isspace(*++fmt))
        continue;
      continue;
    }

    if (*fmt != '%') {
      if (*data == *fmt) {
        ++data;
        ++fmt;
      } else {
        data = NULL;
      }
      continue;
    }

    const char *percent = fmt;
    if (*++fmt == '\0') {
      data = NULL;
      continue;
    }
    switch (*fmt++) {
    case 'Y':
      // Symmetrically with FormatTime(), directly handing %Y avoids the
      // tm.tm_year overflow problem.  However, tm.tm_year will still be
      // used by other specifiers like %D.
      data = ParseInt(data, 0, kyearmin, kyearmax, &year);
      if (data != NULL)
        saw_year = true;
      continue;
    case 'm':
      data = ParseInt(data, 2, 1, 12, &tm_mon);
      if (data != NULL)
        tm_mon -= 1;
      // week_num = -1;
      continue;
    case 'd':
    case 'e':
      data = ParseInt(data, 2, 1, 31, &tm_mday);
      // week_num = -1;
      continue;
    // case 'U':
    //   data = ParseInt(data, 0, 0, 53, &week_num);
    //   week_start = weekday::sunday;
    //   continue;
    // case 'W':
    //   data = ParseInt(data, 0, 0, 53, &week_num);
    //   week_start = weekday::monday;
    //   continue;
    case 'u':
      data = ParseInt(data, 0, 1, 7, &tm_wday);
      if (data != NULL)
        tm_wday %= 7;
      continue;
    case 'w':
      data = ParseInt(data, 0, 0, 6, &tm_wday);
      continue;
    case 'H':
      data = ParseInt(data, 2, 0, 23, &tm_hour);
      twelve_hour = false;
      continue;
    case 'M':
      data = ParseInt(data, 2, 0, 59, &tm_min);
      continue;
    case 'S':
      data = ParseInt(data, 2, 0, 60, &tm_sec);
      continue;
    case 'I':
    case 'l':
    case 'r': // probably uses %I
      twelve_hour = true;
      break;
    case 'R': // uses %H
    case 'T': // uses %H
    case 'c': // probably uses %H
    case 'X': // probably uses %H
      twelve_hour = false;
      break;
    case 'z':
      data = ParseOffset(data, "", &offset);
      if (data != NULL)
        saw_offset = true;
      continue;
    case 'Z': // ignored; zone abbreviations are ambiguous
      data = ParseZone(data, zone);
      continue;
    case 's':
      data = ParseInt64(data, 0, LONG_MIN, LONG_MAX, &percent_s);
      if (data != NULL)
        saw_percent_s = true;
      continue;
    case ':':
      if (fmt[0] == 'z' ||
          (fmt[0] == ':' &&
           (fmt[1] == 'z' || (fmt[1] == ':' && fmt[2] == 'z')))) {
        data = ParseOffset(data, ":", &offset);
        if (data != NULL)
          saw_offset = true;
        fmt += (fmt[0] == 'z') ? 1 : (fmt[1] == 'z') ? 2 : 3;
        continue;
      }
      break;
    case '%':
      data = (*data == '%' ? data + 1 : NULL);
      continue;
    case 'E':
      if (fmt[0] == 'T') {
        if (*data == 'T' || *data == 't') {
          ++data;
          ++fmt;
        } else {
          data = NULL;
        }
        continue;
      }
      if (fmt[0] == 'z' || (fmt[0] == '*' && fmt[1] == 'z')) {
        data = ParseOffset(data, ":", &offset);
        if (data != NULL)
          saw_offset = true;
        fmt += (fmt[0] == 'z') ? 1 : 2;
        continue;
      }
      if (fmt[0] == '*' && fmt[1] == 'S') {
        data = ParseInt(data, 2, 0, 60, &tm_sec);
        if (data != NULL && *data == '.') {
          data = ParseSubSeconds(data + 1, &subseconds);
        }
        fmt += 2;
        continue;
      }
      if (fmt[0] == '*' && fmt[1] == 'f') {
        if (data != NULL && isdigit(*data)) {
          data = ParseSubSeconds(data, &subseconds);
        }
        fmt += 2;
        continue;
      }
      if (fmt[0] == '4' && fmt[1] == 'Y') {
        const char *bp = data;
        data = ParseInt(data, 4, -999, 9999, &year);
        if (data != NULL) {
          if (data - bp == 4) {
            saw_year = true;
          } else {
            data = NULL; // stopped too soon
          }
        }
        fmt += 2;
        continue;
      }
      if (isdigit(*fmt)) {
        int n = 0; // value ignored
        const char *np;
        if (np = ParseInt(fmt, 0, 0, 1024, &n)) {
          if (*np == 'S') {
            data = ParseInt(data, 2, 0, 60, &tm_sec);
            if (data != NULL && *data == '.') {
              data = ParseSubSeconds(data + 1, &subseconds);
            }
            fmt = ++np;
            continue;
          }
          if (*np == 'f') {
            if (data != NULL && isdigit(*data)) {
              data = ParseSubSeconds(data, &subseconds);
            }
            fmt = ++np;
            continue;
          }
        }
      }
      if (*fmt == 'c')
        twelve_hour = false; // probably uses %H
      if (*fmt == 'X')
        twelve_hour = false; // probably uses %H
      if (*fmt != '\0')
        ++fmt;
      break;
    case 'O':
      if (*fmt == 'H')
        twelve_hour = false;
      if (*fmt == 'I')
        twelve_hour = true;
      if (*fmt != '\0')
        ++fmt;
      break;
    }

// Parses the current specifier.
#if 0
    const char* orig_data = data;

    std::string spec(percent, static_cast<std::size_t>(fmt - percent));
    printf("spec is  %s\n",spec.c_str());
    data = ParseTM(data, spec.c_str(), &tm);

    // If we successfully parsed %p we need to remember whether the result
    // was AM or PM so that we can adjust tm_hour before time_zone::lookup().
    // So reparse the input with a known AM hour, and check if it is shifted
    // to a PM hour.
    if (spec == "%p" && data != NULL) {
      std::string test_input = "1";
      
      test_input.append(orig_data, static_cast<std::size_t>(data - orig_data));

      const char* test_data = test_input.c_str();
      std::tm tmp{};
      ParseTM(test_data, "%I%p", &tmp);
      afternoon = (tmp.tm_hour == 13);
    }
#endif
  }

  // Adjust a 12-hour tm_hour value if it should be in the afternoon.
  if (twelve_hour && afternoon && tm_hour < 12) {
    tm_hour += 12;
  }

  if (data == NULL) {
    if (err != NULL)
      *err = "Failed to parse input";
    return false;
  }

  // Skip any remaining whitespace.
  while (isspace(*data))
    ++data;

  // parse() must consume the entire input string.
  if (*data != '\0') {
    if (err != NULL)
      *err = "Illegal trailing data in input string";
    return false;
  }

  // If we saw %s then we ignore anything else and return that time.
  if (saw_percent_s) {
    *sec = percent_s;
    *fs = 0;
    return true;
  }

  // If we saw %z, %Ez, or %E*z then we want to interpret the parsed fields
  // in UTC and then shift by that offset.  Otherwise we want to interpret
  // the fields directly in the passed time_zone.
  // time_zone ptz = saw_offset ? utc_time_zone() ;

  // Allows a leap second of 60 to normalize forward to the following ":00".
  if (tm_sec == 60) {
    tm_sec -= 1;
    offset -= 1;
    subseconds = 0;
  }

  if (!saw_year) {
    year = tm_year;
    if (year > kyearmax - 1900) {
      // Platform-dependent, maybe unreachable.
      if (err != NULL)
        *err = "Out-of-range year";
      return false;
    }
    year += 1900;
  }

  // Compute year, tm.tm_mon and tm.tm_mday if we parsed a week number.
  // if (week_num != -1) {
  //   if (!FromWeek(week_num, week_start, &year, &tm)) {
  //     if (err != NULL) *err = "Out-of-range field";
  //     return false;
  //   }
  // }

  tm_mon = tm_mon + 1;

  // printf("year %d month %d day %d hour %d min %d sec %d msec %ld offset
  // %d\n",year,tm_mon,tm_mday,tm_hour,tm_min,tm_sec,subseconds,offset );
  // printf("kJulianDayOf1970_01_01 is %d \n",kJulianDayOf1970_01_01);

  *sec = (int64_t)(getJulianDayNumber(year, tm_mon, tm_mday) -
                   kJulianDayOf1970_01_01) *
             86400 +
         (tm_hour * 3600 + tm_min * 60 + tm_sec) - offset;
  *fs = subseconds;

  // printf("sec  is %ld \n",*sec);

  return true;

#if 0
  civil_second cs(year, month, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

  // parse() should not allow normalization. Due to the restricted field
  // ranges above (see ParseInt()), the only possibility is for days to roll
  // into months. That is, parsing "Sep 31" should not produce "Oct 1".
  if (cs.month() != month || cs.day() != tm.tm_mday) {
    if (err != NULL) *err = "Out-of-range field";
    return false;
  }

  // Accounts for the offset adjustment before converting to absolute time.
  if ((offset < 0 && cs > civil_second::max() + offset) ||
      (offset > 0 && cs < civil_second::min() + offset)) {
    if (err != NULL) *err = "Out-of-range field";
    return false;
  }
   cs -= offset;
#endif

#if 0
  const auto tp = ptz.lookup(cs).pre;
  // Checks for overflow/underflow and returns an error as necessary.
  if (tp == time_point<seconds>::max()) {
    const auto al = ptz.lookup(time_point<seconds>::max());
    if (cs > al.cs) {
      if (err != NULL) *err = "Out-of-range field";
      return false;
    }
  }
  if (tp == time_point<seconds>::min()) {
    const auto al = ptz.lookup(time_point<seconds>::min());
    if (cs < al.cs) {
      if (err != NULL) *err = "Out-of-range field";
      return false;
    }
  }
#endif
}

#if 0
int main(int argc , char **argv){

  int64_t t;
  int64_t e;
  const char *err = NULL;

  parse("%Y-%m-%d %H:%M:%E*S", "2013-06-28 19:08:09.245", &t, &e,&err);

  parse("%Y-%m-%d %H:%M:%S", "2013-06-28 19:08:09", &t, &e,&err);

 

  printf("erro0 is %s \n",err);


  // But the timezone is ignored when a UTC offset is present.
  parse("%Y-%m-%d %H:%M:%E*S %z", "2013-06-28 19:08:09.489 +0800", &t, &e,&err);

  printf("erro1 is %s \n",err);

  return 0;

}
#endif
