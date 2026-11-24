%{
/*
 * parser.y  -  BISON syntax analyzer
 * ---------------------------------------------------------------
 * Receives tokens from FLEX (lexer.l), checks that they follow the
 * grammar of the date/time language, and runs semantic actions that
 * call the C validation functions in date_time.c.
 *
 * Input language (one expression per line):
 *   DATE dd-mm-yyyy          or   DATE dd/mm/yyyy
 *   TIME hh:mm  [:ss]        (24-hour)
 *   TIME hh:mm  [:ss] AM|PM  (12-hour)
 *   DATE ...  TIME ...       (date-time on one line, either order)
 * ---------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "date_time.h"

int  yylex(void);
void yyerror(const char *message);
void report_error(const char *kind, int column, const char *message);

extern FILE *yyin;
extern int   token_start_col;
extern int   statement_line;
extern int   lex_error_reported;
extern int   header_printed;
extern char  line_text[LINE_TEXT_SIZE];

/* Counters for the final summary */
static int total_count   = 0;
static int valid_count   = 0;
static int invalid_count = 0;

/* Syntax and lexical error messages are collected here and printed
 * when the whole line has been read, so the full line can be shown. */
#define ERROR_BUFFER_SIZE 2048
static char error_buffer[ERROR_BUFFER_SIZE] = "";

static void print_line_header(void);
static void record_result(int valid);
%}

/* Put the NumberToken type in parser.tab.h BEFORE the union that uses it */
%code requires {
    #include "date_time.h"
}

/* Give readable names to tokens in syntax error messages */
%define parse.error verbose

/* Types of values that tokens and rules can carry */
%union {
    NumberToken num;    /* value of a NUMBER token                  */
    int         flag;   /* 1/0 validity, or a FORMAT_xxx constant   */
}

/* ----- Tokens (these names are returned by lexer.l) ----- */
%token END 0 "end of input"
%token DATE_KW    "DATE"
%token TIME_KW    "TIME"
%token <num> NUMBER "number"
%token DASH       "dash (-)"
%token SLASH      "slash (/)"
%token COLON      "colon (:)"
%token AM         "AM"
%token PM         "PM"
%token NEWLINE    "end of line"
%token INVALID_CHAR "invalid character"

/* Non-terminals that carry a value */
%type <flag> date_stmt time_stmt meridiem

%%

/* ---------------------------------------------------------------
 *  GRAMMAR RULES
 * --------------------------------------------------------------- */

/* Rule 1: a program is zero or more lines */
program
    : /* empty */
    | program line
    ;

/* Rule 2: a line is empty, a statement, or a line with a syntax error */
line
    : NEWLINE
    | statement NEWLINE
    | error NEWLINE
        {
            /* Syntax error: throw away partial results, show the line
             * and the collected error messages, then go on with the
             * next line. */
            report_clear();
            print_line_header();
            fputs(error_buffer, stdout);
            printf("  Result: INVALID (syntax error)\n");
            error_buffer[0] = '\0';
            total_count++;
            invalid_count++;
            lex_error_reported = 0;
            yyerrok;
        }
    ;

/* Rule 3: a statement is a date, a time, or a date-time pair */
statement
    : date_stmt
        { record_result($1); }
    | time_stmt
        { record_result($1); }
    | date_stmt time_stmt
        {
            int ok = ($1 && $2);
            report_add("  DATE-TIME  ->  %s\n", ok ? "VALID" : "INVALID");
            record_result(ok);
        }
    | time_stmt date_stmt
        {
            int ok = ($1 && $2);
            report_add("  TIME-DATE  ->  %s\n", ok ? "VALID" : "INVALID");
            record_result(ok);
        }
    ;

/* Rule 4: a date uses '-' or '/' (the same separator twice) */
date_stmt
    : DATE_KW NUMBER DASH NUMBER DASH NUMBER
        { $$ = process_date($2, $4, $6, '-'); }
    | DATE_KW NUMBER SLASH NUMBER SLASH NUMBER
        { $$ = process_date($2, $4, $6, '/'); }
    ;

/* Rule 5: a time has hh:mm or hh:mm:ss, with optional AM/PM */
time_stmt
    : TIME_KW NUMBER COLON NUMBER
        { $$ = process_time($2, $4, 0, make_number(0, 2), FORMAT_24H); }
    | TIME_KW NUMBER COLON NUMBER COLON NUMBER
        { $$ = process_time($2, $4, 1, $6, FORMAT_24H); }
    | TIME_KW NUMBER COLON NUMBER meridiem
        { $$ = process_time($2, $4, 0, make_number(0, 2), $5); }
    | TIME_KW NUMBER COLON NUMBER COLON NUMBER meridiem
        { $$ = process_time($2, $4, 1, $6, $7); }
    ;

/* Rule 6: AM or PM selects the 12-hour format */
meridiem
    : AM { $$ = FORMAT_AM; }
    | PM { $$ = FORMAT_PM; }
    ;

%%

/* ---------------------------------------------------------------
 *  Helper functions
 * --------------------------------------------------------------- */

/* Print "Line N: <text of the line>" once per input line. */
static void print_line_header(void)
{
    char text[LINE_TEXT_SIZE];
    int  length;

    if (header_printed) {
        return;
    }
    header_printed = 1;

    strncpy(text, line_text, sizeof(text) - 1);
    text[sizeof(text) - 1] = '\0';

    /* remove trailing spaces */
    length = (int)strlen(text);
    while (length > 0 && (text[length - 1] == ' ' || text[length - 1] == '\t')) {
        text[--length] = '\0';
    }

    printf("\nLine %d: %s\n", statement_line, text);
}

/* Show the collected report of an accepted statement and count it. */
static void record_result(int valid)
{
    print_line_header();
    report_flush();
    total_count++;
    if (valid) {
        valid_count++;
    } else {
        invalid_count++;
    }
}

/* Remember an error message (used by yyerror and by the lexer).
 * It is printed later, under the line header. */
void report_error(const char *kind, int column, const char *message)
{
    size_t used = strlen(error_buffer);

    if (used < ERROR_BUFFER_SIZE - 1) {
        snprintf(error_buffer + used, ERROR_BUFFER_SIZE - used,
                 "  %s (column %d): %s\n", kind, column, message);
    }
}

/* BISON calls this function when it finds a syntax error. */
void yyerror(const char *message)
{
    /* If the lexer already reported an unknown character, do not
     * print a second, less helpful message for the same problem. */
    if (lex_error_reported) {
        return;
    }
    report_error("SYNTAX ERROR", token_start_col, message);
}

/* ---------------------------------------------------------------
 *  Program entry point
 * --------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (yyin == NULL) {
            perror(argv[1]);
            return 1;
        }
        printf("Reading expressions from file: %s\n", argv[1]);
    } else {
        yyin = stdin;
        if (isatty(fileno(stdin))) {
            printf("Date and Time Expression Parser\n");
            printf("Type one expression per line, for example:\n");
            printf("  DATE 29-02-2024\n");
            printf("  TIME 10:30 PM\n");
            printf("  DATE 31-12-2025 TIME 23:45\n");
            printf("Press Ctrl+D to finish.\n");
        }
    }

    yyparse();

    printf("\n==================== SUMMARY ====================\n");
    printf("  Expressions checked : %d\n", total_count);
    printf("  Valid               : %d\n", valid_count);
    printf("  Invalid             : %d\n", invalid_count);
    printf("=================================================\n");

    if (yyin != stdin) {
        fclose(yyin);
    }
    return 0;
}
