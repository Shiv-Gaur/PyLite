

#ifndef PYLITE_ANALYZER_H
#define PYLITE_ANALYZER_H

#include "ast.h"

#define ANALYZER_MAX_ERRORS 64

typedef struct {
    char message[256];
    int  line;
} AnalyzerError;

typedef struct {
    AnalyzerError errors[ANALYZER_MAX_ERRORS];
    int           error_count;

    
    int in_loop;        
    int in_function;    
} Analyzer;


void analyzer_init(Analyzer *a);


int analyzer_analyze(Analyzer *a, ASTNode *node);


int analyzer_had_error(const Analyzer *a);


void analyzer_print_errors(const Analyzer *a);

#endif 
