#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "interpreter.h"
#include "builtins.h"

static Interpreter *g_interp = NULL;

void interp_set_global(Interpreter *ip) { g_interp = ip; }
Interpreter *interp_get_global(void) { return g_interp; }

static void rt_error(int line, const char *fmt, ...) {
    if (!g_interp || g_interp->had_error) return;
    g_interp->had_error = 1;
    g_interp->error_line = line;
    va_list ap; va_start(ap, fmt);
    vsnprintf(g_interp->error_msg, INTERP_ERROR_MSG_SIZE, fmt, ap);
    va_end(ap);
}

static char *process_escapes(const char *s, int len) {
    char *out = (char*)malloc(len + 1);
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (s[i] == '\\' && i + 1 < len) {
            i++;
            switch(s[i]) {
                case 'n': out[j++] = '\n'; break;
                case 't': out[j++] = '\t'; break;
                case '\\': out[j++] = '\\'; break;
                case '\'': out[j++] = '\''; break;
                case '"': out[j++] = '"'; break;
                default: out[j++] = '\\'; out[j++] = s[i]; break;
            }
        } else {
            out[j++] = s[i];
        }
    }
    out[j] = '\0';
    return out;
}

PyLiteValue eval_expr(ASTNode *node, Environment *env);
void exec_stmt(ASTNode *node, Environment *env, int *ret, PyLiteValue *retval, int *brk, int *cont);

PyLiteValue eval_expr(ASTNode *node, Environment *env) {
    if (!node || (g_interp && g_interp->had_error)) return value_make_none();

    switch (node->type) {
    case NODE_INT_LITERAL:
        return value_make_int((long long)node->as.int_literal.value);
    case NODE_STRING_LITERAL: {
        char *s = process_escapes(node->as.string_literal.value, node->as.string_literal.length);
        return value_make_string_owned(s);
    }
    case NODE_BOOL_LITERAL:
        return value_make_bool(node->as.bool_literal.value);
    case NODE_NONE_LITERAL:
        return value_make_none();
    case NODE_IDENTIFIER: {
        PyLiteValue v;
        if (!env_get(env, node->as.identifier.name, &v)) {
            rt_error(node->line, "NameError: name '%s' is not defined", node->as.identifier.name);
            return value_make_none();
        }
        return value_copy(&v);
    }
    case NODE_UNARY_OP: {
        PyLiteValue op = eval_expr(node->as.unary_op.operand, env);
        if (g_interp && g_interp->had_error) return op;
        if (node->as.unary_op.op == TOKEN_MINUS) {
            if (op.type != VAL_INT) { rt_error(node->line, "TypeError: bad operand type for unary -: '%s'", value_type_name(&op)); value_free(&op); return value_make_none(); }
            long long v = -op.as.int_val; value_free(&op); return value_make_int(v);
        }
        if (node->as.unary_op.op == TOKEN_NOT) {
            int b = !value_to_bool(&op); value_free(&op); return value_make_bool(b);
        }
        value_free(&op); return value_make_none();
    }
    case NODE_BINARY_OP: {
        TokenType op = node->as.binary_op.op;
        if (op == TOKEN_AND) {
            PyLiteValue l = eval_expr(node->as.binary_op.left, env);
            if (!value_to_bool(&l)) return l;
            value_free(&l);
            return eval_expr(node->as.binary_op.right, env);
        }
        if (op == TOKEN_OR) {
            PyLiteValue l = eval_expr(node->as.binary_op.left, env);
            if (value_to_bool(&l)) return l;
            value_free(&l);
            return eval_expr(node->as.binary_op.right, env);
        }
        PyLiteValue l = eval_expr(node->as.binary_op.left, env);
        if (g_interp && g_interp->had_error) return l;
        PyLiteValue r = eval_expr(node->as.binary_op.right, env);
        if (g_interp && g_interp->had_error) { value_free(&l); return r; }

        if (op == TOKEN_PLUS) {
            if (l.type == VAL_INT && r.type == VAL_INT) { long long v = l.as.int_val + r.as.int_val; value_free(&l); value_free(&r); return value_make_int(v); }
            if (l.type == VAL_STRING && r.type == VAL_STRING) {
                size_t len = strlen(l.as.string_val) + strlen(r.as.string_val);
                char *s = (char*)malloc(len+1); strcpy(s, l.as.string_val); strcat(s, r.as.string_val);
                value_free(&l); value_free(&r); return value_make_string_owned(s);
            }
            rt_error(node->line, "TypeError: unsupported operand type(s) for +: '%s' and '%s'", value_type_name(&l), value_type_name(&r));
            value_free(&l); value_free(&r); return value_make_none();
        }
        if (op == TOKEN_MINUS) {
            if (l.type == VAL_INT && r.type == VAL_INT) { long long v = l.as.int_val - r.as.int_val; value_free(&l); value_free(&r); return value_make_int(v); }
            rt_error(node->line, "TypeError: unsupported operand type(s) for -: '%s' and '%s'", value_type_name(&l), value_type_name(&r));
            value_free(&l); value_free(&r); return value_make_none();
        }
        if (op == TOKEN_STAR) {
            if (l.type == VAL_INT && r.type == VAL_INT) { long long v = l.as.int_val * r.as.int_val; value_free(&l); value_free(&r); return value_make_int(v); }
            if (l.type == VAL_STRING && r.type == VAL_INT) {
                int n = (int)r.as.int_val; if (n < 0) n = 0;
                size_t slen = strlen(l.as.string_val);
                char *s = (char*)malloc(slen * n + 1); s[0] = '\0';
                for (int i = 0; i < n; i++) strcat(s, l.as.string_val);
                value_free(&l); value_free(&r); return value_make_string_owned(s);
            }
            if (l.type == VAL_INT && r.type == VAL_STRING) {
                int n = (int)l.as.int_val; if (n < 0) n = 0;
                size_t slen = strlen(r.as.string_val);
                char *s = (char*)malloc(slen * n + 1); s[0] = '\0';
                for (int i = 0; i < n; i++) strcat(s, r.as.string_val);
                value_free(&l); value_free(&r); return value_make_string_owned(s);
            }
            rt_error(node->line, "TypeError: unsupported operand type(s) for *: '%s' and '%s'", value_type_name(&l), value_type_name(&r));
            value_free(&l); value_free(&r); return value_make_none();
        }
        if (op == TOKEN_SLASH || op == TOKEN_DOUBLE_SLASH) {
            if (l.type == VAL_INT && r.type == VAL_INT) {
                if (r.as.int_val == 0) { rt_error(node->line, "ZeroDivisionError: division by zero"); value_free(&l); value_free(&r); return value_make_none(); }
                long long v;
                if (op == TOKEN_DOUBLE_SLASH) { v = l.as.int_val / r.as.int_val; if ((l.as.int_val ^ r.as.int_val) < 0 && v * r.as.int_val != l.as.int_val) v--; }
                else { v = l.as.int_val / r.as.int_val; }
                value_free(&l); value_free(&r); return value_make_int(v);
            }
            rt_error(node->line, "TypeError: unsupported operand type(s) for /: '%s' and '%s'", value_type_name(&l), value_type_name(&r));
            value_free(&l); value_free(&r); return value_make_none();
        }
        if (op == TOKEN_PERCENT) {
            if (l.type == VAL_INT && r.type == VAL_INT) {
                if (r.as.int_val == 0) { rt_error(node->line, "ZeroDivisionError: integer modulo by zero"); value_free(&l); value_free(&r); return value_make_none(); }
                long long v = l.as.int_val % r.as.int_val;
                if (v != 0 && (v ^ r.as.int_val) < 0) v += r.as.int_val;
                value_free(&l); value_free(&r); return value_make_int(v);
            }
            rt_error(node->line, "TypeError: unsupported operand type(s) for %%: '%s' and '%s'", value_type_name(&l), value_type_name(&r));
            value_free(&l); value_free(&r); return value_make_none();
        }
        if (op == TOKEN_DOUBLE_STAR) {
            if (l.type == VAL_INT && r.type == VAL_INT) {
                long long base = l.as.int_val, exp = r.as.int_val, result = 1;
                if (exp < 0) { value_free(&l); value_free(&r); return value_make_int(0); }
                for (long long i = 0; i < exp; i++) result *= base;
                value_free(&l); value_free(&r); return value_make_int(result);
            }
            rt_error(node->line, "TypeError: unsupported operand type(s) for **: '%s' and '%s'", value_type_name(&l), value_type_name(&r));
            value_free(&l); value_free(&r); return value_make_none();
        }
        if (op == TOKEN_EQ) { int e = value_equals(&l,&r); value_free(&l); value_free(&r); return value_make_bool(e); }
        if (op == TOKEN_NEQ) { int e = !value_equals(&l,&r); value_free(&l); value_free(&r); return value_make_bool(e); }
        if (op == TOKEN_LT || op == TOKEN_GT || op == TOKEN_LTE || op == TOKEN_GTE) {
            int res = 0;
            if (l.type == VAL_INT && r.type == VAL_INT) {
                if (op==TOKEN_LT) res=l.as.int_val<r.as.int_val; else if(op==TOKEN_GT) res=l.as.int_val>r.as.int_val;
                else if(op==TOKEN_LTE) res=l.as.int_val<=r.as.int_val; else res=l.as.int_val>=r.as.int_val;
            } else if (l.type == VAL_STRING && r.type == VAL_STRING) {
                int c = strcmp(l.as.string_val, r.as.string_val);
                if(op==TOKEN_LT) res=c<0; else if(op==TOKEN_GT) res=c>0; else if(op==TOKEN_LTE) res=c<=0; else res=c>=0;
            } else {
                rt_error(node->line,"TypeError: '<' not supported between instances of '%s' and '%s'",value_type_name(&l),value_type_name(&r));
            }
            value_free(&l); value_free(&r); return value_make_bool(res);
        }
        value_free(&l); value_free(&r); return value_make_none();
    }
    case NODE_FUNCTION_CALL: {
        char *fname = node->as.function_call.name;
        int argc = node->as.function_call.arg_count;
        PyLiteValue *args = (PyLiteValue*)malloc(sizeof(PyLiteValue)*argc);
        for (int i = 0; i < argc; i++) {
            args[i] = eval_expr(node->as.function_call.args[i], env);
            if (g_interp && g_interp->had_error) { for(int j=0;j<=i;j++) value_free(&args[j]); free(args); return value_make_none(); }
        }
        if (is_builtin(fname)) {
            int he=0; char em[512]={0};
            PyLiteValue r = call_builtin(fname, args, argc, node->line, &he, em, 512);
            if (he) rt_error(node->line, "%s", em);
            for(int i=0;i<argc;i++) value_free(&args[i]); free(args);
            return r;
        }
        PyLiteValue fv;
        if (!env_get(env, fname, &fv) || fv.type != VAL_FUNCTION) {
            rt_error(node->line, "NameError: name '%s' is not defined", fname);
            for(int i=0;i<argc;i++) value_free(&args[i]); free(args); return value_make_none();
        }
        if (argc != fv.as.function.param_count) {
            rt_error(node->line, "TypeError: %s() takes %d arguments but %d were given", fname, fv.as.function.param_count, argc);
            for(int i=0;i<argc;i++) value_free(&args[i]); free(args); return value_make_none();
        }
        Environment *call_env = env_create(fv.as.function.closure);
        for (int i = 0; i < argc; i++) env_set(call_env, fv.as.function.params[i], args[i]);
        free(args);
        int sr=0; PyLiteValue rv=value_make_none(); int sb=0,sc=0;
        if (fv.as.function.body->type == NODE_BLOCK) {
            for (int i = 0; i < fv.as.function.body->as.block.count; i++) {
                exec_stmt(fv.as.function.body->as.block.statements[i], call_env, &sr, &rv, &sb, &sc);
                if (sr || (g_interp && g_interp->had_error)) break;
            }
        } else { exec_stmt(fv.as.function.body, call_env, &sr, &rv, &sb, &sc); }
        env_free(call_env);
        return sr ? rv : value_make_none();
    }
    case NODE_LIST_LITERAL: {
        PyLiteValue list = value_make_list();
        for (int i = 0; i < node->as.list_literal.count; i++) {
            PyLiteValue elem = eval_expr(node->as.list_literal.elements[i], env);
            if (g_interp && g_interp->had_error) { value_free(&list); return value_make_none(); }
            value_list_append(&list, elem);
        }
        return list;
    }
    case NODE_INDEX_ACCESS: {
        PyLiteValue obj = eval_expr(node->as.index_access.object, env);
        PyLiteValue idx = eval_expr(node->as.index_access.index, env);
        if (g_interp && g_interp->had_error) { value_free(&obj); value_free(&idx); return value_make_none(); }
        if (obj.type == VAL_LIST && idx.type == VAL_INT) {
            int i = (int)idx.as.int_val;
            if (i < 0) i += obj.as.list.count;
            if (i < 0 || i >= obj.as.list.count) {
                rt_error(node->line, "IndexError: list index out of range");
                value_free(&obj); value_free(&idx); return value_make_none();
            }
            PyLiteValue r = value_copy(&obj.as.list.items[i]);
            value_free(&obj); value_free(&idx); return r;
        }
        if (obj.type == VAL_STRING && idx.type == VAL_INT) {
            int i = (int)idx.as.int_val;
            int slen = (int)strlen(obj.as.string_val);
            if (i < 0) i += slen;
            if (i < 0 || i >= slen) {
                rt_error(node->line, "IndexError: string index out of range");
                value_free(&obj); value_free(&idx); return value_make_none();
            }
            char buf[2] = {obj.as.string_val[i], '\0'};
            value_free(&obj); value_free(&idx); return value_make_string(buf);
        }
        rt_error(node->line, "TypeError: object is not subscriptable");
        value_free(&obj); value_free(&idx); return value_make_none();
    }
    case NODE_DOT_METHOD_CALL: {
        char *method = node->as.dot_method_call.method;
        const char *obj_name = NULL;
        if (node->as.dot_method_call.object->type == NODE_IDENTIFIER)
            obj_name = node->as.dot_method_call.object->as.identifier.name;

        PyLiteValue *obj_ptr = NULL;
        PyLiteValue obj_tmp;
        if (obj_name) obj_ptr = env_get_ptr(env, obj_name);

        if (!obj_ptr) {
            obj_tmp = eval_expr(node->as.dot_method_call.object, env);
            if (g_interp && g_interp->had_error) { value_free(&obj_tmp); return value_make_none(); }
            obj_ptr = &obj_tmp;
        }

        if (strcmp(method, "append") == 0) {
            if (obj_ptr->type != VAL_LIST) { rt_error(node->line, "AttributeError: '%s' has no attribute 'append'", value_type_name(obj_ptr)); return value_make_none(); }
            if (node->as.dot_method_call.arg_count != 1) { rt_error(node->line, "TypeError: append() takes 1 argument"); return value_make_none(); }
            PyLiteValue arg = eval_expr(node->as.dot_method_call.args[0], env);
            if (g_interp && g_interp->had_error) { value_free(&arg); return value_make_none(); }
            value_list_append(obj_ptr, arg);
            return value_make_none();
        }
        if (strcmp(method, "pop") == 0) {
            if (obj_ptr->type != VAL_LIST) { rt_error(node->line, "AttributeError: '%s' has no attribute 'pop'", value_type_name(obj_ptr)); return value_make_none(); }
            if (obj_ptr->as.list.count == 0) { rt_error(node->line, "IndexError: pop from empty list"); return value_make_none(); }
            PyLiteValue r = value_list_pop(obj_ptr);
            return r;
        }
        rt_error(node->line, "AttributeError: '%s' has no attribute '%s'", value_type_name(obj_ptr), method);
        return value_make_none();
    }
    default:
        return value_make_none();
    }
}
