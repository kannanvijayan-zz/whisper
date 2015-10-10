
#include "gc/local.hpp"
#include "parser/packed_syntax.hpp"
#include "parser/packed_reader.hpp"
#include "vm/wobject.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "interp/property_lookup.hpp"

namespace Whisper {
namespace Interp {


static VM::ControlFlow
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
        return ErrorVal();

    // If binding not found, return void control flow.
    if (!lookupResult.value())
        return VM::ControlFlow::Void();

    // Found binding.
    WH_ASSERT(propDesc->isValid());

    // Handle a value binding by returning the value.
    if (propDesc->isValue())
        return VM::ControlFlow::Value(propDesc->valBox());

    // Handle a method binding by creating a bound FunctionObject
    // from the method.
    if (propDesc->isMethod()) {
        // Create a new function object bound to the scope.
        Local<VM::Function*> func(cx, propDesc->method());
        Local<VM::FunctionObject*> funcObj(cx);
        if (!funcObj.setResult(VM::FunctionObject::Create(
                cx->inHatchery(), func, receiver, lookupState)))
        {
            return ErrorVal();
        }

        return VM::ControlFlow::Value(VM::ValBox::Object(funcObj.get()));
    }

    return cx->setExceptionRaised(
        "Unknown property binding for name",
        name.get());
}

VM::ControlFlow
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

    return cx->setExceptionRaised("Cannot look up property on a "
                                  "primitive value");
}

VM::ControlFlow
GetObjectProperty(ThreadContext* cx,
                  Handle<VM::Wobject*> object,
                  Handle<VM::String*> name)
{
    Local<VM::ValBox> val(cx, VM::ValBox::Object(object));
    return GetPropertyHelper(cx, val, object, name);
}


} // namespace Interp
} // namespace Whisper
