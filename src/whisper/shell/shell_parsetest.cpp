
#include <iostream>
#include <getopt.h>

#include "common.hpp"
#include "allocators.hpp"
#include "spew.hpp"
#include "parser/code_source.hpp"
#include "parser/tokenizer.hpp"
#include "parser/syntax_tree_inlines.hpp"
#include "parser/parser.hpp"
#include "parser/packed_syntax.hpp"
#include "parser/packed_writer.hpp"
#include "parser/packed_reader.hpp"

using namespace Whisper;

struct Printer
{
    void operator ()(const char *s) {
        std::cerr << s;
    }
    void operator ()(const uint8_t *s, uint32_t len) {
        for (size_t i = 0; i < len; i++)
            std::cerr << static_cast<char>(s[i]);
    }
};

static void
print_tokens(CodeSource &code, Tokenizer &tokenizer)
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

        if (tok.isError()) {
            WH_ASSERT(tokenizer.hasError());
            std::cerr << "Token error: " << tokenizer.error() << std::endl;
            break;
        }

        if (tok.isEnd())
            break;
    }
}

enum ParseOpt
{
    Opt_Tokenize = 1,
    Opt_Ast = 2,
    Opt_PackedAst = 3
};

int parse_opts(int argc, char **argv, ParseOpt *parseOptOut)
{
    bool found_opt = false;
    for (;;) {
        int result = getopt(argc, argv, "tap");
        if (result == 't') {
            *parseOptOut = Opt_Tokenize;
            found_opt = true;
            continue;
        }

        if (result == 'a') {
            *parseOptOut = Opt_Ast;
            found_opt = true;
            continue;
        }

        if (result == 'p') {
            *parseOptOut = Opt_PackedAst;
            found_opt = true;
            continue;
        }

        WH_ASSERT(result == -1);
        break;
    }

    if (!found_opt)
        *parseOptOut = Opt_PackedAst;

    return optind;
}

int main(int argc, char **argv)
{
    ParseOpt parseOpt;
    int argIndex = parse_opts(argc, argv, &parseOpt);
    WH_ASSERT(argIndex <= argc);
    if (argIndex >= argc) {
        std::cerr << "No input file provided!" << std::endl;
        exit(1);
    }

    // Initialize static tables.
    InitializeSpew();
    InitializeTokenizer();

    // Open input file.
    FileCodeSource inputFile(argv[optind]);
    if (inputFile.hasError()) {
        std::cerr << "Could not open input file " << argv[optind]
                  << " for reading." << std::endl;
        std::cerr << inputFile.error() << std::endl;
        exit(1);
    }

    Tokenizer tokenizer(inputFile);

    if (parseOpt == Opt_Tokenize) {
        // Handle tokenizer.
        print_tokens(inputFile, tokenizer);
        return 0;
    }

    BumpAllocator allocator;
    STLBumpAllocator<uint8_t> wrappedAllocator(allocator);
    Parser parser(wrappedAllocator, tokenizer);

    FileNode *fileNode = parser.parseFile();
    if (!fileNode) {
        WH_ASSERT(parser.hasError());
        std::cerr << "Parse error: " << parser.error() << std::endl;
        return 1;
    }

    if (parseOpt == Opt_Ast) {
        Printer pr;
        PrintNode(tokenizer.sourceReader(), fileNode, pr, 0);
        return 0;
    }

    WH_ASSERT(parseOpt == Opt_PackedAst);

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

    // Print packed raw data.
    ArrayHandle<uint32_t> buffer = packedWriter->buffer();
    fprintf(stderr, "Packed Syntax Tree:\n");
    for (uint32_t bufi = 0; bufi < buffer.length(); bufi += 4) {
        if (buffer.length() - bufi >= 4) {
            fprintf(stderr, "[%04d]  %08x %08x %08x %08x\n", bufi,
                    buffer[bufi], buffer[bufi+1],
                    buffer[bufi+2], buffer[bufi+3]);
        } else if (buffer.length() - bufi == 3) {
            fprintf(stderr, "[%04d]  %08x %08x %08x\n", bufi,
                    buffer[bufi], buffer[bufi+1], buffer[bufi+2]);
        } else if (buffer.length() - bufi == 2) {
            fprintf(stderr, "[%04d]  %08x %08x\n", bufi,
                    buffer[bufi], buffer[bufi+1]);
        } else {
            fprintf(stderr, "[%04d]  %08x\n", bufi,
                    buffer[bufi]);
        }
    }

    // Print constant pool.
    ArrayHandle<VM::Box> constPool = packedWriter->constPool();
    fprintf(stderr, "Constant Pool:\n");
    for (uint32_t i = 0; i < constPool.length(); i++) {
        VM::Box box = constPool[i];
        char buf[50];
        box.snprint(buf, 50);
        fprintf(stderr, "[%04d]  %p\n", i, buf);
        if (box.isPointer()) {
            HeapThing *thing = box.pointer<HeapThing>();
            fprintf(stderr, "    Ptr to %s (size=%d)\n",
                    thing->header().formatString(),
                    thing->header().size());
        }
    }

    // Print packed syntax tree.
    Printer pr2;
    AST::PrintingPackedVisitor<Printer> packedVisitor(pr2);

    fprintf(stderr, "Visited syntax tree:\n");
    AST::PackedReader packedReader(buffer, constPool);
    packedReader.visit(&packedVisitor);

    return 0;
}
