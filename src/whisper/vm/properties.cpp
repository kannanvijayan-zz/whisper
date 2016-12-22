
#include "vm/core.hpp"
#include "vm/properties.hpp"
#include "vm/wobject.hpp"
#include "vm/function.hpp"

namespace Whisper {
namespace VM {


EvalResult
PropertyLookupResult::toEvalResult(ThreadContext* cx,
                                   Handle<Frame*> frame)
  const
{
    if (isError()) {
        return EvalResult::Error();
    }

    if (isNotFound()) {
        // Throw NameLookupFailedException
        Local<Wobject*> object(cx, lookupState_->receiver());
        Local<String*> name(cx, lookupState_->name());

        Local<NameLookupFailedException*> exc(cx);
        if (!exc.setResult(NameLookupFailedException::Create(
                cx->inHatchery(), object, name)))
        {
            return EvalResult::Error();
        }

        return EvalResult::Exc(frame, exc.get());
    }

    if (isFound()) {
        // Handle a value binding by returning the value.
        if (descriptor_->isSlot()) {
            return EvalResult::Value(descriptor_->slotValue());
        }

        // Handle a method binding by creating a bound FunctionObject
        // from the method and returning that.
        if (descriptor_->isMethod()) {
            // Create a new function object bound to the scope.
            Local<Wobject*> obj(cx, lookupState_->receiver());
            Local<ValBox> objVal(cx, ValBox::Object(obj.get()));
            Local<Function*> func(cx, descriptor_->methodFunction());
            Local<FunctionObject*> funcObj(cx);
            if (!funcObj.setResult(FunctionObject::Create(
                    cx->inHatchery(), func, objVal, lookupState_)))
            {
                return EvalResult::Error();
            }

            return EvalResult::Value(ValBox::Object(funcObj.get()));
        }

        WH_UNREACHABLE("PropertyDescriptor not one of Value, Method.");
        return EvalResult::Error();
    }

    WH_UNREACHABLE("PropertyDescriptor state not Error, NotFound, or Found.");
    return EvalResult::Error();
}


} // namespace VM
} // namespace Whisper
