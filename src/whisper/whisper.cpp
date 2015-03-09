
#include <iostream>
#include "common.hpp"
#include "allocators.hpp"
#include "fnv_hash.hpp"
#include "spew.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"
#include "gc.hpp"
#include "parser/code_source.hpp"
#include "parser/tokenizer.hpp"
#include "parser/syntax_tree_inlines.hpp"
#include "parser/parser.hpp"
#include "parser/packed_syntax.hpp"
#include "parser/packed_writer.hpp"
#include "parser/packed_reader.hpp"
#include "vm/array.hpp"
#include "vm/vector.hpp"
#include "vm/string.hpp"
#include "vm/box.hpp"
#include "vm/source_file.hpp"
#include "vm/shype.hpp"

using namespace Whisper;

void PrintTokens(CodeSource &code, Tokenizer &tokenizer)
{
    // Read and print tokens.
    for (;;) {
        Token tok = tokenizer.readToken();
        char buf[20];
        if (tok.isLineTerminatorSequence() || tok.isWhitespace() ||
            tok.isEnd())
        {
            std::cerr << "Token " << tok.typeString() << std::endl;

        }
        else
        {
            int len = tok.length() < 19 ? tok.length()+1 : 20;
            snprintf(buf, len, "%s", tok.text(code));
            if (tok.length() >= 19) {
                buf[16] = '.';
                buf[17] = '.';
                buf[18] = '.';
            }

            std::cerr << "Token " << tok.typeString() << ":" << buf
                      << std::endl;
        }

        if (tok.isEnd())
            break;
    }
}

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

    // Initialize static tables.
    InitializeSpew();
    InitializeTokenizer();

    // FIXME: re-enable
    //  Interp::InitializeOpcodeInfo();

    // Open input file.
    if (argc <= 1) {
        std::cerr << "No input file provided!" << std::endl;
        exit(1);
    }

    FileCodeSource inputFile(argv[1]);
    if (inputFile.hasError()) {
        std::cerr << "Could not open input file " << argv[1]
                  << " for reading." << std::endl;
        std::cerr << inputFile.error() << std::endl;
        exit(1);
    }
    Tokenizer tokenizer(inputFile);

    // PrintTokens(inputFile, tokenizer);

    BumpAllocator allocator;
    STLBumpAllocator<uint8_t> wrappedAllocator(allocator);
    Parser parser(wrappedAllocator, tokenizer);

    FileNode *fileNode = parser.parseFile();
    if (!fileNode) {
        WH_ASSERT(parser.hasError());
        std::cerr << "Parse error: " << parser.error() << std::endl;
        return 1;
    }

    Printer pr;
    PrintNode(tokenizer.sourceReader(), fileNode, pr, 0);

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
    RunContext runcx(thrcx);
    RunActivationHelper _rah(runcx);

    RunContext *cx = &runcx;
    AllocationContext acx(cx->inTenured());

    // Write out the syntax tree in packed format.
    Local<AST::PackedWriter> packedWriter(cx,
        PackedWriter(
            STLBumpAllocator<uint32_t>(wrappedAllocator),
            tokenizer.sourceReader(),
            acx));
    packedWriter->writeNode(fileNode);

    const uint32_t *buffer = packedWriter->buffer();
    uint32_t bufferSize = packedWriter->bufferSize();
    fprintf(stderr, "Packed Syntax Tree:\n");
    for (uint32_t bufi = 0; bufi < bufferSize; bufi += 4) {
        if (bufferSize - bufi >= 4) {
            fprintf(stderr, "[%04d]  %08x %08x %08x %08x\n", bufi,
                    buffer[bufi], buffer[bufi+1],
                    buffer[bufi+2], buffer[bufi+3]);
        } else if (bufferSize - bufi == 3) {
            fprintf(stderr, "[%04d]  %08x %08x %08x\n", bufi,
                    buffer[bufi], buffer[bufi+1], buffer[bufi+2]);
        } else if (bufferSize - bufi == 2) {
            fprintf(stderr, "[%04d]  %08x %08x\n", bufi,
                    buffer[bufi], buffer[bufi+1]);
        } else {
            fprintf(stderr, "[%04d]  %08x\n", bufi,
                    buffer[bufi]);
        }
    }

    GC::AllocThing **constPool = packedWriter->constPool();
    uint32_t constPoolSize = packedWriter->constPoolSize();
    fprintf(stderr, "Constant Pool:\n");
    for (uint32_t i = 0; i < constPoolSize; i++) {
        fprintf(stderr, "[%04d]  %p\n", i, constPool[i]);
        fprintf(stderr, "    %s (size=%d)\n",
                constPool[i]->header().formatString(),
                constPool[i]->header().size());
    }

    Printer pr2;
    AST::PrintingPackedVisitor<Printer> packedVisitor(pr2);

    fprintf(stderr, "Visited syntax tree:\n");
    AST::PackedReader packedReader(buffer, bufferSize,
                                   constPool, constPoolSize);
    packedReader.visit(&packedVisitor);

    // Create a PackedSyntaxTree object for the new syntax tree.
    uint32_t arraySize =
        VM::Array<uint32_t>::CalculateSize(packedWriter->bufferSize());
    Local<VM::Array<uint32_t> *> packedStData(cx,
        acx.createSized<VM::Array<uint32_t>>(arraySize,
                                             packedWriter->bufferSize(),
                                             packedWriter->buffer()));

    return 0;
}
