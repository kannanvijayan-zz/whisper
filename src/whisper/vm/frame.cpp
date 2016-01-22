
#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/frame.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "interp/heap_interpreter.hpp"

namespace Whisper {
namespace VM {


/* static */ OkResult
Frame::ResolveChild(ThreadContext* cx, Handle<Frame*> frame,
                    ControlFlow const& flow)
{
    // Frame being resolved must be parent of current top frame.
    WH_ASSERT(cx->topFrame()->parent() == frame.get());

#define RESOLVE_CHILD_CASE_(name) \
    if (frame->is##name()) \
        return name::ResolveChildImpl(cx, frame.upConvertTo<name*>(), flow);

    WHISPER_DEFN_FRAME_KINDS(RESOLVE_CHILD_CASE_)

#undef RESOLVE_CHILD_CASE_

    WH_UNREACHABLE("Unrecognized frame type.");
    return ErrorVal();
}

/* static */ OkResult
Frame::Step(ThreadContext* cx, Handle<Frame*> frame)
{
    WH_ASSERT(cx->topFrame() == frame.get());

#define RESOLVE_CHILD_CASE_(name) \
    if (frame->is##name()) \
        return name::StepImpl(cx, frame.upConvertTo<name*>());

    WHISPER_DEFN_FRAME_KINDS(RESOLVE_CHILD_CASE_)

#undef RESOLVE_CHILD_CASE_

    WH_UNREACHABLE("Unrecognized frame type.");
    return ErrorVal();
}

EntryFrame*
Frame::maybeAncestorEntryFrame()
{
    Frame* cur = this;
    while (cur && !cur->isEntryFrame())
        cur = cur->parent();
    WH_ASSERT(!cur || cur->isEntryFrame());
    return reinterpret_cast<EntryFrame*>(cur);
}

/* static */ Result<TerminalFrame*>
TerminalFrame::Create(AllocationContext acx)
{
    return acx.create<TerminalFrame>();
}

/* static */ OkResult
TerminalFrame::ResolveChildImpl(ThreadContext* cx,
                                Handle<TerminalFrame*> frame,
                                ControlFlow const& flow)
{
    // Any resolving of a child returns this frame as-is.
    return OkVal();
}

/* static */ OkResult
TerminalFrame::StepImpl(ThreadContext* cx,
                        Handle<TerminalFrame*> frame)
{
    // TerminalFrame should never be stepped!
    WH_UNREACHABLE("TerminalFrame should never be step-executed.");
    return cx->setInternalError("TerminalFrame should never be step-executed.");
}

/* static */ Result<EntryFrame*>
EntryFrame::Create(AllocationContext acx,
                   Handle<Frame*> parent,
                   Handle<SyntaxTreeFragment*> stFrag,
                   Handle<ScopeObject*> scope)
{
    return acx.create<EntryFrame>(parent, stFrag, scope);
}

/* static */ Result<EntryFrame*>
EntryFrame::Create(AllocationContext acx,
                   Handle<SyntaxTreeFragment*> stFrag,
                   Handle<ScopeObject*> scope)
{
    Local<Frame*> parent(acx, acx.threadContext()->topFrame());
    return Create(acx, parent, stFrag, scope);
}

/* static */ OkResult
EntryFrame::ResolveChildImpl(ThreadContext* cx, Handle<EntryFrame*> frame,
                             ControlFlow const& flow)
{
    // Resolve parent frame with the same controlflow result.
    Local<Frame*> rootedParent(cx, frame->parent());
    return Frame::ResolveChild(cx, rootedParent, flow);
}

/* static */ OkResult
EntryFrame::StepImpl(ThreadContext* cx, Handle<EntryFrame*> frame)
{
    // Call into the interpreter to initialize a SyntaxFrame
    // for the root node of this entry frame.
    Local<Frame*> newFrame(cx);
    if (!newFrame.setResult(Interp::CreateInitialSyntaxFrame(cx, frame)))
        return ErrorVal();

    // Update the top frame.
    cx->setTopFrame(newFrame);
    return OkVal();
}


/* static */ Result<SyntaxNameLookupFrame*>
SyntaxNameLookupFrame::Create(AllocationContext acx,
                              Handle<Frame*> parent,
                              Handle<EntryFrame*> entryFrame,
                              Handle<SyntaxTreeFragment*> stFrag)
{
    return acx.create<SyntaxNameLookupFrame>(parent, entryFrame, stFrag);
}

/* static */ Result<SyntaxNameLookupFrame*>
SyntaxNameLookupFrame::Create(AllocationContext acx,
                              Handle<EntryFrame*> entryFrame,
                              Handle<SyntaxTreeFragment*> stFrag)
{
    Local<Frame*> parent(acx, acx.threadContext()->topFrame());
    return Create(acx, parent, entryFrame, stFrag);
}

/* static */ OkResult
SyntaxNameLookupFrame::ResolveChildImpl(
        ThreadContext* cx,
        Handle<SyntaxNameLookupFrame*> frame,
        ControlFlow const& flow)
{
    WH_ASSERT(flow.isError() || flow.isException() || flow.isValue());

    if (flow.isError() || flow.isException())
        return ErrorVal();

    // Create invocation frame for the looked up value.
    Local<ValBox> syntaxHandler(cx, flow.value());
    Local<EntryFrame*> entryFrame(cx, frame->entryFrame());
    Local<Frame*> parentFrame(cx, frame->parent());

    Local<SyntaxTreeFragment*> stFrag(cx, frame->stFrag());
    Local<Frame*> invokeFrame(cx);
    if (!invokeFrame.setResult(Interp::CreateInvokeSyntaxFrame(cx,
            parentFrame, entryFrame, stFrag, syntaxHandler)))
    {
        return ErrorVal();
    }

    cx->setTopFrame(invokeFrame);
    return OkVal();
}

/* static */ OkResult
SyntaxNameLookupFrame::StepImpl(ThreadContext* cx,
                                Handle<SyntaxNameLookupFrame*> frame)
{
    // Get the name of the syntax handler method.
    Local<String*> name(cx,
        cx->runtimeState()->syntaxHandlerName(frame->stFrag()));
    if (name.get() == nullptr) {
        WH_UNREACHABLE("Handler name not found for SyntaxTreeFragment.");
        cx->setInternalError("Handler name not found for SyntaxTreeFragment.");
        return ErrorVal();
    }

    // Look up the property on the scope object.
    Local<ScopeObject*> scope(cx, frame->entryFrame()->scope());
    Interp::PropertyLookupResult lookupResult = Interp::GetObjectProperty(cx,
        scope.handle().convertTo<Wobject*>(), name);

    if (lookupResult.isError()) {
        return ResolveChildImpl(cx, frame, ControlFlow::Error());
    }

    if (lookupResult.isNotFound()) {
        cx->setExceptionRaised("Lookup name not found", name.get());
        return ResolveChildImpl(cx, frame, ControlFlow::Exception());
    }

    if (lookupResult.isFound()) {
        Local<PropertyDescriptor> descriptor(cx, lookupResult.descriptor());
        Local<LookupState*> lookupState(cx, lookupResult.lookupState());

        // Handle a value binding by returning the value.
        if (descriptor->isValue()) {
            return ResolveChildImpl(cx, frame,
                ControlFlow::Value(descriptor->valBox()));
        }

        // Handle a method binding by creating a bound FunctionObject
        // from the method.
        if (descriptor->isMethod()) {
            // Create a new function object bound to the scope.
            Local<ValBox> scopeVal(cx, ValBox::Object(scope.get()));
            Local<Function*> func(cx, descriptor->method());
            Local<FunctionObject*> funcObj(cx);
            if (!funcObj.setResult(FunctionObject::Create(
                    cx->inHatchery(), func, scopeVal, lookupState)))
            {
                return ErrorVal();
            }

            return ResolveChildImpl(cx, frame,
                    ControlFlow::Value(ValBox::Object(funcObj.get()))); 
        }

        WH_UNREACHABLE("PropertyDescriptor not one of Value, Method.");
        return ErrorVal();
    }

    WH_UNREACHABLE("Property lookup not one of Found, NotFound, Error.");
    return ErrorVal();
}


/* static */ Result<InvokeSyntaxFrame*>
InvokeSyntaxFrame::Create(AllocationContext acx,
                          Handle<Frame*> parent,
                          Handle<EntryFrame*> entryFrame,
                          Handle<SyntaxTreeFragment*> stFrag,
                          Handle<ValBox> syntaxHandler)
{
    return acx.create<InvokeSyntaxFrame>(parent, entryFrame, stFrag,
                                         syntaxHandler);
}

/* static */ Result<InvokeSyntaxFrame*>
InvokeSyntaxFrame::Create(AllocationContext acx,
                          Handle<EntryFrame*> entryFrame,
                          Handle<SyntaxTreeFragment*> stFrag,
                          Handle<ValBox> syntaxHandler)
{
    Local<Frame*> parent(acx, acx.threadContext()->topFrame());
    return Create(acx, parent, entryFrame, stFrag, syntaxHandler);
}

/* static */ OkResult
InvokeSyntaxFrame::ResolveChildImpl(
        ThreadContext* cx,
        Handle<InvokeSyntaxFrame*> frame,
        ControlFlow const& flow)
{
    // Resolve parent frame with the same controlflow result.
    Local<Frame*> rootedParent(cx, frame->parent());
    return Frame::ResolveChild(cx, rootedParent, flow);
}

/* static */ OkResult
InvokeSyntaxFrame::StepImpl(ThreadContext* cx,
                            Handle<InvokeSyntaxFrame*> frame)
{
    Local<ScopeObject*> callerScope(cx, frame->entryFrame()->scope());
    Local<ValBox> syntaxHandler(cx, frame->syntaxHandler());
    Local<SyntaxTreeFragment*> stFrag(cx, frame->stFrag());
    return Interp::InvokeValue(cx, callerScope, syntaxHandler, stFrag);
}


/* static */ Result<FileSyntaxFrame*>
FileSyntaxFrame::Create(AllocationContext acx,
                        Handle<Frame*> parent,
                        Handle<EntryFrame*> entryFrame,
                        Handle<SyntaxTreeFragment*> stFrag,
                        uint32_t statementNo)
{
    return acx.create<FileSyntaxFrame>(parent, entryFrame, stFrag,
                                       statementNo);
}

/* static */ Result<FileSyntaxFrame*>
FileSyntaxFrame::Create(AllocationContext acx,
                        Handle<EntryFrame*> entryFrame,
                        Handle<SyntaxTreeFragment*> stFrag,
                        uint32_t statementNo)
{
    Local<Frame*> parent(acx, acx.threadContext()->topFrame());
    return Create(acx, parent, entryFrame, stFrag, statementNo);
}

/* static */ Result<FileSyntaxFrame*>
FileSyntaxFrame::CreateNext(AllocationContext acx,
                        Handle<FileSyntaxFrame*> curFrame)
{
    WH_ASSERT(curFrame->stFrag()->isNode());
    Local<SyntaxNodeRef> fileNode(acx,
        SyntaxNodeRef(curFrame->stFrag()->toNode()));
    WH_ASSERT(fileNode->nodeType() == AST::File);
    WH_ASSERT(curFrame->statementNo() < fileNode->astFile().numStatements());

    Local<Frame*> parent(acx, curFrame->parent());
    Local<EntryFrame*> entryFrame(acx, curFrame->entryFrame());
    Local<SyntaxTreeFragment*> stFrag(acx, curFrame->stFrag());
    uint32_t nextStatementNo = curFrame->statementNo() + 1;

    return Create(acx, parent, entryFrame, stFrag, nextStatementNo);
}

/* static */ OkResult
FileSyntaxFrame::ResolveChildImpl(
        ThreadContext* cx,
        Handle<FileSyntaxFrame*> frame,
        ControlFlow const& flow)
{
    WH_ASSERT(frame->stFrag()->isNode());
    Local<SyntaxNodeRef> fileNode(cx, SyntaxNodeRef(frame->stFrag()->toNode()));
    WH_ASSERT(fileNode->nodeType() == AST::File);
    WH_ASSERT(frame->statementNo() < fileNode->astFile().numStatements());

    Local<Frame*> rootedParent(cx, frame->parent());

    // If control flow is an error, resolve to parent.
    if (flow.isError())
        return Frame::ResolveChild(cx, rootedParent, flow);

    // Otherwise, create new file syntax frame for executing next
    // statement.
    Local<FileSyntaxFrame*> nextFileFrame(cx);
    if (!nextFileFrame.setResult(FileSyntaxFrame::CreateNext(
            cx->inHatchery(), frame)))
    {
        return ErrorVal();
    }
    cx->setTopFrame(nextFileFrame);
    return OkVal();
}

/* static */ OkResult
FileSyntaxFrame::StepImpl(ThreadContext* cx,
                            Handle<FileSyntaxFrame*> frame)
{
    WH_ASSERT(frame->stFrag()->isNode());
    Local<SyntaxNodeRef> fileNode(cx, SyntaxNodeRef(frame->stFrag()->toNode()));
    WH_ASSERT(fileNode->nodeType() == AST::File);
    WH_ASSERT(frame->statementNo() < fileNode->astFile().numStatements());

    Local<Frame*> rootedParent(cx, frame->parent());

    if (frame->statementNo() == fileNode->astFile().numStatements())
        return Frame::ResolveChild(cx, rootedParent, ControlFlow::Void());

    // Get SyntaxTreeFragment for next statement node.
    Local<SyntaxTreeFragment*> stmtNode(cx);
    if (!stmtNode.setResult(SyntaxNode::Create(
            cx->inHatchery(), fileNode->pst(),
            fileNode->astFile().statement(frame->statementNo()).offset())))
    {
        return ErrorVal();
    }

    // Create a new entry frame for the interpretation of the statement.
    Local<ScopeObject*> scope(cx, frame->scope());
    Local<VM::EntryFrame*> entryFrame(cx);
    if (!entryFrame.setResult(VM::EntryFrame::Create(
            cx->inHatchery(), stmtNode, scope)))
    {
        return ErrorVal();
    }
    cx->setTopFrame(entryFrame);
    return OkVal();
}


} // namespace VM
} // namespace Whisper
