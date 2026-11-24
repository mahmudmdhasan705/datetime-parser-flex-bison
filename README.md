# Date and Time Expression Parser

A Date and Time Expression Parser developed using **Flex, Bison, and
C**. This project analyzes date and time expressions, validates their
formats and values, and provides useful date-time information.

## 🚀 Project Overview

This project demonstrates lexical analysis and syntax parsing using Flex
and Bison. It accepts date and time expressions, checks their validity,
and displays detailed information about valid inputs.

## ✨ Features

-   Date validation (`DD-MM-YYYY`)
-   Time validation (12-hour and 24-hour formats)
-   Date-Time expression parsing
-   Leap year detection
-   Month and day validation
-   Day of the week calculation
-   Day of the year calculation
-   12-hour to 24-hour time conversion
-   24-hour to 12-hour time conversion
-   Lexical and syntax error detection
-   Line-by-line expression processing

## 🛠️ Technologies Used

-   **C** -- Core programming language
-   **Flex** -- Lexical analysis
-   **Bison** -- Syntax analysis and parsing
-   **GCC** -- C compiler

## 📁 Project Structure

``` text
datetime-parser-flex-bison/
│
├── lexer.l
├── parser.y
├── date_time.c
├── date_time.h
├── parser.tab.c
├── parser.tab.h
├── lex.yy.c
├── Makefile
├── test_cases.txt
└── datetime_parser.exe
```

## ⚙️ Installation and Setup

### Prerequisites

Install the following tools:

-   GCC Compiler
-   Flex
-   Bison
-   MSYS2 (for Windows users)

### Build Instructions (Windows)

Open the project folder in PowerShell or VS Code Terminal.

**Step 1: Generate Parser Files**

``` bash
bison -d -o parser.tab.c parser.y
```

**Step 2: Generate Lexer File**

``` bash
flex -o lex.yy.c lexer.l
```

**Step 3: Compile the Project**

``` bash
gcc -o datetime_parser.exe parser.tab.c lex.yy.c date_time.c
```

**Step 4: Run the Application**

``` powershell
.\\datetime_parser.exe
```

## ▶️ Example Input

``` text
DATE 29-02-2024 TIME 10:30 PM
DATE 31-12-2025 TIME 23:45
```

## 📤 Example Output

``` text
DATE  29-02-2024  ->  VALID

Leap year   : YES
Month       : February has 29 days in 2024
Day of week : Thursday
Day of year : 60 of 366

TIME  10:30 PM (12-hour format)  ->  VALID

24-hour equivalent : 22:30

DATE-TIME  ->  VALID
```

## 🎯 Project Objectives

-   Understand lexical analysis using Flex.
-   Understand syntax analysis using Bison.
-   Implement grammar-based parsing.
-   Validate date and time expressions.
-   Handle lexical and syntax errors.
-   Practice compiler construction concepts.

## 👨‍💻 Author

**Md. Hasan Mahmud**

GitHub: [@mahmudmdhasan705](https://github.com/mahmudmdhasan705)

## 📌 Project Status

**Completed -- Academic Project**

------------------------------------------------------------------------

*Developed as a practical implementation of Compiler Design concepts
using Flex and Bison.*
