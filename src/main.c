#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "analyzer.h"
#include "interpreter.h"

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Error: cannot open '%s'\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buf = (char *)malloc(size + 2);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static int run_tokenizer(const char *source) {
    printf("-------------------------------------------\n");
    printf("%-6s %-4s %-18s %s\n", "LINE", "COL", "TYPE", "LEXEME");
    printf("-------------------------------------------\n");
    Lexer lex;
    lexer_init(&lex, source);
    Token t;
    do {
        t = lexer_next_token(&lex);
        printf("%-6d %-4d %-18s ", t.line, t.column, token_type_name(t.type));
        if (t.length > 0) printf("%.*s", t.length, t.lexeme);
        else if (t.type == TOKEN_INDENT) printf(">>>");
        else if (t.type == TOKEN_DEDENT) printf("<<<");
        printf("\n");
    } while (t.type != TOKEN_EOF && t.type != TOKEN_ERROR);
    printf("-------------------------------------------\n");
    return 0;
}

static int run_parser(const char *source) {
    Lexer lex;
    lexer_init(&lex, source);
    int token_count;
    Token *tokens = lexer_tokenize_all(&lex, &token_count);
    if (!tokens) { fprintf(stderr, "Error: tokenization failed\n"); return 1; }
    printf("Tokenized: %d tokens\n\n", token_count);
    Parser parser;
    parser_init(&parser, tokens, token_count);
    ASTNode *program = parser_parse(&parser);
    if (parser_had_error(&parser)) {
        fprintf(stderr, "Parse error: %s\n", parser_error_message(&parser));
        ast_node_free(program); free(tokens); return 1;
    }
    Analyzer analyzer;
    analyzer_init(&analyzer);
    analyzer_analyze(&analyzer, program);
    if (analyzer_had_error(&analyzer)) {
        fprintf(stderr, "\n--- Semantic Errors ---\n");
        analyzer_print_errors(&analyzer);
    } else { printf("Semantic analysis: OK\n\n"); }
    printf("--- Abstract Syntax Tree ---\n");
    ast_print(program, 0);
    printf("----------------------------\n");
    ast_node_free(program); free(tokens);
    return analyzer_had_error(&analyzer) ? 1 : 0;
}

static int run_file(const char *source) {
    Lexer lex;
    lexer_init(&lex, source);
    int token_count;
    Token *tokens = lexer_tokenize_all(&lex, &token_count);
    if (!tokens) { fprintf(stderr, "Error: tokenization failed\n"); return 1; }
    Parser parser;
    parser_init(&parser, tokens, token_count);
    ASTNode *program = parser_parse(&parser);
    if (parser_had_error(&parser)) {
        fprintf(stderr, "SyntaxError: %s\n", parser_error_message(&parser));
        ast_node_free(program); free(tokens); return 1;
    }
    interpret(program);
    ast_node_free(program); free(tokens);
    return 0;
}

static volatile sig_atomic_t repl_interrupted = 0;
static void sigint_handler(int sig) { (void)sig; repl_interrupted = 1; signal(SIGINT, sigint_handler); }

static void run_repl(void) {
    printf("PyLite 1.0 (Python subset interpreter)\n");
    printf("Type \"exit()\" or Ctrl+D to quit.\n");

    signal(SIGINT, sigint_handler);


    Environment *global = env_create(NULL);
    Interpreter interp;
    interpreter_init(&interp);
    interp.repl_mode = 1;

    char line[4096];
    char block[16384];

    for (;;) {
        if (repl_interrupted) { repl_interrupted = 0; printf("\nKeyboardInterrupt\n"); }
        printf(">>> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) { printf("\n"); break; }

        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        len = strlen(line);

        if (len == 0) continue;
        if (strcmp(line, "exit()") == 0) break;

        strcpy(block, line);
        strcat(block, "\n");

        int needs_more = 0;
        if (len > 0 && line[len-1] == ':') needs_more = 1;

        while (needs_more) {
            printf("... ");
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin)) { needs_more = 0; break; }
            len = strlen(line);
            if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
            len = strlen(line);
            if (len == 0) { needs_more = 0; break; }
            strcat(block, line);
            strcat(block, "\n");
            if (len > 0 && line[len-1] == ':') needs_more = 1;
            else {
                int all_space = 1;
                for (size_t i = 0; i < len; i++) { if (line[i] != ' ' && line[i] != '\t') { all_space = 0; break; } }
                if (all_space) needs_more = 0;
                else {
                    int has_indent = (line[0] == ' ' || line[0] == '\t');
                    if (!has_indent) needs_more = 0;
                }
            }
        }

        Lexer lex;
        lexer_init(&lex, block);
        int token_count;
        Token *tokens = lexer_tokenize_all(&lex, &token_count);
        if (!tokens) continue;

        Parser parser;
        parser_init(&parser, tokens, token_count);
        ASTNode *program = parser_parse(&parser);

        if (parser_had_error(&parser)) {
            fprintf(stderr, "SyntaxError: %s\n", parser_error_message(&parser));
            ast_node_free(program); free(tokens); continue;
        }

        interp.had_error = 0;
        interp.error_msg[0] = '\0';
        interpret_with_env(program, global, &interp);

        ast_node_free(program);
        free(tokens);
    }

    env_free(global);
}

int main(int argc, char **argv) {
    int token_mode = 0;
    int ast_mode = 0;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) token_mode = 1;
        else if (strcmp(argv[i], "--ast") == 0) ast_mode = 1;
        else filepath = argv[i];
    }

    if (!filepath) {
        if (token_mode || ast_mode) {
            printf("Usage: pylite [--tokens|--ast] <file.py>\n");
            return 1;
        }
        run_repl();
        return 0;
    }

    char *source = read_file(filepath);
    if (!source) return 1;

    int result;
    if (token_mode) result = run_tokenizer(source);
    else if (ast_mode) result = run_parser(source);
    else result = run_file(source);

    free(source);
    return result;
}
