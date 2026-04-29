#ifndef PYLITE_INTERPRETER_H
#define PYLITE_INTERPRETER_H

#include "ast.h"
#include "environment.h"
#include "value.h"

#define INTERP_ERROR_MSG_SIZE 512

typedef struct {
    int had_error;
    char error_msg[INTERP_ERROR_MSG_SIZE];
    int error_line;
    int repl_mode;
} Interpreter;

void interpreter_init(Interpreter *interp);

void interpret(ASTNode *program);

void interpret_with_env(ASTNode *program, Environment *env, Interpreter *interp);

int interpreter_had_error(const Interpreter *interp);
const char *interpreter_error_message(const Interpreter *interp);

#endif
