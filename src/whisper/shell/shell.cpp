
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <sstream>

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
#include "vm/global_scope.hpp"
#include "vm/source_file.hpp"
#include "vm/packed_syntax_tree.hpp"
#include "vm/function.hpp"
#include "vm/runtime_state.hpp"
#include "vm/control_flow.hpp"
#include "interp/syntax_behaviour.hpp"
// #include "interp/interpreter.hpp"
#include "interp/heap_interpreter.hpp"

#include "shell/shell_tracer.hpp"

using namespace Whisper;


// HeapTracer calculates the rooted object graph.
struct HeapPrintVisitor : public TracerVisitor
{
    StackThing* lastRoot;
    

    HeapPrintVisitor() : lastRoot(nullptr) {}

    virtual void visitStackRoot(StackThing* rootPtr, uint32_t idx) override {
        fprintf(stderr, "stack_%p [label=\"%s\\n@%p\"; shape=box];\n",
                rootPtr, StackFormatString(rootPtr->format()), rootPtr);
        if (lastRoot) {
            fprintf(stderr, "stack_%p -> stack_%p [style=dotted];\n",
                    lastRoot, rootPtr);
        }
        lastRoot = rootPtr;
    }
    virtual void visitStackChild(StackThing* rootPtr, HeapThing* child)
        override
    {
        fprintf(stderr, "stack_%p -> heap_%p;\n", rootPtr, child);
    }

    virtual void visitHeapThing(HeapThing* heapThing) override {
        if (heapThing->format() == HeapFormat::String) {
            std::string stringContents(heapThing->to<VM::String>()->c_chars());
            size_t stringLen = stringContents.length();
            size_t idx = 0;
            while (idx < stringLen) {
                size_t quotePos = stringContents.find('"', idx);
                size_t newlinePos = stringContents.find('\n', idx);
                if (quotePos == std::string::npos)
                    quotePos = stringLen;
                if (newlinePos == std::string::npos)
                    newlinePos = stringLen;
                WH_ASSERT(quotePos <= stringLen);
                WH_ASSERT(newlinePos <= stringLen);
                WH_ASSERT_IF(quotePos == newlinePos, quotePos == stringLen);
                if (quotePos < newlinePos) {
                    stringContents.replace(quotePos, 1, "\\\"");
                    idx = quotePos + 2;
                } else if (newlinePos < quotePos) {
                    stringContents.replace(newlinePos, 1, "\\n");
                    idx = newlinePos + 2;
                } else {
                    idx = stringLen;
                }
            }
            fprintf(stderr, "heap_%p [label=\"%s\\n@%p\\n%s\"];\n",
                    heapThing, HeapFormatString(heapThing->format()),
                    heapThing, heapThing->to<VM::String>()->c_chars());
        } else {
            fprintf(stderr, "heap_%p [label=\"%s\\n@%p\"];\n",
                    heapThing, HeapFormatString(heapThing->format()),
                    heapThing);
        }
    }
    virtual void visitHeapChild(HeapThing* parent, HeapThing* child)
        override
    {
        fprintf(stderr, "heap_%p -> heap_%p;\n", parent, child);
    }
};


static OkResult
InitShellGlobals(AllocationContext acx, VM::GlobalScope* scope);

int
main(int argc, char** argv)
{
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

    ThreadContext* cx = runtime.threadContext();
    AllocationContext acx(cx->inTenured());
    InitShellGlobals(acx, cx->global());

    // Create a new String containing the file name.
    Local<VM::String*> filename(cx);
    if (!filename.setResult(VM::String::Create(acx, argv[1]))) {
        std::cerr << "Error creating filename string." << std::endl;
        return 1;
    }

    // Create a new SourceFile.
    Local<VM::SourceFile*> sourceFile(cx);
    if (!sourceFile.setResult(VM::SourceFile::Create(acx, filename))) {
        std::cerr << "Error creating source file." << std::endl;
        return 1;
    }

    // Parse a syntax tree from the source file.
    Local<VM::PackedSyntaxTree*> packedSt(cx);
    if (!packedSt.setResult(VM::SourceFile::ParseSyntaxTree(cx, sourceFile))) {
        std::cerr << "Error parsing syntax tree." << std::endl;
        return 1;
    }

    // Create a module scope object for the file.
    Local<VM::ModuleScope*> module(cx);
    if (!module.setResult(VM::SourceFile::CreateScope(cx, sourceFile))) {
        std::cerr << "Error creating module scope." << std::endl;
        return 1;
    }

    // Interpret the file.
    VM::ControlFlow res = Interp::HeapInterpretSourceFile(cx, sourceFile,
            module.handle().convertTo<VM::ScopeObject*>());

    Local<VM::ControlFlow> result(cx, res);

    if (result->isError()) {
        std::cerr << "Error interpreting code!" << std::endl;
        WH_ASSERT(cx->hasError());
        char buf[512];
        cx->formatError(buf, 512);
        std::cerr << "ERROR: " << buf << std::endl;
        return 1;
    }

    fprintf(stderr, "digraph G {\n");
    HeapPrintVisitor visitor;
    trace_heap(cx, &visitor);
    fprintf(stderr, "}\n");

    return 0;
}

/*
static VM::ControlFlow Shell_Print(
    ThreadContext* cx,
    Handle<VM::NativeCallInfo> callInfo,
    ArrayHandle<VM::ValBox> args);

static OkResult
BindShellGlobal(AllocationContext acx,
                Handle<VM::GlobalScope*> obj,
                VM::String* name,
                VM::NativeApplicativeFuncPtr appFunc)
{
    Local<VM::String*> rootedName(acx, name);

    // Allocate NativeFunction object.
    Local<VM::NativeFunction*> natF(acx);
    if (!natF.setResult(VM::NativeFunction::Create(acx, appFunc)))
        return ErrorVal();
    Local<VM::PropertyDescriptor> desc(acx, VM::PropertyDescriptor(natF.get()));

    // Bind method on global.
    if (!VM::Wobject::DefineProperty(acx, obj.convertTo<VM::Wobject*>(),
                                     rootedName, desc))
    {
        return ErrorVal();
    }

    return OkVal();
}
*/

static OkResult
InitShellGlobals(AllocationContext acx, VM::GlobalScope* scope)
{
    Local<VM::GlobalScope*> rootedScope(acx, scope);

#define BIND_SHELL_METHOD_(name, proc) \
    do { \
        /* allocate the name string. */ \
        Local<VM::String*> nameString(acx); \
        if (!nameString.setResult(VM::String::Create(acx, name))) \
            return ErrorVal(); \
        \
        if (!BindShellGlobal(acx, rootedScope, nameString, proc)) \
            return ErrorVal(); \
    } while(false)

/*
    BIND_SHELL_METHOD_("print", Shell_Print);
*/

#undef BIND_SHELL_METHOD_

    return OkVal();
}

/*
static VM::ControlFlow Shell_Print(
    ThreadContext* cx,
    Handle<VM::NativeCallInfo> callInfo,
    ArrayHandle<VM::ValBox> args)
{
    std::ostringstream out;

    for (uint32_t i = 0; i < args.length(); i++) {
        if (!args.handle(i)->toString(cx, out))
            return ErrorVal();
    }

    std::cout << out.str() << std::endl;
    return VM::ControlFlow::Value(VM::ValBox::Undefined());
}
*/
