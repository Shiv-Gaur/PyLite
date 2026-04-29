#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "environment.h"

static char *my_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *d = (char *)malloc(len + 1);
    if (d) memcpy(d, s, len + 1);
    return d;
}

static unsigned long djb2_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

Environment *env_create(Environment *parent) {
    Environment *env = (Environment *)calloc(1, sizeof(Environment));
    if (!env) return NULL;

    env->capacity = ENV_INITIAL_CAPACITY;
    env->count = 0;
    env->parent = parent;
    env->ref_count = 1;
    env->buckets = (EnvEntry **)calloc(env->capacity, sizeof(EnvEntry *));

    if (parent) {
        env_ref(parent);
    }

    return env;
}

Environment *env_ref(Environment *env) {
    if (env) env->ref_count++;
    return env;
}

static void env_free_internal(Environment *env) {
    if (!env) return;

    for (int i = 0; i < env->capacity; i++) {
        EnvEntry *entry = env->buckets[i];
        while (entry) {
            EnvEntry *next = entry->next;
            free(entry->name);
            value_free(&entry->value);
            free(entry);
            entry = next;
        }
    }
    free(env->buckets);

    if (env->parent) {
        env_unref(env->parent);
    }
    free(env);
}

void env_unref(Environment *env) {
    if (!env) return;
    env->ref_count--;
    if (env->ref_count <= 0) {
        env_free_internal(env);
    }
}

void env_free(Environment *env) {
    env_unref(env);
}

static void env_resize(Environment *env) {
    int new_capacity = env->capacity * 2;
    EnvEntry **new_buckets = (EnvEntry **)calloc(new_capacity, sizeof(EnvEntry *));

    for (int i = 0; i < env->capacity; i++) {
        EnvEntry *entry = env->buckets[i];
        while (entry) {
            EnvEntry *next = entry->next;
            unsigned long idx = djb2_hash(entry->name) % new_capacity;
            entry->next = new_buckets[idx];
            new_buckets[idx] = entry;
            entry = next;
        }
    }

    free(env->buckets);
    env->buckets = new_buckets;
    env->capacity = new_capacity;
}

void env_set(Environment *env, const char *name, PyLiteValue val) {
    unsigned long idx = djb2_hash(name) % env->capacity;

    EnvEntry *entry = env->buckets[idx];
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            value_free(&entry->value);
            entry->value = val;
            return;
        }
        entry = entry->next;
    }

    EnvEntry *new_entry = (EnvEntry *)malloc(sizeof(EnvEntry));
    new_entry->name = my_strdup(name);
    new_entry->value = val;
    new_entry->next = env->buckets[idx];
    env->buckets[idx] = new_entry;
    env->count++;

    if ((double)env->count / env->capacity > 0.75) {
        env_resize(env);
    }
}

int env_get(Environment *env, const char *name, PyLiteValue *out) {
    Environment *cur = env;
    while (cur) {
        unsigned long idx = djb2_hash(name) % cur->capacity;
        EnvEntry *entry = cur->buckets[idx];
        while (entry) {
            if (strcmp(entry->name, name) == 0) {
                *out = entry->value;
                return 1;
            }
            entry = entry->next;
        }
        cur = cur->parent;
    }
    return 0;
}

PyLiteValue *env_get_ptr(Environment *env, const char *name) {
    Environment *cur = env;
    while (cur) {
        unsigned long idx = djb2_hash(name) % cur->capacity;
        EnvEntry *entry = cur->buckets[idx];
        while (entry) {
            if (strcmp(entry->name, name) == 0) {
                return &entry->value;
            }
            entry = entry->next;
        }
        cur = cur->parent;
    }
    return NULL;
}

int env_update(Environment *env, const char *name, PyLiteValue val) {
    Environment *cur = env;
    while (cur) {
        unsigned long idx = djb2_hash(name) % cur->capacity;
        EnvEntry *entry = cur->buckets[idx];
        while (entry) {
            if (strcmp(entry->name, name) == 0) {
                value_free(&entry->value);
                entry->value = val;
                return 1;
            }
            entry = entry->next;
        }
        cur = cur->parent;
    }
    return 0;
}
