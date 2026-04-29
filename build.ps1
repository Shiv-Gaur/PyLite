Write-Host ""
Write-Host "===== Building PyLite v1.0 (Full Interpreter) =====" -ForegroundColor Cyan
Write-Host ""

if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

if (!(Test-Path "src\lexer.c")) {
    Write-Host "ERROR: src\lexer.c not found!" -ForegroundColor Red
    exit 1
}

$CC = "gcc"
$CFLAGS = "-std=c99 -Wall -Wextra -pedantic -g"

$sources = @(
    @("token.c",       "token.o"),
    @("lexer.c",       "lexer.o"),
    @("ast.c",         "ast.o"),
    @("parser.c",      "parser.o"),
    @("ast_printer.c", "ast_printer.o"),
    @("analyzer.c",    "analyzer.o"),
    @("value.c",       "value.o"),
    @("environment.c", "environment.o"),
    @("eval.c",        "eval.o"),
    @("builtins.c",    "builtins.o"),
    @("interpreter.c", "interpreter.o")
)

foreach ($src in $sources) {
    Write-Host "Compiling $($src[0])..." -ForegroundColor Yellow
    & gcc -std=c99 -Wall -Wextra -pedantic -g -Iinclude -c "src/$($src[0])" -o "build/$($src[1])"
    if ($LASTEXITCODE -ne 0) { Write-Host "FAILED compiling $($src[0])" -ForegroundColor Red; exit 1 }
}

$OBJS = "build/token.o build/lexer.o build/ast.o build/parser.o build/ast_printer.o build/analyzer.o build/value.o build/environment.o build/eval.o build/builtins.o build/interpreter.o"
$OBJS_ARR = $OBJS -split " "

Write-Host "Building pylite.exe..." -ForegroundColor Yellow
& gcc -std=c99 -Wall -Wextra -pedantic -g -Iinclude src/main.c @OBJS_ARR -o build/pylite.exe
if ($LASTEXITCODE -ne 0) { Write-Host "FAILED" -ForegroundColor Red; exit 1 }

Write-Host "Building test_lexer.exe..." -ForegroundColor Yellow
& gcc -std=c99 -Wall -Wextra -pedantic -g -Iinclude tests/test_lexer.c @OBJS_ARR -o build/test_lexer.exe
if ($LASTEXITCODE -ne 0) { Write-Host "FAILED" -ForegroundColor Red; exit 1 }

Write-Host "Building test_parser.exe..." -ForegroundColor Yellow
& gcc -std=c99 -Wall -Wextra -pedantic -g -Iinclude tests/test_parser.c @OBJS_ARR -o build/test_parser.exe
if ($LASTEXITCODE -ne 0) { Write-Host "FAILED" -ForegroundColor Red; exit 1 }

Write-Host "Building test_analyzer.exe..." -ForegroundColor Yellow
& gcc -std=c99 -Wall -Wextra -pedantic -g -Iinclude tests/test_analyzer.c @OBJS_ARR -o build/test_analyzer.exe
if ($LASTEXITCODE -ne 0) { Write-Host "FAILED" -ForegroundColor Red; exit 1 }

if (Test-Path "tests\test_interpreter.c") {
    Write-Host "Building test_interpreter.exe..." -ForegroundColor Yellow
    & gcc -std=c99 -Wall -Wextra -pedantic -g -Iinclude tests/test_interpreter.c @OBJS_ARR -o build/test_interpreter.exe
    if ($LASTEXITCODE -ne 0) { Write-Host "FAILED" -ForegroundColor Red; exit 1 }
}

Write-Host ""
Write-Host "===== Build Successful! =====" -ForegroundColor Green
Write-Host ""

Write-Host "===== Running Lexer Tests =====" -ForegroundColor Cyan
& ./build/test_lexer.exe
Write-Host ""

Write-Host "===== Running Parser Tests =====" -ForegroundColor Cyan
& ./build/test_parser.exe
Write-Host ""

Write-Host "===== Running Analyzer Tests =====" -ForegroundColor Cyan
& ./build/test_analyzer.exe
Write-Host ""

if (Test-Path "build\test_interpreter.exe") {
    Write-Host "===== Running Interpreter Tests =====" -ForegroundColor Cyan
    & ./build/test_interpreter.exe
    Write-Host ""
}

Write-Host "===== Demo: Running examples/factorial.py =====" -ForegroundColor Cyan
& ./build/pylite.exe examples/factorial.py
Write-Host ""
