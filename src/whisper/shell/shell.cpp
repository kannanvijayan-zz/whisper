
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
#include "vm/scope_object.hpp"
#include "vm/source_file.hpp"
#include "vm/packed_syntax_tree.hpp"
#include "vm/function.hpp"
#include "vm/runtime_state.hpp"

#include "shell/shell_tracer.hpp"

using namespace Whisper;


// HeapTracer calculates the rooted object graph.
struct HeapPrintVisitor : public TracerVisitor
{
    StackThing *lastRoot;

    HeapPrintVisitor() : lastRoot(nullptr) {}

    virtual void visitStackRoot(StackThing *rootPtr) override {
        fprintf(stderr, "stack_%p [label=\"%s\\n@%p\"; shape=box];\n",
                rootPtr, StackFormatString(rootPtr->format()), rootPtr);
        if (lastRoot) {
            fprintf(stderr, "stack_%p -> stack_%p [style=dotted];\n",
                    lastRoot, rootPtr);
        }
        lastRoot = rootPtr;
    }
    virtual void visitStackChild(StackThing *rootPtr, HeapThing *child)
        override
    {
        fprintf(stderr, "stack_%p -> heap_%p;\n", rootPtr, child);
    }

    virtual void visitHeapThing(HeapThing *heapThing) override {
        fprintf(stderr, "heap_%p [label=\"%s\\n@%p\"];\n",
                heapThing, HeapFormatString(heapThing->format()), heapThing);
    }
    virtual void visitHeapChild(HeapThing *parent, HeapThing *child)
        override
    {
        fprintf(stderr, "heap_%p -> heap_%p;\n", parent, child);
    }
};

static OkResult initialize_thread_globals(ThreadContext *cx);

int main(int argc, char **argv) {
    std::cout << "Whisper says hello." << std::endl;

    // Open input file.
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
    if (!runtime.registerThread()) {
        std::cerr << "ThreadContext error: " << runtime.error() << std::endl;
        return 1;
    }

    ThreadContext *cx = runtime.threadContext();
    if (!initialize_thread_globals(cx)) {
        std::cerr << "ThreadContext error: " << cx->errorString() << std::endl;
        return 1;
    }
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

    // Parse a syntax tree from the source file.
    Local<VM::PackedSyntaxTree *> packedSt(cx);
    if (!packedSt.setResult(VM::SourceFile::ParseSyntaxTree(cx, sourceFile))) {
        std::cerr << "Error parsing syntax tree." << std::endl;
        return 1;
    }

    // Create a module scope object for the file.
    Local<VM::ModuleScope *> module(cx);
    if (!module.setResult(VM::SourceFile::CreateScope(cx, sourceFile))) {
        std::cerr << "Error creating module scope." << std::endl;
        return 1;
    }

    // Look up '@integer' on the module.
    Local<VM::LookupState *> lookupState(cx);
    Local<VM::PropertyDescriptor> propDesc(cx);
    Local<VM::String *> atIntN(cx, cx->runtimeState()->nm_AtInteger());
    if (!VM::Wobject::LookupProperty(cx,
            module.handle().convertTo<VM::Wobject *>(),
            atIntN, &lookupState, &propDesc))
    {
        std::cerr << "Error looking up @integer on module." << std::endl;
        return 1;
    }

    fprintf(stderr, "digraph G {\n");
    HeapPrintVisitor visitor;
    trace_heap(cx, &visitor);
    fprintf(stderr, "}\n");

    return 0;
}


static OkResult Lift_Integer(
    ThreadContext *cx,
    Handle<VM::LookupState *> lookupState,
    Handle<VM::ScopeObject *> callerScope,
    Handle<VM::NativeFunction *> nativeFunc,
    Handle<VM::Wobject *> receiver,
    ArrayHandle<VM::SyntaxTreeFragment *> stFrag,
    MutHandle<VM::Box> resultOut);

static OkResult
initialize_thread_globals(ThreadContext *cx)
{
    AllocationContext acx = cx->inTenured();

    // Allocate '@integer' operative native function.
    Local<VM::NativeFunction *> atIntF(cx);
    if (!atIntF.setResult(VM::NativeFunction::Create(acx, &Lift_Integer)))
        return ErrorVal();
    Local<VM::String *> atIntN(cx, cx->runtimeState()->nm_AtInteger());
    Local<VM::PropertyDescriptor> propDesc(cx,
        VM::PropertyDescriptor(atIntF.get()));

    // Bind method on global.
    Local<VM::GlobalScope *> global(cx, cx->global());
    if (!VM::GlobalScope::DefineProperty(cx, global, atIntN, propDesc))
        return ErrorVal();

    return Ok();
}


static OkResult Lift_Integer(
    ThreadContext *cx,
    Handle<VM::LookupState *> lookupState,
    Handle<VM::ScopeObject *> callerScope,
    Handle<VM::NativeFunction *> nativeFunc,
    Handle<VM::Wobject *> receiver,
    ArrayHandle<VM::SyntaxTreeFragment *> stFrag,
    MutHandle<VM::Box> resultOut)
{
    return ErrorVal();
}
