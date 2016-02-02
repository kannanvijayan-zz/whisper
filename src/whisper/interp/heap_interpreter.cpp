
#include "parser/syntax_tree.hpp"
#include "parser/packed_syntax.hpp"
#include "vm/function.hpp"
#include "vm/runtime_state.hpp"
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

    Local<VM::SyntaxNameLookupFrame*> stFrame(cx);
    if (!stFrame.setResult(VM::SyntaxNameLookupFrame::Create(cx->inHatchery(),
            parent, entryFrame, stFrag)))
    {
        return ErrorVal();
    }

    return OkVal(stFrame.get());
}

Result<VM::Frame*>
CreateInvokeSyntaxFrame(ThreadContext* cx,
                        Handle<VM::Frame*> parentFrame,
                        Handle<VM::EntryFrame*> entryFrame,
                        Handle<VM::SyntaxTreeFragment*> stFrag,
                        Handle<VM::ValBox> syntaxHandler)
{
    Local<VM::InvokeSyntaxFrame*> stFrame(cx);
    if (!stFrame.setResult(VM::InvokeSyntaxFrame::Create(cx->inHatchery(),
            parentFrame, entryFrame, stFrag, syntaxHandler)))
    {
        return ErrorVal();
    }

    return OkVal(stFrame.get());
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
InvokeValue(ThreadContext* cx,
            Handle<VM::Frame*> frame,
            Handle<VM::ScopeObject*> callerScope,
            Handle<VM::ValBox> callee,
            ArrayHandle<VM::SyntaxTreeFragment*> args)
{
    Local<VM::FunctionObject*> rootedCallee(cx);
    if (!rootedCallee.setMaybe(FunctionObjectForValue(cx, callee)))
        return ErrorVal();
    return InvokeFunction(cx, frame, callerScope, callee, rootedCallee, args);
}

VM::CallResult
InvokeFunction(ThreadContext* cx,
               Handle<VM::Frame*> frame,
               Handle<VM::ScopeObject*> callerScope,
               Handle<VM::ValBox> callee,
               Handle<VM::FunctionObject*> calleeFunc,
               ArrayHandle<VM::SyntaxTreeFragment*> args)
{
    if (calleeFunc->isOperative()) {
        return InvokeOperativeFunction(cx, frame, callerScope,
                                       callee, calleeFunc, args);
    } else {
        return InvokeApplicativeFunction(cx, frame, callerScope,
                                         callee, calleeFunc, args);
    }
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
InvokeApplicativeFunction(ThreadContext* cx,
                          Handle<VM::Frame*> frame,
                          Handle<VM::ScopeObject*> callerScope,
                          Handle<VM::ValBox> callee,
                          Handle<VM::FunctionObject*> calleeFunc,
                          ArrayHandle<VM::SyntaxTreeFragment*> args)
{
    return cx->setInternalError("InvokeApplicativeFunction not implemented.");
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
        return PropertyLookupResult::NotFound();

    // Found binding.
    WH_ASSERT(propDesc->isValid());
    return PropertyLookupResult::Found(lookupState.get(), propDesc.get());
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


} // namespace Interp
} // namespace Whisper
