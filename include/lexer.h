#ifndef PYLITE_LEXER_H
#define PYLITE_LEXER_H

#include "token.h"


#define LEXER_MAX_INDENT_DEPTH 128  

typedef struct {
    
    const char *source;     
    const char *current;    
    int         line;       
    int         column;     

    
    int indent_stack[LEXER_MAX_INDENT_DEPTH];
    int indent_top;         

    
    int pending_dedents;    
    int pending_indent;     

    
    int at_line_start;      
    int paren_depth;        
    int emit_eof_dedents;   
} Lexer;


void  lexer_init(Lexer *lexer, const char *source);


Token lexer_next_token(Lexer *lexer);


Token *lexer_tokenize_all(Lexer *lexer, int *out_count);

#endif
