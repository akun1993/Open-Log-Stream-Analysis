// 简单的时间解析测试
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 直接包含源文件（简化编译）
#include "../../libols/util/time-parse.c"

// 测试计数器
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("Testing: %s... ", name)
#define PASS()              \
    do {                    \
        printf("PASSED\n"); \
        tests_passed++;     \
    } while (0)
#define FAIL(msg)                    \
    do {                             \
        printf("FAILED: %s\n", msg); \
        tests_failed++;              \
    } while (0)
#define CHECK(cond)      \
    do {                 \
        if (cond) {      \
            PASS();      \
        } else {         \
            FAIL(#cond); \
        }                \
    } while (0)
#define CHECK_EQ(expected, actual, msg)                                                                        \
    do {                                                                                                       \
        if ((expected) == (actual)) {                                                                          \
            PASS();                                                                                            \
        } else {                                                                                               \
            printf("FAILED: %s (expected %lld, got %lld)\n", msg, (long long)(expected), (long long)(actual)); \
            tests_failed++;                                                                                    \
        }                                                                                                      \
    } while (0)

int main(int argc, char **argv) {
    printf("==================================================\n");
    printf("Time Parse Library - Simple Test Suite\n");
    printf("==================================================\n\n");

    int64_t sec, nsec;
    const char *err;

    // ====== 日期验证测试 ======
    printf("=== Date Validation Tests ===\n");

    TEST("is_leap_year(2000)");
    CHECK(is_leap_year(2000) == true);

    TEST("is_leap_year(2004)");
    CHECK(is_leap_year(2004) == true);

    TEST("is_leap_year(1900)");
    CHECK(is_leap_year(1900) == false);

    TEST("is_leap_year(2001)");
    CHECK(is_leap_year(2001) == false);

    TEST("get_days_in_month(2023, 2)");
    CHECK_EQ(28, get_days_in_month(2023, 2), "Feb 2023 days");

    TEST("get_days_in_month(2024, 2)");
    CHECK_EQ(29, get_days_in_month(2024, 2), "Feb 2024 days (leap year)");

    TEST("is_valid_date(2023, 6, 28)");
    CHECK(is_valid_date(2023, 6, 28) == true);

    TEST("is_valid_date(2023, 2, 29)");
    CHECK(is_valid_date(2023, 2, 29) == false);  // 非闰年

    TEST("is_valid_time(12, 30, 45)");
    CHECK(is_valid_time(12, 30, 45) == true);

    TEST("is_valid_time(24, 0, 0)");
    CHECK(is_valid_time(24, 0, 0) == false);

    // ====== 基本解析测试 ======
    printf("\n=== Basic Parsing Tests ===\n");

    TEST("parse basic date time");
    err = NULL;
    if (parse("%Y-%m-%d %H:%M:%S", "2013-06-28 19:08:09", &sec, &nsec, &err)) {
        CHECK_EQ(1372414089LL, sec, "seconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("parse with milliseconds");
    err = NULL;
    if (parse("%Y-%m-%d %H:%M:%E*S", "2013-06-28 19:08:09.245", &sec, &nsec, &err)) {
        CHECK_EQ(245000000LL, nsec, "nanoseconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("parse with microseconds");
    err = NULL;
    if (parse("%Y-%m-%d %H:%M:%E*S", "2013-06-28 19:08:09.123456", &sec, &nsec, &err)) {
        CHECK_EQ(123456000LL, nsec, "nanoseconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("parse with nanoseconds");
    err = NULL;
    if (parse("%Y-%m-%d %H:%M:%E*S", "2013-06-28 19:08:09.123456789", &sec, &nsec, &err)) {
        CHECK_EQ(123456789LL, nsec, "nanoseconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("parse with timezone +0800");
    err = NULL;
    if (parse("%Y-%m-%d %H:%M:%S %z", "2013-06-28 19:08:09 +0800", &sec, &nsec, &err)) {
        CHECK_EQ(1372384089LL, sec, "seconds with timezone");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("parse with timezone -0500");
    err = NULL;
    if (parse("%Y-%m-%d %H:%M:%S %z", "2013-06-28 19:08:09 -0500", &sec, &nsec, &err)) {
        CHECK_EQ(1372426089LL, sec, "seconds with negative timezone");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("parse with Z timezone");
    err = NULL;
    if (parse("%Y-%m-%d %H:%M:%S %z", "2013-06-28 19:08:09 Z", &sec, &nsec, &err)) {
        CHECK_EQ(1372414089LL, sec, "seconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("parse unix timestamp");
    err = NULL;
    if (parse("%s", "1372414089", &sec, &nsec, &err)) {
        CHECK_EQ(1372414089LL, sec, "seconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    // ====== ISO8601 解析测试 ======
    printf("\n=== ISO8601 Parsing Tests ===\n");

    TEST("ISO8601 basic (Z)");
    err = NULL;
    if (parse_iso8601("2013-06-28T19:08:09Z", &sec, &nsec, &err)) {
        CHECK_EQ(1372414089LL, sec, "seconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("ISO8601 with milliseconds");
    err = NULL;
    if (parse_iso8601("2013-06-28T19:08:09.245Z", &sec, &nsec, &err)) {
        CHECK_EQ(245000000LL, nsec, "nanoseconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("ISO8601 with timezone +08:00");
    err = NULL;
    if (parse_iso8601("2013-06-28T19:08:09+08:00", &sec, &nsec, &err)) {
        CHECK_EQ(1372384089LL, sec, "seconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    // ====== 日志格式解析测试 ======
    printf("\n=== Log Time Parsing Tests ===\n");

    TEST("log time basic");
    err = NULL;
    if (parse_log_time("2013-06-28 19:08:09", &sec, &nsec, &err)) {
        CHECK_EQ(1372414089LL, sec, "seconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("log time with milliseconds");
    err = NULL;
    if (parse_log_time("2013-06-28 19:08:09.123", &sec, &nsec, &err)) {
        CHECK_EQ(123000000LL, nsec, "nanoseconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    // ====== 自动检测解析测试 ======
    printf("\n=== Auto Parsing Tests ===\n");

    TEST("auto detect ISO8601");
    err = NULL;
    if (parse_auto("2013-06-28T19:08:09.245Z", &sec, &nsec, &err)) {
        CHECK_EQ(1372414089LL, sec, "seconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("auto detect log time");
    err = NULL;
    if (parse_auto("2013-06-28 19:08:09", &sec, &nsec, &err)) {
        CHECK_EQ(1372414089LL, sec, "seconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("auto detect unix timestamp");
    err = NULL;
    if (parse_auto("1372414089", &sec, &nsec, &err)) {
        CHECK_EQ(1372414089LL, sec, "seconds");
    } else {
        FAIL(err ? err : "unknown error");
    }

    TEST("auto detect timestamp with decimal");
    err = NULL;
    if (parse_auto("1372414089.123", &sec, &nsec, &err)) {
        CHECK_EQ(1372414089LL, sec, "seconds");
        if (nsec == 123000000LL) {
            PASS();
        } else {
            printf("FAILED: nanoseconds mismatch (expected 123000000, got %lld)\n", (long long)nsec);
            tests_failed++;
        }
    } else {
        FAIL(err ? err : "unknown error");
    }

    // ====== 格式化测试 ======
    printf("\n=== Formatting Tests ===\n");

    char buf[64];
    sec = 1372414089;
    nsec = 123456789;

    TEST("format basic time");
    int len = format_time(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", sec, nsec);
    if (len > 0 && strcmp(buf, "2013-06-28 19:08:09") == 0) {
        PASS();
    } else {
        printf("FAILED: got '%s'\n", buf);
        tests_failed++;
    }

    TEST("format with nanoseconds");
    len = format_time(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S.%f", sec, nsec);
    if (len > 0 && strcmp(buf, "2013-06-28 19:08:09.123456789") == 0) {
        PASS();
    } else {
        printf("FAILED: got '%s'\n", buf);
        tests_failed++;
    }

    // ====== 边界情况测试 ======
    printf("\n=== Edge Cases Tests ===\n");

    TEST("invalid date (Feb 30)");
    err = NULL;
    if (parse("%Y-%m-%d", "2013-02-30", &sec, &nsec, &err)) {
        FAIL("should have failed for invalid date");
    } else {
        PASS();
    }

    TEST("invalid time (hour 24)");
    err = NULL;
    if (parse("%Y-%m-%d %H:%M:%S", "2013-06-28 24:00:00", &sec, &nsec, &err)) {
        FAIL("should have failed for invalid time");
    } else {
        PASS();
    }

    TEST("trailing data rejection");
    err = NULL;
    if (parse("%Y-%m-%d %H:%M:%S", "2013-06-28 19:08:09 extra", &sec, &nsec, &err)) {
        FAIL("should have failed for trailing data");
    } else {
        PASS();
    }

    TEST("invalid format string");
    err = NULL;
    if (parse("%Y-%m-%d", "2013-13-32", &sec, &nsec, &err)) {
        FAIL("should have failed for invalid date");
    } else {
        PASS();
    }

    // ====== 打印结果摘要 ======
    printf("\n==================================================\n");
    printf("Test Results Summary:\n");
    printf("==================================================\n");
    printf("Tests Passed:     %d\n", tests_passed);
    printf("Tests Failed:     %d\n", tests_failed);
    printf("Total Tests:      %d\n", tests_passed + tests_failed);
    printf("Success Rate:     %.1f%%\n", (tests_passed + tests_failed) > 0 ? (tests_passed * 100.0 / (tests_passed + tests_failed)) : 0.0);
    printf("==================================================\n");

    return (tests_failed == 0) ? 0 : 1;
}
