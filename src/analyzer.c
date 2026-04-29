#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "analyzer.h"

static void report_error(Analyzer *a, int line, const char *fmt, ...) {
    if (a->error_count >= ANALYZER_MAX_ERRORS) return;

    AnalyzerError *err = &a->errors[a->error_count++];
    err->line = line;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err->message, sizeof(err->message), fmt, ap);
    va_end(ap);
}





static void analyze_node(Analyzer *a, ASTNode *node);

static void analyze_block(Analyzer *a, ASTNode *block) {
    if (!block || block->type != NODE_BLOCK) return;
    for (int i = 0; i < block->as.block.count; i++) {
        analyze_node(a, block->as.block.statements[i]);
    }
}

static void analyze_expr(Analyzer *a, ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_BINARY_OP:
            analyze_expr(a, node->as.binary_op.left);
            analyze_expr(a, node->as.binary_op.right);
            break;

        case NODE_UNARY_OP:
            analyze_expr(a, node->as.unary_op.operand);
            break;

        case NODE_FUNCTION_CALL:
            
            if (strcmp(node->as.function_call.name, "len") == 0) {
                if (node->as.function_call.arg_count != 1) {
                    report_error(a, node->line,
                        "len() takes exactly 1 argument (%d given)",
                        node->as.function_call.arg_count);
                }
            }
            for (int i = 0; i < node->as.function_call.arg_count; i++) {
                analyze_expr(a, node->as.function_call.args[i]);
            }
            break;

        case NODE_LIST_LITERAL:
            for (int i = 0; i < node->as.list_literal.count; i++) {
                analyze_expr(a, node->as.list_literal.elements[i]);
            }
            break;

        case NODE_INDEX_ACCESS:
            analyze_expr(a, node->as.index_access.object);
            analyze_expr(a, node->as.index_access.index);
            break;

        case NODE_DOT_METHOD_CALL:
            analyze_expr(a, node->as.dot_method_call.object);
            for (int i = 0; i < node->as.dot_method_call.arg_count; i++) {
                analyze_expr(a, node->as.dot_method_call.args[i]);
            }
            break;

        default:
            break;
    }
}

static void analyze_node(Analyzer *a, ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->as.program.count; i++) {
                analyze_node(a, node->as.program.statements[i]);
            }
            break;

        case NODE_BLOCK:
            analyze_block(a, node);
            break;

        case NODE_BREAK_STATEMENT:
            if (a->in_loop == 0) {
                report_error(a, node->line,
                    "'break' outside loop");
            }
            break;

        case NODE_CONTINUE_STATEMENT:
            if (a->in_loop == 0) {
                report_error(a, node->line,
                    "'continue' outside loop");
            }
            break;

        case NODE_RETURN_STATEMENT:
            if (a->in_function == 0) {
                report_error(a, node->line,
                    "'return' outside function");
            }
            if (node->as.return_statement.value) {
                analyze_expr(a, node->as.return_statement.value);
            }
            break;

        case NODE_ASSIGNMENT:
            analyze_expr(a, node->as.assignment.value);
            break;

        case NODE_AUG_ASSIGNMENT:
            analyze_expr(a, node->as.aug_assignment.value);
            break;

        case NODE_PRINT_STATEMENT:
            for (int i = 0; i < node->as.print_statement.arg_count; i++) {
                analyze_expr(a, node->as.print_statement.args[i]);
            }
            break;

        case NODE_EXPR_STATEMENT:
            analyze_expr(a, node->as.expr_statement.expr);
            break;

        case NODE_IF_STATEMENT:
            analyze_expr(a, node->as.if_statement.condition);
            analyze_block(a, node->as.if_statement.if_body);
            for (int i = 0; i < node->as.if_statement.elif_count; i++) {
                analyze_expr(a, node->as.if_statement.elifs[i].condition);
                analyze_block(a, node->as.if_statement.elifs[i].body);
            }
            if (node->as.if_statement.else_body) {
                analyze_block(a, node->as.if_statement.else_body);
            }
            break;

        case NODE_WHILE_LOOP:
            analyze_expr(a, node->as.while_loop.condition);
            a->in_loop++;
            analyze_block(a, node->as.while_loop.body);
            a->in_loop--;
            break;

        case NODE_FOR_LOOP:
            analyze_expr(a, node->as.for_loop.iterable);
            a->in_loop++;
            analyze_block(a, node->as.for_loop.body);
            a->in_loop--;
            break;

        case NODE_FUNCTION_DEF: {
            
            for (int i = 0; i < node->as.function_def.param_count; i++) {
                for (int j = i + 1; j < node->as.function_def.param_count; j++) {
                    if (strcmp(node->as.function_def.params[i],
                              node->as.function_def.params[j]) == 0) {
                        report_error(a, node->line,
                            "duplicate parameter '%s' in function '%s'",
                            node->as.function_def.params[i],
                            node->as.function_def.name);
                    }
                }
            }
            a->in_function++;
            analyze_block(a, node->as.function_def.body);
            a->in_function--;
            break;
        }

        case NODE_PASS_STATEMENT:
            
            break;

        default:
            
            analyze_expr(a, node);
            break;
    }
}





void analyzer_init(Analyzer *a) {
    a->error_count = 0;
    a->in_loop = 0;
    a->in_function = 0;
}

int analyzer_analyze(Analyzer *a, ASTNode *node) {
    analyze_node(a, node);
    return a->error_count > 0;
}

int analyzer_had_error(const Analyzer *a) {
    return a->error_count > 0;
}

void analyzer_print_errors(const Analyzer *a) {
    for (int i = 0; i < a->error_count; i++) {
        fprintf(stderr, "Semantic error (line %d): %s\n",
                a->errors[i].line, a->errors[i].message);
    }
}
