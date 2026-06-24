#pragma once
#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Core Parsing Functions
// ============================================================================

/**
 * Parse time string
 *
 * @param format Format string, supports standard POSIX format and extended format:
 *   - %Y: year (4 digits)
 *   - %m: month (01-12)
 *   - %d: day (01-31)
 *   - %H: hour (00-23)
 *   - %M: minute (00-59)
 *   - %S: second (00-60, 60 for leap second)
 *   - %z: timezone offset (+/-hhmm)
 *   - %Ez: timezone offset (+/-hh:mm)
 *   - %E*S: second with subsecond precision (e.g.: 09.123456789)
 *   - %s: Unix timestamp
 * @param input Input time string
 * @param sec Output: Unix timestamp (seconds)
 * @param fs Output: Nanoseconds part (0-999999999)
 * @param err Output: Error message (can be NULL)
 * @return Returns true on success, false on failure
 */
EXPORT bool parse(const char *format, const char *input, int64_t *sec, int64_t *fs, const char **err);

// ============================================================================
// Convenience Parsing Functions - Support Common Time Formats
// ============================================================================

/**
 * Parse ISO8601 format time
 * Supported formats: 2013-06-28T19:08:09.245Z, 2013-06-28T19:08:09+08:00
 */
EXPORT bool parse_iso8601(const char *input, int64_t *sec, int64_t *nsec, const char **err);

/**
 * Parse RFC3339 format time
 * Supported formats: 2013-06-28T19:08:09+08:00
 */
EXPORT bool parse_rfc3339(const char *input, int64_t *sec, int64_t *nsec, const char **err);

/**
 * Parse common log format time
 * Supported formats: 2013-06-28 19:08:09.245, 2013-06-28 19:08:09
 */
EXPORT bool parse_log_time(const char *input, int64_t *sec, int64_t *nsec, const char **err);

/**
 * Parse Syslog format time
 * Note: syslog format does not include year, this function is not fully supported
 */
EXPORT bool parse_syslog_time(const char *input, int64_t *sec, int64_t *nsec, const char **err);

/**
 * Parse Apache/CLF format time
 * Supported formats: 28/Jun/2013:19:08:09 +0800
 */
EXPORT bool parse_clf_time(const char *input, int64_t *sec, int64_t *nsec, const char **err);

/**
 * Auto-detect and parse time string
 * Supported formats: ISO8601, RFC3339, log format, Unix timestamp
 */
EXPORT bool parse_auto(const char *input, int64_t *sec, int64_t *nsec, const char **err);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Check if a year is a leap year
 */
EXPORT bool is_leap_year(int year);

/**
 * Get the number of days in a specific month
 * @param year Year (used to determine leap year)
 * @param month Month (1-12)
 * @return Number of days in the month, returns 0 on error
 */
EXPORT int get_days_in_month(int year, int month);

/**
 * Validate if a date is valid
 */
EXPORT bool is_valid_date(int year, int month, int day);

/**
 * Validate if a time is valid
 */
EXPORT bool is_valid_time(int hour, int minute, int second);

/**
 * Format timestamp to string
 * @param buf Output buffer
 * @param size Buffer size
 * @param format Format string (supports: %Y-%m-%d %H:%M:%S, %Y-%m-%dT%H:%M:%S etc.)
 * @param sec Unix timestamp (seconds)
 * @param nsec Nanoseconds part
 * @return Returns number of characters written on success, -1 on failure
 */
EXPORT int format_time(char *buf, size_t size, const char *format, int64_t sec, int64_t nsec);

/**
 * Calculate Julian day number
 */
EXPORT int getJulianDayNumber(int year, int month, int day);

// ============================================================================
// Internal Functions (usually not needed to call directly)
// ============================================================================

/**
 * Parse integer
 */
EXPORT const char *ParseInt(const char *dp, int width, int min, int max, int *vp);

/**
 * Parse 64-bit integer
 */
EXPORT const char *ParseInt64(const char *dp, int width, int64_t min, int64_t max, int64_t *vp);

/**
 * Parse timezone offset
 */
EXPORT const char *ParseOffset(const char *dp, const char *mode, int *offset);

/**
 * Parse timezone name
 */
EXPORT const char *ParseZone(const char *dp, char *zone);

/**
 * Parse subseconds part
 * @param dp Input pointer
 * @param subseconds Output: nanosecond value (0-999999999)
 * @param precision Output: original precision digits (can be NULL)
 */
EXPORT const char *ParseSubSeconds(const char *dp, int64_t *subseconds, int *precision);

/**
 * Format 64-bit integer
 */
EXPORT char *Format64(char *ep, int width, int64_t v);

/**
 * Format two-digit number
 */
EXPORT char *Format02d(char *ep, int v);

/**
 * Format timezone offset
 */
EXPORT char *FormatOffset(char *ep, int offset, const char *mode);

#ifdef __cplusplus
}
#endif
