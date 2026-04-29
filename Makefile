CC       = gcc
CFLAGS   = -std=c99 -Wall -Wextra -pedantic -g
INCLUDES = -Iinclude

SRC_DIR   = src
TEST_DIR  = tests
BUILD_DIR = build

SRCS     = $(SRC_DIR)/lexer.c $(SRC_DIR)/token.c $(SRC_DIR)/ast.c \
           $(SRC_DIR)/parser.c $(SRC_DIR)/ast_printer.c $(SRC_DIR)/analyzer.c \
           $(SRC_DIR)/value.c $(SRC_DIR)/environment.c $(SRC_DIR)/eval.c \
           $(SRC_DIR)/interpreter.c $(SRC_DIR)/builtins.c
OBJS     = $(BUILD_DIR)/lexer.o $(BUILD_DIR)/token.o $(BUILD_DIR)/ast.o \
           $(BUILD_DIR)/parser.o $(BUILD_DIR)/ast_printer.o $(BUILD_DIR)/analyzer.o \
           $(BUILD_DIR)/value.o $(BUILD_DIR)/environment.o $(BUILD_DIR)/eval.o \
           $(BUILD_DIR)/interpreter.o $(BUILD_DIR)/builtins.o

TEST_LEXER_SRC  = $(TEST_DIR)/test_lexer.c
TEST_LEXER_BIN  = $(BUILD_DIR)/test_lexer
TEST_PARSER_SRC = $(TEST_DIR)/test_parser.c
TEST_PARSER_BIN = $(BUILD_DIR)/test_parser
TEST_ANALYZER_SRC = $(TEST_DIR)/test_analyzer.c
TEST_ANALYZER_BIN = $(BUILD_DIR)/test_analyzer
TEST_INTERP_SRC = $(TEST_DIR)/test_interpreter.c
TEST_INTERP_BIN = $(BUILD_DIR)/test_interpreter

MAIN_SRC = $(SRC_DIR)/main.c
MAIN_BIN = $(BUILD_DIR)/pylite

.PHONY: all test test-lexer test-parser test-analyzer test-interpreter clean demo

all: $(MAIN_BIN) test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/lexer.o: $(SRC_DIR)/lexer.c include/lexer.h include/token.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/token.o: $(SRC_DIR)/token.c include/token.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/ast.o: $(SRC_DIR)/ast.c include/ast.h include/token.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/parser.o: $(SRC_DIR)/parser.c include/parser.h include/ast.h include/lexer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/ast_printer.o: $(SRC_DIR)/ast_printer.c include/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/analyzer.o: $(SRC_DIR)/analyzer.c include/analyzer.h include/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/value.o: $(SRC_DIR)/value.c include/value.h include/environment.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/environment.o: $(SRC_DIR)/environment.c include/environment.h include/value.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/eval.o: $(SRC_DIR)/eval.c include/interpreter.h include/builtins.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/interpreter.o: $(SRC_DIR)/interpreter.c include/interpreter.h include/builtins.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/builtins.o: $(SRC_DIR)/builtins.c include/builtins.h include/value.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_LEXER_BIN): $(TEST_LEXER_SRC) $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(TEST_PARSER_BIN): $(TEST_PARSER_SRC) $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(TEST_ANALYZER_BIN): $(TEST_ANALYZER_SRC) $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(TEST_INTERP_BIN): $(TEST_INTERP_SRC) $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(MAIN_BIN): $(MAIN_SRC) $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

test: test-lexer test-parser test-analyzer test-interpreter

test-lexer: $(TEST_LEXER_BIN)
	@echo ""
	@./$(TEST_LEXER_BIN)

test-parser: $(TEST_PARSER_BIN)
	@echo ""
	@./$(TEST_PARSER_BIN)

test-analyzer: $(TEST_ANALYZER_BIN)
	@echo ""
	@./$(TEST_ANALYZER_BIN)

test-interpreter: $(TEST_INTERP_BIN)
	@echo ""
	@./$(TEST_INTERP_BIN)

demo: $(MAIN_BIN)
	@echo ""
	@./$(MAIN_BIN) examples/fizzbuzz.py

clean:
	rm -rf $(BUILD_DIR)
