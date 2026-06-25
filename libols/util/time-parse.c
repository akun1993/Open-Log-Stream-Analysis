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

#include <stdlib.h>
#ifdef _MSC_VER
#define strtoll _strtoi64
#endif

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Compile-time assertion for integer size
static const char require_32_bit_integer_at_least[sizeof(int) >= sizeof(int32_t) ? 1 : -1] = {0};

// Days per month (non-leap year)
static const int8_t kDaysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Julian day of Unix epoch (1970-01-01)
static const int kJulianDayOf1970_01_01 = 2440588;

// Maximum base-10 digits for signed 64-bit integer
#define kDigits10_64 (18)

// Powers of 10 table (static const for better optimization)
static const int64_t kExp10[kDigits10_64 + 1] = {1,
                                                 10,
                                                 100,
                                                 1000,
                                                 10000,
                                                 100000,
                                                 1000000,
                                                 10000000,
                                                 100000000,
                                                 1000000000,
                                                 10000000000LL,
                                                 100000000000LL,
                                                 1000000000000LL,
                                                 10000000000000LL,
                                                 100000000000000LL,
                                                 1000000000000000LL,
                                                 10000000000000000LL,
                                                 100000000000000000LL,
                                                 1000000000000000000LL};

// ============================================================================
// Utility Functions
// ============================================================================

bool is_leap_year(int year) {
    // Optimized: bit operations reduce branch predictions
    return ((year & 3) == 0) && ((year % 100) != 0 || (year % 400) == 0);
}

int get_days_in_month(int year, int month) {
    // Single unsigned comparison for range check
    if ((unsigned int)(month - 1) >= 12) return 0;
    if (month == 2 && is_leap_year(year)) return 29;
    return kDaysPerMonth[month - 1];
}

bool is_valid_date(int year, int month, int day) {
    if ((unsigned int)(month - 1) >= 12) return false;
    if (day < 1) return false;
    if (year < -999 || year > 9999) return false;
    return (unsigned int)day <= (unsigned int)get_days_in_month(year, month);
}

bool is_valid_time(int hour, int minute, int second) {
    // Unsigned comparisons for efficient range checks
    return ((unsigned int)hour < 24) && ((unsigned int)minute < 60) && ((unsigned int)second <= 60);  // 60 for leap second
}

int getJulianDayNumber(int year, int month, int day) {
    (void)require_32_bit_integer_at_least;  // Suppress unused warning
    const int a = (14 - month) / 12;
    const int y = year + 4800 - a;
    const int m = month + 12 * a - 3;
    return day + (153 * m + 2) / 5 + y * 365 + y / 4 - y / 100 + y / 400 - 32045;
}

// ============================================================================
// Formatting Functions
// ============================================================================

// Format 64-bit integer backwards into buffer (helper for Format64/Format02d)
static char *format_uint64_backwards(char *ep, uint64_t v, int *width) {
    do {
        *--ep = (char)('0' + (v % 10));
        if (width) --*width;
        v /= 10;
    } while (v != 0);
    return ep;
}

char *Format64(char *ep, int width, int64_t v) {
    bool neg = false;

    if (v < 0) {
        --width;
        neg = true;
        // Handle INT64_MIN specially to avoid overflow
        if (v == INT64_MIN) {
            *--ep = '8';
            v = INT64_MIN / 10;
            --width;
        }
        v = -v;
    }

    ep = format_uint64_backwards(ep, (uint64_t)v, &width);

    // Zero pad
    while (width-- > 0) {
        *--ep = '0';
    }

    if (neg) {
        *--ep = '-';
    }

    return ep;
}

char *Format02d(char *ep, int v) {
    // Clamp to valid range
    if (v < 0) v = 0;
    if (v > 99) v = 99;

    *--ep = (char)('0' + (v % 10));
    *--ep = (char)('0' + (v / 10));
    return ep;
}

char *FormatOffset(char *ep, int offset, const char *mode) {
    char sign = '+';
    if (offset < 0) {
        offset = -offset;
        sign = '-';
    }

    const int seconds = offset % 60;
    const int minutes = (offset / 60) % 60;
    const int hours = offset / 3600;

    const char sep = mode[0];
    const bool extended = (sep != '\0' && mode[1] == '*');
    const bool compact = (extended && mode[2] == ':');

    // Format seconds if needed
    if (extended && (!compact || seconds != 0)) {
        ep = Format02d(ep, seconds);
        *--ep = sep;
    } else if (hours == 0 && minutes == 0) {
        sign = '+';  // Sub-minute negative offsets show as "+00:00"
    }

    // Format minutes
    if (!compact || minutes != 0 || seconds != 0) {
        ep = Format02d(ep, minutes);
        if (sep != '\0') {
            *--ep = sep;
        }
    }

    // Format hours and sign
    ep = Format02d(ep, hours);
    *--ep = sign;

    return ep;
}

// ============================================================================
// Parsing Functions
// ============================================================================

static const char *ParseIntGeneric(const char *dp, int width, int64_t min, int64_t max, int64_t *vp) {
    if (dp == NULL) return NULL;

    bool neg = false;
    int64_t value = 0;

    // Handle optional sign
    if (*dp == '-') {
        neg = true;
        if (width > 0 && --width == 0) return NULL;
        ++dp;
    }

    const char *const bp = dp;

    // Parse digits with overflow detection
    while (*dp >= '0' && *dp <= '9') {
        const int d = *dp - '0';

        // Overflow check: value * 10 - d must not underflow
        if (value < (INT64_MIN / 10)) return NULL;
        value *= 10;
        if (value < (INT64_MIN + d)) return NULL;
        value -= d;

        ++dp;
        if (width > 0 && --width == 0) break;
    }

    // Must parse at least one digit
    if (dp == bp) return NULL;

    // Negative zero is invalid
    if (neg && value == 0) return NULL;

    // Handle INT64_MIN special case for positive numbers
    if (!neg && value == INT64_MIN) return NULL;

    // Convert to positive
    if (!neg) value = -value;

    // Range check
    if (value < min || value > max) return NULL;

    *vp = value;
    return dp;
}

const char *ParseInt(const char *dp, int width, int min, int max, int *vp) {
    int64_t value;
    const char *result = ParseIntGeneric(dp, width, min, max, &value);
    if (result != NULL) {
        *vp = (int)value;
    }
    return result;
}

const char *ParseInt64(const char *dp, int width, int64_t min, int64_t max, int64_t *vp) {
    return ParseIntGeneric(dp, width, min, max, vp);
}

const char *ParseOffset(const char *dp, const char *mode, int *offset) {
    if (dp == NULL) return NULL;

    const char first = *dp++;

    // Handle Zulu timezone (UTC)
    if (first == 'Z' || first == 'z') {
        *offset = 0;
        return dp;
    }

    if (first != '+' && first != '-') return NULL;

    const char sep = mode[0];
    int hours = 0, minutes = 0, seconds = 0;

    // Parse hours (exactly 2 digits)
    const char *ap = ParseInt(dp, 2, 0, 23, &hours);
    if (ap == NULL || ap - dp != 2) return NULL;
    dp = ap;

    // Skip separator if present
    if (sep != '\0' && *ap == sep) ++ap;

    // Parse minutes (exactly 2 digits)
    const char *bp = ParseInt(ap, 2, 0, 59, &minutes);
    if (bp == NULL || bp - ap != 2) {
        *offset = hours * 3600;
        if (first == '-') *offset = -*offset;
        return dp;
    }
    dp = bp;

    // Skip separator and parse seconds (optional)
    if (sep != '\0' && *bp == sep) ++bp;
    const char *cp = ParseInt(bp, 2, 0, 59, &seconds);
    if (cp != NULL && cp - bp == 2) {
        dp = cp;
    }

    *offset = ((hours * 60 + minutes) * 60) + seconds;
    if (first == '-') *offset = -*offset;

    return dp;
}

const char *ParseZone(const char *dp, char *zone) {
    if (dp == NULL) return NULL;

    int idx = 0;
    while (*dp != '\0' && !isspace((unsigned char)*dp)) {
        zone[idx++] = *dp++;
    }
    zone[idx] = '\0';

    return (idx == 0) ? NULL : dp;
}

const char *ParseSubSeconds(const char *dp, int64_t *subseconds, int *precision) {
    if (dp == NULL) return NULL;

    int64_t v = 0;
    int digits = 0;

    // Parse up to 9 digits for nanosecond precision
    while (digits < 9 && *dp >= '0' && *dp <= '9') {
        v = v * 10 + (*dp - '0');
        ++digits;
        ++dp;
    }

    // Skip additional digits
    while (*dp >= '0' && *dp <= '9') ++dp;

    if (digits == 0) return NULL;

    // Pad to nanoseconds
    if (digits < 9) {
        v *= kExp10[9 - digits];
    }

    *subseconds = v;
    if (precision) *precision = digits;
    return dp;
}

// ============================================================================
// Main Parse Function
// ============================================================================

bool parse(const char *format, const char *input, int64_t *sec, int64_t *fs, const char **err) {
    if (format == NULL || input == NULL || sec == NULL || fs == NULL) {
        if (err) *err = "NULL argument";
        return false;
    }

    const char *data = input;
    const char *fmt = format;

    // Skip leading whitespace
    while (isspace((unsigned char)*data)) ++data;

    // Year range constants
    enum { kYearMin = -999, kYearMax = 9999 };

    // Default values
    bool saw_year = false;
    int year = 1970;
    int tm_mon = 0, tm_mday = 1;
    int tm_hour = 0, tm_min = 0, tm_sec = 0;
    int tm_wday = 4;  // Thursday (Unix epoch)
    int64_t subseconds = 0;
    int offset = 0;
    bool saw_percent_s = false;
    int64_t percent_s = 0;
    bool twelve_hour = false;
    bool afternoon = false;

    while (data != NULL && *fmt != '\0') {
        // Handle whitespace
        if (isspace((unsigned char)*fmt)) {
            while (isspace((unsigned char)*data)) ++data;
            while (isspace((unsigned char)*++fmt));
            continue;
        }

        // Match literal characters
        if (*fmt != '%') {
            if (*data == *fmt) {
                ++data;
                ++fmt;
            } else {
                data = NULL;
            }
            continue;
        }

        // Handle format specifier
        if (*++fmt == '\0') {
            data = NULL;
            break;
        }

        switch (*fmt++) {
            case 'Y':
                data = ParseInt(data, 0, kYearMin, kYearMax, &year);
                if (data) saw_year = true;
                continue;
            case 'm':
                data = ParseInt(data, 2, 1, 12, &tm_mon);
                if (data) tm_mon -= 1;
                continue;
            case 'd':
            case 'e':
                data = ParseInt(data, 2, 1, 31, &tm_mday);
                continue;
            case 'u':
                data = ParseInt(data, 0, 1, 7, &tm_wday);
                if (data) tm_wday %= 7;
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
            case 'r':
                twelve_hour = true;
                break;
            case 'R':
            case 'T':
            case 'c':
            case 'X':
                twelve_hour = false;
                break;
            case 'z':
                data = ParseOffset(data, "", &offset);
                continue;
            case 'Z':
                data = ParseZone(data, (char[64]){0});
                continue;
            case 's':
                data = ParseInt64(data, 0, INT64_MIN, INT64_MAX, &percent_s);
                if (data) saw_percent_s = true;
                continue;
            case ':':
                if (fmt[0] == 'z' || (fmt[0] == ':' && (fmt[1] == 'z' || (fmt[1] == ':' && fmt[2] == 'z')))) {
                    data = ParseOffset(data, ":", &offset);
                    fmt += (fmt[0] == 'z') ? 1 : (fmt[1] == 'z') ? 2 : 3;
                    continue;
                }
                break;
            case '%':
                data = (*data == '%') ? data + 1 : NULL;
                continue;
            case 'E':
                if (fmt[0] == 'T') {
                    data = (*data == 'T' || *data == 't') ? data + 1 : NULL;
                    ++fmt;
                    continue;
                }
                if (fmt[0] == 'z' || (fmt[0] == '*' && fmt[1] == 'z')) {
                    data = ParseOffset(data, ":", &offset);
                    fmt += (fmt[0] == 'z') ? 1 : 2;
                    continue;
                }
                if (fmt[0] == '*' && fmt[1] == 'S') {
                    data = ParseInt(data, 2, 0, 60, &tm_sec);
                    if (data && *data == '.') {
                        data = ParseSubSeconds(data + 1, &subseconds, NULL);
                    }
                    fmt += 2;
                    continue;
                }
                if (fmt[0] == '*' && fmt[1] == 'f') {
                    if (data && isdigit((unsigned char)*data)) {
                        data = ParseSubSeconds(data, &subseconds, NULL);
                    }
                    fmt += 2;
                    continue;
                }
                if (fmt[0] == '4' && fmt[1] == 'Y') {
                    const char *bp = data;
                    data = ParseInt(data, 4, -999, 9999, &year);
                    if (data && data - bp == 4) {
                        saw_year = true;
                    } else {
                        data = NULL;
                    }
                    fmt += 2;
                    continue;
                }
                if (isdigit((unsigned char)*fmt)) {
                    int n = 0;
                    const char *np = ParseInt(fmt, 0, 0, 9, &n);
                    if (np && (*np == 'S' || *np == 'f')) {
                        data = ParseInt(data, 2, 0, 60, &tm_sec);
                        if (data && *data == '.') {
                            data = ParseSubSeconds(data + 1, &subseconds, NULL);
                        }
                        fmt = ++np;
                        continue;
                    }
                }
                if (*fmt == 'c' || *fmt == 'X') twelve_hour = false;
                if (*fmt) ++fmt;
                break;
            case 'O':
                if (*fmt == 'H') twelve_hour = false;
                if (*fmt == 'I') twelve_hour = true;
                if (*fmt) ++fmt;
                break;
        }
    }

    // Adjust 12-hour time for afternoon
    if (twelve_hour && afternoon && tm_hour < 12) {
        tm_hour += 12;
    }

    if (data == NULL) {
        if (err) *err = "Failed to parse input";
        return false;
    }

    // Skip trailing whitespace
    while (isspace((unsigned char)*data)) ++data;

    if (*data != '\0') {
        if (err) *err = "Trailing characters in input";
        return false;
    }

    // Return Unix timestamp directly if %s was used
    if (saw_percent_s) {
        *sec = percent_s;
        *fs = 0;
        return true;
    }

    // Handle leap second
    if (tm_sec == 60) {
        tm_sec = 59;
        offset -= 1;
        subseconds = 0;
    }

    // Default year from tm_year if not seen
    if (!saw_year) {
        year = 1970;
    }

    // Validate date
    const int month = tm_mon + 1;
    if (!is_valid_date(year, month, tm_mday)) {
        if (err) *err = "Invalid date";
        return false;
    }

    // Validate time
    if (!is_valid_time(tm_hour, tm_min, tm_sec)) {
        if (err) *err = "Invalid time";
        return false;
    }

    // Calculate Unix timestamp
    *sec = (int64_t)(getJulianDayNumber(year, month, tm_mday) - kJulianDayOf1970_01_01) * 86400 + tm_hour * 3600 + tm_min * 60 + tm_sec -
           offset;
    *fs = subseconds;

    return true;
}

// ============================================================================
// Format Time Function
// ============================================================================

// Helper: append formatted number to buffer
static inline void append_formatted(char *buf, size_t *out_pos, size_t size, const char *start, const char *end) {
    while (start < end && *out_pos < size - 1) {
        buf[(*out_pos)++] = *start++;
    }
}

int format_time(char *buf, size_t size, const char *format, int64_t sec, int64_t nsec) {
    if (buf == NULL || size == 0 || format == NULL) return -1;

    // Convert to UTC
    time_t time_sec = (time_t)sec;
    struct tm tm;

#ifdef _WIN32
    gmtime_s(&tm, &time_sec);
#else
    gmtime_r(&time_sec, &tm);
#endif

    char num_buf[3 + kDigits10_64];
    char *const ep = num_buf + sizeof(num_buf);
    size_t out_pos = 0;

    for (const char *cur = format; *cur && out_pos < size - 1; ++cur) {
        if (*cur != '%') {
            buf[out_pos++] = *cur;
            continue;
        }

        if (*++cur == '\0') break;

        char *bp = NULL;
        switch (*cur) {
            case 'Y':
                bp = Format64(ep, 4, tm.tm_year + 1900);
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'm':
                bp = Format02d(ep, tm.tm_mon + 1);
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'd':
                bp = Format02d(ep, tm.tm_mday);
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'e':
                bp = Format02d(ep, tm.tm_mday);
                if (*bp == '0') *bp = ' ';
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'H':
                bp = Format02d(ep, tm.tm_hour);
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'M':
                bp = Format02d(ep, tm.tm_min);
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'S':
                bp = Format02d(ep, tm.tm_sec);
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'f':
                bp = Format64(ep, 9, nsec);
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 's':
                bp = Format64(ep, 0, sec);
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'z':
                bp = FormatOffset(ep, 0, "");
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'w':
                bp = Format64(ep, 0, tm.tm_wday);
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'u':
                bp = Format64(ep, 0, tm.tm_wday == 0 ? 7 : tm.tm_wday);
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'j':
                bp = Format64(ep, 3, tm.tm_yday + 1);
                append_formatted(buf, &out_pos, size, bp, ep);
                break;
            case 'T':
                if (out_pos < size - 1) buf[out_pos++] = 'T';
                break;
            case 'Z':
                if (out_pos < size - 1) buf[out_pos++] = 'Z';
                break;
            case '%':
                if (out_pos < size - 1) buf[out_pos++] = '%';
                break;
            case ':':
                if (cur[1] == 'z') {
                    bp = FormatOffset(ep, 0, ":");
                    append_formatted(buf, &out_pos, size, bp, ep);
                    ++cur;
                } else if (cur[1] == ':' && cur[2] == 'z') {
                    bp = FormatOffset(ep, 0, ":*");
                    append_formatted(buf, &out_pos, size, bp, ep);
                    cur += 2;
                } else if (cur[1] == ':' && cur[2] == ':' && cur[3] == 'z') {
                    bp = FormatOffset(ep, 0, ":*:");
                    append_formatted(buf, &out_pos, size, bp, ep);
                    cur += 3;
                } else {
                    if (out_pos < size - 1) buf[out_pos++] = ':';
                }
                break;
            case 'E':
                ++cur;
                if (*cur == 'z') {
                    bp = FormatOffset(ep, 0, ":");
                    append_formatted(buf, &out_pos, size, bp, ep);
                } else if (*cur == '*' && cur[1] == 'z') {
                    bp = FormatOffset(ep, 0, ":*");
                    append_formatted(buf, &out_pos, size, bp, ep);
                    ++cur;
                } else if (*cur == '*' && (cur[1] == 'S' || cur[1] == 'f')) {
                    bp = Format64(ep, 9, nsec);
                    char *cp = ep;
                    while (cp != bp && cp[-1] == '0') --cp;
                    if (cur[1] == 'S') {
                        if (cp != bp) *--bp = '.';
                        bp = Format02d(bp, tm.tm_sec);
                    } else if (cp == bp) {
                        *--bp = '0';
                    }
                    append_formatted(buf, &out_pos, size, bp, cp);
                    ++cur;
                } else if (*cur == '4' && cur[1] == 'Y') {
                    bp = Format64(ep, 4, tm.tm_year + 1900);
                    append_formatted(buf, &out_pos, size, bp, ep);
                    ++cur;
                } else if (*cur == 'T') {
                    if (out_pos < size - 1) buf[out_pos++] = 'T';
                } else if (isdigit((unsigned char)*cur)) {
                    int n = 0;
                    const char *np = ParseInt(cur, 0, 0, 9, &n);
                    if (np && (*np == 'S' || *np == 'f')) {
                        if (n > 0) {
                            if (n > 9) n = 9;
                            int64_t scaled = (n > 9) ? nsec * kExp10[n - 9] : nsec / kExp10[9 - n];
                            bp = Format64(ep, n, scaled);
                            if (*np == 'S') *--bp = '.';
                        }
                        if (*np == 'S') bp = Format02d(bp, tm.tm_sec);
                        append_formatted(buf, &out_pos, size, bp, ep);
                        cur = np;
                    }
                }
                break;
            default:
                if (out_pos < size - 1) buf[out_pos++] = *cur;
                break;
        }
    }

    buf[out_pos] = '\0';
    return (int)out_pos;
}

// ============================================================================
// Convenience Parsing Functions
// ============================================================================

static bool parse_with_detection(const char *input, int64_t *sec, int64_t *nsec, const char **err, char date_sep, char time_sep) {
    if (input == NULL || sec == NULL || nsec == NULL) {
        if (err) *err = "NULL argument";
        return false;
    }

    // Detect format characteristics
    bool has_tz = false, has_zulu = false, has_subseconds = false;

    // First, find where the time part starts (after the date separator)
    // For ISO8601: after 'T', for log format: after space
    const char *time_start = NULL;
    for (const char *p = input; *p; ++p) {
        if (*p == time_sep || (time_sep == '\0' && *p == ' ')) {
            time_start = p + 1;
            break;
        }
    }

    for (const char *p = input; *p; ++p) {
        if (*p == '.') {
            has_subseconds = true;
        } else if (*p == 'Z' || *p == 'z') {
            has_zulu = has_tz = true;
            break;
        } else if (*p == '+' && time_start && p > time_start) {
            // Timezone '+' must be in the time part
            has_tz = true;
            break;
        } else if (*p == '-' && time_start && p > time_start) {
            // Timezone '-' must be in the time part (after seconds)
            const char prev = *(p - 1);
            if (prev == ':' || (isdigit((unsigned char)prev) && p - time_start > 5)) {
                // After seconds: either ':' before, or digit with enough time components parsed
                has_tz = true;
                break;
            }
        }
    }

    // Build format string
    char format[64];
    int i = 0;

    // Date part: %Y<sep>%m<sep>%d
    format[i++] = '%';
    format[i++] = 'Y';
    format[i++] = date_sep;
    format[i++] = '%';
    format[i++] = 'm';
    format[i++] = date_sep;
    format[i++] = '%';
    format[i++] = 'd';

    // Time separator
    format[i++] = (time_sep != '\0') ? time_sep : ' ';

    // Time part: %H:%M:%S
    format[i++] = '%';
    format[i++] = 'H';
    format[i++] = ':';
    format[i++] = '%';
    format[i++] = 'M';
    format[i++] = ':';

    // Seconds
    if (has_subseconds) {
        format[i++] = '%';
        format[i++] = 'E';
        format[i++] = '*';
        format[i++] = 'S';
    } else {
        format[i++] = '%';
        format[i++] = 'S';
    }

    // Timezone
    if (has_zulu) {
        format[i++] = 'Z';
    } else if (has_tz) {
        format[i++] = '%';
        format[i++] = 'E';
        format[i++] = 'z';
    }

    format[i] = '\0';
    return parse(format, input, sec, nsec, err);
}

bool parse_iso8601(const char *input, int64_t *sec, int64_t *nsec, const char **err) {
    return parse_with_detection(input, sec, nsec, err, '-', 'T');
}

bool parse_rfc3339(const char *input, int64_t *sec, int64_t *nsec, const char **err) {
    return parse_iso8601(input, sec, nsec, err);
}

bool parse_log_time(const char *input, int64_t *sec, int64_t *nsec, const char **err) {
    return parse_with_detection(input, sec, nsec, err, '-', ' ');
}

bool parse_syslog_time(const char *input, int64_t *sec, int64_t *nsec, const char **err) {
    (void)input;
    (void)sec;
    (void)nsec;
    if (err) *err = "Syslog format not supported (missing year)";
    return false;
}

bool parse_clf_time(const char *input, int64_t *sec, int64_t *nsec, const char **err) {
    return parse("%d/%b/%Y:%H:%M:%S %z", input, sec, nsec, err);
}

bool parse_auto(const char *input, int64_t *sec, int64_t *nsec, const char **err) {
    if (input == NULL || sec == NULL || nsec == NULL) {
        if (err) *err = "NULL argument";
        return false;
    }

    // Skip whitespace
    while (isspace((unsigned char)*input)) ++input;

    if (*input == '\0') {
        if (err) *err = "Empty input";
        return false;
    }

    // Check for Unix timestamp (numeric string)
    if (isdigit((unsigned char)*input) || (*input == '-' && isdigit((unsigned char)input[1]))) {
        char *endptr;
        int64_t ts = strtoll(input, &endptr, 10);

        if (*endptr == '\0') {
            *sec = ts;
            *nsec = 0;
            return true;
        }

        if (*endptr == '.') {
            *sec = ts;
            int64_t subsec = 0;
            int digits = 0;
            ++endptr;

            while (isdigit((unsigned char)*endptr) && digits < 9) {
                subsec = subsec * 10 + (*endptr++ - '0');
                ++digits;
            }

            while (digits++ < 9) subsec *= 10;
            *nsec = subsec;
            return true;
        }
    }

    // Check for CLF format (DD/Mon/YYYY)
    if (isdigit((unsigned char)input[0]) && isdigit((unsigned char)input[1]) && input[2] == '/') {
        return parse_clf_time(input, sec, nsec, err);
    }

    // Check for ISO8601 (contains 'T')
    if (strchr(input, 'T')) {
        return parse_iso8601(input, sec, nsec, err);
    }

    // Try log format (space separator)
    if (strchr(input, ' ')) {
        return parse_log_time(input, sec, nsec, err);
    }

    if (err) *err = "Unknown time format";
    return false;
}
