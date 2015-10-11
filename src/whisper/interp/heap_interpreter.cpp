
#include "parser/syntax_tree.hpp"
#include "parser/packed_syntax.hpp"
#include "vm/function.hpp"
#include "vm/runtime_state.hpp"
#include "interp/heap_interpreter.hpp"

namespace Whisper {
namespace Interp {


VM::ControlFlow
HeapInterpretSourceFile(ThreadContext* cx,
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
            cx->inHatchery(), fileNode, scope)))
    {
        return ErrorVal();
    }
    cx->setTopFrame(entryFrame);

    // Run interpreter loop.
    return HeapInterpretLoop(cx);
}

VM::ControlFlow
HeapInterpretLoop(ThreadContext* cx)
{
    while (!cx->atTerminalFrame()) {
        OkResult result = cx->topFrame()->step(cx);
        if (!result) {
            // Fatal error, immediate halt of computation
            // with frame-stack intact.
            WH_ASSERT(cx->hasError());
            return VM::ControlFlow::Error();
        }
    }
    return cx->bottomFrame()->flow();
}


static OkResult
ResolveSyntaxNameLookup(ThreadContext* cx,
                             Handle<VM::SyntaxFrame*> frame,
                             VM::ControlFlow const& flow)
{
    return ErrorVal();
}

static OkResult
StepSyntaxNameLookup(ThreadContext* cx, Handle<VM::SyntaxFrame*> frame)
{
    Local<VM::SyntaxTreeFragment*> stFrag(cx, frame->stFrag());

    // Get the name of the syntax handler method.
    Local<VM::String*> name(cx, cx->runtimeState()->syntaxHandlerName(stFrag));
    if (name.get() == nullptr) {
        WH_UNREACHABLE("Handler name not found for SyntaxTreeFragment.");
        cx->setInternalError("Handler name not found for SyntaxTreeFragment.");
        return ErrorVal();
    }

    // Look up the property on the scope object.
    Local<VM::ScopeObject*> scope(cx, frame->entryFrame()->scope());
    PropertyLookupResult lookupResult = GetObjectProperty(cx,
        scope.handle().convertTo<VM::Wobject*>(), name);

    if (lookupResult.isError()) {
        return ResolveSyntaxNameLookup(cx, frame,
                VM::ControlFlow::Error());
    }

    if (lookupResult.isNotFound()) {
        cx->setExceptionRaised("Lookup name not found", name.get());
        return ResolveSyntaxNameLookup(cx, frame,
            VM::ControlFlow::Exception());
    }

    if (lookupResult.isFound()) {
        Local<VM::PropertyDescriptor> descriptor(cx, lookupResult.descriptor());
        Local<VM::LookupState*> lookupState(cx, lookupResult.lookupState());

        // Handle a value binding by returning the value.
        if (descriptor->isValue())
            return ResolveSyntaxNameLookup(cx, frame,
                    VM::ControlFlow::Value(descriptor->valBox()));

        // Handle a method binding by creating a bound FunctionObject
        // from the method.
        if (descriptor->isMethod()) {
            // Create a new function object bound to the scope.
            Local<VM::ValBox> scopeVal(cx, VM::ValBox::Object(scope.get()));
            Local<VM::Function*> func(cx, descriptor->method());
            Local<VM::FunctionObject*> funcObj(cx);
            if (!funcObj.setResult(VM::FunctionObject::Create(
                    cx->inHatchery(), func, scopeVal, lookupState)))
            {
                return ErrorVal();
            }

            return ResolveSyntaxNameLookup(cx, frame,
                    VM::ControlFlow::Value(VM::ValBox::Object(funcObj.get())));
        }

        WH_UNREACHABLE("PropertyDescriptor not one of Value, Method.");
        return ErrorVal();
    }

    WH_UNREACHABLE("Property lookup not one of Found, NotFound, Error.");
    return ErrorVal();
}


Result<VM::SyntaxFrame*>
CreateInitialSyntaxFrame(ThreadContext* cx,
                         Handle<VM::EntryFrame*> entryFrame)
{
    Local<VM::SyntaxTreeFragment*> stFrag(cx, entryFrame->stFrag());

    Local<VM::SyntaxFrame*> stFrame(cx);
    if (!stFrame.setResult(VM::SyntaxFrame::Create(cx->inHatchery(),
            entryFrame, stFrag,
            ResolveSyntaxNameLookup,
            StepSyntaxNameLookup)))
    {
        return ErrorVal();
    }

    return OkVal(stFrame.get());
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
