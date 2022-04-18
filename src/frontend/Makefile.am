AUTOMAKE_OPTIONS = foreign
AM_YFLAGS = -d
AM_CPPFLAGS = -I./third_parties/argparse -I./include
noinst_LIBRARIES = libzjucc.a
libzjucc_a_SOURCES = src/ast_impl.c src/builtin.cpp src/parser.y src/tokenizer.l src/semantic.cpp src/lib_zjucc.cpp include/config.h include/ast_impl.h include/builtin.h include/semantic.h
noinst_PROGRAMS = zjucc
zjucc_SOURCES = src/entry.cpp src/ast_impl.c src/builtin.cpp src/parser.y src/tokenizer.l src/semantic.cpp include/config.h include/ast_impl.h include/builtin.h include/semantic.h