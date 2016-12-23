
#include <limits>

#include "runtime_inlines.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/frame.hpp"
#include "vm/exception.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "vm/continuation.hpp"
#include "interp/heap_interpreter.hpp"

namespace Whisper {
namespace VM {


/* static */ StepResult
Frame::Resolve(ThreadContext* cx,
               Handle<Frame*> frame,
               Handle<EvalResult> result)
{
#define RESOLVE_CHILD_CASE_(name) \
    if (frame->is##name()) \
        return name::ResolveImpl(cx, frame.upConvertTo<name*>(), result);

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
TerminalFrame::ResolveImpl(ThreadContext* cx,
                           Handle<TerminalFrame*> frame,
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
EntryFrame::ResolveImpl(ThreadContext* cx,
                        Handle<EntryFrame*> frame,
                        Handle<EvalResult> result)
{
    // Resolve parent frame with the same result.
    Local<Frame*> rootedParent(cx, frame->parent());
    return Frame::Resolve(cx, rootedParent, result);
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

/* static */ Result<InvokeSyntaxNodeFrame*>
InvokeSyntaxNodeFrame::Create(AllocationContext acx,
                              Handle<Frame*> parent,
                              Handle<EntryFrame*> entryFrame,
                              Handle<SyntaxTreeFragment*> stFrag)
{
    return acx.create<InvokeSyntaxNodeFrame>(parent, entryFrame, stFrag);
}

/* static */ StepResult
InvokeSyntaxNodeFrame::ResolveImpl(ThreadContext* cx,
                                   Handle<InvokeSyntaxNodeFrame*> frame,
                                   Handle<EvalResult> result)
{
    // Resolve parent frame with the same result.
    Local<Frame*> rootedParent(cx, frame->parent());
    return Frame::Resolve(cx, rootedParent, result);
}

/* static */ StepResult
InvokeSyntaxNodeFrame::StepImpl(ThreadContext* cx,
                                Handle<InvokeSyntaxNodeFrame*> frame)
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
    Local<PropertyLookupResult> lookupResult(cx,
        Interp::GetObjectProperty(cx, scope.handle().convertTo<Wobject*>(),
                                      name));

    Local<Frame*> parent(cx, frame->parent());
    Local<EvalResult> lookupEvalResult(cx,
        lookupResult->toEvalResult(cx, frame));

    WH_ASSERT(lookupEvalResult->isError() || lookupEvalResult->isExc() ||
              lookupEvalResult->isValue());

    if (!lookupEvalResult->isValue())
        return Frame::Resolve(cx, parent, lookupEvalResult.get());

    // Invoke the syntax handler.
    Local<ValBox> syntaxHandler(cx, lookupEvalResult->value());
    Local<ScopeObject*> callerScope(cx, frame->entryFrame()->scope());
    Local<SyntaxTreeFragment*> stFrag(cx, frame->stFrag());
    Local<CallResult> result(cx, Interp::InvokeOperativeValue(
            cx, frame, callerScope, syntaxHandler, stFrag));

    // Forward result from syntax handler.
    if (result->isError())
        return Frame::Resolve(cx, parent, EvalResult::Error());

    if (result->isExc())
        return Frame::Resolve(cx, parent, result->excAsEvalResult());

    if (result->isValue())
        return Frame::Resolve(cx, parent, result->valueAsEvalResult());

    if (result->isVoid())
        return Frame::Resolve(cx, parent, EvalResult::Void());

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
FileSyntaxFrame::ResolveImpl(ThreadContext* cx,
                             Handle<FileSyntaxFrame*> frame,
                             Handle<EvalResult> result)
{
    WH_ASSERT(frame->stFrag()->isNode());
    Local<SyntaxNodeRef> fileNode(cx, SyntaxNodeRef(frame->stFrag()->toNode()));
    WH_ASSERT(fileNode->nodeType() == AST::File);
    WH_ASSERT(frame->statementNo() < fileNode->astFile().numStatements());

    Local<Frame*> rootedParent(cx, frame->parent());

    // If result is an error, resolve to parent.
    if (result->isError() || result->isExc())
        return Frame::Resolve(cx, rootedParent, result);

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
        return Frame::Resolve(cx, rootedParent, EvalResult::Void());

    // Get SyntaxTreeFragment for next statement node.
    Local<SyntaxTreeFragment*> stmtNode(cx);
    if (!stmtNode.setResult(SyntaxNode::Create(
            cx->inHatchery(), fileNode->pst(),
            fileNode->astFile().statement(frame->statementNo()).offset())))
    {
        return ErrorVal();
    }

    // Create a new InvokeSyntaxNode frame for interpreting each statement.
    Local<ScopeObject*> scope(cx, frame->entryFrame()->scope());
    Local<VM::EntryFrame*> entryFrame(cx, frame->entryFrame());
    Local<VM::InvokeSyntaxNodeFrame*> syntaxFrame(cx);
    if (!syntaxFrame.setResult(VM::InvokeSyntaxNodeFrame::Create(
            cx->inHatchery(), frame, entryFrame, stmtNode)))
    {
        return ErrorVal();
    }

    return StepResult::Continue(syntaxFrame);
}


/* static */ Result<BlockSyntaxFrame*>
BlockSyntaxFrame::Create(AllocationContext acx,
                         Handle<Frame*> parent,
                         Handle<EntryFrame*> entryFrame,
                         Handle<SyntaxTreeFragment*> stFrag,
                         uint32_t statementNo)
{
    return acx.create<BlockSyntaxFrame>(parent, entryFrame, stFrag,
                                       statementNo);
}

/* static */ Result<BlockSyntaxFrame*>
BlockSyntaxFrame::CreateNext(AllocationContext acx,
                             Handle<BlockSyntaxFrame*> curFrame)
{
    WH_ASSERT(curFrame->stFrag()->isBlock());
    Local<SyntaxBlockRef> blockRef(acx,
        SyntaxBlockRef(curFrame->stFrag()->toBlock()));
    WH_ASSERT(curFrame->statementNo() < blockRef->astBlock().numStatements());

    Local<Frame*> parent(acx, curFrame->parent());
    Local<EntryFrame*> entryFrame(acx, curFrame->entryFrame());
    Local<SyntaxTreeFragment*> stFrag(acx, curFrame->stFrag());
    uint32_t nextStatementNo = curFrame->statementNo() + 1;

    return Create(acx, parent, entryFrame, stFrag, nextStatementNo);
}

/* static */ StepResult
BlockSyntaxFrame::ResolveImpl(ThreadContext* cx,
                              Handle<BlockSyntaxFrame*> frame,
                              Handle<EvalResult> result)
{
    WH_ASSERT(frame->stFrag()->isBlock());
    Local<SyntaxBlockRef> blockRef(cx,
        SyntaxBlockRef(frame->stFrag()->toBlock()));

    uint32_t stmtNo = frame->statementNo();
    uint32_t numStmts = blockRef->astBlock().numStatements();
    WH_ASSERT(stmtNo < numStmts);

    Local<Frame*> rootedParent(cx, frame->parent());

    // If result is an error, resolve to parent.
    if (result->isError() || result->isExc())
        return Frame::Resolve(cx, rootedParent, result);

    // Otherwise, if all statements have been evaluated, yield the
    // result of the last one.
    if (stmtNo + 1 == numStmts) {
        return Frame::Resolve(cx, rootedParent, result);
    }

    // Otherwise, create new block syntax frame for executing next
    // statement.
    Local<BlockSyntaxFrame*> nextBlockFrame(cx);
    if (!nextBlockFrame.setResult(BlockSyntaxFrame::CreateNext(
            cx->inHatchery(), frame)))
    {
        return ErrorVal();
    }
    return StepResult::Continue(nextBlockFrame);
}

/* static */ StepResult
BlockSyntaxFrame::StepImpl(ThreadContext* cx,
                           Handle<BlockSyntaxFrame*> frame)
{
    WH_ASSERT(frame->stFrag()->isBlock());
    Local<SyntaxBlockRef> blockRef(cx,
        SyntaxBlockRef(frame->stFrag()->toBlock()));
    WH_ASSERT(frame->statementNo() < blockRef->astBlock().numStatements());

    Local<Frame*> rootedParent(cx, frame->parent());

    // Get SyntaxTreeFragment for next statement node.
    Local<SyntaxTreeFragment*> stmtNode(cx);
    if (!stmtNode.setResult(SyntaxNode::Create(
            cx->inHatchery(), blockRef->pst(),
            blockRef->astBlock().statement(frame->statementNo()).offset())))
    {
        return ErrorVal();
    }

    // Create a new InvokeSyntaxNode frame for interpreting each statement.
    Local<ScopeObject*> scope(cx, frame->entryFrame()->scope());
    Local<VM::EntryFrame*> entryFrame(cx, frame->entryFrame());
    Local<VM::InvokeSyntaxNodeFrame*> syntaxFrame(cx);
    if (!syntaxFrame.setResult(VM::InvokeSyntaxNodeFrame::Create(
            cx->inHatchery(), frame, entryFrame, stmtNode)))
    {
        return ErrorVal();
    }

    return StepResult::Continue(syntaxFrame);
}


/* static */ Result<ReturnStmtSyntaxFrame*>
ReturnStmtSyntaxFrame::Create(AllocationContext acx,
                              Handle<Frame*> parent,
                              Handle<EntryFrame*> entryFrame,
                              Handle<SyntaxTreeFragment*> stFrag)
{
    return acx.create<ReturnStmtSyntaxFrame>(parent, entryFrame, stFrag);
}

/* static */ StepResult
ReturnStmtSyntaxFrame::ResolveImpl(ThreadContext* cx,
                                   Handle<ReturnStmtSyntaxFrame*> frame,
                                   Handle<EvalResult> result)
{
    Local<Frame*> rootedParent(cx, frame->parent());

    if (result->isError() || result->isExc())
        return Frame::Resolve(cx, rootedParent, result);

    if (result->isVoid()) {
        Local<Exception*> exc(cx);
        if (!exc.setResult(InternalException::Create(cx->inHatchery(),
                           "return expression yielded void.")))
        {
            return ErrorVal();
        }
        return Frame::Resolve(cx, rootedParent,
                              EvalResult::Exc(frame.get(), exc));
    }

    WH_ASSERT(result->isValue());
    Local<ValBox> returnValue(cx, result->value());

    // Look up the "@retcont" in the scope.
    Local<ScopeObject*> scope(cx, frame->entryFrame()->scope());
    Local<String*> retcontStr(cx, cx->runtimeState()->nm_AtRetcont());
    Local<PropertyLookupResult> retcontResult(cx,
        Interp::GetObjectProperty(cx, scope.handle(), retcontStr));
    if (retcontResult->isError())
        return ErrorVal();

    if (retcontResult->isNotFound()) {
        Local<Exception*> exc(cx);
        if (!exc.setResult(InternalException::Create(cx->inHatchery(),
                           "return used in non-returnable context.")))
        {
            return ErrorVal();
        }
        return Frame::Resolve(cx, rootedParent,
                              EvalResult::Exc(frame.get(), exc));
    }

    WH_ASSERT(retcontResult->isFound());

    Local<EvalResult> retcontEval(cx, retcontResult->toEvalResult(cx, frame));
    if (retcontEval->isError() || retcontEval->isExc())
        return Frame::Resolve(cx, rootedParent, retcontEval.handle());

    WH_ASSERT(retcontEval->isValue());
    Local<ValBox> retcontValue(cx, retcontEval->value());
    if (!retcontValue->isPointer()) {
        Local<Exception*> exc(cx);
        if (!exc.setResult(InternalException::Create(cx->inHatchery(),
                       "@retcont contains a non-object value.")))
        {
            return ErrorVal();
        }
        return Frame::Resolve(cx, rootedParent,
                              EvalResult::Exc(frame.get(), exc));
    }

    Local<Wobject*> retcontObj(cx, retcontValue->objectPointer());
    if (!HeapThing::From(retcontObj.get())->isContObject()) {
        Local<Exception*> exc(cx);
        if (!exc.setResult(InternalException::Create(cx->inHatchery(),
                       "@retcont contains a non-continuation object.")))
        {
            return ErrorVal();
        }
        return Frame::Resolve(cx, rootedParent,
                              EvalResult::Exc(frame.get(), exc));
    }

    Local<ContObject*> contObj(cx,
        reinterpret_cast<ContObject*>(retcontObj.get()));
    Local<Continuation*> cont(cx, contObj->cont());

    return cont->continueWith(cx, returnValue);
}

/* static */ StepResult
ReturnStmtSyntaxFrame::StepImpl(ThreadContext* cx,
                                Handle<ReturnStmtSyntaxFrame*> frame)
{
    Local<SyntaxTreeFragment*> stFrag(cx, frame->stFrag());
    WH_ASSERT(frame->stFrag()->isNode());

    Local<AST::PackedReturnStmtNode> returnStmt(cx,
        stFrag->toNode()->astReturnStmt());

    // If there is no return expression, resolve self with an undefined value.
    if (!returnStmt->hasExpression()) {
        Local<EvalResult> evalResult(cx,
            EvalResult::Value(ValBox::Undefined()));
        return ResolveImpl(cx, frame, evalResult);
    }

    // Otherwise, evaluate the return expression.

    // Create the SyntaxNode for the expression to evaluate.
    Local<AST::PackedBaseNode> exprNode(cx, returnStmt->expression());
    Local<PackedSyntaxTree*> pst(cx, stFrag->pst());
    Local<SyntaxTreeFragment*> exprStFrag(cx);
    if (!exprStFrag.setResult(SyntaxNode::Create(
            cx->inHatchery(), pst, exprNode->offset())))
    {
        return ErrorVal();
    }

    // Create the syntax invocation frame.
    Local<EntryFrame*> entryFrame(cx, frame->entryFrame());
    Local<InvokeSyntaxNodeFrame*> syntaxFrame(cx);
    if (!syntaxFrame.setResult(InvokeSyntaxNodeFrame::Create(
            cx->inHatchery(), frame, entryFrame, exprStFrag)))
    {
        return ErrorVal();
    }

    return StepResult::Continue(syntaxFrame.get());
}


/* static */ Result<VarSyntaxFrame*>
VarSyntaxFrame::Create(AllocationContext acx,
                       Handle<Frame*> parent,
                       Handle<EntryFrame*> entryFrame,
                       Handle<SyntaxTreeFragment*> stFrag,
                       uint32_t bindingNo)
{
    return acx.create<VarSyntaxFrame>(parent, entryFrame, stFrag, bindingNo);
}

/* static */ StepResult
VarSyntaxFrame::ResolveImpl(ThreadContext* cx,
                            Handle<VarSyntaxFrame*> frame,
                            Handle<EvalResult> result)
{
    Local<SyntaxTreeFragment*> stFrag(cx, frame->stFrag());
    WH_ASSERT(stFrag->isNode());

    Local<SyntaxNodeRef> nodeRef(cx, SyntaxNodeRef(stFrag->toNode()));

    bool isConst = frame->isConst();
    uint32_t bindingNo = frame->bindingNo();
    uint32_t numBindings = isConst ? nodeRef->astConstStmt().numBindings()
                                   : nodeRef->astVarStmt().numBindings();
    WH_ASSERT(bindingNo < numBindings);

    Local<Frame*> rootedParent(cx, frame->parent());

    // If result is an error, resolve to parent.
    if (result->isError() || result->isExc())
        return Frame::Resolve(cx, rootedParent, result);

    if (result->isVoid()) {
        WH_UNREACHABLE("Got void eval result for expression.");
        return ErrorVal();
    }

    WH_ASSERT(result->isValue());
    Local<ValBox> value(cx, result->value());

    // Bind the resulting value to the scope.
    uint32_t nameCid = isConst ? nodeRef->astConstStmt().varnameCid(bindingNo)
                               : nodeRef->astVarStmt().varnameCid(bindingNo);
    Local<VM::String*> name(cx, nodeRef->pst()->getConstantString(nameCid));
    Local<VM::ScopeObject*> scope(cx, frame->entryFrame()->scope());
    Local<VM::PropertyDescriptor> propDesc(cx,
            VM::PropertyDescriptor::MakeSlot(value.get(),
                PropertySlotInfo().withWritable(!isConst)));
    if (!Wobject::DefineProperty(cx->inHatchery(), scope.handle(),
                                 name, propDesc))
    {
        return ErrorVal();
    }

    bindingNo += 1;

    // For var-expressions only, automatically bind undefined
    // to any uninitialized properties.
    if (!isConst) {
        Local<AST::PackedVarStmtNode> varStmt(cx, nodeRef->astVarStmt());
        value.set(ValBox::Undefined());
        for ( ; bindingNo < numBindings; bindingNo++) {
            // If there's an expression to evaluate, break out.
            if (varStmt->hasVarexpr(bindingNo))
                break;

            // Otherwise, bind undefined to it.
            uint32_t nameCid = varStmt->varnameCid(bindingNo);
            Local<VM::String*> name(cx,
                nodeRef->pst()->getConstantString(nameCid));
            Local<VM::PropertyDescriptor> propDesc(cx,
                    VM::PropertyDescriptor::MakeSlot(ValBox::Undefined(),
                        PropertySlotInfo().withWritable(true)));
            if (!Wobject::DefineProperty(cx->inHatchery(), scope.handle(),
                                         name, propDesc))
            {
                return ErrorVal();
            }
        }
    }

    // Check if all done with bindings.
    if (bindingNo == numBindings)
        return Frame::Resolve(cx, rootedParent, result);

    // Otherwise, create var syntax frame for evaluating next binding expr.
    Local<EntryFrame*> entryFrame(cx, frame->entryFrame());
    Local<VarSyntaxFrame*> nextVarFrame(cx);
    if (!nextVarFrame.setResult(VarSyntaxFrame::Create(
            cx->inHatchery(), rootedParent, entryFrame, stFrag, bindingNo)))
    {
        return ErrorVal();
    }
    return StepResult::Continue(nextVarFrame);
}

/* static */ StepResult
VarSyntaxFrame::StepImpl(ThreadContext* cx,
                         Handle<VarSyntaxFrame*> frame)
{
    Local<SyntaxTreeFragment*> stFrag(cx, frame->stFrag());
    WH_ASSERT(frame->stFrag()->isNode());

    Local<SyntaxNodeRef> nodeRef(cx, SyntaxNodeRef(stFrag->toNode()));

    bool isConst = frame->isConst();
    uint32_t bindingNo = frame->bindingNo();
    uint32_t numBindings = isConst ? nodeRef->astConstStmt().numBindings()
                                   : nodeRef->astVarStmt().numBindings();
    WH_ASSERT(bindingNo < numBindings);

    Local<Frame*> rootedParent(cx, frame->parent());

    // For var-expressions only, automatically bind undefined
    // to any uninitialized properties.
    if (!isConst) {
        Local<VM::ScopeObject*> scope(cx, frame->entryFrame()->scope());
        Local<AST::PackedVarStmtNode> varStmt(cx, nodeRef->astVarStmt());
        for ( ; bindingNo < numBindings; bindingNo++) {
            // If there's an expression to evaluate, break out.
            if (varStmt->hasVarexpr(bindingNo))
                break;

            // Otherwise, bind undefined to it.
            uint32_t nameCid = varStmt->varnameCid(bindingNo);
            Local<VM::String*> name(cx,
                nodeRef->pst()->getConstantString(nameCid));
            Local<VM::PropertyDescriptor> propDesc(cx,
                    VM::PropertyDescriptor::MakeSlot(ValBox::Undefined(),
                        PropertySlotInfo().withWritable(true)));
            if (!Wobject::DefineProperty(cx->inHatchery(), scope.handle(),
                                         name, propDesc))
            {
                return ErrorVal();
            }
        }
    }

    // Check if all done with bindings.
    if (bindingNo == numBindings) {
        return Frame::Resolve(cx, rootedParent,
                        EvalResult::Value(ValBox::Undefined()));
    }

    // Create the SyntaxNode for the expression to evaluate.
    Local<AST::PackedBaseNode> bindingAstNode(cx,
        isConst ? nodeRef->astConstStmt().varexpr(bindingNo)
                : nodeRef->astVarStmt().varexpr(bindingNo));
    Local<PackedSyntaxTree*> pst(cx, stFrag->pst());
    Local<SyntaxTreeFragment*> bindingStFrag(cx);
    if (!bindingStFrag.setResult(SyntaxNode::Create(
            cx->inHatchery(), pst, bindingAstNode->offset())))
    {
        return ErrorVal();
    }

    // Create new InvokeSyntaxNodeFrame to evaluate it.
    Local<EntryFrame*> entryFrame(cx, frame->entryFrame());
    Local<InvokeSyntaxNodeFrame*> syntaxFrame(cx);
    if (!syntaxFrame.setResult(InvokeSyntaxNodeFrame::Create(
            cx->inHatchery(), frame, entryFrame, bindingStFrag)))
    {
        return ErrorVal();
    }

    return StepResult::Continue(syntaxFrame);
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

/* static */ Result<CallExprSyntaxFrame*>
CallExprSyntaxFrame::CreateNextArg(AllocationContext acx,
                                   Handle<CallExprSyntaxFrame*> calleeFrame,
                                   Handle<Slist<ValBox>*> operands)
{
    Local<Frame*> parent(acx, calleeFrame->parent());
    Local<EntryFrame*> entryFrame(acx, calleeFrame->entryFrame());
    Local<SyntaxTreeFragment*> stFrag(acx, calleeFrame->stFrag());
    Local<ValBox> callee(acx, calleeFrame->callee());
    Local<FunctionObject*> calleeFunc(acx, calleeFrame->calleeFunc());
    uint16_t argNo = calleeFrame->argNo() + 1;
    return acx.create<CallExprSyntaxFrame>(parent.handle(),
                                           entryFrame.handle(),
                                           stFrag.handle(),
                                           State::Arg, argNo,
                                           callee.handle(),
                                           calleeFunc.handle(),
                                           operands);
}

/* static */ Result<CallExprSyntaxFrame*>
CallExprSyntaxFrame::CreateInvoke(AllocationContext acx,
                                  Handle<CallExprSyntaxFrame*> frame,
                                  Handle<Slist<ValBox>*> operands)
{
    Local<Frame*> parent(acx, frame->parent());
    Local<EntryFrame*> entryFrame(acx, frame->entryFrame());
    Local<SyntaxTreeFragment*> stFrag(acx, frame->stFrag());
    Local<ValBox> callee(acx, frame->callee());
    Local<FunctionObject*> calleeFunc(acx, frame->calleeFunc());
    return acx.create<CallExprSyntaxFrame>(parent.handle(),
                                           entryFrame.handle(),
                                           stFrag.handle(),
                                           State::Invoke, 0,
                                           callee.handle(),
                                           calleeFunc.handle(),
                                           operands);
}

/* static */ Result<CallExprSyntaxFrame*>
CallExprSyntaxFrame::CreateInvoke(AllocationContext acx,
                                  Handle<CallExprSyntaxFrame*> frame,
                                  Handle<ValBox> callee,
                                  Handle<FunctionObject*> calleeFunc,
                                  Handle<Slist<ValBox>*> operands)
{
    Local<Frame*> parent(acx, frame->parent());
    Local<EntryFrame*> entryFrame(acx, frame->entryFrame());
    Local<SyntaxTreeFragment*> stFrag(acx, frame->stFrag());
    return acx.create<CallExprSyntaxFrame>(parent.handle(),
                                           entryFrame.handle(),
                                           stFrag.handle(),
                                           State::Invoke, 0,
                                           callee, calleeFunc, operands);
}

/* static */ StepResult
CallExprSyntaxFrame::ResolveImpl(ThreadContext* cx,
                                 Handle<CallExprSyntaxFrame*> frame,
                                 Handle<EvalResult> result)
{
    Local<SyntaxNodeRef> callNodeRef(cx, frame->stFrag()->toNode());
    WH_ASSERT(callNodeRef->nodeType() == AST::CallExpr);

    Local<PackedSyntaxTree*> pst(cx, frame->stFrag()->pst());
    Local<AST::PackedCallExprNode> callExprNode(cx, callNodeRef->astCallExpr());

    Local<VM::Frame*> parent(cx, frame->parent());

    // Always forward errors or exceptions.
    if (result->isError() || result->isExc())
        return Frame::Resolve(cx, parent, result);

    // Switch on state to handle rest of behaviour.
    switch (frame->state_) {
      case State::Callee:
        return ResolveCallee(cx, frame, pst, callExprNode, result);
      case State::Arg:
        return ResolveArg(cx, frame, pst, callExprNode, result);
      case State::Invoke:
        return ResolveInvoke(cx, frame, pst, callExprNode, result);
      default:
        WH_UNREACHABLE("Invalid State.");
        return cx->setError(RuntimeError::InternalError,
                            "Invalid CallExpr frame state.");
    }
}

/* static */ StepResult
CallExprSyntaxFrame::ResolveCallee(ThreadContext* cx,
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

        Local<Exception*> exc(cx);
        if (!exc.setResult(InternalException::Create(cx->inHatchery(),
                           "Callee expression yielded void",
                           subNode.handle())))
        {
            return Frame::Resolve(cx, parent, EvalResult::Error());
        }

        return Frame::Resolve(cx, parent, EvalResult::Exc(frame, exc));
    }

    WH_ASSERT(result->isValue());
    Local<ValBox> calleeBox(cx, result->value());
    Local<FunctionObject*> calleeObj(cx);
    if (!calleeObj.setMaybe(Interp::FunctionObjectForValue(cx, calleeBox))) {
        Local<Exception*> exc(cx);
        if (!exc.setResult(InternalException::Create(cx->inHatchery(),
                           "Callee expression is not callable",
                           calleeBox.handle())))
        {
            return Frame::Resolve(cx, parent, EvalResult::Error());
        }
        return Frame::Resolve(cx, parent, EvalResult::Exc(frame, exc));
    }

    Local<CallExprSyntaxFrame*> nextFrame(cx);

    // If the function is an operative, the next frame to get created
    // will be an Invoke frame, since the args do not need to be evaluated.
    if (calleeObj->isOperative()) {
        // Zero-argument applicative can be invoked immediately.
        Local<Slist<ValBox>*> operands(cx, nullptr);
        if (!nextFrame.setResult(CallExprSyntaxFrame::CreateInvoke(
                cx->inHatchery(), frame, calleeBox, calleeObj, operands)))
        {
            return ErrorVal();
        }

        return StepResult::Continue(nextFrame.get());
    }

    // If the function is an applicative, check the arity of the call.
    WH_ASSERT(calleeObj->isApplicative());
    if (callExprNode->numArgs() == 0) {
        // Zero-argument applicative can be invoked immediately.
        Local<Slist<ValBox>*> operands(cx, nullptr);
        if (!nextFrame.setResult(CallExprSyntaxFrame::CreateInvoke(
                cx->inHatchery(), frame, calleeBox, calleeObj, operands)))
        {
            return ErrorVal();
        }

        return StepResult::Continue(nextFrame.get());
    }

    if (!nextFrame.setResult(CallExprSyntaxFrame::CreateFirstArg(
            cx->inHatchery(), frame, calleeBox, calleeObj)))
    {
        return ErrorVal();
    }

    return StepResult::Continue(nextFrame.get());
}

/* static */ StepResult
CallExprSyntaxFrame::ResolveArg(ThreadContext* cx,
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

        Local<Exception*> exc(cx);
        if (!exc.setResult(InternalException::Create(cx->inHatchery(),
                           "Callee arg expression yielded void",
                           subNode.handle())))
        {
            return Frame::Resolve(cx, parent, EvalResult::Error());
        }
        return Frame::Resolve(cx, parent, EvalResult::Exc(frame, exc));
    }

    // Prepend the value to the operands list.
    Local<Slist<ValBox>*> oldOperands(cx, frame->operands());
    Local<Slist<ValBox>*> operands(cx);
    if (!operands.setResult(Slist<ValBox>::Create(
                cx->inHatchery(), result->value(), oldOperands)))
    {
        return ErrorVal();
    }

    uint32_t nextArgNo = frame->argNo() + 1;
    WH_ASSERT(nextArgNo <= callExprNode->numArgs());
    Local<CallExprSyntaxFrame*> invokeFrame(cx);

    if (nextArgNo == callExprNode->numArgs()) {
        if (!invokeFrame.setResult(CallExprSyntaxFrame::CreateInvoke(
                cx->inHatchery(), frame, operands)))
        {
            return ErrorVal();
        }
    } else {
        if (!invokeFrame.setResult(CallExprSyntaxFrame::CreateNextArg(
                cx->inHatchery(), frame, operands)))
        {
            return ErrorVal();
        }
    }

    return StepResult::Continue(invokeFrame.get());
}

/* static */ StepResult
CallExprSyntaxFrame::ResolveInvoke(ThreadContext* cx,
                                   Handle<CallExprSyntaxFrame*> frame,
                                   Handle<PackedSyntaxTree*> pst,
                                   Handle<AST::PackedCallExprNode> callExprNode,
                                   Handle<EvalResult> result)
{
    WH_ASSERT(frame->inInvokeState());
    WH_ASSERT(result->isVoid() || result->isValue());

    Local<Frame*> parent(cx, frame->parent());
    return Frame::Resolve(cx, parent, result);
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
    Local<ValBox> callee(cx, frame->callee());
    Local<FunctionObject*> calleeFunc(cx, frame->calleeFunc());
    Local<Slist<ValBox>*> operands(cx, frame->operands());

    if (calleeFunc->isApplicative()) {
        Local<InvokeApplicativeFrame*> invokeFrame(cx);
        if (!invokeFrame.setResult(InvokeApplicativeFrame::Create(
                cx->inHatchery(), frame, callee, calleeFunc, operands)))
        {
            return ErrorVal();
        }
        return StepResult::Continue(invokeFrame.get());
    }

    WH_ASSERT(calleeFunc->isOperative());
    WH_ASSERT(operands.get() == nullptr);

    Local<SyntaxTreeFragment*> stFrag(cx, frame->stFrag());

    Local<InvokeOperativeFrame*> invokeFrame(cx);
    if (!invokeFrame.setResult(InvokeOperativeFrame::Create(
            cx->inHatchery(), frame, callee, calleeFunc, stFrag)))
    {
        return ErrorVal();
    }
    return StepResult::Continue(invokeFrame.get());
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

    Local<ScopeObject*> scope(cx, frame->entryFrame()->scope());

    // Create and return entry frame.
    Local<EntryFrame*> entryFrame(cx, frame->entryFrame());
    Local<VM::InvokeSyntaxNodeFrame*> syntaxFrame(cx);
    if (!syntaxFrame.setResult(VM::InvokeSyntaxNodeFrame::Create(
            cx->inHatchery(), frame, entryFrame, node.handle())))
    {
        return ErrorVal();
    }

    return StepResult::Continue(syntaxFrame);
}

/* static */ Result<InvokeApplicativeFrame*>
InvokeApplicativeFrame::Create(
        AllocationContext acx,
        Handle<Frame*> parent,
        Handle<ValBox> callee,
        Handle<FunctionObject*> calleeFunc,
        Handle<Slist<ValBox>*> operands)
{
    return acx.create<InvokeApplicativeFrame>(parent, callee, calleeFunc,
                                              operands);
}

/* static */ StepResult
InvokeApplicativeFrame::ResolveImpl(ThreadContext* cx,
                                    Handle<InvokeApplicativeFrame*> frame,
                                    Handle<EvalResult> result)
{
    Local<Frame*> parent(cx, frame->parent());
    return Frame::Resolve(cx, parent, result);
}

/* static */ StepResult
InvokeApplicativeFrame::StepImpl(ThreadContext* cx,
                                 Handle<InvokeApplicativeFrame*> frame)
{
    Local<ValBox> callee(cx, frame->callee());
    Local<FunctionObject*> calleeFunc(cx, frame->calleeFunc());
    Local<Slist<ValBox>*> operands(cx, frame->operands());
    Local<ScopeObject*> callerScope(cx, frame->ancestorEntryFrame()->scope());

    uint32_t length = operands->length();
    LocalArray<ValBox> args(cx, length);

    // Fill in args (reverse order).
    Slist<ValBox>* curArg = operands;
    for (uint32_t i = 0; i < length; i++) {
        WH_ASSERT(curArg);

        uint32_t idx = (length - 1) - i;
        args[idx] = curArg->value();

        curArg = curArg->rest();
    }

    // Invoke the applicative function.
    Local<CallResult> result(cx, Interp::InvokeApplicativeFunction(
            cx, frame, callerScope, callee, calleeFunc, args));

    Local<Frame*> parent(cx, frame->parent());

    if (result->isError())
        return Frame::Resolve(cx, parent, EvalResult::Error());

    if (result->isExc())
        return Frame::Resolve(cx, parent, result->excAsEvalResult());

    if (result->isValue())
        return Frame::Resolve(cx, parent, result->valueAsEvalResult());

    if (result->isVoid())
        return Frame::Resolve(cx, parent, EvalResult::Void());

    if (result->isContinue())
        return StepResult::Continue(result->continueFrame());

    WH_UNREACHABLE("Unknown CallResult outcome.");
    return cx->setError(RuntimeError::InternalError,
                        "Unknown CallResult outcome.");
}

/* static */ Result<InvokeOperativeFrame*>
InvokeOperativeFrame::Create(AllocationContext acx,
                             Handle<Frame*> parent,
                             Handle<ValBox> callee,
                             Handle<FunctionObject*> calleeFunc,
                             Handle<SyntaxTreeFragment*> stFrag)
{
    return acx.create<InvokeOperativeFrame>(parent, callee, calleeFunc,
                                            stFrag);
}

/* static */ StepResult
InvokeOperativeFrame::ResolveImpl(ThreadContext* cx,
                                  Handle<InvokeOperativeFrame*> frame,
                                  Handle<EvalResult> result)
{
    Local<Frame*> parent(cx, frame->parent());
    return Frame::Resolve(cx, parent, result);
}

/* static */ StepResult
InvokeOperativeFrame::StepImpl(ThreadContext* cx,
                               Handle<InvokeOperativeFrame*> frame)
{
    Local<ValBox> callee(cx, frame->callee());
    Local<FunctionObject*> calleeFunc(cx, frame->calleeFunc());
    Local<SyntaxTreeFragment*> stFrag(cx, frame->stFrag());
    Local<ScopeObject*> callerScope(cx, frame->ancestorEntryFrame()->scope());

    Local<SyntaxNodeRef> callNodeRef(cx, frame->stFrag()->toNode());
    WH_ASSERT(callNodeRef->nodeType() == AST::CallExpr);

    Local<PackedSyntaxTree*> pst(cx, frame->stFrag()->pst());
    Local<AST::PackedCallExprNode> callExprNode(cx, callNodeRef->astCallExpr());

    // Assemble an array of SyntaxTreeFragment pointers.
    LocalArray<SyntaxTreeFragment*> operandExprs(cx, callExprNode->numArgs());
    for (uint32_t i = 0; i < callExprNode->numArgs(); i++) {
        uint32_t offset = callExprNode->arg(i).offset();
        if (!operandExprs.setResult(i,
                SyntaxNode::Create(cx->inHatchery(), pst, offset)))
        {
            return ErrorVal();
        }
    }

    // Invoke the applicative function.
    Local<CallResult> result(cx, Interp::InvokeOperativeFunction(
            cx, frame, callerScope, callee, calleeFunc, operandExprs));
    Local<Frame*> parent(cx, frame->parent());

    if (result->isError())
        return Frame::Resolve(cx, parent, EvalResult::Error());

    if (result->isExc())
        return Frame::Resolve(cx, parent, result->excAsEvalResult());

    if (result->isValue())
        return Frame::Resolve(cx, parent, result->valueAsEvalResult());

    if (result->isVoid())
        return Frame::Resolve(cx, parent, EvalResult::Void());

    if (result->isContinue())
        return StepResult::Continue(result->continueFrame());

    WH_UNREACHABLE("Unknown CallResult outcome.");
    return cx->setError(RuntimeError::InternalError,
                        "Unknown CallResult outcome.");
}

/* static */ Result<NativeCallResumeFrame*>
NativeCallResumeFrame::Create(AllocationContext acx,
                              Handle<Frame*> parent,
                              Handle<NativeCallInfo> callInfo,
                              Handle<ScopeObject*> evalScope,
                              Handle<SyntaxTreeFragment*> syntaxFragment,
                              NativeCallResumeFuncPtr resumeFunc,
                              Handle<HeapThing*> resumeState)
{
    return acx.create<NativeCallResumeFrame>(parent, callInfo, evalScope,
                                             syntaxFragment, resumeFunc,
                                             resumeState);
}

/* static */ StepResult
NativeCallResumeFrame::ResolveImpl(ThreadContext* cx,
                                   Handle<NativeCallResumeFrame*> frame,
                                   Handle<EvalResult> result)
{
    Local<Frame*> parent(cx, frame->parent());

    // When the child completes, call into the native resume func.
    NativeCallResumeFuncPtr resumeFunc = frame->resumeFunc();

    Local<NativeCallInfo> callInfo(cx,
        NativeCallInfo(parent,
                       frame->lookupState(),
                       frame->callerScope(),
                       frame->calleeFunc(),
                       frame->receiver()));
    Local<HeapThing*> resumeState(cx, frame->resumeState());

    Local<CallResult> resumeResult(cx,
        resumeFunc(cx, callInfo, resumeState, result));

    if (resumeResult->isError())
        return Frame::Resolve(cx, parent, EvalResult::Error());

    if (resumeResult->isExc())
        return Frame::Resolve(cx, parent, resumeResult->excAsEvalResult());

    if (resumeResult->isValue())
        return Frame::Resolve(cx, parent, resumeResult->valueAsEvalResult());

    if (resumeResult->isVoid())
        return Frame::Resolve(cx, parent, EvalResult::Void());

    if (resumeResult->isContinue())
        return StepResult::Continue(resumeResult->continueFrame());

    WH_UNREACHABLE("Unknown CallResult.");
    return ErrorVal();
}

/* static */ StepResult
NativeCallResumeFrame::StepImpl(ThreadContext* cx,
                                Handle<NativeCallResumeFrame*> frame)
{
    Local<VM::SyntaxTreeFragment*> stFrag(cx, frame->syntaxFragment());
    Local<VM::ScopeObject*> evalScope(cx, frame->evalScope());

    // Create an EntryFrame for the evaluation of the syntax tree fragment.
    Local<EntryFrame*> entryFrame(cx);
    if (!entryFrame.setResult(EntryFrame::Create(cx->inHatchery(),
                                                 frame, stFrag, evalScope)))
    {
        return ErrorVal();
    }

    // Continue to it.
    return StepResult::Continue(entryFrame);
}


} // namespace VM
} // namespace Whisper
