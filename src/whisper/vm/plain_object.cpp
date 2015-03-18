
#include "runtime_inlines.hpp"
#include "vm/plain_object.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<PlainObject *>
PlainObject::Create(AllocationContext acx,
                    Handle<Array<Wobject *> *> delegates)
{
    // Allocate a dictionary.
    Result<PropertyDict *> mbProps = 
        PropertyDict::Create(acx, InitialPropertyCapacity);
    if (!mbProps)
        return Result<PlainObject *>::Error();

    Local<PropertyDict *> props(acx, mbProps.value());
    return acx.create<PlainObject>(delegates, props.handle());
}

/* static */ void
PlainObject::GetDelegates(ThreadContext *cx,
                          Handle<PlainObject *> obj,
                          MutHandle<Array<Wobject *> *> delegatesOut)
{
    delegatesOut = obj->delegates_;
}

/* static */ bool
PlainObject::LookupPropertyIndex(Handle<PlainObject *> obj,
                                 Handle<PropertyName> name,
                                 uint32_t *indexOut)
{
    WH_ASSERT(indexOut);
    for (uint32_t i = 0; i < obj->dict_->size(); i++) {
        if (name->equals(obj->dict_->name(i))) {
            *indexOut = i;
            return true;
        }
    }
    return false;
}

/* static */ bool
PlainObject::LookupProperty(ThreadContext *cx,
                            Handle<PlainObject *> obj,
                            Handle<PropertyName> name,
                            MutHandle<PropertyDescriptor> result)
{
    uint32_t idx = 0;
    if (!LookupPropertyIndex(obj, name, &idx))
        return false;

    result = PropertyDescriptor(obj->dict_->value(idx));
    return true;
}

/* static */ OkResult
PlainObject::DefineProperty(ThreadContext *cx,
                            Handle<PlainObject *> obj,
                            Handle<PropertyName> name,
                            Handle<PropertyDescriptor> defn)
{
    uint32_t idx = 0;
    if (LookupPropertyIndex(obj, name, &idx)) {
        // Override existing definition.
        WH_ASSERT(name->equals(obj->dict_->name(idx)));
        obj->dict_->setValue(idx, defn);
        return OkResult::Ok();
    }

    // Entry needs to be added.  Either define a string
    // or use existing one.
    Result<String *> maybeNameString = name->createString(cx->inHatchery());
    if (!maybeNameString)
        return OkResult::Error();
    Local<String *> nameString(cx, maybeNameString.value());

    // Property not found.  Add an entry.
    if (obj->dict_->addEntry(nameString.get(), defn))
        return OkResult::Ok();

    // TODO: Try to enlarge dict and add.
    return OkResult::Error();
}


} // namespace VM
} // namespace Whisper
