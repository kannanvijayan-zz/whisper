
#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/frame.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
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
    Local<Frame*> newFrame(cx);
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

OkResult
SyntaxNameLookupFrame::resolveSyntaxNameLookupFrameChild(
        ThreadContext* cx,
        ControlFlow const& flow)
{
    WH_ASSERT(flow.isError() || flow.isException() || flow.isValue());

    if (flow.isError() || flow.isException())
        return ErrorVal();

    // Create invocation frame for the looked up value.
    Local<ValBox> syntaxHandler(cx, flow.value());
    Local<EntryFrame*> entryFrame(cx, this->entryFrame());
    Local<Frame*> parentFrame(cx, parent());

    Local<SyntaxTreeFragment*> arg(cx, stFrag());
    Local<Frame*> invokeFrame(cx);
    if (!invokeFrame.setResult(Interp::CreateInvokeSyntaxFrame(cx,
            entryFrame, parentFrame, syntaxHandler, arg)))
    {
        return ErrorVal();
    }

    cx->setTopFrame(invokeFrame);
    return OkVal();
}

OkResult
SyntaxNameLookupFrame::stepSyntaxNameLookupFrame(ThreadContext* cx)
{
    // Get the name of the syntax handler method.
    Local<String*> name(cx, cx->runtimeState()->syntaxHandlerName(stFrag()));
    if (name.get() == nullptr) {
        WH_UNREACHABLE("Handler name not found for SyntaxTreeFragment.");
        cx->setInternalError("Handler name not found for SyntaxTreeFragment.");
        return ErrorVal();
    }

    // Look up the property on the scope object.
    Local<ScopeObject*> scope(cx, entryFrame()->scope());
    Interp::PropertyLookupResult lookupResult = Interp::GetObjectProperty(cx,
        scope.handle().convertTo<Wobject*>(), name);

    if (lookupResult.isError())
        return resolveSyntaxNameLookupFrameChild(cx, ControlFlow::Error());

    if (lookupResult.isNotFound()) {
        cx->setExceptionRaised("Lookup name not found", name.get());
        return resolveSyntaxNameLookupFrameChild(cx, ControlFlow::Exception());
    }

    if (lookupResult.isFound()) {
        Local<PropertyDescriptor> descriptor(cx, lookupResult.descriptor());
        Local<LookupState*> lookupState(cx, lookupResult.lookupState());

        // Handle a value binding by returning the value.
        if (descriptor->isValue())
            return resolveSyntaxNameLookupFrameChild(cx,
                        ControlFlow::Value(descriptor->valBox()));

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

            return resolveSyntaxNameLookupFrameChild(cx,
                    ControlFlow::Value(ValBox::Object(funcObj.get())));
        }

        WH_UNREACHABLE("PropertyDescriptor not one of Value, Method.");
        return ErrorVal();
    }

    WH_UNREACHABLE("Property lookup not one of Found, NotFound, Error.");
    return ErrorVal();
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
