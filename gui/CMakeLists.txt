








cmake_minimum_required(VERSION 3.16)
project(pylite_gui LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)


find_package(Qt6 COMPONENTS Widgets QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets)
    message(STATUS "Using Qt5")
    set(QT_LIBS Qt5::Widgets)
else()
    message(STATUS "Using Qt6")
    set(QT_LIBS Qt6::Widgets)
endif()


set(PYLITE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(pylite_core STATIC
    ${PYLITE_ROOT}/src/token.c
    ${PYLITE_ROOT}/src/lexer.c
    ${PYLITE_ROOT}/src/ast.c
    ${PYLITE_ROOT}/src/parser.c
    ${PYLITE_ROOT}/src/ast_printer.c
    ${PYLITE_ROOT}/src/analyzer.c
    ${PYLITE_ROOT}/src/value.c
    ${PYLITE_ROOT}/src/environment.c
    ${PYLITE_ROOT}/src/eval.c
    ${PYLITE_ROOT}/src/builtins.c
    ${PYLITE_ROOT}/src/interpreter.c
)

target_include_directories(pylite_core PUBLIC ${PYLITE_ROOT}/include)
set_target_properties(pylite_core PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)


add_executable(pylite_gui
    main.cpp
    mainwindow.h
    mainwindow.cpp
    highlighter.h
    highlighter.cpp
)

target_link_libraries(pylite_gui PRIVATE pylite_core ${QT_LIBS})
target_include_directories(pylite_gui PRIVATE ${PYLITE_ROOT}/include)

add_executable(test_qt_token test_qt_token.cpp)
if (NOT Qt6_FOUND)
    target_link_libraries(test_qt_token PRIVATE pylite_core Qt5::Core)
else()
    target_link_libraries(test_qt_token PRIVATE pylite_core Qt6::Core)
endif()
target_include_directories(test_qt_token PRIVATE ${PYLITE_ROOT}/include)
