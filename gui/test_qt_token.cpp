#include <QCoreApplication>
#include <QString>
#include <QByteArray>
#include <iostream>

extern "C" {
#include "lexer.h"
#include "parser.h"
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QString src = 
        "def fizzbuzz(n):\n"
        "    for i in range(1, n):\n"
        "        if i % 15 == 0:\n"
        "            print(\"FizzBuzz\")\n"
        "        elif i % 3 == 0:\n"
        "            print(\"Fizz\")\n"
        "        elif i % 5 == 0:\n"
        "            print(\"Buzz\")\n"
        "        else:\n"
        "            print(i)\n"
        "\n"
        "fizzbuzz(21)\n";
    QByteArray bytes = src.toUtf8();
    
    Lexer lex;
    lexer_init(&lex, bytes.constData());
    int token_count;
    Token *tokens = lexer_tokenize_all(&lex, &token_count);
    
    if (tokens) {
        Parser parser;
        parser_init(&parser, tokens, token_count);
        ASTNode *program = parser_parse(&parser);
        if (parser_had_error(&parser)) {
            std::cout << "Parse error: " << parser_error_message(&parser) << std::endl;
        } else {
            std::cout << "Parsed OK!" << std::endl;
        }
    }
    return 0;
}
