/*
 * date_time.h
 * ---------------------------------------------------------------
 * Shared declarations for the Date and Time Expression Parser.
 * This header is included by:
 *   - lexer.l      (FLEX)  : to build number tokens
 *   - parser.y     (BISON) : to call the validation functions
 *   - date_time.c  (C)     : which implements them
 * ---------------------------------------------------------------
 */

#ifndef DATE_TIME_H
#define DATE_TIME_H

/* Maximum size of the text of one input line that we remember. */
#define LINE_TEXT_SIZE 512

/* ---------- Number token ---------- */
/* FLEX gives BISON a number as TWO facts:
 *   value  : the numeric value  ("02" -> 2)
 *   digits : how many digits were typed ("02" -> 2)
 * We need the digit count to check formats such as a 4-digit year. */
typedef struct {
    int value;
    int digits;
} NumberToken;

/* ---------- Time formats ---------- */
#define FORMAT_24H 0   /* 24-hour clock, e.g. 23:45          */
#define FORMAT_AM  1   /* 12-hour clock with AM, e.g. 10:30 AM */
#define FORMAT_PM  2   /* 12-hour clock with PM, e.g. 10:30 PM */

/* ---------- Error flags returned by the validators ---------- */
/* Several problems can exist at once, so each problem is one bit. */
#define DATE_OK         0
#define DATE_ERR_YEAR   1
#define DATE_ERR_MONTH  2
#define DATE_ERR_DAY    4

#define TIME_OK         0
#define TIME_ERR_HOUR   1
#define TIME_ERR_MINUTE 2
#define TIME_ERR_SECOND 4

/* ---------- Helpers used by the lexer ---------- */
NumberToken make_number(int value, int digits);
NumberToken number_from_text(const char *text);

/* ---------- Calendar logic ---------- */
int         is_leap_year(int year);
const char *leap_year_reason(int year);
int         days_in_month(int month, int year);
int         day_of_week(int day, int month, int year);
int         day_of_year(int day, int month, int year);

/* ---------- Validation (return a set of error flags) ---------- */
int validate_date(int day, int month, int year);
int validate_time(int hour, int minute, int second, int format);

/* ---------- Conversion between 12-hour and 24-hour time ---------- */
int to_24_hour(int hour, int format);
int to_12_hour(int hour24);

/* ---------- Semantic processing called from BISON actions ----------
 * Each function validates, writes a report to an internal buffer and
 * returns 1 if the input was valid, otherwise 0. */
int process_date(NumberToken day, NumberToken month, NumberToken year,
                 char separator);
int process_time(NumberToken hour, NumberToken minute, int has_seconds,
                 NumberToken second, int format);

/* ---------- Report buffer ----------
 * Output is collected first so that a line with a syntax error never
 * prints half-finished results. */
void report_add(const char *format, ...);
void report_flush(void);
void report_clear(void);

#endif /* DATE_TIME_H */
