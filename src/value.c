#ifdef __MINGW32__
#define __USE_MINGW_ANSI_STDIO 1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"
#include "environment.h"

#define LLFMT "%lld"


static char *my_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *d = (char *)malloc(len + 1);
    if (d) memcpy(d, s, len + 1);
    return d;
}

PyLiteValue value_make_int(long long n) {
    PyLiteValue v;
    v.type = VAL_INT;
    v.as.int_val = n;
    return v;
}

PyLiteValue value_make_string(const char *s) {
    PyLiteValue v;
    v.type = VAL_STRING;
    v.as.string_val = my_strdup(s);
    return v;
}

PyLiteValue value_make_string_owned(char *s) {
    PyLiteValue v;
    v.type = VAL_STRING;
    v.as.string_val = s;
    return v;
}

PyLiteValue value_make_bool(int b) {
    PyLiteValue v;
    v.type = VAL_BOOL;
    v.as.bool_val = b ? 1 : 0;
    return v;
}

PyLiteValue value_make_none(void) {
    PyLiteValue v;
    v.type = VAL_NONE;
    v.as.int_val = 0;
    return v;
}

PyLiteValue value_make_list(void) {
    PyLiteValue v;
    v.type = VAL_LIST;
    v.as.list.capacity = 8;
    v.as.list.count = 0;
    v.as.list.items = (PyLiteValue *)malloc(sizeof(PyLiteValue) * 8);
    return v;
}

static void print_value_internal(const PyLiteValue *val, int repr_mode) {
    if (!val) { printf("None"); return; }
    switch (val->type) {
        case VAL_INT:
            printf(LLFMT, val->as.int_val);
            break;
        case VAL_STRING:
            if (repr_mode) printf("'%s'", val->as.string_val ? val->as.string_val : "");
            else printf("%s", val->as.string_val ? val->as.string_val : "");
            break;
        case VAL_BOOL:
            printf("%s", val->as.bool_val ? "True" : "False");
            break;
        case VAL_NONE:
            printf("None");
            break;
        case VAL_LIST:
            printf("[");
            for (int i = 0; i < val->as.list.count; i++) {
                if (i > 0) printf(", ");
                print_value_internal(&val->as.list.items[i], 1);
            }
            printf("]");
            break;
        case VAL_FUNCTION:
            printf("<function %s>", val->as.function.name ? val->as.function.name : "?");
            break;
    }
}

void value_print(const PyLiteValue *val) { print_value_internal(val, 0); }
void value_repr(const PyLiteValue *val) { print_value_internal(val, 1); }

int value_to_bool(const PyLiteValue *val) {
    if (!val) return 0;
    switch (val->type) {
        case VAL_INT:    return val->as.int_val != 0;
        case VAL_STRING: return val->as.string_val && val->as.string_val[0] != '\0';
        case VAL_BOOL:   return val->as.bool_val;
        case VAL_NONE:   return 0;
        case VAL_LIST:   return val->as.list.count > 0;
        case VAL_FUNCTION: return 1;
    }
    return 0;
}

int value_equals(const PyLiteValue *a, const PyLiteValue *b) {
    if (a->type != b->type) {
        if (a->type == VAL_INT && b->type == VAL_BOOL) return a->as.int_val == (long long)b->as.bool_val;
        if (a->type == VAL_BOOL && b->type == VAL_INT) return (long long)a->as.bool_val == b->as.int_val;
        return 0;
    }
    switch (a->type) {
        case VAL_INT:    return a->as.int_val == b->as.int_val;
        case VAL_STRING: return strcmp(a->as.string_val, b->as.string_val) == 0;
        case VAL_BOOL:   return a->as.bool_val == b->as.bool_val;
        case VAL_NONE:   return 1;
        case VAL_LIST:
            if (a->as.list.count != b->as.list.count) return 0;
            for (int i = 0; i < a->as.list.count; i++)
                if (!value_equals(&a->as.list.items[i], &b->as.list.items[i])) return 0;
            return 1;
        case VAL_FUNCTION: return a->as.function.body == b->as.function.body;
    }
    return 0;
}

PyLiteValue value_copy(const PyLiteValue *val) {
    PyLiteValue copy;
    copy.type = val->type;
    switch (val->type) {
        case VAL_INT: copy.as.int_val = val->as.int_val; break;
        case VAL_STRING: copy.as.string_val = my_strdup(val->as.string_val); break;
        case VAL_BOOL: copy.as.bool_val = val->as.bool_val; break;
        case VAL_NONE: copy.as.int_val = 0; break;
        case VAL_LIST:
            copy.as.list.capacity = val->as.list.capacity;
            copy.as.list.count = val->as.list.count;
            copy.as.list.items = (PyLiteValue *)malloc(sizeof(PyLiteValue) * copy.as.list.capacity);
            for (int i = 0; i < copy.as.list.count; i++)
                copy.as.list.items[i] = value_copy(&val->as.list.items[i]);
            break;
        case VAL_FUNCTION:
            copy.as.function.name = my_strdup(val->as.function.name);
            copy.as.function.param_count = val->as.function.param_count;
            copy.as.function.body = val->as.function.body;
            copy.as.function.closure = val->as.function.closure;
            if (copy.as.function.closure) env_ref(copy.as.function.closure);
            copy.as.function.params = (char **)malloc(sizeof(char *) * copy.as.function.param_count);
            for (int i = 0; i < copy.as.function.param_count; i++)
                copy.as.function.params[i] = my_strdup(val->as.function.params[i]);
            break;
    }
    return copy;
}

void value_free(PyLiteValue *val) {
    if (!val) return;
    switch (val->type) {
        case VAL_STRING: free(val->as.string_val); val->as.string_val = NULL; break;
        case VAL_LIST:
            for (int i = 0; i < val->as.list.count; i++) value_free(&val->as.list.items[i]);
            free(val->as.list.items); val->as.list.items = NULL; val->as.list.count = 0;
            break;
        case VAL_FUNCTION:
            free(val->as.function.name);
            for (int i = 0; i < val->as.function.param_count; i++) free(val->as.function.params[i]);
            free(val->as.function.params);
            if (val->as.function.closure) env_unref(val->as.function.closure);
            val->as.function.name = NULL; val->as.function.params = NULL; val->as.function.closure = NULL;
            break;
        default: break;
    }
    val->type = VAL_NONE;
}

char *value_to_str(const PyLiteValue *val) {
    char buf[256];
    switch (val->type) {
        case VAL_INT: snprintf(buf, sizeof(buf), LLFMT, val->as.int_val); return my_strdup(buf);
        case VAL_STRING: return my_strdup(val->as.string_val);
        case VAL_BOOL: return my_strdup(val->as.bool_val ? "True" : "False");
        case VAL_NONE: return my_strdup("None");
        case VAL_LIST: {
            int total = 2;
            char **parts = (char **)malloc(sizeof(char *) * (val->as.list.count + 1));
            for (int i = 0; i < val->as.list.count; i++) {
                parts[i] = value_to_str(&val->as.list.items[i]);
                total += (int)strlen(parts[i]) + 4;
            }
            char *result = (char *)malloc(total + 1);
            strcpy(result, "[");
            for (int i = 0; i < val->as.list.count; i++) {
                if (i > 0) strcat(result, ", ");
                if (val->as.list.items[i].type == VAL_STRING) {
                    strcat(result, "'"); strcat(result, parts[i]); strcat(result, "'");
                } else strcat(result, parts[i]);
                free(parts[i]);
            }
            strcat(result, "]"); free(parts); return result;
        }
        case VAL_FUNCTION:
            snprintf(buf, sizeof(buf), "<function %s>", val->as.function.name ? val->as.function.name : "?");
            return my_strdup(buf);
    }
    return my_strdup("?");
}

const char *value_type_name(const PyLiteValue *val) {
    switch (val->type) {
        case VAL_INT: return "int"; case VAL_STRING: return "str";
        case VAL_BOOL: return "bool"; case VAL_NONE: return "NoneType";
        case VAL_LIST: return "list"; case VAL_FUNCTION: return "function";
    }
    return "unknown";
}

void value_list_append(PyLiteValue *list, PyLiteValue item) {
    if (list->type != VAL_LIST) return;
    if (list->as.list.count >= list->as.list.capacity) {
        list->as.list.capacity *= 2;
        list->as.list.items = (PyLiteValue *)realloc(list->as.list.items, sizeof(PyLiteValue) * list->as.list.capacity);
    }
    list->as.list.items[list->as.list.count++] = item;
}

PyLiteValue value_list_pop(PyLiteValue *list) {
    if (list->type != VAL_LIST || list->as.list.count == 0) return value_make_none();
    list->as.list.count--;
    return list->as.list.items[list->as.list.count];
}
