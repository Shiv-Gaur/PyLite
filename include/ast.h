

#ifndef PYLITE_AST_H
#define PYLITE_AST_H

#include "token.h"





typedef enum {
    
    NODE_INT_LITERAL,       
    NODE_STRING_LITERAL,    
    NODE_BOOL_LITERAL,      
    NODE_NONE_LITERAL,      
    NODE_IDENTIFIER,        
    NODE_BINARY_OP,         
    NODE_UNARY_OP,          
    NODE_FUNCTION_CALL,     
    NODE_LIST_LITERAL,      
    NODE_INDEX_ACCESS,      
    NODE_DOT_METHOD_CALL,   

    
    NODE_ASSIGNMENT,        
    NODE_AUG_ASSIGNMENT,    
    NODE_PRINT_STATEMENT,   
    NODE_RETURN_STATEMENT,  
    NODE_PASS_STATEMENT,    
    NODE_BREAK_STATEMENT,   
    NODE_CONTINUE_STATEMENT,
    NODE_EXPR_STATEMENT,    

    
    NODE_IF_STATEMENT,      
    NODE_WHILE_LOOP,        
    NODE_FOR_LOOP,          
    NODE_FUNCTION_DEF,      
    NODE_BLOCK,             
    NODE_PROGRAM            
} ASTNodeType;


typedef struct ASTNode ASTNode;






typedef struct {
    ASTNode *condition;
    ASTNode *body;          
} ElifBranch;

struct ASTNode {
    ASTNodeType type;
    int         line;       

    union {
        

        struct { int value; }                           int_literal;

        struct { char *value; int length; }             string_literal;

        struct { int value;  }     bool_literal;

        

        struct { char *name; }                          identifier;

        struct {
            TokenType  op;      
            ASTNode   *left;
            ASTNode   *right;
        } binary_op;

        struct {
            TokenType  op;      
            ASTNode   *operand;
        } unary_op;

        struct {
            char     *name;     
            ASTNode **args;     
            int       arg_count;
        } function_call;

        struct {
            ASTNode **elements;
            int       count;
        } list_literal;

        struct {
            ASTNode *object;    
            ASTNode *index;     
        } index_access;

        struct {
            ASTNode  *object;   
            char     *method;   
            ASTNode **args;
            int       arg_count;
        } dot_method_call;

        

        struct {
            ASTNode *target;    
            ASTNode *value;
        } assignment;

        struct {
            TokenType  op;      
            ASTNode   *target;  
            ASTNode   *value;
        } aug_assignment;

        struct {
            ASTNode **args;
            int       arg_count;
        } print_statement;

        struct {
            ASTNode *value;     
        } return_statement;

        

        struct {
            ASTNode *expr;
        } expr_statement;

        

        struct {
            ASTNode     *condition;     
            ASTNode     *if_body;       
            ElifBranch  *elifs;         
            int          elif_count;
            ASTNode     *else_body;     
        } if_statement;

        struct {
            ASTNode *condition;
            ASTNode *body;              
        } while_loop;

        struct {
            char    *var_name;          
            ASTNode *iterable;          
            ASTNode *body;              
        } for_loop;

        struct {
            char     *name;             
            char    **params;           
            int       param_count;
            ASTNode  *body;             
        } function_def;

        struct {
            ASTNode **statements;
            int       count;
        } block;

        struct {
            ASTNode **statements;
            int       count;
        } program;
    } as;
};






ASTNode *ast_node_create(ASTNodeType type, int line);


void ast_node_free(ASTNode *node);


const char *ast_node_type_name(ASTNodeType type);

void ast_print(const ASTNode *node, int indent);

#endif 
