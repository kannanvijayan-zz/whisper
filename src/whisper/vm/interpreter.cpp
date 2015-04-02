
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
    WH_ASSERT(cx->lastFrame() == nullptr);
    WH_ASSERT(file.get() != nullptr);
    WH_ASSERT(scope.get() != nullptr);

    // Try to make a function for the script.
    Local<PackedSyntaxTree *> st(cx);
    if (!st.setResult(SourceFile::ParseSyntaxTree(cx, file)))
        return OkResult::Error();

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
    WH_ASSERT(cx->lastFrame() == nullptr);
    WH_ASSERT(scope.get() != nullptr);
    WH_ASSERT(pst.get() != nullptr);

    Local<AST::PackedBaseNode> node(cx,
        AST::PackedBaseNode(pst->data(), offset));

    // Interpret values based on type.
    switch (node->type()) {
      case AST::File:
      case AST::EmptyStmt:
      case AST::ExprStmt:
      case AST::ReturnStmt:
      case AST::IfStmt:
      case AST::DefStmt:
      case AST::ConstStmt:
      case AST::VarStmt:
      case AST::LoopStmt:
      case AST::CallExpr:
      case AST::DotExpr:
      case AST::ArrowExpr:
      case AST::PosExpr:
      case AST::NegExpr:
      case AST::AddExpr:
      case AST::SubExpr:
      case AST::MulExpr:
      case AST::DivExpr:
      case AST::ParenExpr:
      case AST::NameExpr:
      case AST::IntegerExpr: {
        // scope.@integer(value)
        Local<String *> name(cx, cx->runtimeState()->nm_AtInteger());
        return DispatchSyntaxMethod(cx, scope, name, pst, node, resultOut);
      }
      default:
        WH_UNREACHABLE("Unknown node type.");
        return OkResult::Error();
    }
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

    // Lookup methodn ame on scope.
    Result<bool> lookupResult = Wobject::LookupProperty(
        cx, scopeObj, name, &lookupState, &propDesc);
    if (!lookupResult)
        return OkResult::Error();

    if (!lookupResult.value()) {
        cx->setError(RuntimeError::MethodLookupFailed,
                     "Could not lookup method on scope", name.get());
        return OkResult::Error();
    }

    // Found binding for scope.@integer
    // Ensure it's a method.
    WH_ASSERT(propDesc->isValid());

    if (!propDesc->isMethod()) {
        cx->setError(RuntimeError::MethodLookupFailed,
                     "Found non-method binding on scope", name.get());
        return OkResult::Error();
    }
    Local<Function *> func(cx, propDesc->method());

    // Function must be an operative.
    if (!func->isOperative()) {
        cx->setError(RuntimeError::MethodLookupFailed,
                     "Found applicative scope syntax method", name.get());
        return OkResult::Error();
    }

    // Create SyntaxTreeFragment.
    Local<SyntaxTreeFragment *> stFrag(cx);
    if (!stFrag.setResult(SyntaxTreeFragment::Create(cx->inHatchery(),
                                                     pst, node->offset())))
    {
        return OkResult::Error();
    }

    // Invoke operative function with given arguments.
    return InvokeOperativeFunction(cx, lookupState, scope, func,
                                   scopeObj, stFrag, resultOut);
}

OkResult
InvokeOperativeFunction(ThreadContext *cx,
                        Handle<LookupState *> lookupState,
                        Handle<ScopeObject *> callerScope,
                        Handle<Function *> func,
                        Handle<Wobject *> receiver,
                        Handle<SyntaxTreeFragment *> stFrag,
                        MutHandle<Box> resultOut)
{
    // Call native if native.
    if (func->isNative()) {
        Local<NativeFunction *> nativeFunc(cx, func->asNative());
        WH_ASSERT(nativeFunc->isOperative());

        NativeOperativeFuncPtr *opNatF = nativeFunc->operative();
        return (*opNatF)(cx, lookupState, callerScope, nativeFunc, receiver,
                         ArrayHandle<SyntaxTreeFragment *>(stFrag),
                         resultOut);
    }

    // If scripted, interpret the scripted function.
    if (func->isScripted()) {
        WH_ASSERT("Cannot interpret scripted operatives yet!");
        cx->setError(RuntimeError::InternalError,
                     "Cannot interpret scripted operatives yet!");
        return OkResult::Error();
    }

    WH_UNREACHABLE("Unknown function type!");
        cx->setError(RuntimeError::InternalError,
                     "Unknown function type seen!",
                     HeapThing::From(func.get()));
    return OkResult::Error();
}


} // namespace VM
} // namespace Whisper
