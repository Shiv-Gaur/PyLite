@echo off
REM PyLite v0.2 — Windows Build Script
REM ====================================
REM Requires: GCC (MinGW-w64) installed and on PATH
REM
REM Usage:
REM   build.bat           — build everything and run tests
REM   build.bat test      — build and run all tests
REM   build.bat demo      — build and run demo
REM   build.bat clean     — remove build artifacts

setlocal

set CC=gcc
set CFLAGS=-std=c99 -Wall -Wextra -pedantic -g
set INCLUDES=-Iinclude

REM Create build directory
if not exist build mkdir build

if "%1"=="clean" goto clean
if "%1"=="demo" goto demo

:build
echo.
echo === Building PyLite v0.2 (Lexer + Parser + Analyzer) ===
echo.

%CC% %CFLAGS% %INCLUDES% -c src\token.c -o build\token.o
if errorlevel 1 goto fail

%CC% %CFLAGS% %INCLUDES% -c src\lexer.c -o build\lexer.o
if errorlevel 1 goto fail

%CC% %CFLAGS% %INCLUDES% -c src\ast.c -o build\ast.o
if errorlevel 1 goto fail

%CC% %CFLAGS% %INCLUDES% -c src\parser.c -o build\parser.o
if errorlevel 1 goto fail

%CC% %CFLAGS% %INCLUDES% -c src\ast_printer.c -o build\ast_printer.o
if errorlevel 1 goto fail

%CC% %CFLAGS% %INCLUDES% -c src\analyzer.c -o build\analyzer.o
if errorlevel 1 goto fail

set OBJS=build\token.o build\lexer.o build\ast.o build\parser.o build\ast_printer.o build\analyzer.o

echo Building test_lexer.exe...
%CC% %CFLAGS% %INCLUDES% tests\test_lexer.c %OBJS% -o build\test_lexer.exe
if errorlevel 1 goto fail

echo Building test_parser.exe...
%CC% %CFLAGS% %INCLUDES% tests\test_parser.c %OBJS% -o build\test_parser.exe
if errorlevel 1 goto fail

echo Building test_analyzer.exe...
%CC% %CFLAGS% %INCLUDES% tests\test_analyzer.c %OBJS% -o build\test_analyzer.exe
if errorlevel 1 goto fail

echo Building pylite.exe...
%CC% %CFLAGS% %INCLUDES% src\main.c %OBJS% -o build\pylite.exe
if errorlevel 1 goto fail

echo.
echo === Build Successful! ===

if "%1"=="demo" goto demo_run

:test
echo.
echo === Running Lexer Tests ===
echo.
build\test_lexer.exe

echo.
echo === Running Parser Tests ===
echo.
build\test_parser.exe

echo.
echo === Running Analyzer Tests ===
echo.
build\test_analyzer.exe
goto end

:demo
call :build
:demo_run
echo.
echo === Demo: Parsing examples/fizzbuzz.py ===
echo.
build\pylite.exe examples\fizzbuzz.py
goto end

:clean
echo Cleaning build artifacts...
if exist build rmdir /s /q build
echo Done.
goto end

:fail
echo.
echo BUILD FAILED
exit /b 1

:end
endlocal
