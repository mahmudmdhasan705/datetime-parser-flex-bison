# Makefile for the Date and Time Expression Parser
# Usage:  make        (build)
#         make test    (run all test cases)
#         make clean   (remove generated files)

CC      = gcc
CFLAGS  = -Wall -Wextra -g
TARGET  = datetime_parser

all: $(TARGET)

# Step 1: BISON creates parser.tab.c and parser.tab.h from parser.y
parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

# Step 2: FLEX creates lex.yy.c from lexer.l (needs parser.tab.h)
lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

# Step 3: GCC compiles all C files into one program
$(TARGET): parser.tab.c lex.yy.c date_time.c date_time.h
	$(CC) $(CFLAGS) -o $(TARGET) parser.tab.c lex.yy.c date_time.c

test: $(TARGET)
	./$(TARGET) test_cases.txt

clean:
	rm -f $(TARGET) parser.tab.c parser.tab.h lex.yy.c
