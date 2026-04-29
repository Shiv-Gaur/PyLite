#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

static int is_at_end(const Lexer *lex) {
    return *lex->current == '\0';
}

static char peek(const Lexer *lex) {
    return *lex->current;
}


static char advance(Lexer *lex) {
    char c = *lex->current;
    lex->current++;
    lex->column++;
    return c;
}

static int match(Lexer *lex, char expected) {
    if (is_at_end(lex)) return 0;
    if (*lex->current != expected) return 0;
    lex->current++;
    lex->column++;
    return 1;
}

static Token make_token(const Lexer *lex, TokenType type,
                        const char *start, int length, int line, int col) {
    (void)lex;  
    Token t;
    t.type   = type;
    t.lexeme = start;
    t.length = length;
    t.line   = line;
    t.column = col;
    return t;
}

static Token error_token(const Lexer *lex, const char *msg) {
    Token t;
    t.type   = TOKEN_ERROR;
    t.lexeme = msg;
    t.length = (int)strlen(msg);
    t.line   = lex->line;
    t.column = lex->column;
    return t;
}


typedef struct {
    const char *word;
    int         length;
    TokenType   type;
} Keyword;

static const Keyword keywords[] = {
    { "if",       2, TOKEN_IF       },
    { "elif",     4, TOKEN_ELIF     },
    { "else",     4, TOKEN_ELSE     },
    { "while",    5, TOKEN_WHILE    },
    { "for",      3, TOKEN_FOR      },
    { "in",       2, TOKEN_IN       },
    { "def",      3, TOKEN_DEF      },
    { "return",   6, TOKEN_RETURN   },
    { "and",      3, TOKEN_AND      },
    { "or",       2, TOKEN_OR       },
    { "not",      3, TOKEN_NOT      },
    { "True",     4, TOKEN_TRUE     },
    { "False",    5, TOKEN_FALSE    },
    { "None",     4, TOKEN_NONE     },
    { "print",    5, TOKEN_PRINT    },
    { "input",    5, TOKEN_INPUT    },
    { "len",      3, TOKEN_LEN      },
    { "range",    5, TOKEN_RANGE    },
    { "type",     4, TOKEN_TYPE     },
    { "append",   6, TOKEN_APPEND   },
    { "pop",      3, TOKEN_POP      },
    { "break",    5, TOKEN_BREAK    },
    { "continue", 8, TOKEN_CONTINUE },
    { "pass",     4, TOKEN_PASS     },
    { NULL,       0, TOKEN_IDENTIFIER }  
};

static TokenType identifier_type(const char *start, int length) {
    for (int i = 0; keywords[i].word != NULL; i++) {
        if (keywords[i].length == length &&
            memcmp(keywords[i].word, start, length) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}


static void skip_comment(Lexer *lex) {
    while (!is_at_end(lex) && peek(lex) != '\n') {
        advance(lex);
    }
}


static Token scan_number(Lexer *lex) {
    const char *start = lex->current - 1;  
    int col = lex->column - 1;

    while (!is_at_end(lex) && isdigit((unsigned char)peek(lex))) {
        advance(lex);
    }

    int length = (int)(lex->current - start);
    return make_token(lex, TOKEN_INT_LITERAL, start, length, lex->line, col);
}

static Token scan_string(Lexer *lex, char quote) {
    const char *start = lex->current - 1;  
    int start_line = lex->line;
    int col = lex->column - 1;

    while (!is_at_end(lex) && peek(lex) != quote) {
        if (peek(lex) == '\\') {
            advance(lex);                   
            if (!is_at_end(lex)) advance(lex); 
        } else if (peek(lex) == '\n') {
            return error_token(lex, "Unterminated string (newline before closing quote)");
        } else {
            advance(lex);
        }
    }

    if (is_at_end(lex)) {
        return error_token(lex, "Unterminated string (EOF)");
    }

    advance(lex); 
    int length = (int)(lex->current - start);
    return make_token(lex, TOKEN_STRING_LITERAL, start, length, start_line, col);
}

static Token scan_identifier(Lexer *lex) {
    const char *start = lex->current - 1;
    int col = lex->column - 1;

    while (!is_at_end(lex) &&
           (isalnum((unsigned char)peek(lex)) || peek(lex) == '_')) {
        advance(lex);
    }

    int length = (int)(lex->current - start);
    TokenType type = identifier_type(start, length);
    return make_token(lex, type, start, length, lex->line, col);
}


static int process_indentation(Lexer *lex, Token *err) {
    int spaces = 0;
    while (!is_at_end(lex) && (peek(lex) == ' ' || peek(lex) == '\t')) {
        if (peek(lex) == '\t') spaces += 1;  
        else                   spaces += 1;
        advance(lex);
    }

    if (is_at_end(lex) || peek(lex) == '\n' || peek(lex) == '#') {
        return 0;
    }

    int current_indent = lex->indent_stack[lex->indent_top];

    if (spaces > current_indent) {
        if (lex->indent_top + 1 >= LEXER_MAX_INDENT_DEPTH) {
            *err = error_token(lex, "Too many indentation levels");
            return -1;
        }
        lex->indent_top++;
        lex->indent_stack[lex->indent_top] = spaces;
        lex->pending_indent = 1;
    } else if (spaces < current_indent) {
        while (lex->indent_top > 0 &&
               lex->indent_stack[lex->indent_top] > spaces) {
            lex->indent_top--;
            lex->pending_dedents++;
        }
        if (lex->indent_stack[lex->indent_top] != spaces) {
            *err = error_token(lex, "Indentation error: unindent does not match any outer level");
            return -1;
        }
    }

    lex->at_line_start = 0;
    return 0;
}


void lexer_init(Lexer *lexer, const char *source) {
    lexer->source          = source;
    lexer->current         = source;
    lexer->line            = 1;
    lexer->column          = 1;
    lexer->indent_stack[0] = 0;
    lexer->indent_top      = 0;
    lexer->pending_dedents = 0;
    lexer->pending_indent  = 0;
    lexer->at_line_start   = 1;
    lexer->paren_depth     = 0;
    lexer->emit_eof_dedents = 0;
}

Token lexer_next_token(Lexer *lexer) {

    if (lexer->pending_dedents > 0) {
        lexer->pending_dedents--;
        return make_token(lexer, TOKEN_DEDENT,
                          lexer->current, 0, lexer->line, lexer->column);
    }

    if (lexer->pending_indent) {
        lexer->pending_indent = 0;
        return make_token(lexer, TOKEN_INDENT,
                          lexer->current, 0, lexer->line, lexer->column);
    }

    if (lexer->emit_eof_dedents) {
        if (lexer->indent_top > 0) {
            lexer->indent_top--;
            return make_token(lexer, TOKEN_DEDENT,
                              lexer->current, 0, lexer->line, lexer->column);
        }
        lexer->emit_eof_dedents = 0;
        return make_token(lexer, TOKEN_EOF,
                          lexer->current, 0, lexer->line, lexer->column);
    }

    if (lexer->at_line_start && lexer->paren_depth == 0) {
        Token err;
        if (process_indentation(lexer, &err) < 0) {
            return err;
        }
        if (lexer->pending_dedents > 0) {
            lexer->pending_dedents--;
            return make_token(lexer, TOKEN_DEDENT,
                              lexer->current, 0, lexer->line, lexer->column);
        }
        if (lexer->pending_indent) {
            lexer->pending_indent = 0;
            return make_token(lexer, TOKEN_INDENT,
                              lexer->current, 0, lexer->line, lexer->column);
        }
    }

    while (!is_at_end(lexer) && (peek(lexer) == ' ' || peek(lexer) == '\t')) {
        advance(lexer);
    }

    if (!is_at_end(lexer) && peek(lexer) == '#') {
        skip_comment(lexer);
    }

    
    if (is_at_end(lexer)) {
        if (lexer->indent_top > 0) {
            lexer->emit_eof_dedents = 1;
            return make_token(lexer, TOKEN_NEWLINE,
                              lexer->current, 0, lexer->line, lexer->column);
        }
        return make_token(lexer, TOKEN_EOF,
                          lexer->current, 0, lexer->line, lexer->column);
    }

    if (peek(lexer) == '\n') {
        int nl_line = lexer->line;
        int nl_col  = lexer->column;
        advance(lexer);
        lexer->line++;
        lexer->column = 1;
        lexer->at_line_start = 1;

        if (lexer->paren_depth > 0) {
            return lexer_next_token(lexer);
        }

        return make_token(lexer, TOKEN_NEWLINE,
                          lexer->current - 1, 1, nl_line, nl_col);
    }

    if (peek(lexer) == '\r') {
        advance(lexer);
        if (!is_at_end(lexer) && peek(lexer) == '\n') {
            advance(lexer);
        }
        lexer->line++;
        lexer->column = 1;
        lexer->at_line_start = 1;
        if (lexer->paren_depth > 0) {
            return lexer_next_token(lexer);
        }
        return make_token(lexer, TOKEN_NEWLINE,
                          lexer->current - 1, 1, lexer->line - 1, 1);
    }

    if (peek(lexer) == ';') {
        advance(lexer);
        return lexer_next_token(lexer);
    }
    const char *start = lexer->current;
    int start_col     = lexer->column;
    char c            = advance(lexer);

    if (isdigit((unsigned char)c)) {
        return scan_number(lexer);
    }

    if (isalpha((unsigned char)c) || c == '_') {
        return scan_identifier(lexer);
    }

    if (c == '"' || c == '\'') {
        return scan_string(lexer, c);
    }

    switch (c) {
        case '+':
            if (match(lexer, '='))
                return make_token(lexer, TOKEN_PLUS_ASSIGN, start, 2, lexer->line, start_col);
            return make_token(lexer, TOKEN_PLUS, start, 1, lexer->line, start_col);

        case '-':
            if (match(lexer, '='))
                return make_token(lexer, TOKEN_MINUS_ASSIGN, start, 2, lexer->line, start_col);
            return make_token(lexer, TOKEN_MINUS, start, 1, lexer->line, start_col);

        case '*':
            if (match(lexer, '*'))
                return make_token(lexer, TOKEN_DOUBLE_STAR, start, 2, lexer->line, start_col);
            if (match(lexer, '='))
                return make_token(lexer, TOKEN_STAR_ASSIGN, start, 2, lexer->line, start_col);
            return make_token(lexer, TOKEN_STAR, start, 1, lexer->line, start_col);

        case '/':
            if (match(lexer, '/'))
                return make_token(lexer, TOKEN_DOUBLE_SLASH, start, 2, lexer->line, start_col);
            if (match(lexer, '='))
                return make_token(lexer, TOKEN_SLASH_ASSIGN, start, 2, lexer->line, start_col);
            return make_token(lexer, TOKEN_SLASH, start, 1, lexer->line, start_col);

        case '%':
            return make_token(lexer, TOKEN_PERCENT, start, 1, lexer->line, start_col);

        case '=':
            if (match(lexer, '='))
                return make_token(lexer, TOKEN_EQ, start, 2, lexer->line, start_col);
            return make_token(lexer, TOKEN_ASSIGN, start, 1, lexer->line, start_col);

        case '!':
            if (match(lexer, '='))
                return make_token(lexer, TOKEN_NEQ, start, 2, lexer->line, start_col);
            return error_token(lexer, "Unexpected character '!'");

        case '<':
            if (match(lexer, '='))
                return make_token(lexer, TOKEN_LTE, start, 2, lexer->line, start_col);
            return make_token(lexer, TOKEN_LT, start, 1, lexer->line, start_col);

        case '>':
            if (match(lexer, '='))
                return make_token(lexer, TOKEN_GTE, start, 2, lexer->line, start_col);
            return make_token(lexer, TOKEN_GT, start, 1, lexer->line, start_col);

        case '(':
            lexer->paren_depth++;
            return make_token(lexer, TOKEN_LPAREN, start, 1, lexer->line, start_col);
        case ')':
            if (lexer->paren_depth > 0) lexer->paren_depth--;
            return make_token(lexer, TOKEN_RPAREN, start, 1, lexer->line, start_col);
        case '[':
            lexer->paren_depth++;
            return make_token(lexer, TOKEN_LBRACKET, start, 1, lexer->line, start_col);
        case ']':
            if (lexer->paren_depth > 0) lexer->paren_depth--;
            return make_token(lexer, TOKEN_RBRACKET, start, 1, lexer->line, start_col);

        case ':':
            return make_token(lexer, TOKEN_COLON, start, 1, lexer->line, start_col);
        case ',':
            return make_token(lexer, TOKEN_COMMA, start, 1, lexer->line, start_col);
        case '.':
            return make_token(lexer, TOKEN_DOT, start, 1, lexer->line, start_col);
    }

    return error_token(lexer, "Unexpected character");
}

Token *lexer_tokenize_all(Lexer *lexer, int *out_count) {
    int capacity = 256;
    int count    = 0;
    Token *tokens = (Token *)malloc(sizeof(Token) * capacity);
    if (!tokens) return NULL;

    for (;;) {
        if (count >= capacity) {
            capacity *= 2;
            Token *tmp = (Token *)realloc(tokens, sizeof(Token) * capacity);
            if (!tmp) { free(tokens); return NULL; }
            tokens = tmp;
        }
        tokens[count] = lexer_next_token(lexer);
        if (tokens[count].type == TOKEN_EOF) {
            count++;
            break;
        }
        count++;
    }

    *out_count = count;
    return tokens;
}
