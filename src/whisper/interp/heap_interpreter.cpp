
#include "parser/syntax_tree.hpp"
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
        OkResult result = cx->topFrame()->step(cx);
        if (!result) {
            // Fatal error, immediate halt of computation
            // with frame-stack intact.
            WH_ASSERT(cx->hasError());
            return VM::ControlFlow::Error();
        }
    }
    return cx->bottomFrame()->flow();
}


Result<VM::Frame*>
CreateInitialSyntaxFrame(ThreadContext* cx,
                         Handle<VM::EntryFrame*> entryFrame)
{
    Local<VM::SyntaxTreeFragment*> stFrag(cx, entryFrame->stFrag());

    // Get the name of the syntax handler method.
    Local<VM::String*> name(cx, cx->runtimeState()->syntaxHandlerName(stFrag));
    if (name.get() == nullptr) {
        WH_UNREACHABLE("Handler name not found for SyntaxTreeFragment.");
        cx->setInternalError("Handler name not found for SyntaxTreeFragment.");
        return ErrorVal();
    }

    // The lookup for the syntax handler method may invoke computation,
    // and thus i

    return ErrorVal();
}


} // namespace Interp
} // namespace Whisper
