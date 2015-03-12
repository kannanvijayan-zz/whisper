
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
    parser/packed_writer.cpp \
    parser/packed_reader.cpp \
    gc/core.cpp \
    vm/string.cpp \
    vm/shype/shype.cpp \
    vm/shype/dict_shype.cpp \
    vm/wobject.cpp \
    vm/plain_object.cpp \
    whisper.cpp
