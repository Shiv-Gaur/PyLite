#ifdef __MINGW32__
#define __USE_MINGW_ANSI_STDIO 1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "builtins.h"

static const char *builtin_names[] = {
    "print","len","range","type","input","int","str","abs","exit",NULL
};

int is_builtin(const char *name) {
    for (int i = 0; builtin_names[i]; i++)
        if (strcmp(name, builtin_names[i]) == 0) return 1;
    return 0;
}

static void rt_err(int *had_err, char *msg, int sz, const char *fmt, ...) {
    *had_err = 1;
    va_list ap; va_start(ap, fmt);
    vsnprintf(msg, sz, fmt, ap);
    va_end(ap);
}



PyLiteValue call_builtin(const char *name, PyLiteValue *args, int argc,
                         int line, int *had_err, char *err_msg, int err_sz) {
    (void)line;
    if (strcmp(name, "print") == 0) {
        for (int i = 0; i < argc; i++) {
            if (i > 0) printf(" ");
            value_print(&args[i]);
        }
        printf("\n");
        return value_make_none();
    }
    if (strcmp(name, "len") == 0) {
        if (argc != 1) { rt_err(had_err,err_msg,err_sz,"TypeError: len() takes exactly one argument (%d given)",argc); return value_make_none(); }
        if (args[0].type == VAL_STRING) return value_make_int((long long)strlen(args[0].as.string_val));
        if (args[0].type == VAL_LIST) return value_make_int(args[0].as.list.count);
        rt_err(had_err,err_msg,err_sz,"TypeError: object of type '%s' has no len()", value_type_name(&args[0]));
        return value_make_none();
    }
    if (strcmp(name, "range") == 0) {
        long long start=0, stop=0, step=1;
        if (argc == 1) { stop = args[0].as.int_val; }
        else if (argc == 2) { start = args[0].as.int_val; stop = args[1].as.int_val; }
        else if (argc == 3) { start = args[0].as.int_val; stop = args[1].as.int_val; step = args[2].as.int_val; }
        else { rt_err(had_err,err_msg,err_sz,"TypeError: range expected 1-3 arguments, got %d",argc); return value_make_none(); }
        if (step == 0) { rt_err(had_err,err_msg,err_sz,"ValueError: range() arg 3 must not be zero"); return value_make_none(); }
        PyLiteValue list = value_make_list();
        if (step > 0) { for (long long i = start; i < stop; i += step) value_list_append(&list, value_make_int(i)); }
        else { for (long long i = start; i > stop; i += step) value_list_append(&list, value_make_int(i)); }
        return list;
    }
    if (strcmp(name, "type") == 0) {
        if (argc != 1) { rt_err(had_err,err_msg,err_sz,"TypeError: type() takes 1 argument (%d given)",argc); return value_make_none(); }
        char buf[64]; snprintf(buf, sizeof(buf), "<class '%s'>", value_type_name(&args[0]));
        return value_make_string(buf);
    }
    if (strcmp(name, "input") == 0) {
        if (argc > 0) { value_print(&args[0]); fflush(stdout); }
        char buf[1024];
        if (!fgets(buf, sizeof(buf), stdin)) return value_make_string("");
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        return value_make_string(buf);
    }
    if (strcmp(name, "int") == 0) {
        if (argc != 1) { rt_err(had_err,err_msg,err_sz,"TypeError: int() takes 1 argument (%d given)",argc); return value_make_none(); }
        if (args[0].type == VAL_INT) return value_copy(&args[0]);
        if (args[0].type == VAL_BOOL) return value_make_int(args[0].as.bool_val);
        if (args[0].type == VAL_STRING) {
            char *end; long long v = strtoll(args[0].as.string_val, &end, 10);
            if (*end != '\0') { rt_err(had_err,err_msg,err_sz,"ValueError: invalid literal for int(): '%s'", args[0].as.string_val); return value_make_none(); }
            return value_make_int(v);
        }
        rt_err(had_err,err_msg,err_sz,"TypeError: int() argument must be a string or a number"); return value_make_none();
    }
    if (strcmp(name, "str") == 0) {
        if (argc != 1) { rt_err(had_err,err_msg,err_sz,"TypeError: str() takes 1 argument (%d given)",argc); return value_make_none(); }
        char *s = value_to_str(&args[0]);
        return value_make_string_owned(s);
    }
    if (strcmp(name, "abs") == 0) {
        if (argc != 1) { rt_err(had_err,err_msg,err_sz,"TypeError: abs() takes 1 argument (%d given)",argc); return value_make_none(); }
        if (args[0].type != VAL_INT) { rt_err(had_err,err_msg,err_sz,"TypeError: bad operand type for abs()"); return value_make_none(); }
        long long v = args[0].as.int_val;
        return value_make_int(v < 0 ? -v : v);
    }
    if (strcmp(name, "exit") == 0) {
        int code = 0;
        if (argc > 0 && args[0].type == VAL_INT) code = (int)args[0].as.int_val;
        exit(code);
    }
    rt_err(had_err,err_msg,err_sz,"NameError: name '%s' is not defined", name);
    return value_make_none();
}
