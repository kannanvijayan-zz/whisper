
bin_PROGRAMS = whisper tokenizer
AM_CXXFLAGS = -Wall -std=c++14
if DEBUG
  AM_CXXFLAGS += -g -O0 -gdwarf-4
endif
if WARNERR
  AM_CXXFLAGS += -Werror
endif

shared_cpp = \
    debug.cpp \
    spew.cpp \
    memalloc.cpp \
    slab.cpp \
    runtime.cpp \
    parser/code_source.cpp \
    parser/tokenizer.cpp \
    parser/syntax_tree.cpp \
    parser/parser.cpp \
    parser/packed_writer.cpp \
    parser/packed_reader.cpp \
    gc/tracing.cpp \
    vm/string.cpp \
    vm/source_file.cpp \
    vm/wobject.cpp \
    vm/plain_object.cpp \
    vm/packed_syntax_tree.cpp \
    vm/property_dict.cpp

whisper_SOURCES = \
    $(shared_cpp) \
    shell/shell.cpp

tokenizer_SOURCES = \
    $(shared_cpp) \
    shell/shell_tokenizer.cpp
