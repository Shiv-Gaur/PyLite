#ifndef PYLITE_VALUE_H
#define PYLITE_VALUE_H

#include "ast.h"

typedef struct Environment Environment;

typedef enum {
    VAL_INT,
    VAL_STRING,
    VAL_BOOL,
    VAL_NONE,
    VAL_LIST,
    VAL_FUNCTION
} ValueType;

typedef struct PyLiteValue PyLiteValue;

struct PyLiteValue {
    ValueType type;
    union {
        long long int_val;
        char *string_val;
        int bool_val;

        struct {
            PyLiteValue *items;
            int count;
            int capacity;
        } list;

        struct {
            char **params;
            int param_count;
            ASTNode *body;
            Environment *closure;
            char *name;
        } function;
    } as;
};

PyLiteValue value_make_int(long long n);
PyLiteValue value_make_string(const char *s);
PyLiteValue value_make_string_owned(char *s);
PyLiteValue value_make_bool(int b);
PyLiteValue value_make_none(void);
PyLiteValue value_make_list(void);

void value_print(const PyLiteValue *val);
void value_repr(const PyLiteValue *val);
int value_to_bool(const PyLiteValue *val);
int value_equals(const PyLiteValue *a, const PyLiteValue *b);
PyLiteValue value_copy(const PyLiteValue *val);
void value_free(PyLiteValue *val);
char *value_to_str(const PyLiteValue *val);
const char *value_type_name(const PyLiteValue *val);

void value_list_append(PyLiteValue *list, PyLiteValue item);
PyLiteValue value_list_pop(PyLiteValue *list);

#endif
