#ifndef PYLITE_ENVIRONMENT_H
#define PYLITE_ENVIRONMENT_H

#include "value.h"

#define ENV_INITIAL_CAPACITY 16

typedef struct EnvEntry {
    char *name;
    PyLiteValue value;
    struct EnvEntry *next;
} EnvEntry;

struct Environment {
    EnvEntry **buckets;
    int capacity;
    int count;
    Environment *parent;
    int ref_count;
};

Environment *env_create(Environment *parent);
void env_set(Environment *env, const char *name, PyLiteValue val);
int env_get(Environment *env, const char *name, PyLiteValue *out);
PyLiteValue *env_get_ptr(Environment *env, const char *name);
int env_update(Environment *env, const char *name, PyLiteValue val);
void env_free(Environment *env);

Environment *env_ref(Environment *env);
void env_unref(Environment *env);

#endif
