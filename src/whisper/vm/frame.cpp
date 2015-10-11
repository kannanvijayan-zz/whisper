
#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/frame.hpp"
#include "interp/heap_interpreter.hpp"

namespace Whisper {
namespace VM {


OkResult
Frame::resolveChild(ThreadContext* cx,
                    ControlFlow const& flow)
{
#define RESOLVE_CHILD_CASE_(name) \
    if (this->is##name()) \
        return this->to##name()->resolve##name##Child(cx, flow);

    WHISPER_DEFN_FRAME_KINDS(RESOLVE_CHILD_CASE_)

#undef RESOLVE_CHILD_CASE_

    WH_UNREACHABLE("Unrecognized frame type.");
    return ErrorVal();
}

OkResult
Frame::step(ThreadContext* cx)
{
#define RESOLVE_CHILD_CASE_(name) \
    if (this->is##name()) \
        return this->to##name()->step##name(cx);

    WHISPER_DEFN_FRAME_KINDS(RESOLVE_CHILD_CASE_)

#undef RESOLVE_CHILD_CASE_

    WH_UNREACHABLE("Unrecognized frame type.");
    return ErrorVal();
}

/* static */ Result<TerminalFrame*>
TerminalFrame::Create(AllocationContext acx)
{
    return acx.create<TerminalFrame>();
}

OkResult
TerminalFrame::resolveTerminalFrameChild(ThreadContext* cx,
                                         ControlFlow const& flow)
{
    // Any resolving of a child returns this frame as-is.
    return OkVal();
}

OkResult
TerminalFrame::stepTerminalFrame(ThreadContext* cx)
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

OkResult
EntryFrame::resolveEntryFrameChild(ThreadContext* cx,
                                   ControlFlow const& flow)
{
    // Resolve parent frame with the same controlflow result.
    return parent_->resolveChild(cx, flow);
}

OkResult
EntryFrame::stepEntryFrame(ThreadContext* cx)
{
    // Call into the interpreter to initialize a SyntaxFrame
    // for the root node of this entry frame.
    Local<EntryFrame*> rootedThis(cx, this);
    Local<SyntaxFrame*> newFrame(cx);
    if (!newFrame.setResult(Interp::CreateInitialSyntaxFrame(cx, rootedThis)))
        return ErrorVal();

    // Update the top frame.
    cx->setTopFrame(newFrame);
    return OkVal();
}


/* static */ Result<EvalFrame*>
EvalFrame::Create(AllocationContext acx,
                  Handle<Frame*> parent,
                  Handle<SyntaxTreeFragment*> syntax)
{
    return acx.create<EvalFrame>(parent, syntax);
}

/* static */ Result<EvalFrame*>
EvalFrame::Create(AllocationContext acx, Handle<SyntaxTreeFragment*> syntax)
{
    Local<Frame*> parent(acx, acx.threadContext()->topFrame());
    return Create(acx, parent, syntax);
}

OkResult
EvalFrame::resolveEvalFrameChild(ThreadContext* cx,
                                 ControlFlow const& flow)
{
    // Resolve parent frame with the same controlflow result.
    return parent_->resolveChild(cx, flow);
}

OkResult
EvalFrame::stepEvalFrame(ThreadContext* cx)
{
    return cx->setInternalError("stepEvalFrame not defined.");
}


/* static */ Result<SyntaxFrame*>
SyntaxFrame::Create(AllocationContext acx,
                    Handle<Frame*> parent,
                    Handle<EntryFrame*> entryFrame,
                    Handle<SyntaxTreeFragment*> stFrag,
                    ResolveChildFunc resolveChildFunc,
                    StepFunc stepFunc)
{
    return acx.create<SyntaxFrame>(parent, entryFrame, stFrag,
                                   resolveChildFunc, stepFunc);
}

/* static */ Result<SyntaxFrame*>
SyntaxFrame::Create(AllocationContext acx,
                    Handle<EntryFrame*> entryFrame,
                    Handle<SyntaxTreeFragment*> stFrag,
                    ResolveChildFunc resolveChildFunc,
                    StepFunc stepFunc)
{
    Local<Frame*> parent(acx, acx.threadContext()->topFrame());
    return Create(acx, parent, entryFrame, stFrag,
                  resolveChildFunc, stepFunc);
}

OkResult
SyntaxFrame::resolveSyntaxFrameChild(ThreadContext* cx,
                                     ControlFlow const& flow)
{
    Local<SyntaxFrame*> rootedThis(cx, this);
    return resolveChildFunc_(cx, rootedThis, flow);
}

OkResult
SyntaxFrame::stepSyntaxFrame(ThreadContext* cx)
{
    Local<SyntaxFrame*> rootedThis(cx, this);
    return stepFunc_(cx, rootedThis);
}


/* static */ Result<FunctionFrame*>
FunctionFrame::Create(AllocationContext acx,
                      Handle<Frame*> parent,
                      Handle<Function*> function)
{
    return acx.create<FunctionFrame>(parent, function);
}

/* static */ Result<FunctionFrame*>
FunctionFrame::Create(AllocationContext acx, Handle<Function*> function)
{
    Local<Frame*> parent(acx, acx.threadContext()->topFrame());
    return Create(acx, parent, function);
}

OkResult
FunctionFrame::resolveFunctionFrameChild(ThreadContext* cx,
                                         ControlFlow const& flow)
{
    return cx->setInternalError("resolveFunctionFrameChild not defined.");
}

OkResult
FunctionFrame::stepFunctionFrame(ThreadContext* cx)
{
    return cx->setInternalError("stepFunctionFrame not defined.");
}


} // namespace VM
} // namespace Whisper
