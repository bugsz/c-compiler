AUTOMAKE_OPTIONS = foreign
AM_YFLAGS = -d
AM_CPPFLAGS = -I./third_parties/argparse -I./include
noinst_PROGRAMS = zjucc
zjucc_SOURCES = include/_config.h include/ast.h src/ast_impl.c include/ast_impl.h src/entry.cpp src/parser.y src/tokenizer.l src/semantic.cpp include/semantic.h include/builtin.h src/builtin.cpp