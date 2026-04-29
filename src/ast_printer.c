

#include <stdio.h>
#include <string.h>
#include "ast.h"





static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
}

static const char *op_symbol(TokenType op) {
    switch (op) {
        case TOKEN_PLUS:         return "+";
        case TOKEN_MINUS:        return "-";
        case TOKEN_STAR:         return "*";
        case TOKEN_SLASH:        return "/";
        case TOKEN_DOUBLE_SLASH: return "//";
        case TOKEN_PERCENT:      return "%";
        case TOKEN_DOUBLE_STAR:  return "**";
        case TOKEN_EQ:           return "==";
        case TOKEN_NEQ:          return "!=";
        case TOKEN_LT:           return "<";
        case TOKEN_GT:           return ">";
        case TOKEN_LTE:          return "<=";
        case TOKEN_GTE:          return ">=";
        case TOKEN_AND:          return "and";
        case TOKEN_OR:           return "or";
        case TOKEN_NOT:          return "not";
        case TOKEN_PLUS_ASSIGN:  return "+=";
        case TOKEN_MINUS_ASSIGN: return "-=";
        case TOKEN_STAR_ASSIGN:  return "*=";
        case TOKEN_SLASH_ASSIGN: return "/=";
        default:                 return "?";
    }
}





static void print_expr_inline(const ASTNode *node) {
    if (!node) { printf("null"); return; }

    switch (node->type) {
        case NODE_INT_LITERAL:
            printf("IntLiteral(%d)", node->as.int_literal.value);
            break;

        case NODE_STRING_LITERAL:
            printf("StringLiteral(\"%s\")", node->as.string_literal.value);
            break;

        case NODE_BOOL_LITERAL:
            printf("BoolLiteral(%s)", node->as.bool_literal.value ? "True" : "False");
            break;

        case NODE_NONE_LITERAL:
            printf("NoneLiteral");
            break;

        case NODE_IDENTIFIER:
            printf("Identifier(%s)", node->as.identifier.name);
            break;

        case NODE_BINARY_OP:
            printf("BinaryOp(%s, ", op_symbol(node->as.binary_op.op));
            print_expr_inline(node->as.binary_op.left);
            printf(", ");
            print_expr_inline(node->as.binary_op.right);
            printf(")");
            break;

        case NODE_UNARY_OP:
            printf("UnaryOp(%s, ", op_symbol(node->as.unary_op.op));
            print_expr_inline(node->as.unary_op.operand);
            printf(")");
            break;

        case NODE_FUNCTION_CALL:
            printf("FunctionCall(%s", node->as.function_call.name);
            for (int i = 0; i < node->as.function_call.arg_count; i++) {
                printf(", ");
                print_expr_inline(node->as.function_call.args[i]);
            }
            printf(")");
            break;

        case NODE_LIST_LITERAL:
            printf("ListLiteral(");
            for (int i = 0; i < node->as.list_literal.count; i++) {
                if (i > 0) printf(", ");
                print_expr_inline(node->as.list_literal.elements[i]);
            }
            printf(")");
            break;

        case NODE_INDEX_ACCESS:
            printf("IndexAccess(");
            print_expr_inline(node->as.index_access.object);
            printf(", ");
            print_expr_inline(node->as.index_access.index);
            printf(")");
            break;

        case NODE_DOT_METHOD_CALL:
            print_expr_inline(node->as.dot_method_call.object);
            printf(".%s(", node->as.dot_method_call.method);
            for (int i = 0; i < node->as.dot_method_call.arg_count; i++) {
                if (i > 0) printf(", ");
                print_expr_inline(node->as.dot_method_call.args[i]);
            }
            printf(")");
            break;

        default:
            printf("<%s>", ast_node_type_name(node->type));
            break;
    }
}





void ast_print(const ASTNode *node, int indent) {
    if (!node) return;

    switch (node->type) {

        case NODE_PROGRAM:
            print_indent(indent);
            printf("Program\n");
            for (int i = 0; i < node->as.program.count; i++) {
                ast_print(node->as.program.statements[i], indent + 1);
            }
            break;

        case NODE_BLOCK:
            print_indent(indent);
            printf("Block\n");
            for (int i = 0; i < node->as.block.count; i++) {
                ast_print(node->as.block.statements[i], indent + 1);
            }
            break;

        

        case NODE_ASSIGNMENT:
            print_indent(indent);
            printf("Assignment: ");
            print_expr_inline(node->as.assignment.target);
            printf(" = ");
            print_expr_inline(node->as.assignment.value);
            printf("\n");
            break;

        case NODE_AUG_ASSIGNMENT:
            print_indent(indent);
            printf("AugAssignment: ");
            print_expr_inline(node->as.aug_assignment.target);
            printf(" %s ", op_symbol(node->as.aug_assignment.op));
            print_expr_inline(node->as.aug_assignment.value);
            printf("\n");
            break;

        case NODE_PRINT_STATEMENT:
            print_indent(indent);
            printf("PrintStatement: ");
            for (int i = 0; i < node->as.print_statement.arg_count; i++) {
                if (i > 0) printf(", ");
                print_expr_inline(node->as.print_statement.args[i]);
            }
            printf("\n");
            break;

        case NODE_RETURN_STATEMENT:
            print_indent(indent);
            printf("ReturnStatement");
            if (node->as.return_statement.value) {
                printf(": ");
                print_expr_inline(node->as.return_statement.value);
            }
            printf("\n");
            break;

        case NODE_PASS_STATEMENT:
            print_indent(indent);
            printf("PassStatement\n");
            break;

        case NODE_BREAK_STATEMENT:
            print_indent(indent);
            printf("BreakStatement\n");
            break;

        case NODE_CONTINUE_STATEMENT:
            print_indent(indent);
            printf("ContinueStatement\n");
            break;

        case NODE_EXPR_STATEMENT:
            print_indent(indent);
            printf("ExprStatement: ");
            print_expr_inline(node->as.expr_statement.expr);
            printf("\n");
            break;

        

        case NODE_IF_STATEMENT:
            print_indent(indent);
            printf("IfStatement\n");
            print_indent(indent + 1);
            printf("Condition: ");
            print_expr_inline(node->as.if_statement.condition);
            printf("\n");
            ast_print(node->as.if_statement.if_body, indent + 1);

            for (int i = 0; i < node->as.if_statement.elif_count; i++) {
                print_indent(indent + 1);
                printf("Elif\n");
                print_indent(indent + 2);
                printf("Condition: ");
                print_expr_inline(node->as.if_statement.elifs[i].condition);
                printf("\n");
                ast_print(node->as.if_statement.elifs[i].body, indent + 2);
            }

            if (node->as.if_statement.else_body) {
                print_indent(indent + 1);
                printf("Else\n");
                ast_print(node->as.if_statement.else_body, indent + 2);
            }
            break;

        case NODE_WHILE_LOOP:
            print_indent(indent);
            printf("WhileLoop\n");
            print_indent(indent + 1);
            printf("Condition: ");
            print_expr_inline(node->as.while_loop.condition);
            printf("\n");
            ast_print(node->as.while_loop.body, indent + 1);
            break;

        case NODE_FOR_LOOP:
            print_indent(indent);
            printf("ForLoop: %s in ", node->as.for_loop.var_name);
            print_expr_inline(node->as.for_loop.iterable);
            printf("\n");
            ast_print(node->as.for_loop.body, indent + 1);
            break;

        case NODE_FUNCTION_DEF:
            print_indent(indent);
            printf("FunctionDef: %s(", node->as.function_def.name);
            for (int i = 0; i < node->as.function_def.param_count; i++) {
                if (i > 0) printf(", ");
                printf("%s", node->as.function_def.params[i]);
            }
            printf(")\n");
            ast_print(node->as.function_def.body, indent + 1);
            break;

        
        default:
            print_indent(indent);
            print_expr_inline(node);
            printf("\n");
            break;
    }
}
