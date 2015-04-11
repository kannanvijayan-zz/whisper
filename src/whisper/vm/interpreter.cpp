
#include "gc/local.hpp"
#include "parser/packed_syntax.hpp"
#include "parser/packed_reader.hpp"
#include "vm/wobject.hpp"
#include "vm/runtime_state.hpp"
#include "vm/interpreter.hpp"
#include "vm/function.hpp"

namespace Whisper {
namespace VM {


OkResult
InterpretSourceFile(ThreadContext *cx,
                    Handle<SourceFile *> file,
                    Handle<ScopeObject *> scope,
                    MutHandle<Box> resultOut)
{
    WH_ASSERT(!cx->hasLastFrame());
    WH_ASSERT(file.get() != nullptr);
    WH_ASSERT(scope.get() != nullptr);

    // Try to make a function for the script.
    Local<PackedSyntaxTree *> st(cx);
    if (!st.setResult(SourceFile::ParseSyntaxTree(cx, file)))
        return ErrorVal();

    // Interpret the syntax tree at the given offset.
    return InterpretSyntax(cx, scope, st, 0, resultOut);
}

OkResult
InterpretSyntax(ThreadContext *cx,
                Handle<ScopeObject *> scope,
                Handle<PackedSyntaxTree *> pst,
                uint32_t offset,
                MutHandle<Box> resultOut)
{
    WH_ASSERT(!cx->hasLastFrame());
    WH_ASSERT(scope.get() != nullptr);
    WH_ASSERT(pst.get() != nullptr);

    Local<AST::PackedBaseNode> node(cx,
        AST::PackedBaseNode(pst->data(), offset));
    SpewInterpNote("InterpretSyntax %s", AST::NodeTypeString(node->type()));

    // Interpret values based on type.
    Local<String *> name(cx);
    switch (node->type()) {
      case AST::File:
        // scope.@File(...)
        name = cx->runtimeState()->nm_AtFile();
        break;

      case AST::EmptyStmt:
        // scope.@EmptyStmt(...)
        name = cx->runtimeState()->nm_AtEmptyStmt();
        break;

      case AST::ExprStmt:
        // scope.@ExprStmt(...)
        name = cx->runtimeState()->nm_AtExprStmt();
        break;

      case AST::ReturnStmt:
        // scope.@ReturnStmt(...)
        name = cx->runtimeState()->nm_AtReturnStmt();
        break;

      case AST::IfStmt:
        // scope.@IfStmt(...)
        name = cx->runtimeState()->nm_AtIfStmt();
        break;

      case AST::DefStmt:
        // scope.@DefStmt(...)
        name = cx->runtimeState()->nm_AtDefStmt();
        break;

      case AST::ConstStmt:
        // scope.@ConstStmt(...)
        name = cx->runtimeState()->nm_AtConstStmt();
        break;

      case AST::VarStmt:
        // scope.@VarStmt(...)
        name = cx->runtimeState()->nm_AtVarStmt();
        break;

      case AST::LoopStmt:
        // scope.@LoopStmt(...)
        name = cx->runtimeState()->nm_AtLoopStmt();
        break;

      case AST::CallExpr:
        // scope.@CallExpr(...)
        name = cx->runtimeState()->nm_AtCallExpr();
        break;

      case AST::DotExpr:
        // scope.@DotExpr(...)
        name = cx->runtimeState()->nm_AtDotExpr();
        break;

      case AST::ArrowExpr:
        // scope.@ArrowExpr(...)
        name = cx->runtimeState()->nm_AtArrowExpr();
        break;

      case AST::PosExpr:
        // scope.@PosExpr(...)
        name = cx->runtimeState()->nm_AtPosExpr();
        break;

      case AST::NegExpr:
        // scope.@NegExpr(...)
        name = cx->runtimeState()->nm_AtNegExpr();
        break;

      case AST::AddExpr:
        // scope.@AddExpr(...)
        name = cx->runtimeState()->nm_AtAddExpr();
        break;

      case AST::SubExpr:
        // scope.@SubExpr(...)
        name = cx->runtimeState()->nm_AtSubExpr();
        break;

      case AST::MulExpr:
        // scope.@MulExpr(...)
        name = cx->runtimeState()->nm_AtMulExpr();
        break;

      case AST::DivExpr:
        // scope.@DivExpr(...)
        name = cx->runtimeState()->nm_AtDivExpr();
        break;

      case AST::ParenExpr:
        // scope.@ParenExpr(...)
        name = cx->runtimeState()->nm_AtParenExpr();
        break;

      case AST::NameExpr:
        // scope.@NameExpr(...)
        name = cx->runtimeState()->nm_AtNameExpr();
        break;

      case AST::IntegerExpr:
        // scope.@Integer(...)
        name = cx->runtimeState()->nm_AtIntegerExpr();
        break;

      default:
        WH_UNREACHABLE("Unknown node type.");
        cx->setError(RuntimeError::InternalError, "Saw unknown node type!");
        return ErrorVal();
    }

    return DispatchSyntaxMethod(cx, scope, name, pst, node, resultOut);
}

OkResult
DispatchSyntaxMethod(ThreadContext *cx,
                     Handle<ScopeObject *> scope,
                     Handle<String *> name,
                     Handle<PackedSyntaxTree *> pst,
                     Handle<AST::PackedBaseNode> node,
                     MutHandle<Box> resultOut)
{
    Local<Wobject *> scopeObj(cx, scope.convertTo<Wobject *>());
    Local<LookupState *> lookupState(cx);
    Local<PropertyDescriptor> propDesc(cx);

    // Lookup method name on scope.
    Result<bool> lookupResult = Wobject::LookupProperty(
        cx->inHatchery(), scopeObj, name, &lookupState, &propDesc);
    if (!lookupResult)
        return ErrorVal();

    if (!lookupResult.value()) {
        return cx->setExceptionRaised("Syntax method binding not found.",
                                      name.get());
    }

    // Found binding for scope.@integer
    // Ensure it's a method.
    WH_ASSERT(propDesc->isValid());

    if (!propDesc->isMethod()) {
        return cx->setExceptionRaised("Syntax method binding is not a method.",
                                      name.get());
    }
    Local<Function *> func(cx, propDesc->method());

    // Function must be an operative.
    if (!func->isOperative()) {
        return cx->setExceptionRaised("Syntax method binding is applicative.",
                                      name.get());
    }

    // Create SyntaxTreeRef.
    Local<SyntaxTreeRef> stRef(cx, SyntaxTreeRef(pst, node->offset()));

    // Invoke operative function with given arguments.
    return InvokeOperativeFunction(cx, lookupState, scope, func,
                                   scopeObj, stRef, resultOut);
}

OkResult
InvokeOperativeFunction(ThreadContext *cx,
                        Handle<LookupState *> lookupState,
                        Handle<ScopeObject *> callerScope,
                        Handle<Function *> func,
                        Handle<Wobject *> receiver,
                        Handle<SyntaxTreeRef> stRef,
                        MutHandle<Box> resultOut)
{
    // Call native if native.
    if (func->isNative()) {
        WH_ASSERT(func->asNative()->isOperative());
        Local<NativeCallInfo> callInfo(cx,
            NativeCallInfo(lookupState, callerScope,
                           func->asNative(), receiver));

        resultOut = Box::Invalid();
        NativeOperativeFuncPtr opNatF = func->asNative()->operative();
        if (!opNatF(cx, callInfo, ArrayHandle<SyntaxTreeRef>(stRef),
                    resultOut))
        {
            return ErrorVal();
        }

        // Ensure that resultOut now has a valid value.
        WH_ASSERT(resultOut->isValid());
        if (!resultOut->isValid()) {
            return cx->setError(RuntimeError::InternalError,
                "Syntax handler method did not set valid result!");
        }
        return OkVal();
    }

    // If scripted, interpret the scripted function.
    if (func->isScripted()) {
        WH_ASSERT("Cannot interpret scripted operatives yet!");
        return cx->setError(RuntimeError::InternalError,
                            "Cannot interpret scripted operatives yet!");
    }

    WH_UNREACHABLE("Unknown function type!");
    return cx->setError(RuntimeError::InternalError,
                        "Unknown function type seen!",
                        HeapThing::From(func.get()));
}


} // namespace VM
} // namespace Whisper
