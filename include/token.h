#ifndef PYLITE_TOKEN_H
#define PYLITE_TOKEN_H


typedef enum {
    
    TOKEN_INT_LITERAL,      
    TOKEN_STRING_LITERAL,   
    TOKEN_TRUE,             
    TOKEN_FALSE,            
    TOKEN_NONE,             

    
    TOKEN_IDENTIFIER,       

    
    TOKEN_IF,
    TOKEN_ELIF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_IN,
    TOKEN_DEF,
    TOKEN_RETURN,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_PRINT,            
    TOKEN_INPUT,
    TOKEN_LEN,
    TOKEN_RANGE,
    TOKEN_TYPE,
    TOKEN_APPEND,
    TOKEN_POP,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_PASS,

    
    TOKEN_PLUS,             
    TOKEN_MINUS,           
    TOKEN_STAR,             
    TOKEN_SLASH,            
    TOKEN_DOUBLE_SLASH,    
    TOKEN_PERCENT,          
    TOKEN_DOUBLE_STAR,     

    
    TOKEN_EQ,              
    TOKEN_NEQ,             
    TOKEN_LT,              
    TOKEN_GT,             
    TOKEN_LTE,             
    TOKEN_GTE,            

    
    TOKEN_ASSIGN,           
    TOKEN_PLUS_ASSIGN,      
    TOKEN_MINUS_ASSIGN,     
    TOKEN_STAR_ASSIGN,      
    TOKEN_SLASH_ASSIGN,     

    
    TOKEN_LPAREN,          
    TOKEN_RPAREN,          
    TOKEN_LBRACKET,        
    TOKEN_RBRACKET,         
    TOKEN_COLON,         
    TOKEN_COMMA,           
    TOKEN_DOT,         

    
    TOKEN_INDENT,        
    TOKEN_DEDENT,         
    TOKEN_NEWLINE,         

  
    TOKEN_EOF,             
    TOKEN_ERROR            
} TokenType;


const char *token_type_name(TokenType type);


typedef struct {
    TokenType   type;
    const char *lexeme;
    int         length;
    int         line;
    int         column;
} Token;

#endif 
