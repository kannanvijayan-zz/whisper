
#include "gc/local.hpp"
#include "parser/packed_syntax.hpp"
#include "parser/packed_reader.hpp"
#include "vm/wobject.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "interp/property_lookup.hpp"
#include "interp/interpreter.hpp"

namespace Whisper {
namespace Interp {


WithPushedFrame::WithPushedFrame(ThreadContext* cx, Handle<VM::Frame*> frame)
  : frame_(cx, frame)
{
    cx->pushTopFrame(frame);
}

WithPushedFrame::~WithPushedFrame()
{
    ThreadContext* cx = frame_.threadContext();
    WH_ASSERT(cx->topFrame() == frame_.get());
    cx->popTopFrame();
}

VM::ControlFlow
InterpretSourceFile(ThreadContext* cx,
                    Handle<VM::SourceFile*> file,
                    Handle<VM::ScopeObject*> scope)
{
    WH_ASSERT(file.get() != nullptr);
    WH_ASSERT(scope.get() != nullptr);

    // Try to make a function for the script.
    Local<VM::PackedSyntaxTree*> st(cx);
    if (!st.setResult(VM::SourceFile::ParseSyntaxTree(cx, file)))
        return ErrorVal();

    // SyntaxTreeFragment.
    Local<VM::SyntaxTreeFragment*> anchor(cx);
    if (!anchor.setResult(VM::SyntaxNode::Create(
            cx->inHatchery(), st, st->startOffset())))
    {
        return ErrorVal();
    }

    // Create a new frame for the interpretation.
    Local<VM::Frame*> parentFrame(cx, cx->topFrame());
    Local<VM::Frame*> frame(cx);
    if (!frame.setResult(VM::EvalFrame::Create(cx->inHatchery(), anchor)))
        return ErrorVal();
    WithPushedFrame pushedFrame(cx, frame);

    // Interpret the syntax tree at the given offset.
    return InterpretSyntax(cx, scope, st, st->startOffset());
}

VM::ControlFlow
InterpretSyntax(ThreadContext* cx,
                Handle<VM::ScopeObject*> scope,
                Handle<VM::PackedSyntaxTree*> pst,
                uint32_t offset)
{
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
    Local<VM::PropertyDescriptor> propDesc(cx);

    // Lookup method name on scope.
    VM::ControlFlow propFlow = GetObjectProperty(cx, scopeObj, name);
    WH_ASSERT(propFlow.isPropertyLookupResult());
    if (propFlow.isVoid()) {
        return cx->setExceptionRaised("Syntax method binding not found.",
                                      name.get());
    }
    if (!propFlow.isValue())
        return propFlow;

    // Found binding for syntax name.  Ensure it's a method.
    if (!propFlow.value().isPointerTo<VM::FunctionObject>()) {
        return cx->setExceptionRaised(
            "Syntax method binding is not a function.", name.get());
    }

    Local<VM::FunctionObject*> funcObj(cx,
        propFlow.value().pointer<VM::FunctionObject>());
    Local<VM::LookupState*> lookupState(cx, funcObj->lookupState());

    // Function must be an operative.
    if (!funcObj->func()->isOperative()) {
        return cx->setExceptionRaised("Syntax method binding is applicative.",
                                      name.get());
    }

    // Create SyntaxNodeRef.
    Local<VM::SyntaxNodeRef> stRef(cx, VM::SyntaxNodeRef(pst, node->offset()));

    // Invoke operative function with given arguments.
    return InvokeOperativeFunction(cx, scope, funcObj, stRef);
}

VM::ControlFlow
InvokeValue(ThreadContext* cx,
            Handle<VM::ScopeObject*> callerScope,
            Handle<VM::ValBox> funcVal,
            ArrayHandle<VM::SyntaxNodeRef> stRefs)
{
    // Ensure callee is a function object.
    if (!funcVal->isPointerTo<VM::FunctionObject>())
        return cx->setExceptionRaised("Cannot call non-function");

    // Obtain callee function.
    Local<VM::FunctionObject*> func(cx,
        funcVal->pointer<VM::FunctionObject>());

    return InvokeFunction(cx, callerScope, func, stRefs);
}

VM::ControlFlow
InvokeFunction(ThreadContext* cx,
               Handle<VM::ScopeObject*> callerScope,
               Handle<VM::FunctionObject*> funcObj,
               ArrayHandle<VM::SyntaxNodeRef> stRefs)
{
    if (funcObj->isOperative())
        return InvokeOperativeFunction(cx, callerScope, funcObj, stRefs);
    else
        return InvokeApplicativeFunction(cx, callerScope, funcObj, stRefs);
}

VM::ControlFlow
InvokeOperativeValue(ThreadContext* cx,
                     Handle<VM::ScopeObject*> callerScope,
                     Handle<VM::ValBox> funcVal,
                     ArrayHandle<VM::SyntaxNodeRef> stRefs)
{
    // Ensure callee is a function object.
    if (!funcVal->isPointerTo<VM::FunctionObject>())
        return cx->setExceptionRaised("Cannot call non-function");

    // Obtain callee function.
    Local<VM::FunctionObject*> func(cx,
        funcVal->pointer<VM::FunctionObject>());

    // Ensure callee is an operative.
    if (!func->isOperative()) {
        return cx->setExceptionRaised("Function is not an operative.",
                                      func.get());
    }

    return InvokeOperativeFunction(cx, callerScope, func, stRefs);
}

VM::ControlFlow
InvokeOperativeFunction(ThreadContext* cx,
                        Handle<VM::ScopeObject*> callerScope,
                        Handle<VM::FunctionObject*> funcObj,
                        ArrayHandle<VM::SyntaxNodeRef> stRefs)
{
    WH_ASSERT(funcObj->isOperative());

    // Call native if native.
    Local<VM::Function*> func(cx, funcObj->func());
    if (func->isNative()) {
        Local<VM::LookupState*> lookupState(cx, funcObj->lookupState());
        Local<VM::ValBox> receiver(cx, funcObj->receiver());

        Local<VM::NativeCallInfo> callInfo(cx,
            VM::NativeCallInfo(lookupState, callerScope, funcObj, receiver));

        // Create a new frame for the interpretation.
        Local<VM::Frame*> frame(cx);
        if (!frame.setResult(VM::FunctionFrame::Create(cx->inHatchery(), func)))
            return ErrorVal();
        WithPushedFrame pushedFrame(cx, frame);

        VM::NativeOperativeFuncPtr opNatF = func->asNative()->operative();
        return opNatF(cx, callInfo, stRefs);
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
InvokeApplicativeValue(ThreadContext* cx,
                       Handle<VM::ScopeObject*> callerScope,
                       Handle<VM::ValBox> funcVal,
                       ArrayHandle<VM::SyntaxNodeRef> stRefs)
{
    // Ensure callee is a function object.
    if (!funcVal->isPointerTo<VM::FunctionObject>())
        return cx->setExceptionRaised("Cannot call non-function");

    // Obtain callee function.
    Local<VM::FunctionObject*> func(cx,
        funcVal->pointer<VM::FunctionObject>());

    // Ensure callee is an operative.
    if (!func->isApplicative()) {
        return cx->setExceptionRaised("Function is not an operative.",
                                      func.get());
    }

    return InvokeApplicativeFunction(cx, callerScope, func, stRefs);
}

VM::ControlFlow
InvokeApplicativeFunction(ThreadContext* cx,
                          Handle<VM::ScopeObject*> callerScope,
                          Handle<VM::FunctionObject*> funcObj,
                          ArrayHandle<VM::SyntaxNodeRef> stRefs)
{
    WH_ASSERT(funcObj->isApplicative());

    // Evaluate each argument stref.
    LocalArray<VM::ValBox> args(cx, stRefs.length());
    for (uint32_t i = 0; i < stRefs.length(); i++) {
        VM::ControlFlow argFlow = InterpretSyntax(cx, callerScope,
                                                  stRefs.handle(i));
        // Must be an expression result.
        WH_ASSERT(argFlow.isExpressionResult());
        if (!argFlow.isValue())
            return argFlow;
        args[i] = argFlow.value();
    }

    // Call native if native.
    Local<VM::Function*> func(cx, funcObj->func());
    if (func->isNative()) {
        Local<VM::LookupState*> lookupState(cx, funcObj->lookupState());
        Local<VM::ValBox> receiver(cx, funcObj->receiver());

        Local<VM::NativeCallInfo> callInfo(cx,
            VM::NativeCallInfo(lookupState, callerScope, funcObj, receiver));

        // Create a new frame for the interpretation.
        Local<VM::Frame*> frame(cx);
        if (!frame.setResult(VM::FunctionFrame::Create(cx->inHatchery(), func)))
            return ErrorVal();
        WithPushedFrame pushedFrame(cx, frame);

        VM::NativeApplicativeFuncPtr appNatF = func->asNative()->applicative();
        return appNatF(cx, callInfo, args);
    }

    // If scripted, interpret the scripted function.
    if (func->isScripted()) {
        Local<VM::ScriptedFunction*> scriptedFunc(cx, func->asScripted());
        if (scriptedFunc->numParams() != args.length())
            return cx->setExceptionRaised("Arguments do not match params.");

        Local<VM::ScopeObject*> scopeChain(cx, scriptedFunc->scopeChain());

        // Create a new scope object for the call.
        Local<VM::CallScope*> funcScope(cx);
        if (!funcScope.setResult(VM::CallScope::Create(cx->inHatchery(),
                                                       scopeChain, func)))
        {
            return cx->setExceptionRaised("Error creating call scope.");
        }

        // Bind argument values to parameter names.
        for (uint32_t i = 0; i < args.length(); i++) {
            Local<VM::String*> paramName(cx, scriptedFunc->paramName(i));
            Local<VM::PropertyDescriptor> propDesc(cx,
                VM::PropertyDescriptor(args[i]));
            if (!VM::Wobject::DefineProperty(cx->inHatchery(),
                    funcScope.handle().convertTo<VM::Wobject*>(),
                    paramName, propDesc))
            {
                return ErrorVal();
            }
        }

        // Obtain the block to evaluate.
        Local<VM::SyntaxBlockRef> bodyBlock(cx, scriptedFunc->bodyBlockRef());

        // Create a new frame for the interpretation.
        Local<VM::Frame*> frame(cx);
        if (!frame.setResult(VM::FunctionFrame::Create(cx->inHatchery(), func)))
            return ErrorVal();
        WithPushedFrame pushedFrame(cx, frame);

        // Evaluate the function syntax.
        VM::ControlFlow callFlow = EvaluateBlock(cx,
            funcScope.handle().convertTo<VM::ScopeObject*>(),
            bodyBlock);
        WH_ASSERT(callFlow.isCallResult());
        if (callFlow.isReturn())
            return VM::ControlFlow::Value(callFlow.returnValue());

        if (callFlow.isVoid())
            return VM::ControlFlow::Value(VM::ValBox::Undefined());

        return callFlow;
    }

    WH_UNREACHABLE("Unknown function type!");
    return cx->setError(RuntimeError::InternalError,
                        "Unknown function type seen!",
                        HeapThing::From(func.get()));
}

VM::ControlFlow
EvaluateBlock(ThreadContext* cx,
              Handle<VM::ScopeObject*> scopeObj,
              Handle<VM::SyntaxBlockRef> bodyBlock)
{
    for (uint32_t i = 0; i < bodyBlock->numStatements(); i++) {
        Local<VM::SyntaxNodeRef> stmtNode(cx, bodyBlock->statement(i));
        VM::ControlFlow stmtFlow = InterpretSyntax(cx, scopeObj, stmtNode);
        WH_ASSERT(stmtFlow.isStatementResult());

        // Statements can yield void or value control flows and still
        // continue.
        if (stmtFlow.isVoid() || stmtFlow.isValue())
            continue;

        return stmtFlow;
    }

    return VM::ControlFlow::Void();
}


} // namespace Interp
} // namespace Whisper
