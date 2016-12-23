
#include "runtime_inlines.hpp"
#include "vm/continuation.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<Continuation*>
Continuation::Create(AllocationContext acx, Handle<Frame*> frame)
{
    return acx.create<Continuation>(frame);
}

StepResult
Continuation::continueWith(ThreadContext* cx, Handle<ValBox> value) const
{
    Local<Frame*> frame(cx, frame_);
    return Frame::Resolve(cx, frame, EvalResult::Value(value.get()));
}

/* static */ Result<ContObject*>
ContObject::Create(AllocationContext acx, Handle<Continuation*> cont)
{
    // Allocate empty array of delegates.
    Local<Array<Wobject*>*> delegates(acx);
    if (!delegates.setResult(Array<Wobject*>::CreateEmpty(acx)))
        return ErrorVal();

    // Allocate a dictionary.
    Local<PropertyDict*> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return ErrorVal();

    return acx.create<ContObject>(delegates.handle(), props.handle(), cont);
}

WobjectHooks const*
ContObject::getContObjectHooks() const
{
    return hashObjectHooks();
}


} // namespace VM
} // namespace Whisper
