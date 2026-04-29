#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"


static Token *current(Parser *p) {
    if (p->pos < p->token_count) return &p->tokens[p->pos];
    return &p->tokens[p->token_count - 1]; 
}

static Token *previous(Parser *p) {
    if (p->pos > 0) return &p->tokens[p->pos - 1];
    return &p->tokens[0];
}

static int check(Parser *p, TokenType type) {
    return current(p)->type == type;
}

static int at_end(Parser *p) {
    return current(p)->type == TOKEN_EOF;
}

static Token *advance_token(Parser *p) {
    if (!at_end(p)) p->pos++;
    return previous(p);
}

static int match(Parser *p, TokenType type) {
    if (check(p, type)) {
        advance_token(p);
        return 1;
    }
    return 0;
}

static void parser_error(Parser *p, const char *msg) {
    if (p->had_error) return; 
    p->had_error = 1;
    p->error_line = current(p)->line;
    snprintf(p->error_msg, sizeof(p->error_msg),
             "Line %d: %s (got %s)",
             p->error_line, msg, token_type_name(current(p)->type));
}

static Token *expect(Parser *p, TokenType type, const char *msg) {
    if (check(p, type)) return advance_token(p);
    parser_error(p, msg);
    return NULL;
}


static char *dup_lexeme(const Token *tok) {
    char *s = (char *)malloc(tok->length + 1);
    if (s) {
        memcpy(s, tok->lexeme, tok->length);
        s[tok->length] = '\0';
    }
    return s;
}


static char *dup_str(const char *src) {
    size_t len = strlen(src);
    char *s = (char *)malloc(len + 1);
    if (s) memcpy(s, src, len + 1);
    return s;
}


static void skip_newlines(Parser *p) {
    while (check(p, TOKEN_NEWLINE)) advance_token(p);
}





static ASTNode *parse_statement(Parser *p);
static ASTNode *parse_expression(Parser *p);
static ASTNode *parse_block(Parser *p);
static ASTNode *parse_unary(Parser *p);






static ASTNode *parse_primary(Parser *p) {
    if (p->had_error) return NULL;

    if (check(p, TOKEN_INT_LITERAL)) {
        Token *tok = advance_token(p);
        ASTNode *node = ast_node_create(NODE_INT_LITERAL, tok->line);
        
        char buf[32];
        int len = tok->length < 31 ? tok->length : 31;
        memcpy(buf, tok->lexeme, len);
        buf[len] = '\0';
        node->as.int_literal.value = atoi(buf);
        return node;
    }

    if (check(p, TOKEN_STRING_LITERAL)) {
        Token *tok = advance_token(p);
        ASTNode *node = ast_node_create(NODE_STRING_LITERAL, tok->line);
        
        int slen = tok->length - 2; 
        if (slen < 0) slen = 0;
        node->as.string_literal.value = (char *)malloc(slen + 1);
        memcpy(node->as.string_literal.value, tok->lexeme + 1, slen);
        node->as.string_literal.value[slen] = '\0';
        node->as.string_literal.length = slen;
        return node;
    }

    if (check(p, TOKEN_TRUE)) {
        Token *tok = advance_token(p);
        ASTNode *node = ast_node_create(NODE_BOOL_LITERAL, tok->line);
        node->as.bool_literal.value = 1;
        return node;
    }

    if (check(p, TOKEN_FALSE)) {
        Token *tok = advance_token(p);
        ASTNode *node = ast_node_create(NODE_BOOL_LITERAL, tok->line);
        node->as.bool_literal.value = 0;
        return node;
    }

    if (check(p, TOKEN_NONE)) {
        Token *tok = advance_token(p);
        return ast_node_create(NODE_NONE_LITERAL, tok->line);
    }

    
    if (check(p, TOKEN_IDENTIFIER) ||
        check(p, TOKEN_RANGE) || check(p, TOKEN_LEN) ||
        check(p, TOKEN_INPUT) || check(p, TOKEN_TYPE)) {
        Token *tok = advance_token(p);
        ASTNode *node = ast_node_create(NODE_IDENTIFIER, tok->line);
        node->as.identifier.name = dup_lexeme(tok);
        return node;
    }

    
    if (check(p, TOKEN_LPAREN)) {
        advance_token(p);
        ASTNode *expr = parse_expression(p);
        expect(p, TOKEN_RPAREN, "Expected ')' after expression");
        return expr;
    }

    
    if (check(p, TOKEN_LBRACKET)) {
        Token *tok = advance_token(p);
        ASTNode *node = ast_node_create(NODE_LIST_LITERAL, tok->line);
        int capacity = 8;
        int count = 0;
        ASTNode **elements = (ASTNode **)malloc(sizeof(ASTNode *) * capacity);

        if (!check(p, TOKEN_RBRACKET)) {
            do {
                if (count >= capacity) {
                    capacity *= 2;
                    elements = (ASTNode **)realloc(elements, sizeof(ASTNode *) * capacity);
                }
                elements[count++] = parse_expression(p);
                if (p->had_error) break;
            } while (match(p, TOKEN_COMMA));
        }
        expect(p, TOKEN_RBRACKET, "Expected ']' after list elements");
        node->as.list_literal.elements = elements;
        node->as.list_literal.count = count;
        return node;
    }

    parser_error(p, "Expected expression");
    return NULL;
}


static ASTNode *parse_call(Parser *p) {
    ASTNode *node = parse_primary(p);
    if (p->had_error) return node;

    for (;;) {
        if (check(p, TOKEN_LPAREN)) {
            
            advance_token(p);
            int capacity = 8;
            int count = 0;
            ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * capacity);

            if (!check(p, TOKEN_RPAREN)) {
                do {
                    if (count >= capacity) {
                        capacity *= 2;
                        args = (ASTNode **)realloc(args, sizeof(ASTNode *) * capacity);
                    }
                    args[count++] = parse_expression(p);
                    if (p->had_error) break;
                } while (match(p, TOKEN_COMMA));
            }
            expect(p, TOKEN_RPAREN, "Expected ')' after arguments");

            
            ASTNode *call = ast_node_create(NODE_FUNCTION_CALL, node->line);
            if (node->type == NODE_IDENTIFIER) {
                call->as.function_call.name = node->as.identifier.name;
                node->as.identifier.name = NULL; 
                ast_node_free(node);
            } else {
                
                call->as.function_call.name = dup_str("<expr>");
                ast_node_free(node);
            }
            call->as.function_call.args = args;
            call->as.function_call.arg_count = count;
            node = call;

        } else if (check(p, TOKEN_LBRACKET)) {
            
            advance_token(p);
            ASTNode *index = parse_expression(p);
            expect(p, TOKEN_RBRACKET, "Expected ']' after index");

            ASTNode *access = ast_node_create(NODE_INDEX_ACCESS, node->line);
            access->as.index_access.object = node;
            access->as.index_access.index = index;
            node = access;

        } else if (check(p, TOKEN_DOT)) {
            
            advance_token(p);
            Token *method_tok = NULL;
            
            if (check(p, TOKEN_IDENTIFIER) || check(p, TOKEN_APPEND) ||
                check(p, TOKEN_POP)) {
                method_tok = advance_token(p);
            } else {
                parser_error(p, "Expected method name after '.'");
                return node;
            }

            expect(p, TOKEN_LPAREN, "Expected '(' after method name");

            int capacity = 4;
            int count = 0;
            ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * capacity);

            if (!check(p, TOKEN_RPAREN)) {
                do {
                    if (count >= capacity) {
                        capacity *= 2;
                        args = (ASTNode **)realloc(args, sizeof(ASTNode *) * capacity);
                    }
                    args[count++] = parse_expression(p);
                    if (p->had_error) break;
                } while (match(p, TOKEN_COMMA));
            }
            expect(p, TOKEN_RPAREN, "Expected ')' after method arguments");

            ASTNode *dot = ast_node_create(NODE_DOT_METHOD_CALL, node->line);
            dot->as.dot_method_call.object = node;
            dot->as.dot_method_call.method = dup_lexeme(method_tok);
            dot->as.dot_method_call.args = args;
            dot->as.dot_method_call.arg_count = count;
            node = dot;

        } else {
            break;
        }
    }

    return node;
}


static ASTNode *parse_power(Parser *p) {
    ASTNode *left = parse_call(p);
    if (p->had_error) return left;

    if (match(p, TOKEN_DOUBLE_STAR)) {
        
        ASTNode *right;
        
        right = parse_unary(p);

        ASTNode *node = ast_node_create(NODE_BINARY_OP, left->line);
        node->as.binary_op.op = TOKEN_DOUBLE_STAR;
        node->as.binary_op.left = left;
        node->as.binary_op.right = right;
        return node;
    }

    return left;
}


static ASTNode *parse_unary(Parser *p) {
    if (p->had_error) return NULL;

    if (check(p, TOKEN_MINUS) || check(p, TOKEN_NOT)) {
        Token *op = advance_token(p);
        ASTNode *operand = parse_unary(p);
        ASTNode *node = ast_node_create(NODE_UNARY_OP, op->line);
        node->as.unary_op.op = op->type;
        node->as.unary_op.operand = operand;
        return node;
    }

    return parse_power(p);
}


static ASTNode *parse_multiplication(Parser *p) {
    ASTNode *left = parse_unary(p);
    if (p->had_error) return left;

    while (check(p, TOKEN_STAR) || check(p, TOKEN_SLASH) ||
           check(p, TOKEN_DOUBLE_SLASH) || check(p, TOKEN_PERCENT)) {
        Token *op = advance_token(p);
        ASTNode *right = parse_unary(p);

        ASTNode *node = ast_node_create(NODE_BINARY_OP, left->line);
        node->as.binary_op.op = op->type;
        node->as.binary_op.left = left;
        node->as.binary_op.right = right;
        left = node;
    }

    return left;
}


static ASTNode *parse_addition(Parser *p) {
    ASTNode *left = parse_multiplication(p);
    if (p->had_error) return left;

    while (check(p, TOKEN_PLUS) || check(p, TOKEN_MINUS)) {
        Token *op = advance_token(p);
        ASTNode *right = parse_multiplication(p);

        ASTNode *node = ast_node_create(NODE_BINARY_OP, left->line);
        node->as.binary_op.op = op->type;
        node->as.binary_op.left = left;
        node->as.binary_op.right = right;
        left = node;
    }

    return left;
}


static ASTNode *parse_comparison(Parser *p) {
    ASTNode *left = parse_addition(p);
    if (p->had_error) return left;

    while (check(p, TOKEN_EQ) || check(p, TOKEN_NEQ) ||
           check(p, TOKEN_LT) || check(p, TOKEN_GT) ||
           check(p, TOKEN_LTE) || check(p, TOKEN_GTE)) {
        Token *op = advance_token(p);
        ASTNode *right = parse_addition(p);

        ASTNode *node = ast_node_create(NODE_BINARY_OP, left->line);
        node->as.binary_op.op = op->type;
        node->as.binary_op.left = left;
        node->as.binary_op.right = right;
        left = node;
    }

    return left;
}


static ASTNode *parse_not_expr(Parser *p) {
    if (p->had_error) return NULL;

    if (check(p, TOKEN_NOT)) {
        Token *op = advance_token(p);
        ASTNode *operand = parse_not_expr(p);
        ASTNode *node = ast_node_create(NODE_UNARY_OP, op->line);
        node->as.unary_op.op = TOKEN_NOT;
        node->as.unary_op.operand = operand;
        return node;
    }

    return parse_comparison(p);
}


static ASTNode *parse_and_expr(Parser *p) {
    ASTNode *left = parse_not_expr(p);
    if (p->had_error) return left;

    while (check(p, TOKEN_AND)) {
        Token *op = advance_token(p);
        ASTNode *right = parse_not_expr(p);

        ASTNode *node = ast_node_create(NODE_BINARY_OP, left->line);
        node->as.binary_op.op = op->type;
        node->as.binary_op.left = left;
        node->as.binary_op.right = right;
        left = node;
    }

    return left;
}


static ASTNode *parse_or_expr(Parser *p) {
    ASTNode *left = parse_and_expr(p);
    if (p->had_error) return left;

    while (check(p, TOKEN_OR)) {
        Token *op = advance_token(p);
        ASTNode *right = parse_and_expr(p);

        ASTNode *node = ast_node_create(NODE_BINARY_OP, left->line);
        node->as.binary_op.op = op->type;
        node->as.binary_op.left = left;
        node->as.binary_op.right = right;
        left = node;
    }

    return left;
}


static ASTNode *parse_expression(Parser *p) {
    return parse_or_expr(p);
}






static ASTNode *parse_block(Parser *p) {
    if (p->had_error) return NULL;

    if (!expect(p, TOKEN_INDENT, "Expected indented block")) return NULL;

    int capacity = 8;
    int count = 0;
    ASTNode **stmts = (ASTNode **)malloc(sizeof(ASTNode *) * capacity);

    while (!check(p, TOKEN_DEDENT) && !at_end(p) && !p->had_error) {
        skip_newlines(p);
        if (check(p, TOKEN_DEDENT) || at_end(p)) break;

        if (count >= capacity) {
            capacity *= 2;
            stmts = (ASTNode **)realloc(stmts, sizeof(ASTNode *) * capacity);
        }
        ASTNode *stmt = parse_statement(p);
        if (stmt) stmts[count++] = stmt;
        if (p->had_error) break;
    }

    if (!p->had_error) {
        expect(p, TOKEN_DEDENT, "Expected dedent after block");
    }

    ASTNode *block = ast_node_create(NODE_BLOCK, stmts[0] ? stmts[0]->line : 0);
    block->as.block.statements = stmts;
    block->as.block.count = count;
    return block;
}






static ASTNode *parse_print_statement(Parser *p) {
    Token *tok = advance_token(p); 
    expect(p, TOKEN_LPAREN, "Expected '(' after 'print'");

    int capacity = 4;
    int count = 0;
    ASTNode **args = (ASTNode **)malloc(sizeof(ASTNode *) * capacity);

    if (!check(p, TOKEN_RPAREN)) {
        do {
            if (count >= capacity) {
                capacity *= 2;
                args = (ASTNode **)realloc(args, sizeof(ASTNode *) * capacity);
            }
            args[count++] = parse_expression(p);
            if (p->had_error) break;
        } while (match(p, TOKEN_COMMA));
    }
    expect(p, TOKEN_RPAREN, "Expected ')' after print arguments");

    ASTNode *node = ast_node_create(NODE_PRINT_STATEMENT, tok->line);
    node->as.print_statement.args = args;
    node->as.print_statement.arg_count = count;
    return node;
}


static ASTNode *parse_assignment_or_expr(Parser *p) {
    
    ASTNode *lhs = parse_expression(p);
    if (p->had_error) return lhs;

    
    if (check(p, TOKEN_ASSIGN)) {
        advance_token(p);
        ASTNode *rhs = parse_expression(p);

        ASTNode *node = ast_node_create(NODE_ASSIGNMENT, lhs->line);
        node->as.assignment.target = lhs;
        node->as.assignment.value = rhs;
        return node;
    }

    
    if (check(p, TOKEN_PLUS_ASSIGN) || check(p, TOKEN_MINUS_ASSIGN) ||
        check(p, TOKEN_STAR_ASSIGN) || check(p, TOKEN_SLASH_ASSIGN)) {
        Token *op = advance_token(p);
        ASTNode *rhs = parse_expression(p);

        ASTNode *node = ast_node_create(NODE_AUG_ASSIGNMENT, lhs->line);
        node->as.aug_assignment.op = op->type;
        node->as.aug_assignment.target = lhs;
        node->as.aug_assignment.value = rhs;
        return node;
    }

    
    ASTNode *node = ast_node_create(NODE_EXPR_STATEMENT, lhs->line);
    node->as.expr_statement.expr = lhs;
    return node;
}


static ASTNode *parse_if_statement(Parser *p) {
    Token *tok = advance_token(p); 
    ASTNode *condition = parse_expression(p);
    expect(p, TOKEN_COLON, "Expected ':' after if condition");
    expect(p, TOKEN_NEWLINE, "Expected newline after ':'");
    skip_newlines(p);
    ASTNode *if_body = parse_block(p);

    
    int elif_cap = 4;
    int elif_count = 0;
    ElifBranch *elifs = (ElifBranch *)malloc(sizeof(ElifBranch) * elif_cap);

    skip_newlines(p);
    while (check(p, TOKEN_ELIF) && !p->had_error) {
        advance_token(p); 
        ASTNode *elif_cond = parse_expression(p);
        expect(p, TOKEN_COLON, "Expected ':' after elif condition");
        expect(p, TOKEN_NEWLINE, "Expected newline after ':'");
        skip_newlines(p);
        ASTNode *elif_body = parse_block(p);

        if (elif_count >= elif_cap) {
            elif_cap *= 2;
            elifs = (ElifBranch *)realloc(elifs, sizeof(ElifBranch) * elif_cap);
        }
        elifs[elif_count].condition = elif_cond;
        elifs[elif_count].body = elif_body;
        elif_count++;
        skip_newlines(p);
    }

    
    ASTNode *else_body = NULL;
    skip_newlines(p);
    if (check(p, TOKEN_ELSE) && !p->had_error) {
        advance_token(p); 
        expect(p, TOKEN_COLON, "Expected ':' after else");
        expect(p, TOKEN_NEWLINE, "Expected newline after ':'");
        skip_newlines(p);
        else_body = parse_block(p);
    }

    ASTNode *node = ast_node_create(NODE_IF_STATEMENT, tok->line);
    node->as.if_statement.condition = condition;
    node->as.if_statement.if_body = if_body;
    node->as.if_statement.elifs = elifs;
    node->as.if_statement.elif_count = elif_count;
    node->as.if_statement.else_body = else_body;
    return node;
}


static ASTNode *parse_while_statement(Parser *p) {
    Token *tok = advance_token(p); 
    ASTNode *condition = parse_expression(p);
    expect(p, TOKEN_COLON, "Expected ':' after while condition");
    expect(p, TOKEN_NEWLINE, "Expected newline after ':'");
    skip_newlines(p);
    ASTNode *body = parse_block(p);

    ASTNode *node = ast_node_create(NODE_WHILE_LOOP, tok->line);
    node->as.while_loop.condition = condition;
    node->as.while_loop.body = body;
    return node;
}


static ASTNode *parse_for_statement(Parser *p) {
    Token *tok = advance_token(p); 
    Token *var = expect(p, TOKEN_IDENTIFIER, "Expected loop variable after 'for'");
    expect(p, TOKEN_IN, "Expected 'in' after loop variable");
    ASTNode *iterable = parse_expression(p);
    expect(p, TOKEN_COLON, "Expected ':' after for expression");
    expect(p, TOKEN_NEWLINE, "Expected newline after ':'");
    skip_newlines(p);
    ASTNode *body = parse_block(p);

    ASTNode *node = ast_node_create(NODE_FOR_LOOP, tok->line);
    node->as.for_loop.var_name = var ? dup_lexeme(var) : dup_str("?");
    node->as.for_loop.iterable = iterable;
    node->as.for_loop.body = body;
    return node;
}


static ASTNode *parse_function_def(Parser *p) {
    Token *tok = advance_token(p); 
    Token *name = expect(p, TOKEN_IDENTIFIER, "Expected function name after 'def'");
    expect(p, TOKEN_LPAREN, "Expected '(' after function name");

    
    int param_cap = 8;
    int param_count = 0;
    char **params = (char **)malloc(sizeof(char *) * param_cap);

    if (!check(p, TOKEN_RPAREN)) {
        do {
            Token *param = expect(p, TOKEN_IDENTIFIER, "Expected parameter name");
            if (param) {
                if (param_count >= param_cap) {
                    param_cap *= 2;
                    params = (char **)realloc(params, sizeof(char *) * param_cap);
                }
                params[param_count++] = dup_lexeme(param);
            }
            if (p->had_error) break;
        } while (match(p, TOKEN_COMMA));
    }
    expect(p, TOKEN_RPAREN, "Expected ')' after parameters");
    expect(p, TOKEN_COLON, "Expected ':' after function signature");
    expect(p, TOKEN_NEWLINE, "Expected newline after ':'");
    skip_newlines(p);
    ASTNode *body = parse_block(p);

    ASTNode *node = ast_node_create(NODE_FUNCTION_DEF, tok->line);
    node->as.function_def.name = name ? dup_lexeme(name) : dup_str("?");
    node->as.function_def.params = params;
    node->as.function_def.param_count = param_count;
    node->as.function_def.body = body;
    return node;
}


static ASTNode *parse_statement(Parser *p) {
    if (p->had_error) return NULL;
    skip_newlines(p);
    if (at_end(p)) return NULL;

    
    if (check(p, TOKEN_IF))    return parse_if_statement(p);
    if (check(p, TOKEN_WHILE)) return parse_while_statement(p);
    if (check(p, TOKEN_FOR))   return parse_for_statement(p);
    if (check(p, TOKEN_DEF))   return parse_function_def(p);

    
    ASTNode *stmt = NULL;

    if (check(p, TOKEN_RETURN)) {
        Token *tok = advance_token(p);
        stmt = ast_node_create(NODE_RETURN_STATEMENT, tok->line);
        
        if (!check(p, TOKEN_NEWLINE) && !at_end(p) && !check(p, TOKEN_DEDENT)) {
            stmt->as.return_statement.value = parse_expression(p);
        }
    } else if (check(p, TOKEN_BREAK)) {
        Token *tok = advance_token(p);
        stmt = ast_node_create(NODE_BREAK_STATEMENT, tok->line);
    } else if (check(p, TOKEN_CONTINUE)) {
        Token *tok = advance_token(p);
        stmt = ast_node_create(NODE_CONTINUE_STATEMENT, tok->line);
    } else if (check(p, TOKEN_PASS)) {
        Token *tok = advance_token(p);
        stmt = ast_node_create(NODE_PASS_STATEMENT, tok->line);
    } else if (check(p, TOKEN_PRINT)) {
        stmt = parse_print_statement(p);
    } else {
        
        stmt = parse_assignment_or_expr(p);
    }

    
    if (!p->had_error && check(p, TOKEN_NEWLINE)) {
        advance_token(p);
    }

    return stmt;
}





void parser_init(Parser *parser, Token *tokens, int token_count) {
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->pos = 0;
    parser->had_error = 0;
    parser->error_msg[0] = '\0';
    parser->error_line = 0;
}

ASTNode *parser_parse(Parser *parser) {
    int capacity = 16;
    int count = 0;
    ASTNode **stmts = (ASTNode **)malloc(sizeof(ASTNode *) * capacity);
    if (!stmts) return NULL;

    skip_newlines(parser);

    while (!at_end(parser) && !parser->had_error) {
        skip_newlines(parser);
        if (at_end(parser)) break;

        if (count >= capacity) {
            capacity *= 2;
            stmts = (ASTNode **)realloc(stmts, sizeof(ASTNode *) * capacity);
        }
        ASTNode *stmt = parse_statement(parser);
        if (stmt) stmts[count++] = stmt;
    }

    ASTNode *program = ast_node_create(NODE_PROGRAM, 1);
    program->as.program.statements = stmts;
    program->as.program.count = count;
    return program;
}

int parser_had_error(const Parser *parser) {
    return parser->had_error;
}

const char *parser_error_message(const Parser *parser) {
    return parser->error_msg;
}
