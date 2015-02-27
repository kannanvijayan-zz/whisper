
bin_PROGRAMS = whisper
whisper_CXXFLAGS = -Wall -std=c++14
if DEBUG
  whisper_CXXFLAGS += -g -O0 -gdwarf-4
endif
if WARNERR
  whisper_CXXFLAGS += -Werror
endif
whisper_SOURCES = \
    debug.cpp \
    spew.cpp \
    memalloc.cpp \
    slab.cpp \
    runtime.cpp \
    parser/code_source.cpp \
    parser/tokenizer.cpp \
    parser/syntax_tree.cpp \
    parser/parser.cpp \
    gc/core.cpp \
    vm/string.cpp \
    vm/lexical_namespace.cpp \
    whisper.cpp
