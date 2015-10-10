
#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/frame.hpp"

namespace Whisper {
namespace VM {


Result<Frame*>
Frame::resolveChild(ThreadContext* cx,
                    Handle<Frame*> child,
                    ControlFlow const& flow)
{
#define RESOLVE_CHILD_CASE_(name) \
    if (this->is##name()) \
        return this->to##name()->resolve##name##Child(cx, child, flow);

    WHISPER_DEFN_FRAME_KINDS(RESOLVE_CHILD_CASE_)

#undef RESOLVE_CHILD_CASE_

    WH_UNREACHABLE("Unrecognized frame type.");
    return ErrorVal();
}

Result<Frame*>
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

Result<Frame*>
TerminalFrame::resolveTerminalFrameChild(ThreadContext* cx,
                                         Handle<Frame*> child,
                                         ControlFlow const& flow)
{
    // Any resolving of a child returns this frame as-is.
    return OkVal<Frame*>(this);
}

Result<Frame*>
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

Result<Frame*>
EntryFrame::resolveEntryFrameChild(ThreadContext* cx,
                                   Handle<Frame*> child,
                                   ControlFlow const& flow)
{
    // Resolve parent frame with the same controlflow result.
    Local<Frame*> rootedThis(cx, this);
    return parent_->resolveChild(cx, rootedThis, flow);
}

Result<Frame*>
EntryFrame::stepEntryFrame(ThreadContext* cx)
{
    return cx->setInternalError("stepEntryFrame not defined.");
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

Result<Frame*>
EvalFrame::resolveEvalFrameChild(ThreadContext* cx,
                                 Handle<Frame*> child,
                                 ControlFlow const& flow)
{
    // Resolve parent frame with the same controlflow result.
    Local<Frame*> rootedThis(cx, this);
    return parent_->resolveChild(cx, rootedThis, flow);
}

Result<Frame*>
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

Result<Frame*>
SyntaxFrame::resolveSyntaxFrameChild(ThreadContext* cx,
                                     Handle<Frame*> child,
                                     ControlFlow const& flow)
{
    Local<SyntaxFrame*> rootedThis(cx, this);
    return resolveChildFunc_(cx, rootedThis, child, flow);
}

Result<Frame*>
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

Result<Frame*>
FunctionFrame::resolveFunctionFrameChild(ThreadContext* cx,
                                         Handle<Frame*> child,
                                         ControlFlow const& flow)
{
    return cx->setInternalError("resolveFunctionFrameChild not defined.");
}

Result<Frame*>
FunctionFrame::stepFunctionFrame(ThreadContext* cx)
{
    return cx->setInternalError("stepFunctionFrame not defined.");
}


} // namespace VM
} // namespace Whisper
