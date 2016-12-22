
#include "parser/syntax_tree.hpp"
#include "parser/packed_syntax.hpp"
#include "vm/function.hpp"
#include "vm/exception.hpp"
#include "vm/runtime_state.hpp"
#include "vm/properties.hpp"
#include "vm/wobject.hpp"
#include "interp/heap_interpreter.hpp"

namespace Whisper {
namespace Interp {


VM::EvalResult
HeapInterpretSourceFile(ThreadContext* cx,
                        Handle<VM::SourceFile*> file,
                        Handle<VM::ScopeObject*> scope)
{
    Local<VM::Frame*> terminalFrame(cx);
    if (!terminalFrame.setResult(VM::TerminalFrame::Create(cx->inHatchery())))
        return ErrorVal();

    return HeapInterpretSourceFile(cx, terminalFrame, file, scope);
}

VM::EvalResult
HeapInterpretSourceFile(ThreadContext* cx,
                        Handle<VM::Frame*> frame,
                        Handle<VM::SourceFile*> file,
                        Handle<VM::ScopeObject*> scope)
{
    WH_ASSERT(file.get() != nullptr);
    WH_ASSERT(scope.get() != nullptr);

    // Parse the source file.
    Local<VM::PackedSyntaxTree*> st(cx);
    if (!st.setResult(VM::SourceFile::ParseSyntaxTree(cx, file)))
        return ErrorVal();

    // Get SyntaxTreeFragment for the parsed File node.
    Local<VM::SyntaxTreeFragment*> fileNode(cx);
    if (!fileNode.setResult(VM::SyntaxNode::Create(
            cx->inHatchery(), st, st->startOffset())))
    {
        return ErrorVal();
    }

    // Create a new entry frame for the interpretation.
    Local<VM::EntryFrame*> entryFrame(cx);
    if (!entryFrame.setResult(VM::EntryFrame::Create(
            cx->inHatchery(), frame, fileNode, scope)))
    {
        return ErrorVal();
    }

    // Run interpreter loop.
    return HeapInterpretLoop(cx, entryFrame.handle());
}

VM::EvalResult
HeapInterpretLoop(ThreadContext* cx,
                  Handle<VM::Frame*> frame)
{
    WH_ASSERT(frame != nullptr);

    Local<VM::Frame*> curFrame(cx, frame);
    while (!curFrame->isTerminalFrame()) {
        {
            constexpr uint32_t BUFSIZE = 2048;
            char buf[BUFSIZE];
            uint32_t offset = 0;
            VM::Frame* c = curFrame;
            while (c) {
                offset += snprintf(&buf[offset], BUFSIZE - offset,
                                " -> (%p)%s",
                                c, HeapThing::From(c)->formatString());
                c = c->parent();
            }
            SpewInterpNote("HeapInterpretLoop step%s", buf);
        }

        VM::StepResult result = VM::Frame::Step(cx, curFrame);
        if (result.isError()) {
            // Fatal error, immediate halt of computation
            // with frame-stack intact.
            WH_ASSERT(cx->hasError());
            return ErrorVal();
        }
        WH_ASSERT(result.isContinue());
        curFrame = result.continueFrame();
    }
    WH_ASSERT(curFrame->isTerminalFrame());
    return curFrame->toTerminalFrame()->result();
}

Result<VM::Frame*>
CreateInitialSyntaxFrame(ThreadContext* cx,
                         Handle<VM::Frame*> parent,
                         Handle<VM::EntryFrame*> entryFrame)
{
    Local<VM::SyntaxTreeFragment*> stFrag(cx, entryFrame->stFrag());

    Local<VM::InvokeSyntaxNodeFrame*> syntaxFrame(cx);
    if (!syntaxFrame.setResult(VM::InvokeSyntaxNodeFrame::Create(
            cx->inHatchery(), parent, entryFrame, stFrag)))
    {
        return ErrorVal();
    }

    return OkVal(syntaxFrame.get());
}

Maybe<VM::FunctionObject*>
FunctionObjectForValue(ThreadContext* cx,
                       Handle<VM::ValBox> value)
{
    if (value->isPointerTo<VM::FunctionObject>()) {
        return Maybe<VM::FunctionObject*>::Some(
                    value->pointer<VM::FunctionObject>());
    }

    return Maybe<VM::FunctionObject*>::None();
}

VM::CallResult
InvokeOperativeValue(ThreadContext* cx,
                     Handle<VM::Frame*> frame,
                     Handle<VM::ScopeObject*> callerScope,
                     Handle<VM::ValBox> callee,
                     ArrayHandle<VM::SyntaxTreeFragment*> args)
{
    Local<VM::FunctionObject*> calleeFunc(cx);
    if (!calleeFunc.setMaybe(FunctionObjectForValue(cx, callee)))
        return ErrorVal();

    if (!calleeFunc->isOperative()) {
        Local<VM::FunctionNotOperativeException*> exc(cx);
        if (!exc.setResult(VM::FunctionNotOperativeException::Create(
                    cx->inHatchery(),
                    calleeFunc)))
        {
            return ErrorVal();
        }
        return VM::CallResult::Exc(frame, exc.get());
    }

    return InvokeOperativeFunction(cx, frame, callerScope,
                                   callee, calleeFunc, args);
}

VM::CallResult
InvokeOperativeFunction(ThreadContext* cx,
                        Handle<VM::Frame*> frame,
                        Handle<VM::ScopeObject*> callerScope,
                        Handle<VM::ValBox> callee,
                        Handle<VM::FunctionObject*> calleeFunc,
                        ArrayHandle<VM::SyntaxTreeFragment*> args)
{
    WH_ASSERT(calleeFunc->isOperative());

    // Check for native callee function.
    Local<VM::Function*> func(cx, calleeFunc->func());
    if (func->isNative()) {
        Local<VM::LookupState*> lookupState(cx, calleeFunc->lookupState());
        Local<VM::ValBox> receiver(cx, calleeFunc->receiver());

        Local<VM::NativeCallInfo> callInfo(cx,
            VM::NativeCallInfo(frame, lookupState,
                               callerScope, calleeFunc, receiver));

        // Call the function. (FIXME).
        VM::NativeOperativeFuncPtr opNatF = func->asNative()->operative();
        return opNatF(cx, callInfo, args);
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

VM::CallResult
InvokeApplicativeValue(ThreadContext* cx,
                       Handle<VM::Frame*> frame,
                       Handle<VM::ScopeObject*> callerScope,
                       Handle<VM::ValBox> callee,
                       ArrayHandle<VM::ValBox> args)
{
    Local<VM::FunctionObject*> calleeFunc(cx);
    if (!calleeFunc.setMaybe(FunctionObjectForValue(cx, callee)))
        return ErrorVal();
    if (!calleeFunc->isApplicative()) {
        return cx->setError(RuntimeError::InternalError,
                            "InvokeApplicativeValue: Function is not an "
                            "operative.");
    }
    return InvokeApplicativeFunction(cx, frame, callerScope,
                                     callee, calleeFunc, args);
}

VM::CallResult
InvokeApplicativeFunction(ThreadContext* cx,
                          Handle<VM::Frame*> frame,
                          Handle<VM::ScopeObject*> callerScope,
                          Handle<VM::ValBox> callee,
                          Handle<VM::FunctionObject*> calleeFunc,
                          ArrayHandle<VM::ValBox> args)
{
    WH_ASSERT(calleeFunc->isApplicative());

    // Check for native callee function.
    Local<VM::Function*> func(cx, calleeFunc->func());
    if (func->isNative()) {
        Local<VM::LookupState*> lookupState(cx, calleeFunc->lookupState());
        Local<VM::ValBox> receiver(cx, calleeFunc->receiver());

        Local<VM::NativeCallInfo> callInfo(cx,
            VM::NativeCallInfo(frame, lookupState,
                               callerScope, calleeFunc, receiver));

        // Call the function.
        VM::NativeApplicativeFuncPtr apNatF = func->asNative()->applicative();
        return apNatF(cx, callInfo, args);
    }

    // If scripted, interpret the scripted function.
    if (func->isScripted()) {
        // Get the scripted function.
        Local<VM::ScriptedFunction*> calleeScript(cx, func->asScripted());
        return InvokeScriptedApplicativeFunction(cx, frame, callerScope,
                                                 callee, calleeScript, args);
    }

    WH_UNREACHABLE("Unknown function type!");
    return cx->setError(RuntimeError::InternalError,
                        "Unknown function type seen!",
                        HeapThing::From(func.get()));
}

VM::CallResult
InvokeScriptedApplicativeFunction(ThreadContext* cx,
                                  Handle<VM::Frame*> frame,
                                  Handle<VM::ScopeObject*> callerScope,
                                  Handle<VM::ValBox> callee,
                                  Handle<VM::ScriptedFunction*> calleeScript,
                                  ArrayHandle<VM::ValBox> args)
{
    // Ensure arguments match parameter spec.
    if (calleeScript->numParams() != args.length()) {
        // FIXME: Replace with a more specialized exception.
        Local<VM::Exception*> exc(cx);
        if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                           "Call arguments don't match function spec.")))
        {
            return ErrorVal();
        }

        return VM::CallResult::Exc(frame, exc);
    }

    // Create a new scope for the activation.
    Local<VM::ScopeObject*> enclosingScope(cx, calleeScript->scopeChain());
    Local<VM::ScopeObject*> scope(cx);
    if (!scope.setResult(VM::CallScope::Create(
            cx->inHatchery(), enclosingScope, calleeScript)))
    {
        return ErrorVal();
    }

    // Bind the arguments in the scope.
    uint32_t params = calleeScript->numParams();
    // Ensure arguments match parameter spec.
    for (uint32_t i = 0; i < params; i++) {
        Local<VM::String*> paramName(cx, calleeScript->paramName(i));
        Local<VM::PropertyDescriptor> propDesc(cx,
            VM::PropertyDescriptor::MakeSlot(args[i]));
        if (!VM::Wobject::DefineProperty(
                cx->inHatchery(),
                scope.handle(),
                paramName,
                propDesc))
        {
            return ErrorVal();
        }
    }

    // Create a SyntaxBlock to evaluate.
    Local<VM::SyntaxBlockRef> stBlockRef(cx, calleeScript->bodyBlockRef());
    Local<VM::SyntaxTreeFragment*> stFrag(cx);
    if (!stFrag.setResult(VM::SyntaxBlock::Create(
            cx->inHatchery(), stBlockRef)))
    {
        return ErrorVal();
    }

    // Create an EntryFrame for the block to evaluate.
    Local<VM::EntryFrame*> entryFrame(cx);
    if (!entryFrame.setResult(VM::EntryFrame::Create(
            cx->inHatchery(), frame, stFrag, scope)))
    {
        return ErrorVal();
    }

    // Continue with the EntryFrame.
    return VM::CallResult::Continue(entryFrame.get());
}


static PropertyLookupResult
GetPropertyHelper(ThreadContext* cx,
                  Handle<VM::ValBox> receiver,
                  Handle<VM::Wobject*> object,
                  Handle<VM::String*> name)
{
    Local<VM::LookupState*> lookupState(cx);
    Local<VM::PropertyDescriptor> propDesc(cx);

    Result<bool> lookupResult = VM::Wobject::LookupProperty(
        cx->inHatchery(), object, name, &lookupState, &propDesc);
    if (!lookupResult)
        return PropertyLookupResult::Error();

    // If binding not found, return void control flow.
    if (!lookupResult.value())
        return PropertyLookupResult::NotFound(lookupState.get());

    // Found binding.
    WH_ASSERT(propDesc->isValid());
    return PropertyLookupResult::Found(lookupState.get(), propDesc.get());
}

VM::EvalResult
PropertyLookupResult::toEvalResult(ThreadContext* cx,
                                   Handle<VM::Frame*> frame)
  const
{
    if (isError()) {
        return VM::EvalResult::Error();
    }

    if (isNotFound()) {
        // Throw NameLookupFailedException
        Local<VM::Wobject*> object(cx, lookupState_->receiver());
        Local<VM::String*> name(cx, lookupState_->name());

        Local<VM::NameLookupFailedException*> exc(cx);
        if (!exc.setResult(VM::NameLookupFailedException::Create(
                cx->inHatchery(), object, name)))
        {
            return VM::EvalResult::Error();
        }

        return VM::EvalResult::Exc(frame, exc.get());
    }

    if (isFound()) {
        // Handle a value binding by returning the value.
        if (descriptor_->isSlot()) {
            return VM::EvalResult::Value(descriptor_->slotValue());
        }

        // Handle a method binding by creating a bound FunctionObject
        // from the method and returning that.
        if (descriptor_->isMethod()) {
            // Create a new function object bound to the scope.
            Local<VM::Wobject*> obj(cx, lookupState_->receiver());
            Local<VM::ValBox> objVal(cx, VM::ValBox::Object(obj.get()));
            Local<VM::Function*> func(cx, descriptor_->methodFunction());
            Local<VM::FunctionObject*> funcObj(cx);
            if (!funcObj.setResult(VM::FunctionObject::Create(
                    cx->inHatchery(), func, objVal, lookupState_)))
            {
                return VM::EvalResult::Error();
            }

            return VM::EvalResult::Value(VM::ValBox::Object(funcObj.get()));
        }

        WH_UNREACHABLE("PropertyDescriptor not one of Value, Method.");
        return VM::EvalResult::Error();
    }

    WH_UNREACHABLE("PropertyDescriptor state not Error, NotFound, or Found.");
    return VM::EvalResult::Error();
}

PropertyLookupResult
GetValueProperty(ThreadContext* cx,
                 Handle<VM::ValBox> value,
                 Handle<VM::String*> name)
{
    // Check if the value is an object.
    if (value->isPointer()) {
        Local<VM::Wobject*> object(cx, value->objectPointer());
        return GetPropertyHelper(cx, value, object, name);
    }

    // Check if the value is a fixed integer.
    if (value->isInteger()) {
        Local<VM::Wobject*> immInt(cx, cx->threadState()->immIntDelegate());
        return GetPropertyHelper(cx, value, immInt, name);
    }

    // Check if the value is a fixed boolean.
    if (value->isBoolean()) {
        Local<VM::Wobject*> immBool(cx, cx->threadState()->immBoolDelegate());
        return GetPropertyHelper(cx, value, immBool, name);
    }

    return cx->setInternalError("Cannot look up property on a given "
                                "primitive value");
}

PropertyLookupResult
GetObjectProperty(ThreadContext* cx,
                  Handle<VM::Wobject*> object,
                  Handle<VM::String*> name)
{
    Local<VM::ValBox> val(cx, VM::ValBox::Object(object));
    return GetPropertyHelper(cx, val, object, name);
}



OkResult
DefValueProperty(ThreadContext* cx,
                 Handle<VM::ValBox> value,
                 Handle<VM::String*> name,
                 Handle<VM::PropertyDescriptor> descr)
{
    // Check if the value is an object.
    if (value->isPointer()) {
        Local<VM::Wobject*> object(cx, value->objectPointer());
        return VM::Wobject::DefineProperty(cx->inHatchery(), object,
                                           name, descr);
    }

    return cx->setInternalError("Cannot set property on a given "
                                "primitive value");
}

OkResult
DefObjectProperty(ThreadContext* cx,
                  Handle<VM::Wobject*> object,
                  Handle<VM::String*> name,
                  Handle<VM::PropertyDescriptor> descr)
{
    return VM::Wobject::DefineProperty(cx->inHatchery(), object, name, descr);
}


} // namespace Interp
} // namespace Whisper
