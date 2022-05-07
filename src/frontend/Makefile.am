AUTOMAKE_OPTIONS = foreign
AM_YFLAGS = -d
AM_CPPFLAGS = -I./third_parties/argparse -I./include -I./third_parties/simplecpp -I./third_parties/tinyexpr
noinst_LIBRARIES = libzjucc.a
libzjucc_a_SOURCES = src/ast_impl.c src/builtin.cpp src/parser.y src/tokenizer.l src/semantic.cpp src/cmdline.cpp src/export.cpp third_parties/simplecpp/simplecpp.cpp src/preprocessor.cpp third_parties/tinyexpr/tinyexpr.c