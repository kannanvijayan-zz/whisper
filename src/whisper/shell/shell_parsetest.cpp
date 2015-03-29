
#include <iostream>

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
#include "vm/source_file.hpp"
#include "vm/packed_syntax_tree.hpp"

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

int main(int argc, char **argv)
{
    if (argc <= 1) {
        std::cerr << "No input file provided!" << std::endl;
        exit(1);
    }

    // Initialize static tables.
    InitializeRuntime();

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
    ThreadContext *cx = runtime.threadContext();
    AllocationContext acx(cx->inTenured());

    // Create a new String containing the file name.
    Local<VM::String *> filename(cx);
    if (!filename.setResult(VM::String::Create(acx, argv[1]))) {
        std::cerr << "Error creating filename string." << std::endl;
        return 1;
    }

    // Create a new SourceFile.
    Local<VM::SourceFile *> sourceFile(cx);
    if (!sourceFile.setResult(VM::SourceFile::Create(acx, filename))) {
        std::cerr << "Error creating source file." << std::endl;
        return 1;
    }

    // Prase a syntax tree from the source file.
    Local<VM::PackedSyntaxTree *> packedSt(cx);
    if (!packedSt.setResult(VM::SourceFile::ParseSyntaxTree(cx, sourceFile))) {
        std::cerr << "Error parsing syntax tree." << std::endl;
        return 1;
    }

    // Print packed raw data.
    Local<VM::Array<uint32_t> *> stData(cx, packedSt->data());
    fprintf(stderr, "Packed Syntax Tree:\n");
    for (uint32_t bufi = 0; bufi < stData->length(); bufi += 4) {
        if (stData->length() - bufi >= 4) {
            fprintf(stderr, "[%04d]  %08x %08x %08x %08x\n", bufi,
                    stData->get(bufi), stData->get(bufi+1),
                    stData->get(bufi+2), stData->get(bufi+3));

        } else if (stData->length() - bufi == 3) {
            fprintf(stderr, "[%04d]  %08x %08x %08x\n", bufi,
                    stData->get(bufi), stData->get(bufi+1),
                    stData->get(bufi+2));

        } else if (stData->length() - bufi == 2) {
            fprintf(stderr, "[%04d]  %08x %08x\n", bufi,
                    stData->get(bufi), stData->get(bufi+1));

        } else {
            fprintf(stderr, "[%04d]  %08x\n", bufi,
                    stData->get(bufi));
        }
    }

    // Print constant pool.
    Local<VM::Array<VM::Box> *> stConstants(cx, packedSt->constants());
    fprintf(stderr, "Constant Pool:\n");
    for (uint32_t i = 0; i < stConstants->length(); i++) {
        VM::Box box = stConstants->get(i);
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
    AST::PackedReader packedReader(stData, stConstants);
    packedReader.visit(&packedVisitor);

    return 0;
}
