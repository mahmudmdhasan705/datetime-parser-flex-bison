/*
 * date_time.c
 * ---------------------------------------------------------------
 * Calendar and clock validation written in plain C.
 * BISON calls process_date() and process_time() from its
 * semantic actions; those functions use everything below.
 * ---------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "date_time.h"

/* ================================================================
 *  Constant tables
 * ================================================================ */

static const char *MONTH_NAMES[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static const char *DAY_NAMES[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

/* Days in each month of a NON-leap year. */
static const int DAYS_PER_MONTH[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/* ================================================================
 *  Report buffer (collects output until the line is accepted)
 * ================================================================ */

#define REPORT_SIZE 4096

static char   report_buffer[REPORT_SIZE];
static size_t report_length = 0;

/* Add formatted text to the buffer (works like printf). */
void report_add(const char *format, ...)
{
    va_list args;
    int written;

    if (report_length >= REPORT_SIZE - 1) {
        return; /* buffer full: ignore extra text */
    }

    va_start(args, format);
    written = vsnprintf(report_buffer + report_length,
                        REPORT_SIZE - report_length, format, args);
    va_end(args);

    if (written > 0) {
        report_length += (size_t)written;
        if (report_length > REPORT_SIZE - 1) {
            report_length = REPORT_SIZE - 1;
        }
    }
}

/* Throw away everything collected so far. */
void report_clear(void)
{
    report_length = 0;
    report_buffer[0] = '\0';
}

/* Print everything collected so far, then empty the buffer. */
void report_flush(void)
{
    fputs(report_buffer, stdout);
    report_clear();
}

/* ================================================================
 *  Number token helpers (used by the lexer)
 * ================================================================ */

NumberToken make_number(int value, int digits)
{
    NumberToken token;
    token.value  = value;
    token.digits = digits;
    return token;
}

/* Convert the text "0042" into a NumberToken {42, 4}.
 * Very long numbers are capped so they cannot overflow an int. */
NumberToken number_from_text(const char *text)
{
    int digits = (int)strlen(text);
    int value;

    if (digits > 9) {
        value = 999999999;
    } else {
        value = atoi(text);
    }
    return make_number(value, digits);
}

/* ================================================================
 *  Calendar logic
 * ================================================================ */

/* Leap year rule (Gregorian calendar):
 *   divisible by 400                  -> leap year
 *   divisible by 100 (but not by 400) -> NOT a leap year
 *   divisible by 4   (but not by 100) -> leap year
 *   otherwise                         -> NOT a leap year */
int is_leap_year(int year)
{
    if (year % 400 == 0) return 1;
    if (year % 100 == 0) return 0;
    if (year % 4   == 0) return 1;
    return 0;
}

/* Human-readable explanation of the leap year decision. */
const char *leap_year_reason(int year)
{
    if (year % 400 == 0) return "divisible by 400";
    if (year % 100 == 0) return "divisible by 100 but not by 400";
    if (year % 4   == 0) return "divisible by 4 but not by 100";
    return "not divisible by 4";
}

/* Number of days in a month (month must be 1..12). */
int days_in_month(int month, int year)
{
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return DAYS_PER_MONTH[month - 1];
}

/* Day of the week using Sakamoto's method.
 * Returns 0 = Sunday, 1 = Monday, ... 6 = Saturday. */
int day_of_week(int day, int month, int year)
{
    static const int offset[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

    if (month < 3) {
        year = year - 1;
    }
    return (year + year / 4 - year / 100 + year / 400
            + offset[month - 1] + day) % 7;
}

/* Position of the date inside its year (1 January = 1). */
int day_of_year(int day, int month, int year)
{
    int total = day;
    int m;

    for (m = 1; m < month; m++) {
        total += days_in_month(m, year);
    }
    return total;
}

/* ================================================================
 *  Validation
 * ================================================================ */

/* Check a date. Returns DATE_OK (0) or a combination of
 * DATE_ERR_YEAR, DATE_ERR_MONTH, DATE_ERR_DAY. */
int validate_date(int day, int month, int year)
{
    int errors = DATE_OK;
    int max_day;

    if (year < 1 || year > 9999) {
        errors |= DATE_ERR_YEAR;
    }

    if (month < 1 || month > 12) {
        errors |= DATE_ERR_MONTH;
        max_day = 31;               /* month unknown: use the largest */
    } else {
        max_day = days_in_month(month, year);
    }

    if (day < 1 || day > max_day) {
        errors |= DATE_ERR_DAY;
    }

    return errors;
}

/* Check a time. Returns TIME_OK (0) or a combination of
 * TIME_ERR_HOUR, TIME_ERR_MINUTE, TIME_ERR_SECOND.
 *   FORMAT_24H : hour must be 0..23
 *   FORMAT_AM / FORMAT_PM : hour must be 1..12 */
int validate_time(int hour, int minute, int second, int format)
{
    int errors = TIME_OK;

    if (format == FORMAT_24H) {
        if (hour < 0 || hour > 23) errors |= TIME_ERR_HOUR;
    } else {
        if (hour < 1 || hour > 12) errors |= TIME_ERR_HOUR;
    }

    if (minute < 0 || minute > 59) errors |= TIME_ERR_MINUTE;
    if (second < 0 || second > 59) errors |= TIME_ERR_SECOND;

    return errors;
}

/* ================================================================
 *  12-hour <-> 24-hour conversion
 * ================================================================ */

/* 12-hour clock (with AM/PM) -> 24-hour hour.
 * 12 AM -> 0, 1..11 AM -> same, 12 PM -> 12, 1..11 PM -> +12. */
int to_24_hour(int hour, int format)
{
    int result;

    if (format == FORMAT_24H) {
        return hour;
    }
    result = hour % 12;
    if (format == FORMAT_PM) {
        result += 12;
    }
    return result;
}

/* 24-hour hour -> 12-hour hour (1..12). AM/PM is decided by hour < 12. */
int to_12_hour(int hour24)
{
    int result = hour24 % 12;
    if (result == 0) {
        result = 12;
    }
    return result;
}

/* ================================================================
 *  Semantic processing called from BISON
 * ================================================================ */

/* Validate a date, write a report, return 1 (valid) or 0 (invalid). */
int process_date(NumberToken day, NumberToken month, NumberToken year,
                 char separator)
{
    int errors = validate_date(day.value, month.value, year.value);
    int format_ok = (day.digits <= 2 && month.digits <= 2 &&
                     year.digits == 4);
    int valid = (errors == DATE_OK && format_ok);

    report_add("  DATE  %02d%c%02d%c%04d  ->  %s\n",
               day.value, separator, month.value, separator, year.value,
               valid ? "VALID" : "INVALID");

    if (valid) {
        int leap = is_leap_year(year.value);

        report_add("        Leap year   : %s (%d is %s)\n",
                   leap ? "YES" : "NO", year.value,
                   leap_year_reason(year.value));
        report_add("        Month       : %s has %d days in %d\n",
                   MONTH_NAMES[month.value - 1],
                   days_in_month(month.value, year.value), year.value);
        report_add("        Day of week : %s\n",
                   DAY_NAMES[day_of_week(day.value, month.value,
                                         year.value)]);
        report_add("        Day of year : %d of %d\n",
                   day_of_year(day.value, month.value, year.value),
                   leap ? 366 : 365);
        return 1;
    }

    /* ---- Explain every problem that was found ---- */
    if (!format_ok) {
        report_add("        Error: wrong format. Use DD-MM-YYYY or DD/MM/YYYY "
                   "(day and month: 1-2 digits, year: exactly 4 digits; "
                   "found %d, %d and %d digits)\n",
                   day.digits, month.digits, year.digits);
    }

    if (errors & DATE_ERR_YEAR) {
        report_add("        Error: year %d is out of range (must be 1 to 9999)\n",
                   year.value);
    }

    if (errors & DATE_ERR_MONTH) {
        report_add("        Error: month %d is invalid (must be between 1 and 12)\n",
                   month.value);
    }

    if (errors & DATE_ERR_DAY) {
        if (errors & DATE_ERR_MONTH) {
            /* Month is unknown, so we can only say the day is impossible. */
            report_add("        Error: day %d is invalid (no month has fewer "
                       "than 1 or more than 31 days)\n", day.value);
        } else if (day.value < 1) {
            report_add("        Error: day %d is invalid (day must be at "
                       "least 1)\n", day.value);
        } else if (month.value == 2) {
            report_add("        Error: day %d is invalid for February %d "
                       "(February %d has only %d days because %d is %s)\n",
                       day.value, year.value, year.value,
                       days_in_month(2, year.value), year.value,
                       is_leap_year(year.value) ? "a leap year"
                                                : "not a leap year");
        } else {
            report_add("        Error: day %d is invalid for %s "
                       "(%s has only %d days)\n",
                       day.value, MONTH_NAMES[month.value - 1],
                       MONTH_NAMES[month.value - 1],
                       days_in_month(month.value, year.value));
        }
    }

    return 0;
}

/* Validate a time, write a report, return 1 (valid) or 0 (invalid). */
int process_time(NumberToken hour, NumberToken minute, int has_seconds,
                 NumberToken second, int format)
{
    int errors = validate_time(hour.value, minute.value, second.value,
                               format);
    int format_ok = (hour.digits <= 2 && minute.digits == 2 &&
                     (!has_seconds || second.digits == 2));
    int valid = (errors == TIME_OK && format_ok);
    int is_24h = (format == FORMAT_24H);
    char time_text[32];
    const char *suffix = "";
    int length;

    if (format == FORMAT_AM) suffix = " AM";
    if (format == FORMAT_PM) suffix = " PM";

    /* Rebuild the time as text so we can echo it in the report. */
    length = snprintf(time_text, sizeof(time_text), "%02d:%02d",
                      hour.value, minute.value);
    if (has_seconds && length > 0 && length < (int)sizeof(time_text)) {
        length += snprintf(time_text + length, sizeof(time_text) - length,
                           ":%02d", second.value);
    }
    if (length > 0 && length < (int)sizeof(time_text)) {
        snprintf(time_text + length, sizeof(time_text) - length,
                 "%s", suffix);
    }

    report_add("  TIME  %s (%s format)  ->  %s\n", time_text,
               is_24h ? "24-hour" : "12-hour", valid ? "VALID" : "INVALID");

    if (valid) {
        char seconds_text[8] = "";

        if (has_seconds) {
            snprintf(seconds_text, sizeof(seconds_text), ":%02d",
                     second.value);
        }

        if (is_24h) {
            report_add("        12-hour equivalent : %02d:%02d%s %s\n",
                       to_12_hour(hour.value), minute.value, seconds_text,
                       hour.value < 12 ? "AM" : "PM");
        } else {
            report_add("        24-hour equivalent : %02d:%02d%s\n",
                       to_24_hour(hour.value, format), minute.value,
                       seconds_text);
        }
        return 1;
    }

    /* ---- Explain every problem that was found ---- */
    if (!format_ok) {
        report_add("        Error: wrong format. Use HH:MM or HH:MM:SS "
                   "(hour: 1-2 digits, minutes and seconds: exactly 2 "
                   "digits)\n");
    }

    if (errors & TIME_ERR_HOUR) {
        if (is_24h) {
            report_add("        Error: hour %d is invalid for 24-hour "
                       "format (must be 0 to 23)\n", hour.value);
        } else {
            report_add("        Error: hour %d is invalid for 12-hour "
                       "format (must be 1 to 12)\n", hour.value);
        }
    }

    if (errors & TIME_ERR_MINUTE) {
        report_add("        Error: minute %d is invalid (must be 0 to 59)\n",
                   minute.value);
    }

    if (errors & TIME_ERR_SECOND) {
        report_add("        Error: second %d is invalid (must be 0 to 59)\n",
                   second.value);
    }

    return 0;
}
