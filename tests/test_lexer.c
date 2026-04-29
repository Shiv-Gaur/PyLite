

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"





static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_EQ_INT(expected, actual, msg) \
    do { \
        if ((expected) != (actual)) { \
            printf("  FAIL: %s (expected %d, got %d)\n", msg, (int)(expected), (int)(actual)); \
            return 1; \
        } \
    } while (0)

#define ASSERT_EQ_STR_N(expected, actual, len, msg) \
    do { \
        if (memcmp((expected), (actual), (len)) != 0) { \
            printf("  FAIL: %s (lexeme mismatch)\n", msg); \
            return 1; \
        } \
    } while (0)

#define ASSERT_TOKEN(tok, exp_type, exp_lexeme) \
    do { \
        ASSERT_EQ_INT(exp_type, (tok).type, \
            "token type '" exp_lexeme "' == " #exp_type); \
        if (exp_lexeme[0] != '\0' && (tok).length > 0) { \
            ASSERT_EQ_INT((int)strlen(exp_lexeme), (tok).length, \
                "token length '" exp_lexeme "'"); \
            ASSERT_EQ_STR_N(exp_lexeme, (tok).lexeme, (tok).length, \
                "token lexeme '" exp_lexeme "'"); \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        tests_run++; \
        printf("%-50s", #fn); \
        int r = fn(); \
        if (r == 0) { tests_passed++; printf(" PASS\n"); } \
        else        { tests_failed++; printf(" FAILED\n"); } \
    } while (0)





static Token *tokenize(const char *src, int *count) {
    Lexer lex;
    lexer_init(&lex, src);
    return lexer_tokenize_all(&lex, count);
}





static int test_empty_source(void) {
    int n;
    Token *tokens = tokenize("", &n);
    ASSERT_EQ_INT(1, n, "empty source → 1 token (EOF)");
    ASSERT_EQ_INT(TOKEN_EOF, tokens[0].type, "token is EOF");
    free(tokens);
    return 0;
}

static int test_integer_literal(void) {
    int n;
    Token *tokens = tokenize("42\n", &n);
    ASSERT_TOKEN(tokens[0], TOKEN_INT_LITERAL, "42");
    free(tokens);
    return 0;
}

static int test_string_single_quotes(void) {
    int n;
    Token *tokens = tokenize("'hello'\n", &n);
    ASSERT_TOKEN(tokens[0], TOKEN_STRING_LITERAL, "'hello'");
    free(tokens);
    return 0;
}

static int test_string_double_quotes(void) {
    int n;
    Token *tokens = tokenize("\"world\"\n", &n);
    ASSERT_TOKEN(tokens[0], TOKEN_STRING_LITERAL, "\"world\"");
    free(tokens);
    return 0;
}

static int test_string_escape(void) {
    int n;
    Token *tokens = tokenize("\"he\\\"llo\"\n", &n);
    ASSERT_TOKEN(tokens[0], TOKEN_STRING_LITERAL, "\"he\\\"llo\"");
    free(tokens);
    return 0;
}

static int test_unterminated_string(void) {
    int n;
    Token *tokens = tokenize("\"oops\n", &n);
    ASSERT_EQ_INT(TOKEN_ERROR, tokens[0].type, "unterminated string → ERROR");
    free(tokens);
    return 0;
}





static int test_keywords(void) {
    const char *src = "if elif else while for in def return and or not True False None print input len range type break continue pass\n";
    int n;
    Token *tokens = tokenize(src, &n);

    TokenType expected[] = {
        TOKEN_IF, TOKEN_ELIF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_FOR,
        TOKEN_IN, TOKEN_DEF, TOKEN_RETURN, TOKEN_AND, TOKEN_OR,
        TOKEN_NOT, TOKEN_TRUE, TOKEN_FALSE, TOKEN_NONE, TOKEN_PRINT,
        TOKEN_INPUT, TOKEN_LEN, TOKEN_RANGE, TOKEN_TYPE,
        TOKEN_BREAK, TOKEN_CONTINUE, TOKEN_PASS
    };
    int num_kw = sizeof(expected) / sizeof(expected[0]);

    for (int i = 0; i < num_kw; i++) {
        ASSERT_EQ_INT(expected[i], tokens[i].type, "keyword token type");
    }

    free(tokens);
    return 0;
}

static int test_identifier(void) {
    int n;
    Token *tokens = tokenize("my_var_123\n", &n);
    ASSERT_TOKEN(tokens[0], TOKEN_IDENTIFIER, "my_var_123");
    free(tokens);
    return 0;
}

static int test_identifier_not_keyword(void) {
    
    int n;
    Token *tokens = tokenize("iffy\n", &n);
    ASSERT_TOKEN(tokens[0], TOKEN_IDENTIFIER, "iffy");
    free(tokens);
    return 0;
}





static int test_arithmetic_operators(void) {
    int n;
    Token *tokens = tokenize("+ - * / // % **\n", &n);
    ASSERT_EQ_INT(TOKEN_PLUS,         tokens[0].type, "+");
    ASSERT_EQ_INT(TOKEN_MINUS,        tokens[1].type, "-");
    ASSERT_EQ_INT(TOKEN_STAR,         tokens[2].type, "*");
    ASSERT_EQ_INT(TOKEN_SLASH,        tokens[3].type, "/");
    ASSERT_EQ_INT(TOKEN_DOUBLE_SLASH, tokens[4].type, "//");
    ASSERT_EQ_INT(TOKEN_PERCENT,      tokens[5].type, "%");
    ASSERT_EQ_INT(TOKEN_DOUBLE_STAR,  tokens[6].type, "**");
    free(tokens);
    return 0;
}

static int test_comparison_operators(void) {
    int n;
    Token *tokens = tokenize("== != < > <= >=\n", &n);
    ASSERT_EQ_INT(TOKEN_EQ,  tokens[0].type, "==");
    ASSERT_EQ_INT(TOKEN_NEQ, tokens[1].type, "!=");
    ASSERT_EQ_INT(TOKEN_LT,  tokens[2].type, "<");
    ASSERT_EQ_INT(TOKEN_GT,  tokens[3].type, ">");
    ASSERT_EQ_INT(TOKEN_LTE, tokens[4].type, "<=");
    ASSERT_EQ_INT(TOKEN_GTE, tokens[5].type, ">=");
    free(tokens);
    return 0;
}

static int test_assignment_operators(void) {
    int n;
    Token *tokens = tokenize("= += -= *= /=\n", &n);
    ASSERT_EQ_INT(TOKEN_ASSIGN,       tokens[0].type, "=");
    ASSERT_EQ_INT(TOKEN_PLUS_ASSIGN,  tokens[1].type, "+=");
    ASSERT_EQ_INT(TOKEN_MINUS_ASSIGN, tokens[2].type, "-=");
    ASSERT_EQ_INT(TOKEN_STAR_ASSIGN,  tokens[3].type, "*=");
    ASSERT_EQ_INT(TOKEN_SLASH_ASSIGN, tokens[4].type, "/=");
    free(tokens);
    return 0;
}

static int test_delimiters(void) {
    int n;
    Token *tokens = tokenize("( ) [ ] : , .\n", &n);
    ASSERT_EQ_INT(TOKEN_LPAREN,   tokens[0].type, "(");
    ASSERT_EQ_INT(TOKEN_RPAREN,   tokens[1].type, ")");
    ASSERT_EQ_INT(TOKEN_LBRACKET, tokens[2].type, "[");
    ASSERT_EQ_INT(TOKEN_RBRACKET, tokens[3].type, "]");
    ASSERT_EQ_INT(TOKEN_COLON,    tokens[4].type, ":");
    ASSERT_EQ_INT(TOKEN_COMMA,    tokens[5].type, ",");
    ASSERT_EQ_INT(TOKEN_DOT,      tokens[6].type, ".");
    free(tokens);
    return 0;
}





static int test_comment_skipped(void) {
    int n;
    Token *tokens = tokenize("x = 5  # this is a comment\n", &n);
    ASSERT_TOKEN(tokens[0], TOKEN_IDENTIFIER, "x");
    ASSERT_EQ_INT(TOKEN_ASSIGN,      tokens[1].type, "=");
    ASSERT_TOKEN(tokens[2], TOKEN_INT_LITERAL, "5");
    ASSERT_EQ_INT(TOKEN_NEWLINE, tokens[3].type, "NEWLINE after comment");
    free(tokens);
    return 0;
}

static int test_comment_only_line(void) {
    int n;
    Token *tokens = tokenize("# just a comment\n", &n);
    
    ASSERT_EQ_INT(TOKEN_NEWLINE, tokens[0].type, "comment line → NEWLINE");
    free(tokens);
    return 0;
}





static int test_simple_indent_dedent(void) {
    const char *src =
        "if x:\n"
        "    y = 1\n"
        "z = 2\n";

    int n;
    Token *tokens = tokenize(src, &n);

    
    int i = 0;
    ASSERT_EQ_INT(TOKEN_IF,          tokens[i++].type, "if");
    ASSERT_EQ_INT(TOKEN_IDENTIFIER,  tokens[i++].type, "x");
    ASSERT_EQ_INT(TOKEN_COLON,       tokens[i++].type, ":");
    ASSERT_EQ_INT(TOKEN_NEWLINE,     tokens[i++].type, "NL");
    ASSERT_EQ_INT(TOKEN_INDENT,      tokens[i++].type, "INDENT");
    ASSERT_EQ_INT(TOKEN_IDENTIFIER,  tokens[i++].type, "y");
    ASSERT_EQ_INT(TOKEN_ASSIGN,      tokens[i++].type, "=");
    ASSERT_EQ_INT(TOKEN_INT_LITERAL, tokens[i++].type, "1");
    ASSERT_EQ_INT(TOKEN_NEWLINE,     tokens[i++].type, "NL");
    ASSERT_EQ_INT(TOKEN_DEDENT,      tokens[i++].type, "DEDENT");
    ASSERT_EQ_INT(TOKEN_IDENTIFIER,  tokens[i++].type, "z");

    free(tokens);
    return 0;
}

static int test_nested_indent(void) {
    const char *src =
        "if a:\n"
        "    if b:\n"
        "        c = 1\n"
        "d = 2\n";

    int n;
    Token *tokens = tokenize(src, &n);

    
    int indent_count = 0, dedent_count = 0;
    for (int i = 0; i < n; i++) {
        if (tokens[i].type == TOKEN_INDENT) indent_count++;
        if (tokens[i].type == TOKEN_DEDENT) dedent_count++;
    }
    ASSERT_EQ_INT(2, indent_count, "2 INDENTs for nested blocks");
    ASSERT_EQ_INT(2, dedent_count, "2 DEDENTs when returning to top level");

    free(tokens);
    return 0;
}

static int test_multiple_dedent(void) {
    
    const char *src =
        "if a:\n"
        "    if b:\n"
        "        x = 1\n"
        "y = 2\n";

    int n;
    Token *tokens = tokenize(src, &n);

    
    int found_double_dedent = 0;
    for (int i = 0; i < n - 1; i++) {
        if (tokens[i].type == TOKEN_DEDENT && tokens[i + 1].type == TOKEN_DEDENT) {
            found_double_dedent = 1;
            break;
        }
    }
    ASSERT_EQ_INT(1, found_double_dedent, "two consecutive DEDENTs");

    free(tokens);
    return 0;
}

static int test_dedent_at_eof(void) {
    
    const char *src =
        "if x:\n"
        "    y = 1\n";

    int n;
    Token *tokens = tokenize(src, &n);

    
    ASSERT_EQ_INT(TOKEN_EOF,    tokens[n - 1].type, "last is EOF");
    
    int found_dedent = 0;
    for (int i = 0; i < n; i++) {
        if (tokens[i].type == TOKEN_DEDENT) found_dedent = 1;
    }
    ASSERT_EQ_INT(1, found_dedent, "DEDENT emitted before EOF");

    free(tokens);
    return 0;
}

static int test_blank_lines_ignored(void) {
    const char *src =
        "x = 1\n"
        "\n"
        "\n"
        "y = 2\n";

    int n;
    Token *tokens = tokenize(src, &n);

    
    for (int i = 0; i < n; i++) {
        if (tokens[i].type == TOKEN_INDENT || tokens[i].type == TOKEN_DEDENT) {
            printf("  FAIL: unexpected INDENT/DEDENT for blank lines\n");
            free(tokens);
            return 1;
        }
    }

    free(tokens);
    return 0;
}





static int test_paren_line_continuation(void) {
    const char *src =
        "x = (1 +\n"
        "     2)\n";

    int n;
    Token *tokens = tokenize(src, &n);

    
    
    ASSERT_TOKEN(tokens[0], TOKEN_IDENTIFIER,  "x");
    ASSERT_EQ_INT(TOKEN_ASSIGN,      tokens[1].type, "=");
    ASSERT_EQ_INT(TOKEN_LPAREN,      tokens[2].type, "(");
    ASSERT_TOKEN(tokens[3], TOKEN_INT_LITERAL, "1");
    ASSERT_EQ_INT(TOKEN_PLUS,        tokens[4].type, "+");
    ASSERT_TOKEN(tokens[5], TOKEN_INT_LITERAL, "2");
    ASSERT_EQ_INT(TOKEN_RPAREN,      tokens[6].type, ")");
    ASSERT_EQ_INT(TOKEN_NEWLINE,     tokens[7].type, "NL");

    free(tokens);
    return 0;
}

static int test_bracket_line_continuation(void) {
    const char *src =
        "a = [1,\n"
        "     2,\n"
        "     3]\n";

    int n;
    Token *tokens = tokenize(src, &n);

    
    int newline_count = 0;
    for (int i = 0; i < n; i++) {
        if (tokens[i].type == TOKEN_NEWLINE) newline_count++;
    }
    
    ASSERT_EQ_INT(1, newline_count, "only 1 NEWLINE (outside brackets)");

    free(tokens);
    return 0;
}





static int test_line_numbers(void) {
    const char *src =
        "x = 1\n"
        "y = 2\n"
        "z = 3\n";

    int n;
    Token *tokens = tokenize(src, &n);

    
    ASSERT_EQ_INT(1, tokens[0].line, "x on line 1");
    
    for (int i = 0; i < n; i++) {
        if (tokens[i].type == TOKEN_IDENTIFIER && tokens[i].length == 1 &&
            tokens[i].lexeme[0] == 'y') {
            ASSERT_EQ_INT(2, tokens[i].line, "y on line 2");
            break;
        }
    }
    for (int i = 0; i < n; i++) {
        if (tokens[i].type == TOKEN_IDENTIFIER && tokens[i].length == 1 &&
            tokens[i].lexeme[0] == 'z') {
            ASSERT_EQ_INT(3, tokens[i].line, "z on line 3");
            break;
        }
    }

    free(tokens);
    return 0;
}





static int test_fizzbuzz_tokens(void) {
    const char *src =
        "for i in range(20):\n"
        "    if i % 3 == 0:\n"
        "        print(i)\n"
        "x = 0\n";

    int n;
    Token *tokens = tokenize(src, &n);

    
    ASSERT_EQ_INT(TOKEN_FOR,    tokens[0].type, "for");
    ASSERT_EQ_INT(TOKEN_IDENTIFIER, tokens[1].type, "i");
    ASSERT_EQ_INT(TOKEN_IN,     tokens[2].type, "in");
    ASSERT_EQ_INT(TOKEN_RANGE,  tokens[3].type, "range");
    ASSERT_EQ_INT(TOKEN_LPAREN, tokens[4].type, "(");
    ASSERT_TOKEN(tokens[5], TOKEN_INT_LITERAL, "20");
    ASSERT_EQ_INT(TOKEN_RPAREN, tokens[6].type, ")");
    ASSERT_EQ_INT(TOKEN_COLON,  tokens[7].type, ":");
    ASSERT_EQ_INT(TOKEN_NEWLINE, tokens[8].type, "NL");
    ASSERT_EQ_INT(TOKEN_INDENT, tokens[9].type, "INDENT1");
    ASSERT_EQ_INT(TOKEN_IF,     tokens[10].type, "if");

    free(tokens);
    return 0;
}

static int test_function_def_tokens(void) {
    const char *src =
        "def add(a, b):\n"
        "    return a + b\n"
        "\n";

    int n;
    Token *tokens = tokenize(src, &n);

    ASSERT_EQ_INT(TOKEN_DEF,        tokens[0].type, "def");
    ASSERT_TOKEN(tokens[1], TOKEN_IDENTIFIER, "add");
    ASSERT_EQ_INT(TOKEN_LPAREN,     tokens[2].type, "(");
    ASSERT_TOKEN(tokens[3], TOKEN_IDENTIFIER, "a");
    ASSERT_EQ_INT(TOKEN_COMMA,      tokens[4].type, ",");
    ASSERT_TOKEN(tokens[5], TOKEN_IDENTIFIER, "b");
    ASSERT_EQ_INT(TOKEN_RPAREN,     tokens[6].type, ")");
    ASSERT_EQ_INT(TOKEN_COLON,      tokens[7].type, ":");
    ASSERT_EQ_INT(TOKEN_NEWLINE,    tokens[8].type, "NL");
    ASSERT_EQ_INT(TOKEN_INDENT,     tokens[9].type, "INDENT");
    ASSERT_EQ_INT(TOKEN_RETURN,     tokens[10].type, "return");

    free(tokens);
    return 0;
}

static int test_list_tokens(void) {
    const char *src = "a = [1, 2, 3]\n";
    int n;
    Token *tokens = tokenize(src, &n);

    ASSERT_TOKEN(tokens[0], TOKEN_IDENTIFIER, "a");
    ASSERT_EQ_INT(TOKEN_ASSIGN,      tokens[1].type, "=");
    ASSERT_EQ_INT(TOKEN_LBRACKET,    tokens[2].type, "[");
    ASSERT_TOKEN(tokens[3], TOKEN_INT_LITERAL, "1");
    ASSERT_EQ_INT(TOKEN_COMMA,       tokens[4].type, ",");
    ASSERT_TOKEN(tokens[5], TOKEN_INT_LITERAL, "2");
    ASSERT_EQ_INT(TOKEN_COMMA,       tokens[6].type, ",");
    ASSERT_TOKEN(tokens[7], TOKEN_INT_LITERAL, "3");
    ASSERT_EQ_INT(TOKEN_RBRACKET,    tokens[8].type, "]");

    free(tokens);
    return 0;
}

static int test_elif_else_chain(void) {
    const char *src =
        "if x:\n"
        "    a = 1\n"
        "elif y:\n"
        "    a = 2\n"
        "else:\n"
        "    a = 3\n";

    int n;
    Token *tokens = tokenize(src, &n);

    
    int indent_count = 0, dedent_count = 0;
    for (int i = 0; i < n; i++) {
        if (tokens[i].type == TOKEN_INDENT) indent_count++;
        if (tokens[i].type == TOKEN_DEDENT) dedent_count++;
    }
    ASSERT_EQ_INT(3, indent_count, "3 INDENTs for if/elif/else bodies");
    ASSERT_EQ_INT(3, dedent_count, "3 DEDENTs matching");

    free(tokens);
    return 0;
}

static int test_while_loop(void) {
    const char *src =
        "while x > 0:\n"
        "    x = x - 1\n";

    int n;
    Token *tokens = tokenize(src, &n);

    ASSERT_EQ_INT(TOKEN_WHILE, tokens[0].type, "while");
    int has_indent = 0, has_dedent = 0;
    for (int i = 0; i < n; i++) {
        if (tokens[i].type == TOKEN_INDENT) has_indent = 1;
        if (tokens[i].type == TOKEN_DEDENT) has_dedent = 1;
    }
    ASSERT_EQ_INT(1, has_indent, "has INDENT");
    ASSERT_EQ_INT(1, has_dedent, "has DEDENT");

    free(tokens);
    return 0;
}

static int test_dot_method(void) {
    int n;
    Token *tokens = tokenize("a.append(1)\n", &n);
    ASSERT_TOKEN(tokens[0], TOKEN_IDENTIFIER, "a");
    ASSERT_EQ_INT(TOKEN_DOT,    tokens[1].type, ".");
    ASSERT_EQ_INT(TOKEN_APPEND, tokens[2].type, "append");
    ASSERT_EQ_INT(TOKEN_LPAREN, tokens[3].type, "(");
    ASSERT_TOKEN(tokens[4], TOKEN_INT_LITERAL, "1");
    ASSERT_EQ_INT(TOKEN_RPAREN, tokens[5].type, ")");
    free(tokens);
    return 0;
}





static int test_indentation_error(void) {
    const char *src =
        "if x:\n"
        "    y = 1\n"
        "  z = 2\n";   

    int n;
    Token *tokens = tokenize(src, &n);

    
    int found_error = 0;
    for (int i = 0; i < n; i++) {
        if (tokens[i].type == TOKEN_ERROR) {
            found_error = 1;
            break;
        }
    }
    ASSERT_EQ_INT(1, found_error, "indentation error detected");

    free(tokens);
    return 0;
}

static int test_unexpected_char(void) {
    int n;
    Token *tokens = tokenize("@\n", &n);
    ASSERT_EQ_INT(TOKEN_ERROR, tokens[0].type, "@ → ERROR");
    free(tokens);
    return 0;
}

static int test_token_type_name(void) {
    
    const char *name = token_type_name(TOKEN_IF);
    ASSERT_EQ_INT(1, name != NULL && strlen(name) > 0, "token_type_name(IF)");
    name = token_type_name(TOKEN_EOF);
    ASSERT_EQ_INT(1, name != NULL && strlen(name) > 0, "token_type_name(EOF)");
    return 0;
}





int main(void) {
    printf("\n===== PyLite Lexer — Unit Tests =====\n\n");

    
    RUN_TEST(test_empty_source);
    RUN_TEST(test_integer_literal);
    RUN_TEST(test_string_single_quotes);
    RUN_TEST(test_string_double_quotes);
    RUN_TEST(test_string_escape);
    RUN_TEST(test_unterminated_string);

    
    RUN_TEST(test_keywords);
    RUN_TEST(test_identifier);
    RUN_TEST(test_identifier_not_keyword);

    
    RUN_TEST(test_arithmetic_operators);
    RUN_TEST(test_comparison_operators);
    RUN_TEST(test_assignment_operators);
    RUN_TEST(test_delimiters);

    
    RUN_TEST(test_comment_skipped);
    RUN_TEST(test_comment_only_line);

    
    RUN_TEST(test_simple_indent_dedent);
    RUN_TEST(test_nested_indent);
    RUN_TEST(test_multiple_dedent);
    RUN_TEST(test_dedent_at_eof);
    RUN_TEST(test_blank_lines_ignored);

    
    RUN_TEST(test_paren_line_continuation);
    RUN_TEST(test_bracket_line_continuation);

    
    RUN_TEST(test_line_numbers);

    
    RUN_TEST(test_fizzbuzz_tokens);
    RUN_TEST(test_function_def_tokens);
    RUN_TEST(test_list_tokens);
    RUN_TEST(test_elif_else_chain);
    RUN_TEST(test_while_loop);
    RUN_TEST(test_dot_method);

    
    RUN_TEST(test_indentation_error);
    RUN_TEST(test_unexpected_char);
    RUN_TEST(test_token_type_name);

    printf("\n===== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0)
        printf(" (%d FAILED)", tests_failed);
    printf(" =====\n\n");

    return tests_failed > 0 ? 1 : 0;
}
