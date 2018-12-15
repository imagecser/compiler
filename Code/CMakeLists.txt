project(compiler)
cmake_minimum_required(VERSION 3.12)
add_executable(compiler intercode.c intercode.h main.c node.c node.h semantic.c semantic.h lex.yy.c syntax.tab.c syntax.tab.h util.c)
