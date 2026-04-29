#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "interpreter.h"

static int tests_run = 0;
static int tests_passed = 0;

static ASTNode *parse_source(const char *src, Token **out_tokens) {
    Lexer lex;
    lexer_init(&lex, src);
    int tc;
    *out_tokens = lexer_tokenize_all(&lex, &tc);
    Parser p;
    parser_init(&p, *out_tokens, tc);
    return parser_parse(&p);
}

static void run_test(const char *name, const char *src) {
    tests_run++;
    Token *tokens = NULL;
    ASTNode *prog = parse_source(src, &tokens);
    if (!prog) { printf("  FAIL: %s (parse returned NULL)\n", name); free(tokens); return; }
    interpret(prog);
    ast_node_free(prog);
    free(tokens);
    tests_passed++;
    printf("  PASS: %s\n", name);
}

int main(void) {
    printf("===== PyLite Interpreter Tests =====\n\n");

    run_test("Variable assignment",
        "x = 42\nprint(x)\n");

    run_test("Arithmetic",
        "x = 2 + 3 * 4\nprint(x)\n");

    run_test("String concat",
        "s = \"hello\" + \" \" + \"world\"\nprint(s)\n");

    run_test("String repeat",
        "s = \"ha\" * 3\nprint(s)\n");

    run_test("Boolean logic",
        "x = True and False\nprint(x)\ny = True or False\nprint(y)\n");

    run_test("If/elif/else",
        "x = 10\nif x > 20:\n    print(\"big\")\nelif x > 5:\n    print(\"medium\")\nelse:\n    print(\"small\")\n");

    run_test("While loop (factorial)",
        "n = 5\nresult = 1\nwhile n > 0:\n    result = result * n\n    n = n - 1\nprint(result)\n");

    run_test("For loop with range",
        "total = 0\nfor i in range(10):\n    total = total + i\nprint(total)\n");

    run_test("Nested loops",
        "for i in range(1, 4):\n    for j in range(1, 4):\n        print(i * j)\n");

    run_test("Function def and call",
        "def add(a, b):\n    return a + b\nprint(add(3, 4))\n");

    run_test("Recursive factorial",
        "def fact(n):\n    if n <= 1:\n        return 1\n    return n * fact(n - 1)\nprint(fact(5))\n");

    run_test("List operations",
        "nums = [1, 2, 3]\nnums.append(4)\nprint(nums)\nprint(len(nums))\n");

    run_test("List iteration",
        "total = 0\nfor x in [1, 2, 3, 4, 5]:\n    total = total + x\nprint(total)\n");

    run_test("List pop",
        "nums = [10, 20, 30]\nx = nums.pop()\nprint(x)\nprint(nums)\n");

    run_test("Break and continue",
        "for i in range(10):\n    if i == 3:\n        continue\n    if i == 7:\n        break\n    print(i)\n");

    run_test("Augmented assignment",
        "x = 5\nx += 3\nprint(x)\nx -= 2\nprint(x)\nx *= 4\nprint(x)\n");

    run_test("Built-in len",
        "print(len(\"hello\"))\nprint(len([1, 2, 3]))\n");

    run_test("Built-in type",
        "print(type(42))\nprint(type(\"hi\"))\nprint(type(True))\nprint(type(None))\nprint(type([]))\n");

    run_test("Built-in str and int",
        "print(str(42))\nprint(int(\"99\"))\n");

    run_test("Built-in abs",
        "print(abs(-5))\nprint(abs(3))\n");

    run_test("FizzBuzz",
        "for i in range(1, 16):\n    if i % 15 == 0:\n        print(\"FizzBuzz\")\n    elif i % 3 == 0:\n        print(\"Fizz\")\n    elif i % 5 == 0:\n        print(\"Buzz\")\n    else:\n        print(i)\n");

    run_test("Fibonacci",
        "def fib(n):\n    a = 0\n    b = 1\n    for i in range(n):\n        print(a)\n        temp = a + b\n        a = b\n        b = temp\nfib(10)\n");

    run_test("Comparison operators",
        "print(1 < 2)\nprint(3 > 5)\nprint(2 == 2)\nprint(1 != 2)\nprint(3 <= 3)\nprint(4 >= 5)\n");

    run_test("Unary minus and not",
        "print(-5)\nprint(not True)\nprint(not False)\n");

    run_test("Floor division and modulo",
        "print(7 // 2)\nprint(7 % 3)\nprint(-7 // 2)\n");

    run_test("Power operator",
        "print(2 ** 10)\nprint(3 ** 3)\n");

    run_test("String comparison",
        "print(\"abc\" < \"def\")\nprint(\"xyz\" > \"abc\")\n");

    run_test("Nested function calls",
        "def double(x):\n    return x * 2\ndef add_one(x):\n    return x + 1\nprint(double(add_one(4)))\n");

    run_test("Pass statement",
        "def empty():\n    pass\nempty()\nprint(\"ok\")\n");

    run_test("For with range(start, stop, step)",
        "for i in range(0, 10, 2):\n    print(i)\n");

    printf("\n===== Results: %d/%d passed =====\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
