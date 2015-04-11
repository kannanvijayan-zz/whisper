
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
PlainObject::GetDelegates(AllocationContext acx,
                          Handle<PlainObject *> obj,
                          MutHandle<Array<Wobject *> *> delegatesOut)
{
    HashObject::GetDelegates(acx,
        obj.convertTo<HashObject *>(),
        delegatesOut);
}

/* static */ bool
PlainObject::GetProperty(AllocationContext acx,
                         Handle<PlainObject *> obj,
                         Handle<String *> name,
                         MutHandle<PropertyDescriptor> result)
{
    return HashObject::GetProperty(acx,
        obj.convertTo<HashObject *>(),
        name, result);
}

/* static */ OkResult
PlainObject::DefineProperty(AllocationContext acx,
                            Handle<PlainObject *> obj,
                            Handle<String *> name,
                            Handle<PropertyDescriptor> defn)
{
    return HashObject::DefineProperty(acx,
        obj.convertTo<HashObject *>(),
        name, defn);
}


} // namespace VM
} // namespace Whisper
