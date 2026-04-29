#include <stdlib.h>
#include <string.h>
#include "ast.h"

ASTNode *ast_node_create(ASTNodeType type, int line) {
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    node->type = type;
    node->line = line;
    return node;
}

void ast_node_free(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_INT_LITERAL:
        case NODE_BOOL_LITERAL:
        case NODE_NONE_LITERAL:
        case NODE_PASS_STATEMENT:
        case NODE_BREAK_STATEMENT:
        case NODE_CONTINUE_STATEMENT:
            break;

        case NODE_STRING_LITERAL:
            free(node->as.string_literal.value);
            break;

        case NODE_IDENTIFIER:
            free(node->as.identifier.name);
            break;

        case NODE_BINARY_OP:
            ast_node_free(node->as.binary_op.left);
            ast_node_free(node->as.binary_op.right);
            break;

        case NODE_UNARY_OP:
            ast_node_free(node->as.unary_op.operand);
            break;

        case NODE_FUNCTION_CALL:
            free(node->as.function_call.name);
            for (int i = 0; i < node->as.function_call.arg_count; i++)
                ast_node_free(node->as.function_call.args[i]);
            free(node->as.function_call.args);
            break;

        case NODE_LIST_LITERAL:
            for (int i = 0; i < node->as.list_literal.count; i++)
                ast_node_free(node->as.list_literal.elements[i]);
            free(node->as.list_literal.elements);
            break;

        case NODE_INDEX_ACCESS:
            ast_node_free(node->as.index_access.object);
            ast_node_free(node->as.index_access.index);
            break;

        case NODE_DOT_METHOD_CALL:
            ast_node_free(node->as.dot_method_call.object);
            free(node->as.dot_method_call.method);
            for (int i = 0; i < node->as.dot_method_call.arg_count; i++)
                ast_node_free(node->as.dot_method_call.args[i]);
            free(node->as.dot_method_call.args);
            break;

        case NODE_ASSIGNMENT:
            ast_node_free(node->as.assignment.target);
            ast_node_free(node->as.assignment.value);
            break;

        case NODE_AUG_ASSIGNMENT:
            ast_node_free(node->as.aug_assignment.target);
            ast_node_free(node->as.aug_assignment.value);
            break;

        case NODE_PRINT_STATEMENT:
            for (int i = 0; i < node->as.print_statement.arg_count; i++)
                ast_node_free(node->as.print_statement.args[i]);
            free(node->as.print_statement.args);
            break;

        case NODE_RETURN_STATEMENT:
            ast_node_free(node->as.return_statement.value);
            break;

        case NODE_EXPR_STATEMENT:
            ast_node_free(node->as.expr_statement.expr);
            break;

        case NODE_IF_STATEMENT:
            ast_node_free(node->as.if_statement.condition);
            ast_node_free(node->as.if_statement.if_body);
            for (int i = 0; i < node->as.if_statement.elif_count; i++) {
                ast_node_free(node->as.if_statement.elifs[i].condition);
                ast_node_free(node->as.if_statement.elifs[i].body);
            }
            free(node->as.if_statement.elifs);
            ast_node_free(node->as.if_statement.else_body);
            break;

        case NODE_WHILE_LOOP:
            ast_node_free(node->as.while_loop.condition);
            ast_node_free(node->as.while_loop.body);
            break;

        case NODE_FOR_LOOP:
            free(node->as.for_loop.var_name);
            ast_node_free(node->as.for_loop.iterable);
            ast_node_free(node->as.for_loop.body);
            break;

        case NODE_FUNCTION_DEF:
            free(node->as.function_def.name);
            for (int i = 0; i < node->as.function_def.param_count; i++)
                free(node->as.function_def.params[i]);
            free(node->as.function_def.params);
            ast_node_free(node->as.function_def.body);
            break;

        case NODE_BLOCK:
            for (int i = 0; i < node->as.block.count; i++)
                ast_node_free(node->as.block.statements[i]);
            free(node->as.block.statements);
            break;

        case NODE_PROGRAM:
            for (int i = 0; i < node->as.program.count; i++)
                ast_node_free(node->as.program.statements[i]);
            free(node->as.program.statements);
            break;
    }

    free(node);
}

const char *ast_node_type_name(ASTNodeType type) {
    switch (type) {
        case NODE_INT_LITERAL:        return "IntLiteral";
        case NODE_STRING_LITERAL:     return "StringLiteral";
        case NODE_BOOL_LITERAL:       return "BoolLiteral";
        case NODE_NONE_LITERAL:       return "NoneLiteral";
        case NODE_IDENTIFIER:         return "Identifier";
        case NODE_BINARY_OP:          return "BinaryOp";
        case NODE_UNARY_OP:           return "UnaryOp";
        case NODE_FUNCTION_CALL:      return "FunctionCall";
        case NODE_LIST_LITERAL:       return "ListLiteral";
        case NODE_INDEX_ACCESS:       return "IndexAccess";
        case NODE_DOT_METHOD_CALL:    return "DotMethodCall";
        case NODE_ASSIGNMENT:         return "Assignment";
        case NODE_AUG_ASSIGNMENT:     return "AugmentedAssignment";
        case NODE_PRINT_STATEMENT:    return "PrintStatement";
        case NODE_RETURN_STATEMENT:   return "ReturnStatement";
        case NODE_PASS_STATEMENT:     return "PassStatement";
        case NODE_BREAK_STATEMENT:    return "BreakStatement";
        case NODE_CONTINUE_STATEMENT: return "ContinueStatement";
        case NODE_EXPR_STATEMENT:     return "ExpressionStatement";
        case NODE_IF_STATEMENT:       return "IfStatement";
        case NODE_WHILE_LOOP:         return "WhileLoop";
        case NODE_FOR_LOOP:           return "ForLoop";
        case NODE_FUNCTION_DEF:       return "FunctionDef";
        case NODE_BLOCK:              return "Block";
        case NODE_PROGRAM:            return "Program";
    }
    return "Unknown";
}
