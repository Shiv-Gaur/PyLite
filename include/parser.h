

#ifndef PYLITE_PARSER_H
#define PYLITE_PARSER_H

#include "token.h"
#include "ast.h"

#define PARSER_MAX_ERRORS 64

typedef struct {
    Token *tokens;          
    int    token_count;     
    int    pos;             

    int    had_error;       
    char   error_msg[512];  
    int    error_line;      
} Parser;


void parser_init(Parser *parser, Token *tokens, int token_count);


ASTNode *parser_parse(Parser *parser);


int parser_had_error(const Parser *parser);


const char *parser_error_message(const Parser *parser);

#endif 
