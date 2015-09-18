
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
    static Result<ControlFlow> Lift_##name( \
        ThreadContext *cx, \
        Handle<NativeCallInfo> callInfo, \
        ArrayHandle<SyntaxTreeRef> args, \
        MutHandle<ValBox> resultOut);

    DECLARE_LIFT_FN_(File)
    DECLARE_LIFT_FN_(VarStmt)
    DECLARE_LIFT_FN_(DefStmt)
    DECLARE_LIFT_FN_(ExprStmt)
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

    if (!BindGlobalMethod(acx, obj, rtState->nm_AtFile(), &Lift_File))
        return ErrorVal();

    if (!BindGlobalMethod(acx, obj, rtState->nm_AtVarStmt(), &Lift_VarStmt))
        return ErrorVal();

    if (!BindGlobalMethod(acx, obj, rtState->nm_AtDefStmt(), &Lift_DefStmt))
        return ErrorVal();

    if (!BindGlobalMethod(acx, obj, rtState->nm_AtExprStmt(), &Lift_ExprStmt))
        return ErrorVal();

    if (!BindGlobalMethod(acx, obj, rtState->nm_AtIntegerExpr(),
                          &Lift_IntegerExpr))
    {
        return ErrorVal();
    }

    return OkVal();
}

#define IMPL_LIFT_FN_(name) \
    static Result<ControlFlow> Lift_##name( \
        ThreadContext *cx, \
        Handle<NativeCallInfo> callInfo, \
        ArrayHandle<SyntaxTreeRef> args, \
        MutHandle<ValBox> resultOut)

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
    Local<ValBox> stmtResult(cx);

    SpewInterpNote("Lift_File: Interpreting %u statements",
                   unsigned(fileNode->numStatements()));
    for (uint32_t i = 0; i < fileNode->numStatements(); i++) {
        Local<AST::PackedBaseNode> stmtNode(cx, fileNode->statement(i));
        SpewInterpNote("Lift_File: statement %u is %s",
                       unsigned(i), AST::NodeTypeString(stmtNode->type()));

        if (!InterpretSyntax(cx, callInfo->callerScope(), pst,
                             stmtNode->offset(), &stmtResult))
        {
            return ErrorVal();
        }
    }

    resultOut = ValBox::Undefined();
    return OkVal(ControlFlow::Void);
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
            if (!InterpretSyntax(cx, callInfo->callerScope(), pst,
                                 exprNode->offset(), &varvalBox))
            {
                return ErrorVal();
            }
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
    resultOut = ValBox::Undefined();
    return OkVal(ControlFlow::Void);
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
    
    resultOut = ValBox::Undefined();
    return OkVal(ControlFlow::Void);
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

    if (!InterpretSyntax(cx, callInfo->callerScope(), pst,
                         exprNode->offset(), resultOut))
    {
        return ErrorVal();
    }

    return OkVal(ControlFlow::Value);
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
    resultOut = ValBox::Integer(intExpr->value());
    return OkVal(ControlFlow::Value);
}


} // namespace VM
} // namespace Whisper
