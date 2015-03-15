
#include <iostream>
#include <vector>
#include <map>
#include <set>

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
#include "vm/string.hpp"
#include "vm/box.hpp"
#include "vm/source_file.hpp"
#include "vm/packed_syntax_tree.hpp"

#include "shell/shell_tracer.hpp"

using namespace Whisper;


// HeapTracer calculates the rooted object graph.
struct HeapPrintVisitor : public TracerVisitor
{
    virtual void visitStackRoot(StackThing *rootPtr) override {
        fprintf(stderr, "  Stack root %p(%s)\n",
                rootPtr, StackFormatString(rootPtr->format()));
    }
    virtual void visitStackChild(StackThing *rootPtr, HeapThing *child)
        override
    {
        fprintf(stderr, "    child %p(%s) --> %p(%s)\n",
                rootPtr, StackFormatString(rootPtr->format()),
                child, HeapFormatString(child->format()));
    }
    virtual void visitHeapThing(HeapThing *heapThing) override {
        fprintf(stderr, "  Heap thing %p(%s)\n",
                heapThing, HeapFormatString(heapThing->format()));
    }
    virtual void visitHeapChild(HeapThing *parent, HeapThing *child)
        override
    {
        fprintf(stderr, "    child %p(%s) --> %p(%s)\n",
                parent, HeapFormatString(parent->format()),
                child, HeapFormatString(child->format()));
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

    BumpAllocator allocator;
    STLBumpAllocator<uint8_t> wrappedAllocator(allocator);
    Parser parser(wrappedAllocator, tokenizer);

    FileNode *fileNode = parser.parseFile();
    if (!fileNode) {
        WH_ASSERT(parser.hasError());
        std::cerr << "Parse error: " << parser.error() << std::endl;
        return 1;
    }

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

    ArrayHandle<uint32_t> buffer = packedWriter->buffer();
    ArrayHandle<VM::Box> constPool = packedWriter->constPool();

    Local<VM::PackedSyntaxTree *> packedSt(cx,
        VM::PackedSyntaxTree::Create(acx, buffer, constPool));
    fprintf(stderr, "packedSt local @%p\n", packedSt.stackThing());

    fprintf(stderr, "STACK SCAN!\n");
    HeapPrintVisitor visitor;
    trace_heap(thrcx, &visitor);

    return 0;
}
