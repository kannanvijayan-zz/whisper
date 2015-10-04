
#include <iostream>

#include "parser/syntax_defn.hpp"
#include "parser/packed_syntax.hpp"
#include "runtime_inlines.hpp"
#include "vm/global_scope.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "interp/interpreter.hpp"
#include "interp/syntax_behaviour.hpp"

namespace Whisper {
namespace Interp {


// Declare a lift function for each syntax node type.
#define DECLARE_SYNTAX_FN_(name) \
    static VM::ControlFlow Syntax_##name( \
        ThreadContext* cx, \
        Handle<VM::NativeCallInfo> callInfo, \
        ArrayHandle<VM::SyntaxNodeRef> args);

    DECLARE_SYNTAX_FN_(File)

    DECLARE_SYNTAX_FN_(EmptyStmt)
    DECLARE_SYNTAX_FN_(ExprStmt)
    DECLARE_SYNTAX_FN_(ReturnStmt)
    DECLARE_SYNTAX_FN_(DefStmt)
    DECLARE_SYNTAX_FN_(ConstStmt)
    DECLARE_SYNTAX_FN_(VarStmt)

    DECLARE_SYNTAX_FN_(CallExpr)
    DECLARE_SYNTAX_FN_(DotExpr)
    DECLARE_SYNTAX_FN_(PosExpr)
    DECLARE_SYNTAX_FN_(NegExpr)
    DECLARE_SYNTAX_FN_(AddExpr)
    DECLARE_SYNTAX_FN_(SubExpr)
    DECLARE_SYNTAX_FN_(ParenExpr)
    DECLARE_SYNTAX_FN_(NameExpr)
    DECLARE_SYNTAX_FN_(IntegerExpr)
    //WHISPER_DEFN_SYNTAX_NODES(DECLARE_SYNTAX_FN_)

#undef DECLARE_SYNTAX_FN_

static OkResult
BindGlobalMethod(AllocationContext acx,
                 Handle<VM::GlobalScope*> obj,
                 VM::String* name,
                 VM::NativeOperativeFuncPtr opFunc)
{
    Local<VM::String*> rootedName(acx, name);

    // Allocate NativeFunction object.
    Local<VM::NativeFunction*> natF(acx);
    if (!natF.setResult(VM::NativeFunction::Create(acx, opFunc)))
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

OkResult
BindSyntaxHandlers(AllocationContext acx, VM::GlobalScope* scope)
{
    Local<VM::GlobalScope*> rootedScope(acx, scope);

    ThreadContext* cx = acx.threadContext();
    Local<VM::RuntimeState*> rtState(acx, cx->runtimeState());

#define BIND_GLOBAL_METHOD_(name) \
    do { \
        if (!BindGlobalMethod(acx, rootedScope, rtState->nm_At##name(), \
                              &Syntax_##name)) \
        { \
            return ErrorVal(); \
        } \
    } while(false)

    BIND_GLOBAL_METHOD_(File);

    BIND_GLOBAL_METHOD_(EmptyStmt);
    BIND_GLOBAL_METHOD_(ExprStmt);
    BIND_GLOBAL_METHOD_(ReturnStmt);
    BIND_GLOBAL_METHOD_(DefStmt);
    BIND_GLOBAL_METHOD_(ConstStmt);
    BIND_GLOBAL_METHOD_(VarStmt);

    BIND_GLOBAL_METHOD_(CallExpr);
    BIND_GLOBAL_METHOD_(DotExpr);
    BIND_GLOBAL_METHOD_(PosExpr);
    BIND_GLOBAL_METHOD_(NegExpr);
    BIND_GLOBAL_METHOD_(AddExpr);
    BIND_GLOBAL_METHOD_(SubExpr);
    BIND_GLOBAL_METHOD_(ParenExpr);
    BIND_GLOBAL_METHOD_(NameExpr);
    BIND_GLOBAL_METHOD_(IntegerExpr);

#undef BIND_GLOBAL_METHOD_

    return OkVal();
}

#define IMPL_SYNTAX_FN_(name) \
    static VM::ControlFlow Syntax_##name( \
        ThreadContext* cx, \
        Handle<VM::NativeCallInfo> callInfo, \
        ArrayHandle<VM::SyntaxNodeRef> args)

IMPL_SYNTAX_FN_(File)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@File called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::File);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedFileNode> fileNode(cx,
        AST::PackedFileNode(pst->data(), stRef->offset()));

    SpewInterpNote("Syntax_File: Interpreting %u statements",
                   unsigned(fileNode->numStatements()));
    for (uint32_t i = 0; i < fileNode->numStatements(); i++) {
        Local<AST::PackedBaseNode> stmtNode(cx, fileNode->statement(i));
        SpewInterpNote("Syntax_File: statement %u is %s",
                       unsigned(i), AST::NodeTypeString(stmtNode->type()));

        VM::ControlFlow stmtFlow = InterpretSyntax(cx, callInfo->callerScope(),
                                                   pst, stmtNode->offset());
        WH_ASSERT(stmtFlow.isStatementResult());

        // Statements can yield void or value control flows and still
        // continue.
        if (stmtFlow.isVoid() || stmtFlow.isValue())
            continue;

        return stmtFlow;
    }

    return VM::ControlFlow::Void();
}

IMPL_SYNTAX_FN_(EmptyStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@EmptyStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::EmptyStmt);
    return VM::ControlFlow::Void();
}

IMPL_SYNTAX_FN_(ExprStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@ExprStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::ExprStmt);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedExprStmtNode> exprStmtNode(cx,
        AST::PackedExprStmtNode(pst->data(), stRef->offset()));

    Local<AST::PackedBaseNode> exprNode(cx, exprStmtNode->expression());

    VM::ControlFlow exprFlow = InterpretSyntax(cx, callInfo->callerScope(), pst,
                                               exprNode->offset());
    // An expression should only ever resolve to a value, error, or exception.
    WH_ASSERT(exprFlow.isExpressionResult());
    return exprFlow;
}

IMPL_SYNTAX_FN_(ReturnStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@ReturnStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::ReturnStmt);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedReturnStmtNode> returnStmtNode(cx,
        AST::PackedReturnStmtNode(pst->data(), stRef->offset()));

    // If it's a bare return, resolve to a return control flow with
    // an undefined value.
    if (!returnStmtNode->hasExpression()) {
        SpewInterpNote("Syntax_ReturnStmt: Empty return.");
        return VM::ControlFlow::Return(VM::ValBox::Undefined());
    }

    SpewInterpNote("Syntax_ReturnStmt: Evaluating expression.");
    Local<AST::PackedBaseNode> exprNode(cx, returnStmtNode->expression());
    VM::ControlFlow exprFlow = InterpretSyntax(cx, callInfo->callerScope(), pst,
                                               exprNode->offset());
    // An expression should only ever resolve to a value, error, or exception.
    WH_ASSERT(exprFlow.isExpressionResult());
    // If a value, wrap the value in a return, otherwise return straight.
    if (exprFlow.isValue())
        return VM::ControlFlow::Return(exprFlow.value());

    return exprFlow;
}

IMPL_SYNTAX_FN_(DefStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@DefStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::DefStmt);

    Local<VM::ValBox> receiverBox(cx, callInfo->receiver());
    if (receiverBox->isPrimitive())
        return cx->setExceptionRaised("Cannot define method on primitive.");
    Local<VM::Wobject*> receiver(cx, receiverBox->objectPointer());

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedDefStmtNode> defStmtNode(cx,
        AST::PackedDefStmtNode(pst->data(), stRef->offset()));

    AllocationContext acx = cx->inHatchery();

    // Create the scripted function.
    Local<VM::ScriptedFunction*> func(cx);
    if (!func.setResult(VM::ScriptedFunction::Create(
            acx, pst, stRef->offset(), callInfo->callerScope(), false)))
    {
        return ErrorVal();
    }

    // Bind the name to the function.
    Local<VM::String*> funcname(cx,
        pst->getConstantString(defStmtNode->nameCid()));
    Local<VM::PropertyDescriptor> descr(cx, VM::PropertyDescriptor(func.get()));
    if (!VM::Wobject::DefineProperty(acx, receiver, funcname, descr))
        return ErrorVal();

    SpewInterpNote("DefStmt defining '%s' on receiver %p",
            funcname->c_chars(), receiver.get());

    return VM::ControlFlow::Void();
}

IMPL_SYNTAX_FN_(ConstStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@ConstStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::ConstStmt);

    Local<VM::ValBox> receiverBox(cx, callInfo->receiver());
    if (receiverBox->isPrimitive())
        return cx->setExceptionRaised("Cannot define var on primitive.");
    Local<VM::Wobject*> receiver(cx, receiverBox->objectPointer());

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedConstStmtNode> constStmtNode(cx,
        AST::PackedConstStmtNode(pst->data(), stRef->offset()));
    Local<VM::String*> varname(cx);

    AllocationContext acx = cx->inHatchery();

    // Iterate through all bindings.
    SpewInterpNote("Syntax_ConstStmt: Defining %u consts!",
                   unsigned(constStmtNode->numBindings()));
    for (uint32_t i = 0; i < constStmtNode->numBindings(); i++) {
        varname = pst->getConstantString(constStmtNode->varnameCid(i));

        SpewInterpNote("Syntax_ConstStmt var %d evaluating initial value!",
                       unsigned(i));
        Local<AST::PackedBaseNode> exprNode(cx, constStmtNode->varexpr(i));
        VM::ControlFlow varExprFlow =
            InterpretSyntax(cx, callInfo->callerScope(), pst,
                            exprNode->offset());

        // The underlying expression can return a value, error out,
        // or throw an exception.  It should never conclude with
        // a void control flow, or a return control flow.
        WH_ASSERT(varExprFlow.isExpressionResult());
        if (!varExprFlow.isValue())
            return varExprFlow;
        Local<VM::ValBox> varvalBox(cx, varExprFlow.value());

        WH_ASSERT_IF(varvalBox->isPointer(),
            VM::Wobject::IsWobject(varvalBox->pointer<HeapThing>()));

        // Bind the name and value onto the receiver.
        Local<VM::PropertyDescriptor> descr(cx,
            VM::PropertyDescriptor(varvalBox));
        if (!VM::Wobject::DefineProperty(acx, receiver, varname, descr))
            return ErrorVal();
    }

    return VM::ControlFlow::Void();
}


IMPL_SYNTAX_FN_(VarStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@VarStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::VarStmt);

    Local<VM::ValBox> receiverBox(cx, callInfo->receiver());
    if (receiverBox->isPrimitive())
        return cx->setExceptionRaised("Cannot define var on primitive.");
    Local<VM::Wobject*> receiver(cx, receiverBox->objectPointer());

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedVarStmtNode> varStmtNode(cx,
        AST::PackedVarStmtNode(pst->data(), stRef->offset()));
    Local<VM::String*> varname(cx);
    Local<VM::ValBox> varvalBox(cx);

    AllocationContext acx = cx->inHatchery();

    // Iterate through all bindings.
    SpewInterpNote("Syntax_VarStmt: Defining %u vars!",
                   unsigned(varStmtNode->numBindings()));
    for (uint32_t i = 0; i < varStmtNode->numBindings(); i++) {
        varname = pst->getConstantString(varStmtNode->varnameCid(i));

        if (varStmtNode->hasVarexpr(i)) {
            SpewInterpNote("Syntax_VarStmt var %d evaluating initial value!",
                           unsigned(i));
            Local<AST::PackedBaseNode> exprNode(cx, varStmtNode->varexpr(i));
            VM::ControlFlow varExprFlow =
                InterpretSyntax(cx, callInfo->callerScope(), pst,
                                exprNode->offset());

            // The underlying expression can return a value, error out,
            // or throw an exception.  It should never conclude with
            // a void control flow, or a return control flow.
            WH_ASSERT(varExprFlow.isExpressionResult());
            if (!varExprFlow.isValue())
                return varExprFlow;
            varvalBox = varExprFlow.value();

        } else {
            varvalBox = VM::ValBox::Undefined();
        }

        WH_ASSERT_IF(varvalBox->isPointer(),
            VM::Wobject::IsWobject(varvalBox->pointer<HeapThing>()));

        // Bind the name and value onto the receiver.
        Local<VM::PropertyDescriptor> descr(cx,
            VM::PropertyDescriptor(varvalBox));
        if (!VM::Wobject::DefineProperty(acx, receiver, varname, descr))
            return ErrorVal();
    }

    return VM::ControlFlow::Void();
}

IMPL_SYNTAX_FN_(CallExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@CallExpr called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::CallExpr);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedCallExprNode> callExpr(cx,
        AST::PackedCallExprNode(pst->data(), stRef->offset()));

    // Evaluate the call target.
    Local<AST::PackedBaseNode> calleeExpr(cx, callExpr->callee());
    VM::ControlFlow calleeFlow =
        InterpretSyntax(cx, callInfo->callerScope(), pst,
                        calleeExpr->offset());

    WH_ASSERT(calleeFlow.isExpressionResult());
    if (!calleeFlow.isValue())
        return calleeFlow;
    Local<VM::ValBox> calleeBox(cx, calleeFlow.value());

    // Ensure callee is a function object.
    if (!calleeBox->isPointerTo<VM::FunctionObject>())
        return cx->setExceptionRaised("Cannot call non-function");

    // Obtain callee function.
    Local<VM::FunctionObject*> calleeFunc(cx,
        calleeBox->pointer<VM::FunctionObject>());

    // Compose array of syntax tree references.
    uint32_t numArgs = callExpr->numArgs();
    LocalArray<VM::SyntaxNodeRef> stRefs(cx, numArgs);
    for (uint32_t i = 0; i < numArgs; i++)
        stRefs[i] = VM::SyntaxNodeRef(pst, callExpr->arg(i).offset());

    // Check if callee is operative or applicaive.
    if (calleeFunc->isOperative()) {
        // Invoke the operative function.
        return InvokeOperativeFunction(cx, callInfo->callerScope(),
                                       calleeFunc, stRefs);
    } else {
        // Invoke the operative function.
        return InvokeApplicativeFunction(cx, callInfo->callerScope(),
                                         calleeFunc, stRefs);
    }
}

IMPL_SYNTAX_FN_(DotExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@DotExpr called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::DotExpr);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedDotExprNode> dotExpr(cx,
        AST::PackedDotExprNode(pst->data(), stRef->offset()));

    // Evaluate the lookup target.
    Local<AST::PackedBaseNode> targetExpr(cx, dotExpr->target());
    VM::ControlFlow targetFlow =
        InterpretSyntax(cx, callInfo->callerScope(), pst,
                        targetExpr->offset());
    WH_ASSERT(targetFlow.isExpressionResult());
    if (!targetFlow.isValue())
        return targetFlow;

    // Look up the '@DotExpr' method on the target.
    Local<VM::ValBox> targetBox(cx, targetFlow.value());
    Local<VM::String*> dotExprName(cx, cx->runtimeState()->nm_AtDotExpr());
    VM::ControlFlow lookupFlow = GetValueProperty(cx, targetBox, dotExprName);
    WH_ASSERT(lookupFlow.isPropertyLookupResult());

    if (lookupFlow.isVoid())
        return cx->setExceptionRaised("@DotExpr() not found on object.");

    if (!lookupFlow.isValue())
        return lookupFlow;

    // Invoke "@DotExpr" result as an operative.
    Local<VM::ValBox> dotHandler(cx, lookupFlow.value());

    return InvokeOperativeValue(cx, callInfo->callerScope(),
                                dotHandler, stRef);
}

IMPL_SYNTAX_FN_(PosExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@PosExpr called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::PosExpr);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedPosExprNode> posExpr(cx,
        AST::PackedPosExprNode(pst->data(), stRef->offset()));

    // Evaluate the lookup target.
    Local<AST::PackedBaseNode> subExpr(cx, posExpr->subexpr());
    VM::ControlFlow subFlow =
        InterpretSyntax(cx, callInfo->callerScope(), pst, subExpr->offset());
    WH_ASSERT(subFlow.isExpressionResult());
    if (!subFlow.isValue())
        return subFlow;

    // Look up the '@PosExpr' method on the target.
    Local<VM::ValBox> value(cx, subFlow.value());
    Local<VM::String*> posExprName(cx, cx->runtimeState()->nm_AtPosExpr());
    VM::ControlFlow lookupFlow = GetValueProperty(cx, value, posExprName);
    WH_ASSERT(lookupFlow.isPropertyLookupResult());

    if (lookupFlow.isVoid())
        return cx->setExceptionRaised("@PosExpr() not found on object.");

    if (!lookupFlow.isValue())
        return lookupFlow;

    // Invoke "@PosExpr" result as an operative, no arguments.
    Local<VM::ValBox> posHandler(cx, lookupFlow.value());

    return InvokeValue(cx,
        callInfo->callerScope(), posHandler,
        ArrayHandle<VM::SyntaxNodeRef>::Empty());
}

IMPL_SYNTAX_FN_(NegExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@NegExpr called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::NegExpr);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedNegExprNode> negExpr(cx,
        AST::PackedNegExprNode(pst->data(), stRef->offset()));

    // Evaluate the lookup target.
    Local<AST::PackedBaseNode> subExpr(cx, negExpr->subexpr());
    VM::ControlFlow subFlow =
        InterpretSyntax(cx, callInfo->callerScope(), pst, subExpr->offset());
    WH_ASSERT(subFlow.isExpressionResult());
    if (!subFlow.isValue())
        return subFlow;

    // Look up the '@NegExpr' method on the target.
    Local<VM::ValBox> value(cx, subFlow.value());
    Local<VM::String*> negExprName(cx, cx->runtimeState()->nm_AtNegExpr());
    VM::ControlFlow lookupFlow = GetValueProperty(cx, value, negExprName);
    WH_ASSERT(lookupFlow.isPropertyLookupResult());

    if (lookupFlow.isVoid())
        return cx->setExceptionRaised("@NegExpr() not found on object.");

    if (!lookupFlow.isValue())
        return lookupFlow;

    // Invoke "@NegExpr" result as an operative, no arguments.
    Local<VM::ValBox> negHandler(cx, lookupFlow.value());

    return InvokeValue(cx,
        callInfo->callerScope(), negHandler,
        ArrayHandle<VM::SyntaxNodeRef>::Empty());
}

IMPL_SYNTAX_FN_(AddExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@AddExpr called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::AddExpr);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedAddExprNode> addExpr(cx,
        AST::PackedAddExprNode(pst->data(), stRef->offset()));

    // Evaluate the lookup target.
    Local<AST::PackedBaseNode> lhsExpr(cx, addExpr->lhs());
    VM::ControlFlow lhsFlow =
        InterpretSyntax(cx, callInfo->callerScope(), pst, lhsExpr->offset());
    WH_ASSERT(lhsFlow.isExpressionResult());
    if (!lhsFlow.isValue())
        return lhsFlow;

    // Look up the '@AddExpr' method on the target.
    Local<VM::ValBox> lhsValue(cx, lhsFlow.value());
    Local<VM::String*> addExprName(cx, cx->runtimeState()->nm_AtAddExpr());
    VM::ControlFlow lookupFlow = GetValueProperty(cx, lhsValue, addExprName);
    WH_ASSERT(lookupFlow.isPropertyLookupResult());

    if (lookupFlow.isVoid())
        return cx->setExceptionRaised("@AddExpr() not found on object.");

    if (!lookupFlow.isValue())
        return lookupFlow;

    Local<VM::ValBox> addHandler(cx, lookupFlow.value());

    // Invoke "@AddExpr" method, pass rhs as argument.
    Local<VM::SyntaxNodeRef> rhsExpr(cx,
        VM::SyntaxNodeRef(pst, addExpr->rhs().offset()));

    return InvokeValue(cx,
        callInfo->callerScope(), addHandler,
        ArrayHandle<VM::SyntaxNodeRef>(rhsExpr));
}

IMPL_SYNTAX_FN_(SubExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@SubExpr called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::SubExpr);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedSubExprNode> subExpr(cx,
        AST::PackedSubExprNode(pst->data(), stRef->offset()));

    // Evaluate the lookup target.
    Local<AST::PackedBaseNode> lhsExpr(cx, subExpr->lhs());
    VM::ControlFlow lhsFlow =
        InterpretSyntax(cx, callInfo->callerScope(), pst, lhsExpr->offset());
    WH_ASSERT(lhsFlow.isExpressionResult());
    if (!lhsFlow.isValue())
        return lhsFlow;

    // Look up the '@SubExpr' method on the target.
    Local<VM::ValBox> lhsValue(cx, lhsFlow.value());
    Local<VM::String*> subExprName(cx, cx->runtimeState()->nm_AtSubExpr());
    VM::ControlFlow lookupFlow = GetValueProperty(cx, lhsValue, subExprName);
    WH_ASSERT(lookupFlow.isPropertyLookupResult());

    if (lookupFlow.isVoid())
        return cx->setExceptionRaised("@SubExpr() not found on object.");

    if (!lookupFlow.isValue())
        return lookupFlow;

    Local<VM::ValBox> subHandler(cx, lookupFlow.value());

    // Invoke "@SubExpr" method, pass rhs as argument.
    Local<VM::SyntaxNodeRef> rhsExpr(cx,
        VM::SyntaxNodeRef(pst, subExpr->rhs().offset()));

    return InvokeValue(cx,
        callInfo->callerScope(), subHandler,
        ArrayHandle<VM::SyntaxNodeRef>(rhsExpr));
}

IMPL_SYNTAX_FN_(ParenExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@ParenExpr called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::ParenExpr);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedParenExprNode> parenExpr(cx,
        AST::PackedParenExprNode(pst->data(), stRef->offset()));

    Local<AST::PackedBaseNode> subexprNode(cx, parenExpr->subexpr());
    VM::ControlFlow exprFlow =
        InterpretSyntax(cx, callInfo->callerScope(), pst,
                        subexprNode->offset());
    WH_ASSERT(exprFlow.isExpressionResult());
    return exprFlow;
}

IMPL_SYNTAX_FN_(NameExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@NameExpr called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::NameExpr);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedNameExprNode> nameExpr(cx,
        AST::PackedNameExprNode(pst->data(), stRef->offset()));

    // Get the scope to look up on.
    Local<VM::Wobject*> scopeObj(cx,
        callInfo->callerScope().convertTo<VM::Wobject*>());

    // Get the constant name to look up.
    Local<VM::String*> name(cx, pst->getConstantString(nameExpr->nameCid()));

    SpewInterpNote("Syntax_NameExpr: Looking up '%s' on scope %p!",
                name->c_chars(), scopeObj.get());

    // Do the lookup.
    Local<VM::LookupState*> lookupState(cx);
    Local<VM::PropertyDescriptor> propDesc(cx);

    VM::ControlFlow propFlow = GetObjectProperty(cx, scopeObj, name);
    WH_ASSERT(propFlow.isPropertyLookupResult());
    if (propFlow.isValue())
        return VM::ControlFlow::Value(propFlow.value());

    // Void control flow means that property was not found.
    // Throw an error.
    if (propFlow.isVoid())
        return cx->setExceptionRaised("Name not found", name.get());

    return propFlow;
}

IMPL_SYNTAX_FN_(IntegerExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@IntegerExpr called with wrong number of arguments.");
    }
    SpewInterpNote("Syntax_IntegerExpr: Returning integer!");

    WH_ASSERT(args.get(0).nodeType() == AST::IntegerExpr);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0));
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedIntegerExprNode> intExpr(cx,
        AST::PackedIntegerExprNode(pst->data(), stRef->offset()));

    // Make an integer box and return it.
    return VM::ControlFlow::Value(VM::ValBox::Integer(intExpr->value()));
}


} // namespace Interp
} // namespace Whisper
