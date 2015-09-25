
#include "gc/local.hpp"
#include "parser/packed_syntax.hpp"
#include "parser/packed_reader.hpp"
#include "vm/wobject.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "interp/interpreter.hpp"

namespace Whisper {
namespace Interp {


VM::ControlFlow
InterpretSourceFile(ThreadContext* cx,
                    Handle<VM::SourceFile*> file,
                    Handle<VM::ScopeObject*> scope)
{
    WH_ASSERT(!cx->hasLastFrame());
    WH_ASSERT(file.get() != nullptr);
    WH_ASSERT(scope.get() != nullptr);

    // Try to make a function for the script.
    Local<VM::PackedSyntaxTree*> st(cx);
    if (!st.setResult(VM::SourceFile::ParseSyntaxTree(cx, file)))
        return ErrorVal();

    // Interpret the syntax tree at the given offset.
    return InterpretSyntax(cx, scope, st, 0);
}

VM::ControlFlow
InterpretSyntax(ThreadContext* cx,
                Handle<VM::ScopeObject*> scope,
                Handle<VM::PackedSyntaxTree*> pst,
                uint32_t offset)
{
    WH_ASSERT(!cx->hasLastFrame());
    WH_ASSERT(scope.get() != nullptr);
    WH_ASSERT(pst.get() != nullptr);

    Local<AST::PackedBaseNode> node(cx,
        AST::PackedBaseNode(pst->data(), offset));
    SpewInterpNote("InterpretSyntax %s", AST::NodeTypeString(node->type()));

    // Interpret values based on type.
    Local<VM::String*> name(cx);
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

    return DispatchSyntaxMethod(cx, scope, name, pst, node);
}

VM::ControlFlow
DispatchSyntaxMethod(ThreadContext* cx,
                     Handle<VM::ScopeObject*> scope,
                     Handle<VM::String*> name,
                     Handle<VM::PackedSyntaxTree*> pst,
                     Handle<AST::PackedBaseNode> node)
{
    Local<VM::Wobject*> scopeObj(cx, scope.convertTo<VM::Wobject*>());
    Local<VM::LookupState*> lookupState(cx);
    Local<VM::PropertyDescriptor> propDesc(cx);

    // Lookup method name on scope.
    Result<bool> lookupResult = VM::Wobject::LookupProperty(
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
    Local<VM::Function*> func(cx, propDesc->method());

    // Function must be an operative.
    if (!func->isOperative()) {
        return cx->setExceptionRaised("Syntax method binding is applicative.",
                                      name.get());
    }

    // Create SyntaxTreeRef.
    Local<VM::SyntaxTreeRef> stRef(cx, VM::SyntaxTreeRef(pst, node->offset()));

    // Invoke operative function with given arguments.
    return InvokeOperativeFunction(cx, lookupState, scope, func,
                                   scopeObj, stRef);
}

VM::ControlFlow
InvokeOperativeFunction(ThreadContext* cx,
                        Handle<VM::LookupState*> lookupState,
                        Handle<VM::ScopeObject*> callerScope,
                        Handle<VM::Function*> func,
                        Handle<VM::Wobject*> receiver,
                        Handle<VM::SyntaxTreeRef> stRef)
{
    // Call native if native.
    if (func->isNative()) {
        WH_ASSERT(func->asNative()->isOperative());
        Local<VM::NativeCallInfo> callInfo(cx,
            VM::NativeCallInfo(lookupState, callerScope,
                               func->asNative(),
                               VM::ValBox::Pointer(receiver.get())));

        VM::NativeOperativeFuncPtr opNatF = func->asNative()->operative();
        return opNatF(cx, callInfo, ArrayHandle<VM::SyntaxTreeRef>(stRef));
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

VM::ControlFlow
GetValueProperty(ThreadContext* cx,
                 Handle<VM::ValBox> value,
                 Handle<VM::String*> name)
{
    // Check if the value is an object.
    if (!value->isPointer()) {
        return cx->setExceptionRaised("Cannot look up property on a "
                                      "primitive value");
    }

    // Convert to wobject.
    Local<VM::Wobject*> object(cx, value->objectPointer());
    return GetObjectProperty(cx, object, name);
}

VM::ControlFlow
GetObjectProperty(ThreadContext* cx,
                  Handle<VM::Wobject*> object,
                  Handle<VM::String*> name)
{
    Local<VM::LookupState*> lookupState(cx);
    Local<VM::PropertyDescriptor> propDesc(cx);

    Result<bool> lookupResult = VM::Wobject::LookupProperty(
        cx->inHatchery(), object, name, &lookupState, &propDesc);
    if (!lookupResult)
        return ErrorVal();

    // Found binding.
    WH_ASSERT(propDesc->isValid());

    // Handle a value binding.
    if (propDesc->isValue())
        return VM::ControlFlow::Value(propDesc->valBox());

    if (propDesc->isMethod()) {
        return cx->setExceptionRaised(
            "Can't handle method bindings for name lookups yet.",
            name.get());
    }

    return cx->setExceptionRaised(
        "Unknown property binding for name",
        name.get());
}


} // namespace Interp
} // namespace Whisper
