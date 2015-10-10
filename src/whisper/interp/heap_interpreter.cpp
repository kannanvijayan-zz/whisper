
#include "parser/packed_syntax.hpp"
#include "vm/runtime_state.hpp"
#include "interp/heap_interpreter.hpp"

namespace Whisper {
namespace Interp {


VM::ControlFlow
HeapInterpretSourceFile(ThreadContext* cx,
                        Handle<VM::SourceFile*> file,
                        Handle<VM::ScopeObject*> scope)
{
    WH_ASSERT(file.get() != nullptr);
    WH_ASSERT(scope.get() != nullptr);

    // Parse the source file.
    Local<VM::PackedSyntaxTree*> st(cx);
    if (!st.setResult(VM::SourceFile::ParseSyntaxTree(cx, file)))
        return ErrorVal();

    // Get SyntaxTreeFragment for the parsed File node.
    Local<VM::SyntaxTreeFragment*> fileNode(cx);
    if (!fileNode.setResult(VM::SyntaxNode::Create(
            cx->inHatchery(), st, st->startOffset())))
    {
        return ErrorVal();
    }

    // Create a new entry frame for the interpretation.
    Local<VM::EntryFrame*> entryFrame(cx);
    if (!entryFrame.setResult(VM::EntryFrame::Create(
            cx->inHatchery(), fileNode, scope)))
    {
        return ErrorVal();
    }
    cx->setTopFrame(entryFrame);

    // Run interpreter loop.
    return HeapInterpretLoop(cx);
}

VM::ControlFlow
HeapInterpretLoop(ThreadContext* cx)
{
    while (!cx->atTerminalFrame()) {
        Result<VM::Frame*> nextFrame = cx->topFrame()->step(cx);
        if (!nextFrame) {
            // Fatal error, immediate halt of computation
            // with frame-stack intact.
            return VM::ControlFlow::Error();
        }

        cx->setTopFrame(nextFrame.value());
    }
    return cx->bottomFrame()->flow();
}

Result<VM::Frame*>
CreateInitialSyntaxFrame(ThreadContext* cx,
                         Handle<VM::EntryFrame*> entryFrame)
{
    return cx->setInternalError("CreateInitialSyntaxFrame not implemented.");
}


} // namespace Interp
} // namespace Whisper
