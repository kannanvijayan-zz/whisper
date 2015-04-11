
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
GlobalScope::GetDelegates(ThreadContext *cx,
                          Handle<GlobalScope *> obj,
                          MutHandle<Array<Wobject *> *> delegatesOut)
{
    HashObject::GetDelegates(cx,
        Handle<HashObject *>::Convert(obj),
        delegatesOut);
}

/* static */ bool
GlobalScope::GetProperty(ThreadContext *cx,
                         Handle<GlobalScope *> obj,
                         Handle<String *> name,
                         MutHandle<PropertyDescriptor> result)
{
    return HashObject::GetProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, result);
}

/* static */ OkResult
GlobalScope::DefineProperty(ThreadContext *cx,
                            Handle<GlobalScope *> obj,
                            Handle<String *> name,
                            Handle<PropertyDescriptor> defn)
{
    return HashObject::DefineProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, defn);
}

// Declare a lift function for each syntax node type.
#define DECLARE_LIFT_FN_(name) \
    static OkResult Lift_##name( \
        ThreadContext *cx, \
        Handle<NativeCallInfo> callInfo, \
        ArrayHandle<SyntaxTreeRef> args, \
        MutHandle<Box> resultOut);

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
    ThreadContext *cx = acx.threadContext();
    Local<String *> rootedName(acx, name);

    // Allocate NativeFunction object.
    Local<NativeFunction *> natF(cx);
    if (!natF.setResult(NativeFunction::Create(acx, opFunc)))
        return ErrorVal();
    Local<PropertyDescriptor> desc(cx, PropertyDescriptor(natF.get()));

    // Bind method on global.
    if (!GlobalScope::DefineProperty(cx, obj, rootedName, desc))
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
    static OkResult Lift_##name( \
        ThreadContext *cx, \
        Handle<NativeCallInfo> callInfo, \
        ArrayHandle<SyntaxTreeRef> args, \
        MutHandle<Box> resultOut)

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
    Local<Box> stmtResult(cx);

    SpewInterpNote("Lift_File: Interpreting %u statements",
                   unsigned(fileNode->numStatements()));
    SpewInterpNote("Lift_File: Receiver is %s",
               HeapThing::From(callInfo->receiver().get())->header().formatString());
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

    resultOut = Box::Undefined();
    return OkVal();
}

IMPL_LIFT_FN_(VarStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@VarStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::VarStmt);

    Local<Wobject *> receiver(cx, callInfo->receiver());
    Local<SyntaxTreeRef> stRef(cx, args.get(0));
    Local<PackedSyntaxTree *> pst(cx, stRef->pst());
    Local<AST::PackedVarStmtNode> varStmtNode(cx,
        AST::PackedVarStmtNode(pst->data(), stRef->offset()));
    Local<Box> varnameBox(cx);
    Local<String *> varname(cx);
    Local<Box> varvalBox(cx);

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
            varvalBox = Box::Undefined();
        }

        WH_ASSERT_IF(varvalBox->isPointer(),
            Wobject::IsWobject(varvalBox->pointer<HeapThing>()));

        // Bind the name and value onto the receiver.
        Local<PropertyDescriptor> descr(cx, PropertyDescriptor(varvalBox));
        if (!Wobject::DefineProperty(cx, receiver, varname, descr))
            return ErrorVal();
    }

    // TODO: Implement VarStmt
    resultOut = Box::Undefined();
    return OkVal();
}

IMPL_LIFT_FN_(DefStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@DefStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::DefStmt);

    Local<Wobject *> receiver(cx, callInfo->receiver());
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
    if (!Wobject::DefineProperty(cx, receiver, funcname, descr))
        return ErrorVal();
    
    resultOut = Box::Undefined();
    return OkVal();
}

IMPL_LIFT_FN_(ExprStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@ExprStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0).nodeType() == AST::ExprStmt);

    // TODO: Implement VarStmt
    SpewInterpNote("Lift_ExprStmt: Interpreting!\n");
    resultOut = Box::Undefined();
    return OkVal();
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
    resultOut = Box::Integer(intExpr->value());
    return OkVal();
}


} // namespace VM
} // namespace Whisper
