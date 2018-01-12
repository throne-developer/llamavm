AUTOMAKE_OPTIONS = foreign
SUBDIRS =
CC = gcc
CXX = g++
noinst_PROGRAMS = llamavm

llamavm_LDADD = 

llamavm_SOURCES = \
  src/main.cpp \
  src/test/test.cpp \
  src/test/test_gc.cpp \
  src/base/memory.cpp \
  src/base/lobject.cpp \
  src/base/dump.cpp \
  src/base/common.cpp \
  src/base/table.cpp \
  src/base/const.cpp \
  src/base/state.cpp \
  src/code/lex.cpp \
  src/code/gram_func.cpp \
  src/code/gram_expr.cpp \
  src/code/gram_cond.cpp \
  src/code/gram.cpp \
  src/vm/gc.cpp \
  src/vm/vm.cpp \
  src/lib/builtin.cpp 

LIBS_INCLUDES = -I$(top_srcdir)/src

INCLUDES = $(LIBS_INCLUDES)

AM_CXXFLAGS = -D_linux_ -DDEBUG -D__x86_64__

CXXFLAGS += -ggdb -pg -O0

