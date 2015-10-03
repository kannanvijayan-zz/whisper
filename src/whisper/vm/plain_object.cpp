
#include "runtime_inlines.hpp"
#include "vm/plain_object.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<PlainObject*>
PlainObject::Create(AllocationContext acx,
                    Handle<Array<Wobject*>*> delegates)
{
    // Allocate a dictionary.
    Local<PropertyDict*> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return ErrorVal();

    return acx.create<PlainObject>(delegates, props.handle());
}

WobjectHooks const*
PlainObject::getPlainObjectHooks() const
{
    return hashObjectHooks();
}


} // namespace VM
} // namespace Whisper
