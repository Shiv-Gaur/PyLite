

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"





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

#define ASSERT_EQ_STR(expected, actual, msg) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("  FAIL: %s (expected '%s', got '%s')\n", \
                   msg, (expected), (actual)); \
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





static ASTNode *parse_source(const char *src, Parser *parser) {
    Lexer lex;
    lexer_init(&lex, src);
    int count;
    Token *tokens = lexer_tokenize_all(&lex, &count);
    parser_init(parser, tokens, count);
    return parser_parse(parser);
}





static int test_simple_assignment(void) {
    Parser parser;
    ASTNode *prog = parse_source("x = 42\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");
    ASSERT_EQ_INT(NODE_PROGRAM, prog->type, "root is Program");
    ASSERT_EQ_INT(1, prog->as.program.count, "1 statement");

    ASTNode *stmt = prog->as.program.statements[0];
    ASSERT_EQ_INT(NODE_ASSIGNMENT, stmt->type, "statement is Assignment");
    ASSERT_EQ_INT(NODE_IDENTIFIER, stmt->as.assignment.target->type, "target is Identifier");
    ASSERT_EQ_STR("x", stmt->as.assignment.target->as.identifier.name, "target name");
    ASSERT_EQ_INT(NODE_INT_LITERAL, stmt->as.assignment.value->type, "value is IntLiteral");
    ASSERT_EQ_INT(42, stmt->as.assignment.value->as.int_literal.value, "value == 42");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_arithmetic_precedence(void) {
    Parser parser;
    ASTNode *prog = parse_source("x = 1 + 2 * 3\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *val = prog->as.program.statements[0]->as.assignment.value;
    ASSERT_EQ_INT(NODE_BINARY_OP, val->type, "value is BinaryOp");
    ASSERT_EQ_INT(TOKEN_PLUS, val->as.binary_op.op, "top operator is +");

    
    ASTNode *right = val->as.binary_op.right;
    ASSERT_EQ_INT(NODE_BINARY_OP, right->type, "right is BinaryOp");
    ASSERT_EQ_INT(TOKEN_STAR, right->as.binary_op.op, "right operator is *");
    ASSERT_EQ_INT(2, right->as.binary_op.left->as.int_literal.value, "2");
    ASSERT_EQ_INT(3, right->as.binary_op.right->as.int_literal.value, "3");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_string_literal(void) {
    Parser parser;
    ASTNode *prog = parse_source("s = \"hello\"\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *val = prog->as.program.statements[0]->as.assignment.value;
    ASSERT_EQ_INT(NODE_STRING_LITERAL, val->type, "value is StringLiteral");
    ASSERT_EQ_STR("hello", val->as.string_literal.value, "string value");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_bool_none_literals(void) {
    Parser parser;
    ASTNode *prog = parse_source("a = True\nb = False\nc = None\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");
    ASSERT_EQ_INT(3, prog->as.program.count, "3 statements");

    ASSERT_EQ_INT(NODE_BOOL_LITERAL,
                  prog->as.program.statements[0]->as.assignment.value->type, "True");
    ASSERT_EQ_INT(1,
                  prog->as.program.statements[0]->as.assignment.value->as.bool_literal.value,
                  "True == 1");
    ASSERT_EQ_INT(NODE_BOOL_LITERAL,
                  prog->as.program.statements[1]->as.assignment.value->type, "False");
    ASSERT_EQ_INT(0,
                  prog->as.program.statements[1]->as.assignment.value->as.bool_literal.value,
                  "False == 0");
    ASSERT_EQ_INT(NODE_NONE_LITERAL,
                  prog->as.program.statements[2]->as.assignment.value->type, "None");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_if_elif_else(void) {
    const char *src =
        "if x:\n"
        "    a = 1\n"
        "elif y:\n"
        "    a = 2\n"
        "else:\n"
        "    a = 3\n";

    Parser parser;
    ASTNode *prog = parse_source(src, &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");
    ASSERT_EQ_INT(1, prog->as.program.count, "1 statement (if)");

    ASTNode *if_stmt = prog->as.program.statements[0];
    ASSERT_EQ_INT(NODE_IF_STATEMENT, if_stmt->type, "IfStatement");
    ASSERT_EQ_INT(1, if_stmt->as.if_statement.elif_count, "1 elif");
    ASSERT_TRUE(if_stmt->as.if_statement.else_body != NULL, "has else body");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_while_loop(void) {
    const char *src =
        "while x > 0:\n"
        "    x -= 1\n";

    Parser parser;
    ASTNode *prog = parse_source(src, &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *stmt = prog->as.program.statements[0];
    ASSERT_EQ_INT(NODE_WHILE_LOOP, stmt->type, "WhileLoop");
    ASSERT_EQ_INT(NODE_BLOCK, stmt->as.while_loop.body->type, "body is Block");

    
    ASTNode *body = stmt->as.while_loop.body;
    ASSERT_EQ_INT(1, body->as.block.count, "1 statement in body");
    ASSERT_EQ_INT(NODE_AUG_ASSIGNMENT, body->as.block.statements[0]->type, "aug assign");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_for_loop_range(void) {
    const char *src =
        "for i in range(10):\n"
        "    print(i)\n";

    Parser parser;
    ASTNode *prog = parse_source(src, &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *stmt = prog->as.program.statements[0];
    ASSERT_EQ_INT(NODE_FOR_LOOP, stmt->type, "ForLoop");
    ASSERT_EQ_STR("i", stmt->as.for_loop.var_name, "loop var");

    
    ASTNode *iter = stmt->as.for_loop.iterable;
    ASSERT_EQ_INT(NODE_FUNCTION_CALL, iter->type, "iterable is FunctionCall");
    ASSERT_EQ_STR("range", iter->as.function_call.name, "function is range");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_function_def(void) {
    const char *src =
        "def add(a, b):\n"
        "    return a + b\n";

    Parser parser;
    ASTNode *prog = parse_source(src, &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *stmt = prog->as.program.statements[0];
    ASSERT_EQ_INT(NODE_FUNCTION_DEF, stmt->type, "FunctionDef");
    ASSERT_EQ_STR("add", stmt->as.function_def.name, "function name");
    ASSERT_EQ_INT(2, stmt->as.function_def.param_count, "2 params");
    ASSERT_EQ_STR("a", stmt->as.function_def.params[0], "param 1");
    ASSERT_EQ_STR("b", stmt->as.function_def.params[1], "param 2");

    
    ASTNode *body = stmt->as.function_def.body;
    ASSERT_EQ_INT(1, body->as.block.count, "1 statement");
    ASSERT_EQ_INT(NODE_RETURN_STATEMENT, body->as.block.statements[0]->type, "return");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_function_call(void) {
    Parser parser;
    ASTNode *prog = parse_source("result = add(1, 2)\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *val = prog->as.program.statements[0]->as.assignment.value;
    ASSERT_EQ_INT(NODE_FUNCTION_CALL, val->type, "FunctionCall");
    ASSERT_EQ_STR("add", val->as.function_call.name, "function name");
    ASSERT_EQ_INT(2, val->as.function_call.arg_count, "2 args");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_list_literal(void) {
    Parser parser;
    ASTNode *prog = parse_source("a = [1, 2, 3]\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *val = prog->as.program.statements[0]->as.assignment.value;
    ASSERT_EQ_INT(NODE_LIST_LITERAL, val->type, "ListLiteral");
    ASSERT_EQ_INT(3, val->as.list_literal.count, "3 elements");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_list_indexing(void) {
    Parser parser;
    ASTNode *prog = parse_source("x = a[0]\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *val = prog->as.program.statements[0]->as.assignment.value;
    ASSERT_EQ_INT(NODE_INDEX_ACCESS, val->type, "IndexAccess");
    ASSERT_EQ_INT(NODE_IDENTIFIER, val->as.index_access.object->type, "object is Identifier");
    ASSERT_EQ_INT(NODE_INT_LITERAL, val->as.index_access.index->type, "index is IntLiteral");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_method_call(void) {
    Parser parser;
    ASTNode *prog = parse_source("a.append(1)\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *stmt = prog->as.program.statements[0];
    ASSERT_EQ_INT(NODE_EXPR_STATEMENT, stmt->type, "ExprStatement");

    ASTNode *expr = stmt->as.expr_statement.expr;
    ASSERT_EQ_INT(NODE_DOT_METHOD_CALL, expr->type, "DotMethodCall");
    ASSERT_EQ_STR("append", expr->as.dot_method_call.method, "method name");
    ASSERT_EQ_INT(1, expr->as.dot_method_call.arg_count, "1 arg");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_augmented_assignment(void) {
    Parser parser;
    ASTNode *prog = parse_source("x += 5\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *stmt = prog->as.program.statements[0];
    ASSERT_EQ_INT(NODE_AUG_ASSIGNMENT, stmt->type, "AugAssignment");
    ASSERT_EQ_INT(TOKEN_PLUS_ASSIGN, stmt->as.aug_assignment.op, "op is +=");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_unary_operators(void) {
    Parser parser;
    ASTNode *prog = parse_source("x = -y\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *val = prog->as.program.statements[0]->as.assignment.value;
    ASSERT_EQ_INT(NODE_UNARY_OP, val->type, "UnaryOp");
    ASSERT_EQ_INT(TOKEN_MINUS, val->as.unary_op.op, "op is -");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_power_operator(void) {
    Parser parser;
    ASTNode *prog = parse_source("y = x ** 2\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *val = prog->as.program.statements[0]->as.assignment.value;
    ASSERT_EQ_INT(NODE_BINARY_OP, val->type, "BinaryOp");
    ASSERT_EQ_INT(TOKEN_DOUBLE_STAR, val->as.binary_op.op, "op is **");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_nested_blocks(void) {
    const char *src =
        "def fizzbuzz(n):\n"
        "    for i in range(n):\n"
        "        if i > 0:\n"
        "            print(i)\n";

    Parser parser;
    ASTNode *prog = parse_source(src, &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *def = prog->as.program.statements[0];
    ASSERT_EQ_INT(NODE_FUNCTION_DEF, def->type, "FunctionDef");

    ASTNode *for_loop = def->as.function_def.body->as.block.statements[0];
    ASSERT_EQ_INT(NODE_FOR_LOOP, for_loop->type, "ForLoop inside def");

    ASTNode *if_stmt = for_loop->as.for_loop.body->as.block.statements[0];
    ASSERT_EQ_INT(NODE_IF_STATEMENT, if_stmt->type, "IfStatement inside for");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_print_multiple_args(void) {
    Parser parser;
    ASTNode *prog = parse_source("print(1, 2, 3)\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *stmt = prog->as.program.statements[0];
    ASSERT_EQ_INT(NODE_PRINT_STATEMENT, stmt->type, "PrintStatement");
    ASSERT_EQ_INT(3, stmt->as.print_statement.arg_count, "3 args");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_return_statement(void) {
    const char *src =
        "def foo():\n"
        "    return 42\n";

    Parser parser;
    ASTNode *prog = parse_source(src, &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *body = prog->as.program.statements[0]->as.function_def.body;
    ASTNode *ret = body->as.block.statements[0];
    ASSERT_EQ_INT(NODE_RETURN_STATEMENT, ret->type, "ReturnStatement");
    ASSERT_TRUE(ret->as.return_statement.value != NULL, "has return value");
    ASSERT_EQ_INT(42, ret->as.return_statement.value->as.int_literal.value, "returns 42");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_pass_break_continue(void) {
    const char *src =
        "while True:\n"
        "    pass\n"
        "    break\n"
        "    continue\n";

    Parser parser;
    ASTNode *prog = parse_source(src, &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *body = prog->as.program.statements[0]->as.while_loop.body;
    ASSERT_EQ_INT(3, body->as.block.count, "3 statements");
    ASSERT_EQ_INT(NODE_PASS_STATEMENT,     body->as.block.statements[0]->type, "pass");
    ASSERT_EQ_INT(NODE_BREAK_STATEMENT,    body->as.block.statements[1]->type, "break");
    ASSERT_EQ_INT(NODE_CONTINUE_STATEMENT, body->as.block.statements[2]->type, "continue");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_logical_operators(void) {
    Parser parser;
    ASTNode *prog = parse_source("x = a and b or not c\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    
    ASTNode *val = prog->as.program.statements[0]->as.assignment.value;
    ASSERT_EQ_INT(NODE_BINARY_OP, val->type, "BinaryOp");
    ASSERT_EQ_INT(TOKEN_OR, val->as.binary_op.op, "top is OR");

    
    ASSERT_EQ_INT(TOKEN_AND, val->as.binary_op.left->as.binary_op.op, "left is AND");

    
    ASSERT_EQ_INT(NODE_UNARY_OP, val->as.binary_op.right->type, "right is UnaryOp");
    ASSERT_EQ_INT(TOKEN_NOT, val->as.binary_op.right->as.unary_op.op, "right is NOT");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_comparisons(void) {
    Parser parser;
    ASTNode *prog = parse_source("x = a > b\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *val = prog->as.program.statements[0]->as.assignment.value;
    ASSERT_EQ_INT(NODE_BINARY_OP, val->type, "BinaryOp");
    ASSERT_EQ_INT(TOKEN_GT, val->as.binary_op.op, "op is >");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_empty_list(void) {
    Parser parser;
    ASTNode *prog = parse_source("a = []\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *val = prog->as.program.statements[0]->as.assignment.value;
    ASSERT_EQ_INT(NODE_LIST_LITERAL, val->type, "ListLiteral");
    ASSERT_EQ_INT(0, val->as.list_literal.count, "0 elements");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_error_missing_colon(void) {
    const char *src =
        "if x\n"
        "    y = 1\n";

    Parser parser;
    ASTNode *prog = parse_source(src, &parser);
    ASSERT_TRUE(parser_had_error(&parser), "should have parse error");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_error_missing_indent(void) {
    const char *src =
        "if x:\n"
        "y = 1\n";  

    Parser parser;
    ASTNode *prog = parse_source(src, &parser);
    ASSERT_TRUE(parser_had_error(&parser), "should have parse error");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_multiple_top_level(void) {
    const char *src =
        "x = 1\n"
        "y = 2\n"
        "z = x + y\n";

    Parser parser;
    ASTNode *prog = parse_source(src, &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");
    ASSERT_EQ_INT(3, prog->as.program.count, "3 top-level statements");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_power_vs_unary_precedence(void) {
    Parser parser;
    ASTNode *prog = parse_source("y = -x ** 2\n", &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");

    ASTNode *val = prog->as.program.statements[0]->as.assignment.value;
    
    ASSERT_EQ_INT(NODE_UNARY_OP, val->type, "top is UnaryOp");
    ASSERT_EQ_INT(TOKEN_MINUS, val->as.unary_op.op, "op is -");
    ASSERT_EQ_INT(NODE_BINARY_OP, val->as.unary_op.operand->type, "operand is BinaryOp");
    ASSERT_EQ_INT(TOKEN_DOUBLE_STAR, val->as.unary_op.operand->as.binary_op.op, "operand op is **");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





static int test_fizzbuzz_program(void) {
    const char *src =
        "def fizzbuzz(n):\n"
        "    for i in range(n):\n"
        "        if i % 15 == 0:\n"
        "            print(\"FizzBuzz\")\n"
        "        elif i % 3 == 0:\n"
        "            print(\"Fizz\")\n"
        "        elif i % 5 == 0:\n"
        "            print(\"Buzz\")\n"
        "        else:\n"
        "            print(i)\n"
        "fizzbuzz(20)\n";

    Parser parser;
    ASTNode *prog = parse_source(src, &parser);
    ASSERT_TRUE(!parser_had_error(&parser), "no parse error");
    ASSERT_EQ_INT(2, prog->as.program.count, "2 top-level (def + call)");

    ASTNode *def = prog->as.program.statements[0];
    ASSERT_EQ_INT(NODE_FUNCTION_DEF, def->type, "FunctionDef");
    ASSERT_EQ_STR("fizzbuzz", def->as.function_def.name, "name");

    ast_node_free(prog);
    free(parser.tokens);
    return 0;
}





int main(void) {
    printf("\n===== PyLite Parser — Unit Tests =====\n\n");

    RUN_TEST(test_simple_assignment);
    RUN_TEST(test_arithmetic_precedence);
    RUN_TEST(test_string_literal);
    RUN_TEST(test_bool_none_literals);
    RUN_TEST(test_if_elif_else);
    RUN_TEST(test_while_loop);
    RUN_TEST(test_for_loop_range);
    RUN_TEST(test_function_def);
    RUN_TEST(test_function_call);
    RUN_TEST(test_list_literal);
    RUN_TEST(test_list_indexing);
    RUN_TEST(test_method_call);
    RUN_TEST(test_augmented_assignment);
    RUN_TEST(test_unary_operators);
    RUN_TEST(test_power_operator);
    RUN_TEST(test_nested_blocks);
    RUN_TEST(test_print_multiple_args);
    RUN_TEST(test_return_statement);
    RUN_TEST(test_pass_break_continue);
    RUN_TEST(test_logical_operators);
    RUN_TEST(test_comparisons);
    RUN_TEST(test_empty_list);
    RUN_TEST(test_error_missing_colon);
    RUN_TEST(test_error_missing_indent);
    RUN_TEST(test_multiple_top_level);
    RUN_TEST(test_power_vs_unary_precedence);
    RUN_TEST(test_fizzbuzz_program);

    printf("\n===== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0)
        printf(" (%d FAILED)", tests_failed);
    printf(" =====\n\n");

    return tests_failed > 0 ? 1 : 0;
}
