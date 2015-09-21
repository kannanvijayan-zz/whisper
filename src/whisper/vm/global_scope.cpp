
#include <iostream>

#include "parser/syntax_defn.hpp"
#include "parser/packed_syntax.hpp"
#include "runtime_inlines.hpp"
#include "vm/global_scope.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "vm/interpreter.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<GlobalScope *>
GlobalScope::Create(AllocationContext acx)
{
    // Allocate empty array of delegates.
    Local<Array<Wobject *> *> delegates(acx);
    if (!delegates.setResult(Array<Wobject *>::CreateEmpty(acx)))
        return ErrorVal();

    // Allocate a dictionary.
    Local<PropertyDict *> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return ErrorVal();

    // Allocate global.
    Local<GlobalScope *> global(acx);
    if (!global.setResult(acx.create<GlobalScope>(delegates.handle(),
                                                  props.handle())))
    {
        return ErrorVal();
    }

    // Bind syntax handlers to global.
    if (!BindSyntaxHandlers(acx, global))
        return ErrorVal();

    return OkVal(global.get());
}

/* static */ void
GlobalScope::GetDelegates(AllocationContext acx,
                          Handle<GlobalScope *> obj,
                          MutHandle<Array<Wobject *> *> delegatesOut)
{
    HashObject::GetDelegates(acx,
        Handle<HashObject *>::Convert(obj),
        delegatesOut);
}

/* static */ bool
GlobalScope::GetProperty(AllocationContext acx,
                         Handle<GlobalScope *> obj,
                         Handle<String *> name,
                         MutHandle<PropertyDescriptor> result)
{
    return HashObject::GetProperty(acx,
        Handle<HashObject *>::Convert(obj),
        name, result);
}

/* static */ OkResult
GlobalScope::DefineProperty(AllocationContext acx,
                            Handle<GlobalScope *> obj,
                            Handle<String *> name,
                            Handle<PropertyDescriptor> defn)
{
    return HashObject::DefineProperty(acx,
        Handle<HashObject *>::Convert(obj),
        name, defn);
}

// Declare a lift function for each syntax node type.
#define DECLARE_LIFT_FN_(name) \
    static ControlFlow Lift_##name( \
        ThreadContext *cx, \
        Handle<NativeCallInfo> callInfo, \
        ArrayHandle<SyntaxTreeRef> args);

    DECLARE_LIFT_FN_(File)
    DECLARE_LIFT_FN_(EmptyStmt)
    DECLARE_LIFT_FN_(ExprStmt)
    DECLARE_LIFT_FN_(ReturnStmt)
    DECLARE_LIFT_FN_(DefStmt)
    DECLARE_LIFT_FN_(VarStmt)
    DECLARE_LIFT_FN_(IntegerExpr)
    //WHISPER_DEFN_SYNTAX_NODES(DECLARE_LIFT_FN_)

#undef DECLARE_LIFT_FN_

static OkResult
BindGlobalMethod(AllocationContext acx,
                 Handle<GlobalScope *> obj,
                 String *name,
                 NativeOperativeFuncPtr opFunc)
{
    Local<String *> rootedName(acx, name);

    // Allocate NativeFunction object.
    Local<NativeFunction *> natF(acx);
    if (!natF.setResult(NativeFunction::Create(acx, opFunc)))
        return ErrorVal();
    Local<PropertyDescriptor> desc(acx, PropertyDescriptor(natF.get()));

    // Bind method on global.
    if (!GlobalScope::DefineProperty(acx, obj, rootedName, desc))
        return ErrorVal();

    return OkVal();
}

/* static */ OkResult
GlobalScope::BindSyntaxHandlers(AllocationContext acx,
                                Handle<GlobalScope *> obj)
{
    ThreadContext *cx = acx.threadContext();
    Local<RuntimeState *> rtState(acx, cx->runtimeState());

#define BIND_GLOBAL_METHOD_(name) \
    do { \
        if (!BindGlobalMethod(acx, obj, rtState->nm_At##name(), \
                              &Lift_##name)) \
        { \
            return ErrorVal(); \
        } \
    } while(false)

    BIND_GLOBAL_METHOD_(File);
    BIND_GLOBAL_METHOD_(EmptyStmt);
    BIND_GLOBAL_METHOD_(ExprStmt);
    BIND_GLOBAL_METHOD_(ReturnStmt);
    BIND_GLOBAL_METHOD_(DefStmt);
    BIND_GLOBAL_METHOD_(VarStmt);
    BIND_GLOBAL_METHOD_(IntegerExpr);

#undef BIND_GLOBAL_METHOD_

    return OkVal();
}

#define IMPL_LIFT_FN_(name) \
    static ControlFlow Lift_##name( \
        ThreadContext *cx, \
        Handle<NativeCallInfo> callInfo, \
        ArrayHandle<SyntaxTreeRef> args)

IMPL_LIFT_FN_(File)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@File called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::File);

    Local<SyntaxTreeRef> stRef(cx, args.get(0));
    Local<PackedSyntaxTree *> pst(cx, stRef->pst());
    Local<AST::PackedFileNode> fileNode(cx,
        AST::PackedFileNode(pst->data(), stRef->offset()));

    SpewInterpNote("Lift_File: Interpreting %u statements",
                   unsigned(fileNode->numStatements()));
    for (uint32_t i = 0; i < fileNode->numStatements(); i++) {
        Local<AST::PackedBaseNode> stmtNode(cx, fileNode->statement(i));
        SpewInterpNote("Lift_File: statement %u is %s",
                       unsigned(i), AST::NodeTypeString(stmtNode->type()));

        ControlFlow stmtFlow = InterpretSyntax(cx, callInfo->callerScope(),
                                               pst, stmtNode->offset());
        // Statements can yield void or value control flows and still
        // continue.
        if (stmtFlow.isVoid() || stmtFlow.isValue())
            continue;

        return stmtFlow;
    }

    return ControlFlow::Void();
}

IMPL_LIFT_FN_(EmptyStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@EmptyStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::EmptyStmt);
    return ControlFlow::Void();
}

IMPL_LIFT_FN_(ExprStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@ExprStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::ExprStmt);

    Local<SyntaxTreeRef> stRef(cx, args.get(0));
    Local<PackedSyntaxTree *> pst(cx, stRef->pst());
    Local<AST::PackedExprStmtNode> exprStmtNode(cx,
        AST::PackedExprStmtNode(pst->data(), stRef->offset()));

    Local<AST::PackedBaseNode> exprNode(cx, exprStmtNode->expression());

    ControlFlow exprFlow = InterpretSyntax(cx, callInfo->callerScope(), pst,
                                           exprNode->offset());
    // An expression should only ever resolve to a value, error, or exception.
    WH_ASSERT(exprFlow.isValue() ||
              exprFlow.isError() ||
              exprFlow.isException());
    return exprFlow;
}

IMPL_LIFT_FN_(ReturnStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@ReturnStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::ReturnStmt);

    Local<SyntaxTreeRef> stRef(cx, args.get(0));
    Local<PackedSyntaxTree *> pst(cx, stRef->pst());
    Local<AST::PackedReturnStmtNode> returnStmtNode(cx,
        AST::PackedReturnStmtNode(pst->data(), stRef->offset()));

    // If it's a bare return, resolve to a return control flow with
    // an undefined value.
    if (!returnStmtNode->hasExpression()) {
        SpewInterpNote("Lift_ReturnStmt: Empty return.");
        return ControlFlow::Return(ValBox::Undefined());
    }

    SpewInterpNote("Lift_ReturnStmt: Evaluating expression.");
    Local<AST::PackedBaseNode> exprNode(cx, returnStmtNode->expression());
    ControlFlow exprFlow = InterpretSyntax(cx, callInfo->callerScope(), pst,
                                           exprNode->offset());
    // An expression should only ever resolve to a value, error, or exception.
    WH_ASSERT(exprFlow.isValue() ||
              exprFlow.isError() ||
              exprFlow.isException());
    // If a value, wrap the value in a return, otherwise return straight.
    if (exprFlow.isValue())
        return ControlFlow::Return(exprFlow.value());

    return exprFlow;
}

IMPL_LIFT_FN_(DefStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@DefStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::DefStmt);

    Local<ValBox> receiverBox(cx, callInfo->receiver());
    if (receiverBox->isPrimitive())
        return cx->setExceptionRaised("Cannot define var on primitive.");
    Local<Wobject *> receiver(cx, receiverBox->objPointer());

    Local<SyntaxTreeRef> stRef(cx, args.get(0));
    Local<PackedSyntaxTree *> pst(cx, stRef->pst());
    Local<AST::PackedDefStmtNode> defStmtNode(cx,
        AST::PackedDefStmtNode(pst->data(), stRef->offset()));

    AllocationContext acx = cx->inHatchery();

    // Create the scripted function.
    Local<ScriptedFunction *> func(cx);
    if (!func.setResult(ScriptedFunction::Create(
            acx, pst, stRef->offset(), callInfo->callerScope(), false)))
    {
        return ErrorVal();
    }

    // Bind the name to the function.
    Local<Box> funcnameBox(cx, pst->getConstant(defStmtNode->nameCid()));
    WH_ASSERT(funcnameBox->isPointer());
    WH_ASSERT(funcnameBox->pointer<HeapThing>()->header().isFormat_String());
    Local<String *> funcname(cx, funcnameBox->pointer<String>());
    Local<PropertyDescriptor> descr(cx, PropertyDescriptor(func.get()));
    if (!Wobject::DefineProperty(acx, receiver, funcname, descr))
        return ErrorVal();
    
    return ControlFlow::Void();
}

IMPL_LIFT_FN_(VarStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@VarStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::VarStmt);

    Local<ValBox> receiverBox(cx, callInfo->receiver());
    if (receiverBox->isPrimitive())
        return cx->setExceptionRaised("Cannot define var on primitive.");
    Local<Wobject *> receiver(cx, receiverBox->objPointer());

    Local<SyntaxTreeRef> stRef(cx, args.get(0));
    Local<PackedSyntaxTree *> pst(cx, stRef->pst());
    Local<AST::PackedVarStmtNode> varStmtNode(cx,
        AST::PackedVarStmtNode(pst->data(), stRef->offset()));
    Local<Box> varnameBox(cx);
    Local<String *> varname(cx);
    Local<ValBox> varvalBox(cx);

    AllocationContext acx = cx->inHatchery();

    // Iterate through all bindings.
    SpewInterpNote("Lift_VarStmt: Defining %u vars!",
                   unsigned(varStmtNode->numBindings()));
    for (uint32_t i = 0; i < varStmtNode->numBindings(); i++) {
        varnameBox = pst->getConstant(varStmtNode->varnameCid(i));
        WH_ASSERT(varnameBox->isPointer());
        WH_ASSERT(varnameBox->pointer<HeapThing>()->header().isFormat_String());
        varname = varnameBox->pointer<String>();
        
        if (varStmtNode->hasVarexpr(i)) {
            SpewInterpNote("Lift_VarStmt var %d evaluating initial value!",
                           unsigned(i));
            Local<AST::PackedBaseNode> exprNode(cx, varStmtNode->varexpr(i));
            ControlFlow varExprFlow =
                InterpretSyntax(cx, callInfo->callerScope(), pst,
                                exprNode->offset());

            // The underlying expression can return a value, error out,
            // or throw an exception.  It should never conclude with
            // a void control flow, or a return control flow.
            WH_ASSERT(varExprFlow.isValue() ||
                      varExprFlow.isError() ||
                      varExprFlow.isException());
            if (!varExprFlow.isValue())
                return varExprFlow;
            varvalBox = varExprFlow.value();

        } else {
            varvalBox = ValBox::Undefined();
        }

        WH_ASSERT_IF(varvalBox->isPointer(),
            Wobject::IsWobject(varvalBox->pointer<HeapThing>()));

        // Bind the name and value onto the receiver.
        Local<PropertyDescriptor> descr(cx, PropertyDescriptor(varvalBox));
        if (!Wobject::DefineProperty(acx, receiver, varname, descr))
            return ErrorVal();
    }

    // TODO: Implement VarStmt
    return ControlFlow::Void();
}

IMPL_LIFT_FN_(IntegerExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@ExprStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::IntegerExpr);

    Local<SyntaxTreeRef> stRef(cx, args.get(0));
    Local<PackedSyntaxTree *> pst(cx, stRef->pst());
    Local<AST::PackedIntegerExprNode> intExpr(cx,
        AST::PackedIntegerExprNode(pst->data(), stRef->offset()));

    // Make an integer box and return it.
    return ControlFlow::Value(ValBox::Integer(intExpr->value()));
}


} // namespace VM
} // namespace Whisper
