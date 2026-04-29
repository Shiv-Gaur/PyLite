#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "builtins.h"

static char *my_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *d = (char *)malloc(len + 1);
    if (d) memcpy(d, s, len + 1);
    return d;
}

extern void interp_set_global(Interpreter *ip);
extern Interpreter *interp_get_global(void);
extern PyLiteValue eval_expr(ASTNode *node, Environment *env);

void exec_stmt(ASTNode *node, Environment *env, int *ret, PyLiteValue *retval, int *brk, int *cont) {
    Interpreter *gi = interp_get_global();
    if (!node || (gi && gi->had_error) || *ret || *brk || *cont) return;

    switch (node->type) {
    case NODE_PRINT_STATEMENT: {
        for (int i = 0; i < node->as.print_statement.arg_count; i++) {
            if (i > 0) printf(" ");
            PyLiteValue v = eval_expr(node->as.print_statement.args[i], env);
            if (gi && gi->had_error) { value_free(&v); return; }
            value_print(&v);
            value_free(&v);
        }
        printf("\n");
        break;
    }
    case NODE_ASSIGNMENT: {
        PyLiteValue val = eval_expr(node->as.assignment.value, env);
        if (gi && gi->had_error) { value_free(&val); return; }
        if (node->as.assignment.target->type == NODE_IDENTIFIER) {
            env_set(env, node->as.assignment.target->as.identifier.name, val);
        } else if (node->as.assignment.target->type == NODE_INDEX_ACCESS) {
            ASTNode *ia = node->as.assignment.target;
            const char *obj_name = NULL;
            if (ia->as.index_access.object->type == NODE_IDENTIFIER)
                obj_name = ia->as.index_access.object->as.identifier.name;
            if (obj_name) {
                PyLiteValue *obj_ptr = env_get_ptr(env, obj_name);
                if (obj_ptr && obj_ptr->type == VAL_LIST) {
                    PyLiteValue idx = eval_expr(ia->as.index_access.index, env);
                    if (gi && gi->had_error) { value_free(&idx); value_free(&val); return; }
                    int i = (int)idx.as.int_val;
                    if (i < 0) i += obj_ptr->as.list.count;
                    if (i < 0 || i >= obj_ptr->as.list.count) {
                        fprintf(stderr, "IndexError: list assignment index out of range\n");
                        if (gi) { gi->had_error = 1; snprintf(gi->error_msg, INTERP_ERROR_MSG_SIZE, "IndexError: list assignment index out of range"); gi->error_line = node->line; }
                        value_free(&idx); value_free(&val); return;
                    }
                    value_free(&obj_ptr->as.list.items[i]);
                    obj_ptr->as.list.items[i] = val;
                    value_free(&idx);
                } else { value_free(&val); }
            } else { value_free(&val); }
        } else { value_free(&val); }
        break;
    }
    case NODE_AUG_ASSIGNMENT: {
        if (node->as.aug_assignment.target->type != NODE_IDENTIFIER) break;
        const char *name = node->as.aug_assignment.target->as.identifier.name;
        PyLiteValue cur;
        if (!env_get(env, name, &cur)) {
            fprintf(stderr, "NameError: name '%s' is not defined\n", name);
            if (gi) { gi->had_error = 1; snprintf(gi->error_msg, INTERP_ERROR_MSG_SIZE, "NameError: name '%s' is not defined", name); gi->error_line = node->line; }
            return;
        }
        cur = value_copy(&cur);
        PyLiteValue rhs = eval_expr(node->as.aug_assignment.value, env);
        if (gi && gi->had_error) { value_free(&cur); value_free(&rhs); return; }
        PyLiteValue result;
        TokenType op = node->as.aug_assignment.op;
        if (op == TOKEN_PLUS_ASSIGN) {
            if (cur.type == VAL_INT && rhs.type == VAL_INT) result = value_make_int(cur.as.int_val + rhs.as.int_val);
            else if (cur.type == VAL_STRING && rhs.type == VAL_STRING) {
                size_t len = strlen(cur.as.string_val) + strlen(rhs.as.string_val);
                char *s = (char*)malloc(len+1); strcpy(s, cur.as.string_val); strcat(s, rhs.as.string_val);
                result = value_make_string_owned(s);
            } else { result = value_make_none(); }
        } else if (op == TOKEN_MINUS_ASSIGN) {
            result = value_make_int(cur.as.int_val - rhs.as.int_val);
        } else if (op == TOKEN_STAR_ASSIGN) {
            result = value_make_int(cur.as.int_val * rhs.as.int_val);
        } else if (op == TOKEN_SLASH_ASSIGN) {
            if (rhs.as.int_val == 0) { fprintf(stderr,"ZeroDivisionError\n"); if(gi){gi->had_error=1;} value_free(&cur); value_free(&rhs); return; }
            result = value_make_int(cur.as.int_val / rhs.as.int_val);
        } else { result = value_make_none(); }
        value_free(&cur); value_free(&rhs);
        env_set(env, name, result);
        break;
    }
    case NODE_EXPR_STATEMENT: {
        PyLiteValue v = eval_expr(node->as.expr_statement.expr, env);
        if (gi && gi->repl_mode && v.type != VAL_NONE && !(gi->had_error)) {
            value_repr(&v); printf("\n");
        }
        value_free(&v);
        break;
    }
    case NODE_IF_STATEMENT: {
        PyLiteValue cond = eval_expr(node->as.if_statement.condition, env);
        if (gi && gi->had_error) { value_free(&cond); return; }
        if (value_to_bool(&cond)) {
            value_free(&cond);
            if (node->as.if_statement.if_body->type == NODE_BLOCK) {
                for (int i = 0; i < node->as.if_statement.if_body->as.block.count; i++) {
                    exec_stmt(node->as.if_statement.if_body->as.block.statements[i], env, ret, retval, brk, cont);
                    if (*ret || *brk || *cont || (gi && gi->had_error)) return;
                }
            } else exec_stmt(node->as.if_statement.if_body, env, ret, retval, brk, cont);
            return;
        }
        value_free(&cond);
        for (int i = 0; i < node->as.if_statement.elif_count; i++) {
            PyLiteValue ec = eval_expr(node->as.if_statement.elifs[i].condition, env);
            if (gi && gi->had_error) { value_free(&ec); return; }
            if (value_to_bool(&ec)) {
                value_free(&ec);
                ASTNode *body = node->as.if_statement.elifs[i].body;
                if (body->type == NODE_BLOCK) {
                    for (int j = 0; j < body->as.block.count; j++) {
                        exec_stmt(body->as.block.statements[j], env, ret, retval, brk, cont);
                        if (*ret || *brk || *cont || (gi && gi->had_error)) return;
                    }
                } else exec_stmt(body, env, ret, retval, brk, cont);
                return;
            }
            value_free(&ec);
        }
        if (node->as.if_statement.else_body) {
            ASTNode *body = node->as.if_statement.else_body;
            if (body->type == NODE_BLOCK) {
                for (int i = 0; i < body->as.block.count; i++) {
                    exec_stmt(body->as.block.statements[i], env, ret, retval, brk, cont);
                    if (*ret || *brk || *cont || (gi && gi->had_error)) return;
                }
            } else exec_stmt(body, env, ret, retval, brk, cont);
        }
        break;
    }
    case NODE_WHILE_LOOP: {
        for (;;) {
            PyLiteValue cond = eval_expr(node->as.while_loop.condition, env);
            if (gi && gi->had_error) { value_free(&cond); return; }
            if (!value_to_bool(&cond)) { value_free(&cond); break; }
            value_free(&cond);
            ASTNode *body = node->as.while_loop.body;
            if (body->type == NODE_BLOCK) {
                for (int i = 0; i < body->as.block.count; i++) {
                    exec_stmt(body->as.block.statements[i], env, ret, retval, brk, cont);
                    if (*cont) { *cont = 0; break; }
                    if (*brk || *ret || (gi && gi->had_error)) break;
                }
            } else exec_stmt(body, env, ret, retval, brk, cont);
            if (*brk) { *brk = 0; break; }
            if (*ret || (gi && gi->had_error)) break;
        }
        break;
    }
    case NODE_FOR_LOOP: {
        PyLiteValue iter = eval_expr(node->as.for_loop.iterable, env);
        if (gi && gi->had_error) { value_free(&iter); return; }
        if (iter.type == VAL_LIST) {
            for (int i = 0; i < iter.as.list.count; i++) {
                PyLiteValue item = value_copy(&iter.as.list.items[i]);
                env_set(env, node->as.for_loop.var_name, item);
                ASTNode *body = node->as.for_loop.body;
                if (body->type == NODE_BLOCK) {
                    for (int j = 0; j < body->as.block.count; j++) {
                        exec_stmt(body->as.block.statements[j], env, ret, retval, brk, cont);
                        if (*cont) { *cont = 0; break; }
                        if (*brk || *ret || (gi && gi->had_error)) break;
                    }
                } else exec_stmt(body, env, ret, retval, brk, cont);
                if (*brk) { *brk = 0; break; }
                if (*ret || (gi && gi->had_error)) break;
            }
        } else if (iter.type == VAL_STRING) {
            int slen = (int)strlen(iter.as.string_val);
            for (int i = 0; i < slen; i++) {
                char buf[2] = {iter.as.string_val[i], '\0'};
                env_set(env, node->as.for_loop.var_name, value_make_string(buf));
                ASTNode *body = node->as.for_loop.body;
                if (body->type == NODE_BLOCK) {
                    for (int j = 0; j < body->as.block.count; j++) {
                        exec_stmt(body->as.block.statements[j], env, ret, retval, brk, cont);
                        if (*cont) { *cont = 0; break; }
                        if (*brk || *ret || (gi && gi->had_error)) break;
                    }
                } else exec_stmt(body, env, ret, retval, brk, cont);
                if (*brk) { *brk = 0; break; }
                if (*ret || (gi && gi->had_error)) break;
            }
        }
        value_free(&iter);
        break;
    }
    case NODE_FUNCTION_DEF: {
        PyLiteValue fn;
        fn.type = VAL_FUNCTION;
        fn.as.function.name = my_strdup(node->as.function_def.name);
        fn.as.function.param_count = node->as.function_def.param_count;
        fn.as.function.params = (char**)malloc(sizeof(char*) * fn.as.function.param_count);
        for (int i = 0; i < fn.as.function.param_count; i++)
            fn.as.function.params[i] = my_strdup(node->as.function_def.params[i]);
        fn.as.function.body = node->as.function_def.body;
        fn.as.function.closure = env;
        env_ref(env);
        env_set(env, node->as.function_def.name, fn);
        break;
    }
    case NODE_RETURN_STATEMENT: {
        *ret = 1;
        if (node->as.return_statement.value) {
            *retval = eval_expr(node->as.return_statement.value, env);
        } else {
            *retval = value_make_none();
        }
        break;
    }
    case NODE_BREAK_STATEMENT: *brk = 1; break;
    case NODE_CONTINUE_STATEMENT: *cont = 1; break;
    case NODE_PASS_STATEMENT: break;
    case NODE_BLOCK:
        for (int i = 0; i < node->as.block.count; i++) {
            exec_stmt(node->as.block.statements[i], env, ret, retval, brk, cont);
            if (*ret || *brk || *cont || (gi && gi->had_error)) return;
        }
        break;
    case NODE_PROGRAM:
        for (int i = 0; i < node->as.program.count; i++) {
            exec_stmt(node->as.program.statements[i], env, ret, retval, brk, cont);
            if (gi && gi->had_error) return;
        }
        break;
    default: {
        PyLiteValue v = eval_expr(node, env);
        value_free(&v);
        break;
    }
    }
}

void interpreter_init(Interpreter *interp) {
    interp->had_error = 0;
    interp->error_msg[0] = '\0';
    interp->error_line = 0;
    interp->repl_mode = 0;
}

void interpret(ASTNode *program) {
    Interpreter interp;
    interpreter_init(&interp);
    interp_set_global(&interp);
    Environment *global = env_create(NULL);
    int ret=0; PyLiteValue rv=value_make_none(); int brk=0,cont=0;
    exec_stmt(program, global, &ret, &rv, &brk, &cont);
    if (interp.had_error) {
        fprintf(stderr, "Traceback (line %d):\n  %s\n", interp.error_line, interp.error_msg);
    }
    value_free(&rv);
    env_free(global);
    interp_set_global(NULL);
}

void interpret_with_env(ASTNode *program, Environment *env, Interpreter *interp) {
    interp_set_global(interp);
    int ret=0; PyLiteValue rv=value_make_none(); int brk=0,cont=0;
    exec_stmt(program, env, &ret, &rv, &brk, &cont);
    if (interp->had_error) {
        fprintf(stderr, "Traceback (line %d):\n  %s\n", interp->error_line, interp->error_msg);
    }
    value_free(&rv);
}

int interpreter_had_error(const Interpreter *interp) { return interp->had_error; }
const char *interpreter_error_message(const Interpreter *interp) { return interp->error_msg; }
