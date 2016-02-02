
#include <limits>

#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/frame.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "interp/heap_interpreter.hpp"

namespace Whisper {
namespace VM {


/* static */ StepResult
Frame::ResolveChild(ThreadContext* cx,
                    Handle<Frame*> frame,
                    Handle<Frame*> childFrame,
                    Handle<EvalResult> result)
{
    WH_ASSERT(childFrame->parent() == frame);

#define RESOLVE_CHILD_CASE_(name) \
    if (frame->is##name()) \
        return name::ResolveChildImpl(cx, frame.upConvertTo<name*>(), \
                                      childFrame, result);

    WHISPER_DEFN_FRAME_KINDS(RESOLVE_CHILD_CASE_)

#undef RESOLVE_CHILD_CASE_

    WH_UNREACHABLE("Unrecognized frame type.");
    return ErrorVal();
}

/* static */ StepResult
Frame::Step(ThreadContext* cx, Handle<Frame*> frame)
{
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

/* static */ StepResult
TerminalFrame::ResolveChildImpl(ThreadContext* cx,
                                Handle<TerminalFrame*> frame,
                                Handle<Frame*> childFrame,
                                Handle<EvalResult> result)
{
    // Any resolving of a child of this frame just continues with
    // the terminal frame.
    frame->result_.set(result, frame.get());
    return StepResult::Continue(frame);
}

/* static */ StepResult
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

/* static */ StepResult
EntryFrame::ResolveChildImpl(ThreadContext* cx,
                             Handle<EntryFrame*> frame,
                             Handle<Frame*> childFrame,
                             Handle<EvalResult> result)
{
    WH_ASSERT(childFrame->isSyntaxNameLookupFrame() ||
              childFrame->isInvokeSyntaxFrame());

    // If a SyntaxNameLookup operation resolved,
    // forward its result to a InvokeSyntax operation.
    if (childFrame->isSyntaxNameLookupFrame()) {
        Local<Frame*> parent(cx, frame->parent());

        if (result->isError() || result->isException())
            return Frame::ResolveChild(cx, parent, frame, result);

        if (result->isVoid()) {
            SpewInterpNote("EntryFrame::ResolveChildImpl -"
                           " SyntaxNameLookup resolved with notFound -"
                           " raising exception.");
            Local<String*> name(cx,
                cx->runtimeState()->syntaxHandlerName(frame->stFrag()));
            cx->setExceptionRaised("Syntax method binding not found.",
                                   name.get());
            return Frame::ResolveChild(cx, parent, frame,
                            EvalResult::Exception(childFrame));
        }

        // Create invocation frame for the looked up value.
        Local<SyntaxTreeFragment*> stFrag(cx, frame->stFrag());
        Local<Frame*> invokeFrame(cx);
        if (!invokeFrame.setResult(Interp::CreateInvokeSyntaxFrame(cx,
                frame, frame, stFrag, result->value())))
        {
            return ErrorVal();
        }

        return StepResult::Continue(invokeFrame);
    }

    WH_ASSERT(childFrame->isInvokeSyntaxFrame());
    // Resolve parent frame with the same result.
    Local<Frame*> rootedParent(cx, frame->parent());
    return Frame::ResolveChild(cx, rootedParent, frame, result);
}

/* static */ StepResult
EntryFrame::StepImpl(ThreadContext* cx, Handle<EntryFrame*> frame)
{
    // Call into the interpreter to initialize a SyntaxFrame
    // for the root node of this entry frame.
    Local<Frame*> newFrame(cx);
    if (!newFrame.setResult(Interp::CreateInitialSyntaxFrame(cx, frame, frame)))
        return ErrorVal();

    // Update the top frame.
    return StepResult::Continue(newFrame);
}


/* static */ Result<SyntaxNameLookupFrame*>
SyntaxNameLookupFrame::Create(AllocationContext acx,
                              Handle<Frame*> parent,
                              Handle<EntryFrame*> entryFrame,
                              Handle<SyntaxTreeFragment*> stFrag)
{
    return acx.create<SyntaxNameLookupFrame>(parent, entryFrame, stFrag);
}

/* static */ StepResult
SyntaxNameLookupFrame::ResolveChildImpl(
        ThreadContext* cx,
        Handle<SyntaxNameLookupFrame*> frame,
        Handle<Frame*> childFrame,
        Handle<EvalResult> result)
{
    WH_UNREACHABLE("SyntaxNameLookupFrame should never be resolved!");
    return ErrorVal();
}

/* static */ StepResult
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

    Local<Frame*> parent(cx, frame->parent());

    if (lookupResult.isError()) {
        SpewInterpNote("SyntaxNameLookupFrame::StepImpl -"
                       " lookupResult returned error!");
        return ResolveChild(cx, parent, frame, ErrorVal());
    }

    if (lookupResult.isNotFound()) {
        SpewInterpNote("SyntaxNameLookupFrame::StepImpl -"
                       " lookupResult returned notFound - returning void!");
        return ResolveChild(cx, parent, frame, EvalResult::Void());
    }

    if (lookupResult.isFound()) {
        SpewInterpNote("SyntaxNameLookupFrame::StepImpl -"
                       " lookupResult returned found");
        Local<PropertyDescriptor> descriptor(cx, lookupResult.descriptor());
        Local<LookupState*> lookupState(cx, lookupResult.lookupState());

        // Handle a value binding by returning the value.
        if (descriptor->isValue()) {
            return ResolveChild(cx, parent, frame,
                EvalResult::Value(descriptor->valBox()));
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

            return ResolveChild(cx, parent, frame,
                    EvalResult::Value(ValBox::Object(funcObj.get())));
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

/* static */ StepResult
InvokeSyntaxFrame::ResolveChildImpl(
        ThreadContext* cx,
        Handle<InvokeSyntaxFrame*> frame,
        Handle<Frame*> childFrame,
        Handle<EvalResult> result)
{
    // Resolve parent frame with the same result.
    Local<Frame*> rootedParent(cx, frame->parent());
    return Frame::ResolveChild(cx, rootedParent, frame, result);
}

/* static */ StepResult
InvokeSyntaxFrame::StepImpl(ThreadContext* cx,
                            Handle<InvokeSyntaxFrame*> frame)
{
    Local<ScopeObject*> callerScope(cx, frame->entryFrame()->scope());
    Local<ValBox> syntaxHandler(cx, frame->syntaxHandler());
    Local<SyntaxTreeFragment*> stFrag(cx, frame->stFrag());
    Local<CallResult> result(cx, Interp::InvokeValue(cx, frame, callerScope,
                                            syntaxHandler, stFrag));

    if (result->isError())
        return ErrorVal();

    Local<Frame*> parent(cx, frame->parent());
    if (result->isException()) {
        return Frame::ResolveChild(cx, parent, frame,
                        EvalResult::Exception(result->throwingFrame()));
    }

    if (result->isValue()) {
        return Frame::ResolveChild(cx, parent, frame,
                            EvalResult::Value(result->value()));
    }

    if (result->isVoid())
        return Frame::ResolveChild(cx, parent, frame, EvalResult::Void());

    if (result->isContinue())
        return StepResult::Continue(result->continueFrame());

    WH_UNREACHABLE("Unknown CallResult.");
    return ErrorVal();
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

/* static */ StepResult
FileSyntaxFrame::ResolveChildImpl(
        ThreadContext* cx,
        Handle<FileSyntaxFrame*> frame,
        Handle<Frame*> childFrame,
        Handle<EvalResult> result)
{
    WH_ASSERT(frame->stFrag()->isNode());
    Local<SyntaxNodeRef> fileNode(cx, SyntaxNodeRef(frame->stFrag()->toNode()));
    WH_ASSERT(fileNode->nodeType() == AST::File);
    WH_ASSERT(frame->statementNo() < fileNode->astFile().numStatements());

    Local<Frame*> rootedParent(cx, frame->parent());

    // If result is an error, resolve to parent.
    if (result->isError() || result->isException())
        return Frame::ResolveChild(cx, rootedParent, frame, result);

    // Otherwise, create new file syntax frame for executing next
    // statement.
    Local<FileSyntaxFrame*> nextFileFrame(cx);
    if (!nextFileFrame.setResult(FileSyntaxFrame::CreateNext(
            cx->inHatchery(), frame)))
    {
        return ErrorVal();
    }
    return StepResult::Continue(nextFileFrame);
}

/* static */ StepResult
FileSyntaxFrame::StepImpl(ThreadContext* cx,
                            Handle<FileSyntaxFrame*> frame)
{
    WH_ASSERT(frame->stFrag()->isNode());
    Local<SyntaxNodeRef> fileNode(cx, SyntaxNodeRef(frame->stFrag()->toNode()));
    WH_ASSERT(fileNode->nodeType() == AST::File);
    WH_ASSERT(frame->statementNo() <= fileNode->astFile().numStatements());

    Local<Frame*> rootedParent(cx, frame->parent());

    if (frame->statementNo() == fileNode->astFile().numStatements())
        return Frame::ResolveChild(cx, rootedParent, frame, EvalResult::Void());

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
            cx->inHatchery(), frame, stmtNode, scope)))
    {
        return ErrorVal();
    }

    return StepResult::Continue(entryFrame);
}


/* static */ Result<CallExprSyntaxFrame*>
CallExprSyntaxFrame::CreateCallee(AllocationContext acx,
                                  Handle<Frame*> parent,
                                  Handle<EntryFrame*> entryFrame,
                                  Handle<SyntaxTreeFragment*> stFrag)
{
    return acx.create<CallExprSyntaxFrame>(parent, entryFrame, stFrag,
                                           State::Callee, 0,
                                           ValBox(), nullptr, nullptr);
}

/* static */ Result<CallExprSyntaxFrame*>
CallExprSyntaxFrame::CreateFirstArg(AllocationContext acx,
                                    Handle<CallExprSyntaxFrame*> calleeFrame,
                                    Handle<ValBox> callee,
                                    Handle<FunctionObject*> calleeFunc)
{
    Local<Frame*> parent(acx, calleeFrame->parent());
    Local<EntryFrame*> entryFrame(acx, calleeFrame->entryFrame());
    Local<SyntaxTreeFragment*> stFrag(acx, calleeFrame->stFrag());
    return acx.create<CallExprSyntaxFrame>(parent.handle(),
                                           entryFrame.handle(),
                                           stFrag.handle(),
                                           State::Arg, 0,
                                           callee, calleeFunc, nullptr);
}

/* static */ StepResult
CallExprSyntaxFrame::ResolveChildImpl(
        ThreadContext* cx,
        Handle<CallExprSyntaxFrame*> frame,
        Handle<Frame*> childFrame,
        Handle<EvalResult> result)
{
    Local<SyntaxNodeRef> callNodeRef(cx, frame->stFrag()->toNode());
    WH_ASSERT(callNodeRef->nodeType() == AST::CallExpr);

    Local<PackedSyntaxTree*> pst(cx, frame->stFrag()->pst());
    Local<AST::PackedCallExprNode> callExprNode(cx, callNodeRef->astCallExpr());

    Local<VM::Frame*> parent(cx, frame->parent());

    // Always forward errors or exceptions.
    if (result->isError() || result->isException())
        return Frame::ResolveChild(cx, parent, frame, result);

    // Switch on state to handle rest of behaviour.
    switch (frame->state_) {
      case State::Callee:
        return ResolveCalleeChild(cx, frame, pst, callExprNode, result);
      case State::Arg:
        return ResolveArgChild(cx, frame, pst, callExprNode, result);
      case State::Invoke:
        return ResolveInvokeChild(cx, frame, pst, callExprNode, result);
      default:
        WH_UNREACHABLE("Invalid State.");
        return cx->setError(RuntimeError::InternalError,
                            "Invalid CallExpr frame state.");
    }
}

/* static */ StepResult
CallExprSyntaxFrame::ResolveCalleeChild(
        ThreadContext* cx,
        Handle<CallExprSyntaxFrame*> frame,
        Handle<PackedSyntaxTree*> pst,
        Handle<AST::PackedCallExprNode> callExprNode,
        Handle<EvalResult> result)
{
    WH_ASSERT(frame->state_ == State::Callee);
    WH_ASSERT(result->isVoid() || result->isValue());

    Local<VM::Frame*> parent(cx, frame->parent());

    uint32_t offset = callExprNode->callee().offset();

    // A void result is forwarded as an exception.
    // Involving the syntax tree in question.
    if (result->isVoid()) {
        Local<SyntaxNodeRef> subNodeRef(cx,
            SyntaxNodeRef(pst, offset));
        Local<SyntaxNode*> subNode(cx);
        if (!subNode.setResult(subNodeRef->createSyntaxNode(
                                    cx->inHatchery())))
        {
            return ErrorVal();
        }
        cx->setExceptionRaised("Call callee expression yielded void.",
                               subNode.get());
        return Frame::ResolveChild(cx, parent, frame,
                        EvalResult::Exception(frame.get()));
    }

    WH_ASSERT(result->isValue());
    Local<ValBox> calleeBox(cx, result->value());
    Local<FunctionObject*> calleeObj(cx);
    if (!calleeObj.setMaybe(Interp::FunctionObjectForValue(cx, calleeBox))) {
        return cx->setExceptionRaised("Callee value is not callable.",
                                      calleeBox.get());
    }

    // If the function is an operative, the next frame to get created
    // will be an Invoke frame, since the args do not need to be evaluated.
    if (calleeObj->isOperative()) {
        return cx->setError(RuntimeError::InternalError,
                            "TODO: CallExprSyntaxFrame::ResolveCalleeChild - "
                            "create invocation frame for operative.");
    }

    // If the function is an applicative, check the arity of the call.
    WH_ASSERT(calleeObj->isApplicative());
    if (callExprNode->numArgs() == 0) {
        // For an applicative function with zero arguments, we can
        // invoke immediately.
        return cx->setError(RuntimeError::InternalError,
                            "TODO: CallExprSyntaxFrame::ResolveCalleeChild - "
                            "create invocation frame for zero-arg call.");
    }

    Local<CallExprSyntaxFrame*> nextFrame(cx);
    if (!nextFrame.setResult(CallExprSyntaxFrame::CreateFirstArg(
            cx->inHatchery(), frame, calleeBox, calleeObj)))
    {
        return ErrorVal();
    }

    return StepResult::Continue(nextFrame.get());
}

/* static */ StepResult
CallExprSyntaxFrame::ResolveArgChild(
        ThreadContext* cx,
        Handle<CallExprSyntaxFrame*> frame,
        Handle<PackedSyntaxTree*> pst,
        Handle<AST::PackedCallExprNode> callExprNode,
        Handle<EvalResult> result)
{
    WH_ASSERT(frame->state_ == State::Arg);
    WH_ASSERT(frame->argNo() < callExprNode->numArgs());
    WH_ASSERT(result->isVoid() || result->isValue());

    Local<VM::Frame*> parent(cx, frame->parent());

    uint32_t offset = callExprNode->arg(frame->argNo()).offset();

    // A void result is forwarded as an exception.
    // Involving the syntax tree in question.
    if (result->isVoid()) {
        Local<SyntaxNodeRef> subNodeRef(cx,
            SyntaxNodeRef(pst, offset));
        Local<SyntaxNode*> subNode(cx);
        if (!subNode.setResult(subNodeRef->createSyntaxNode(
                                    cx->inHatchery())))
        {
            return ErrorVal();
        }
        cx->setExceptionRaised("Call arg expression yielded void.",
                               subNode.get());
        return Frame::ResolveChild(cx, parent, frame,
                        EvalResult::Exception(frame.get()));
    }

    uint32_t nextArgNo = frame->argNo() + 1;
    WH_ASSERT(nextArgNo <= callExprNode->numArgs());
    if (nextArgNo == callExprNode->numArgs()) {
        return cx->setError(RuntimeError::InternalError,
                            "TODO: CallExprSyntaxFrame::ResolveArgChild - "
                            "create invoke frame for call with args.");
    }

    return cx->setError(RuntimeError::InternalError,
                        "TODO: CallExprSyntaxFrame::ResolveArgChild - "
                        "create evaluation frame for next argument.");
}

/* static */ StepResult
CallExprSyntaxFrame::ResolveInvokeChild(
        ThreadContext* cx,
        Handle<CallExprSyntaxFrame*> frame,
        Handle<PackedSyntaxTree*> pst,
        Handle<AST::PackedCallExprNode> callExprNode,
        Handle<EvalResult> result)
{
    WH_ASSERT(frame->inInvokeState());
    WH_ASSERT(result->isVoid() || result->isValue());

    Local<Frame*> parent(cx, frame->parent());
    return Frame::ResolveChild(cx, parent, frame, result);
}

/* static */ StepResult
CallExprSyntaxFrame::StepImpl(ThreadContext* cx,
                              Handle<CallExprSyntaxFrame*> frame)
{
    WH_ASSERT(frame->stFrag()->isNode());

    // On initial step, just set up the entry frame for evaluating the
    // underlying callee or arg expression.

    switch (frame->state()) {
      case State::Callee:
        return StepCallee(cx, frame);
      case State::Arg:
        return StepArg(cx, frame);
      case State::Invoke:
        return StepInvoke(cx, frame);
      default:
        WH_UNREACHABLE("Invalid CallExprSyntaxFrame::State.");
        return cx->setError(RuntimeError::InternalError,
                            "Invalid CallExprSyntaxFrame::State.");
    }
}

/* static */ StepResult
CallExprSyntaxFrame::StepCallee(ThreadContext* cx,
                                Handle<CallExprSyntaxFrame*> frame)
{
    WH_ASSERT(frame->inCalleeState());

    Local<SyntaxNodeRef> callNodeRef(cx, frame->stFrag()->toNode());
    WH_ASSERT(callNodeRef->nodeType() == AST::CallExpr);

    Local<PackedSyntaxTree*> pst(cx, frame->stFrag()->pst());
    Local<AST::PackedCallExprNode> callExprNode(cx, callNodeRef->astCallExpr());

    return StepSubexpr(cx, frame, pst, callExprNode->callee().offset());
}

/* static */ StepResult
CallExprSyntaxFrame::StepArg(ThreadContext* cx,
                             Handle<CallExprSyntaxFrame*> frame)
{
    WH_ASSERT(frame->inArgState());

    // Only applicatives need evaluation of arguments.
    WH_ASSERT(frame->calleeFunc()->isApplicative());

    Local<SyntaxNodeRef> callNodeRef(cx, frame->stFrag()->toNode());
    WH_ASSERT(callNodeRef->nodeType() == AST::CallExpr);

    Local<PackedSyntaxTree*> pst(cx, frame->stFrag()->pst());
    Local<AST::PackedCallExprNode> callExprNode(cx, callNodeRef->astCallExpr());

    uint16_t argNo = frame->argNo();
    WH_ASSERT(argNo < callExprNode->numArgs());

    return StepSubexpr(cx, frame, pst, callExprNode->arg(argNo).offset());
}

/* static */ StepResult
CallExprSyntaxFrame::StepInvoke(ThreadContext* cx,
                                Handle<CallExprSyntaxFrame*> frame)
{
    WH_ASSERT(frame->inInvokeState());
    // TODO: create an invoke frame for the function being called.
    return cx->setError(RuntimeError::InternalError,
                        "CallExprSyntaxFrame::StepInvoke not implemented.");
}

/* static */ StepResult
CallExprSyntaxFrame::StepSubexpr(ThreadContext* cx,
                                 Handle<CallExprSyntaxFrame*> frame,
                                 Handle<PackedSyntaxTree*> pst,
                                 uint32_t offset)
{
    // Create a new SyntaxNode for the subexpression (callee or argN).
    Local<SyntaxNodeRef> nodeRef(cx, SyntaxNodeRef(pst, offset));
    Local<SyntaxNode*> node(cx);
    if (!node.setResult(nodeRef->createSyntaxNode(cx->inHatchery())))
        return ErrorVal();

    Local<ScopeObject*> scope(cx, frame->scope());

    // Create and return entry frame.
    Local<EntryFrame*> entryFrame(cx);
    if (!entryFrame.setResult(VM::EntryFrame::Create(
            cx->inHatchery(), frame, node.handle(), scope)))
    {
        return ErrorVal();
    }

    return StepResult::Continue(entryFrame);
}



} // namespace VM
} // namespace Whisper
