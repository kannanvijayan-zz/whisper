
#include "runtime_inlines.hpp"
#include "vm/plain_object.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<PlainObject *>
PlainObject::Create(AllocationContext acx,
                    Handle<Array<Wobject *> *> delegates)
{
    // Allocate a dictionary.
    Local<PropertyDict *> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return ErrorVal();

    return acx.create<PlainObject>(delegates, props.handle());
}

/* static */ void
PlainObject::GetDelegates(ThreadContext *cx,
                          Handle<PlainObject *> obj,
                          MutHandle<Array<Wobject *> *> delegatesOut)
{
    HashObject::GetDelegates(cx,
        Handle<HashObject *>::Convert(obj),
        delegatesOut);
}

/* static */ bool
PlainObject::GetProperty(ThreadContext *cx,
                         Handle<PlainObject *> obj,
                         Handle<String *> name,
                         MutHandle<PropertyDescriptor> result)
{
    return HashObject::GetProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, result);
}

/* static */ OkResult
PlainObject::DefineProperty(ThreadContext *cx,
                            Handle<PlainObject *> obj,
                            Handle<String *> name,
                            Handle<PropertyDescriptor> defn)
{
    return HashObject::DefineProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, defn);
}


} // namespace VM
} // namespace Whisper
