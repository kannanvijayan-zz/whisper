
#include <iostream>
#include "common.hpp"
#include "allocators.hpp"
#include "spew.hpp"
#include "parser/code_source.hpp"
#include "parser/tokenizer.hpp"
#include "parser/syntax_tree.hpp"
#include "parser/syntax_tree_inlines.hpp"
#include "parser/parser.hpp"
#include "value.hpp"
#include "slab.hpp"
#include "vm/heap_thing.hpp"
#include "vm/double.hpp"
#include "vm/string.hpp"
#include "runtime.hpp"
#include "rooting.hpp"
#include "ref_scanner.hpp"

#include "vm/reference.hpp"
#include "vm/property_descriptor.hpp"

#include "vm/tuple.hpp"
//#include "vm/shape_tree.hpp"
#include "vm/class.hpp"

using namespace Whisper;

struct Printer {
    void operator ()(const char *s) {
        std::cerr << s;
    }
    void operator ()(const uint8_t *s, uint32_t len) {
        for (size_t i = 0; i < len; i++)
            std::cerr << static_cast<char>(s[i]);
    }
};

int main(int argc, char **argv) {
    std::cout << "Whisper says hello." << std::endl;

    InitializeSpew();

    // Open input file.
    if (argc <= 1) {
        std::cerr << "No input file provided!" << std::endl;
        exit(1);
    }

    FileCodeSource inputFile(argv[1]);
    if (!inputFile.initialize()) {
        std::cerr << "Could not open input file " << argv[1]
                  << " for reading." << std::endl;
        std::cerr << inputFile.error() << std::endl;
        exit(1);
    }
    BumpAllocator allocator;
    STLBumpAllocator<uint8_t> wrappedAllocator(allocator);
    InitializeKeywordTable();
    InitializeQuickTokenTable();
    Tokenizer tokenizer(wrappedAllocator, inputFile);
    Parser parser(tokenizer);

    ProgramNode *program = parser.parseProgram();
    if (!program) {
        WH_ASSERT(parser.hasError());
        std::cerr << "Parse error: " << parser.error() << std::endl;
        return 1;
    }

    Printer pr;
    PrintNode(tokenizer.source(), program, pr, 0);

    // Initialize a runtime.
    Runtime runtime;
    if (!runtime.initialize()) {
        WH_ASSERT(runtime.hasError());
        std::cerr << "Runtime error: " << runtime.error() << std::endl;
        return 1;
    }

    // Create a new thread context.
    const char *err = runtime.registerThread();
    if (err) {
        std::cerr << "ThreadContext error: " << err << std::endl;
        return 1;
    }
    ThreadContext *thrcx = runtime.threadContext();

    // Create a run context for execution.
    RunContext cx = thrcx->makeRunContext();
    cx.makeActive();

    return 0;
}
