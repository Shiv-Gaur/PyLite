#include "token.h"

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_INT_LITERAL:    return "INT_LITERAL";
        case TOKEN_STRING_LITERAL: return "STRING_LITERAL";
        case TOKEN_TRUE:           return "TRUE";
        case TOKEN_FALSE:          return "FALSE";
        case TOKEN_NONE:           return "NONE";
        case TOKEN_IDENTIFIER:     return "IDENTIFIER";
        case TOKEN_IF:             return "IF";
        case TOKEN_ELIF:           return "ELIF";
        case TOKEN_ELSE:           return "ELSE";
        case TOKEN_WHILE:          return "WHILE";
        case TOKEN_FOR:            return "FOR";
        case TOKEN_IN:             return "IN";
        case TOKEN_DEF:            return "DEF";
        case TOKEN_RETURN:         return "RETURN";
        case TOKEN_AND:            return "AND";
        case TOKEN_OR:             return "OR";
        case TOKEN_NOT:            return "NOT";
        case TOKEN_PRINT:          return "PRINT";
        case TOKEN_INPUT:          return "INPUT";
        case TOKEN_LEN:            return "LEN";
        case TOKEN_RANGE:          return "RANGE";
        case TOKEN_TYPE:           return "TYPE";
        case TOKEN_APPEND:         return "APPEND";
        case TOKEN_POP:            return "POP";
        case TOKEN_BREAK:          return "BREAK";
        case TOKEN_CONTINUE:       return "CONTINUE";
        case TOKEN_PASS:           return "PASS";
        case TOKEN_PLUS:           return "PLUS";
        case TOKEN_MINUS:          return "MINUS";
        case TOKEN_STAR:           return "STAR";
        case TOKEN_SLASH:          return "SLASH";
        case TOKEN_DOUBLE_SLASH:   return "DOUBLE_SLASH";
        case TOKEN_PERCENT:        return "PERCENT";
        case TOKEN_DOUBLE_STAR:    return "DOUBLE_STAR";
        case TOKEN_EQ:             return "EQ";
        case TOKEN_NEQ:            return "NEQ";
        case TOKEN_LT:             return "LT";
        case TOKEN_GT:             return "GT";
        case TOKEN_LTE:            return "LTE";
        case TOKEN_GTE:            return "GTE";
        case TOKEN_ASSIGN:         return "ASSIGN";
        case TOKEN_PLUS_ASSIGN:    return "PLUS_ASSIGN";
        case TOKEN_MINUS_ASSIGN:   return "MINUS_ASSIGN";
        case TOKEN_STAR_ASSIGN:    return "STAR_ASSIGN";
        case TOKEN_SLASH_ASSIGN:   return "SLASH_ASSIGN";
        case TOKEN_LPAREN:         return "LPAREN";
        case TOKEN_RPAREN:         return "RPAREN";
        case TOKEN_LBRACKET:       return "LBRACKET";
        case TOKEN_RBRACKET:       return "RBRACKET";
        case TOKEN_COLON:          return "COLON";
        case TOKEN_COMMA:          return "COMMA";
        case TOKEN_DOT:            return "DOT";
        case TOKEN_INDENT:         return "INDENT";
        case TOKEN_DEDENT:         return "DEDENT";
        case TOKEN_NEWLINE:        return "NEWLINE";
        case TOKEN_EOF:            return "EOF";
        case TOKEN_ERROR:          return "ERROR";
    }
    return "UNKNOWN";
}
