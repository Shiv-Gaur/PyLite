

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "analyzer.h"





static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  FAIL: %s\n", msg); \
            return 1; \
        } \
    } while (0)

#define ASSERT_EQ_INT(expected, actual, msg) \
    do { \
        if ((expected) != (actual)) { \
            printf("  FAIL: %s (expected %d, got %d)\n", \
                   msg, (int)(expected), (int)(actual)); \
            return 1; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        tests_run++; \
        printf("%-55s", #fn); \
        int r = fn(); \
        if (r == 0) { tests_passed++; printf(" PASS\n"); } \
        else        { tests_failed++; printf(" FAILED\n"); } \
    } while (0)





static int analyze_source(const char *src, Analyzer *a) {
    Lexer lex;
    lexer_init(&lex, src);
    int count;
    Token *tokens = lexer_tokenize_all(&lex, &count);

    Parser parser;
    parser_init(&parser, tokens, count);
    ASTNode *prog = parser_parse(&parser);

    if (parser_had_error(&parser)) {
        ast_node_free(prog);
        free(tokens);
        return -1; 
    }

    analyzer_init(a);
    int result = analyzer_analyze(a, prog);

    ast_node_free(prog);
    free(tokens);
    return result;
}





static int test_valid_program(void) {
    const char *src =
        "def foo(a, b):\n"
        "    return a + b\n"
        "x = foo(1, 2)\n"
        "print(x)\n";

    Analyzer a;
    int result = analyze_source(src, &a);
    ASSERT_EQ_INT(0, result, "no semantic errors");
    return 0;
}





static int test_break_outside_loop(void) {
    const char *src = "break\n";

    Analyzer a;
    int result = analyze_source(src, &a);
    ASSERT_EQ_INT(1, result, "should have error");
    ASSERT_TRUE(a.error_count > 0, "at least 1 error");
    ASSERT_TRUE(strstr(a.errors[0].message, "break") != NULL,
                "error mentions break");
    return 0;
}





static int test_continue_outside_loop(void) {
    const char *src = "continue\n";

    Analyzer a;
    int result = analyze_source(src, &a);
    ASSERT_EQ_INT(1, result, "should have error");
    ASSERT_TRUE(a.error_count > 0, "at least 1 error");
    ASSERT_TRUE(strstr(a.errors[0].message, "continue") != NULL,
                "error mentions continue");
    return 0;
}





static int test_return_outside_function(void) {
    const char *src = "return 42\n";

    Analyzer a;
    int result = analyze_source(src, &a);
    ASSERT_EQ_INT(1, result, "should have error");
    ASSERT_TRUE(a.error_count > 0, "at least 1 error");
    ASSERT_TRUE(strstr(a.errors[0].message, "return") != NULL,
                "error mentions return");
    return 0;
}





static int test_break_inside_loop(void) {
    const char *src =
        "while True:\n"
        "    break\n";

    Analyzer a;
    int result = analyze_source(src, &a);
    ASSERT_EQ_INT(0, result, "no error for break inside loop");
    return 0;
}





static int test_return_inside_function(void) {
    const char *src =
        "def foo():\n"
        "    return 1\n";

    Analyzer a;
    int result = analyze_source(src, &a);
    ASSERT_EQ_INT(0, result, "no error for return inside function");
    return 0;
}





static int test_duplicate_params(void) {
    const char *src =
        "def foo(a, b, a):\n"
        "    pass\n";

    Analyzer a;
    int result = analyze_source(src, &a);
    ASSERT_EQ_INT(1, result, "should have error");
    ASSERT_TRUE(a.error_count > 0, "at least 1 error");
    ASSERT_TRUE(strstr(a.errors[0].message, "duplicate") != NULL,
                "error mentions duplicate");
    return 0;
}





static int test_len_wrong_args(void) {
    const char *src = "x = len(1, 2)\n";

    Analyzer a;
    int result = analyze_source(src, &a);
    ASSERT_EQ_INT(1, result, "should have error");
    ASSERT_TRUE(a.error_count > 0, "at least 1 error");
    ASSERT_TRUE(strstr(a.errors[0].message, "len") != NULL,
                "error mentions len");
    return 0;
}





static int test_nested_loop_break(void) {
    const char *src =
        "for i in range(10):\n"
        "    while True:\n"
        "        break\n"
        "    continue\n";

    Analyzer a;
    int result = analyze_source(src, &a);
    ASSERT_EQ_INT(0, result, "no error for break/continue in nested loops");
    return 0;
}





static int test_break_in_if_no_loop(void) {
    const char *src =
        "if True:\n"
        "    break\n";

    Analyzer a;
    int result = analyze_source(src, &a);
    ASSERT_EQ_INT(1, result, "should have error");
    return 0;
}





int main(void) {
    printf("\n===== PyLite Analyzer — Unit Tests =====\n\n");

    RUN_TEST(test_valid_program);
    RUN_TEST(test_break_outside_loop);
    RUN_TEST(test_continue_outside_loop);
    RUN_TEST(test_return_outside_function);
    RUN_TEST(test_break_inside_loop);
    RUN_TEST(test_return_inside_function);
    RUN_TEST(test_duplicate_params);
    RUN_TEST(test_len_wrong_args);
    RUN_TEST(test_nested_loop_break);
    RUN_TEST(test_break_in_if_no_loop);

    printf("\n===== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0)
        printf(" (%d FAILED)", tests_failed);
    printf(" =====\n\n");

    return tests_failed > 0 ? 1 : 0;
}
