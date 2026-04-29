#ifndef PYLITE_BUILTINS_H
#define PYLITE_BUILTINS_H
#include "value.h"
#include "environment.h"
int is_builtin(const char *name);
PyLiteValue call_builtin(const char *name, PyLiteValue *args, int argc, int line, int *had_err, char *err_msg, int err_sz);
#endif
