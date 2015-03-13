
#include "vm/plain_object.hpp"

namespace Whisper {
namespace VM {


/* static */ bool
PlainObject::GetDelegates(RunContext *cx,
                          Handle<PlainObject *> obj,
                          MutHandle<Array<Wobject *> *> delegatesOut)
{
    delegatesOut = obj->delegates_;
    return true;
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
PlainObject::LookupProperty(RunContext *cx,
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

/* static */ bool
PlainObject::DefineProperty(RunContext *cx,
                            Handle<PlainObject *> obj,
                            Handle<PropertyName> name,
                            Handle<PropertyDescriptor> defn)
{
    uint32_t idx = 0;
    if (LookupPropertyIndex(obj, name, &idx)) {
        // Override existing definition.
        WH_ASSERT(name->equals(obj->dict_->name(idx)));
        obj->dict_->setValue(idx, defn);
        return true;
    }

    // Entry needs to be added.  Either define a string
    // or use existing one.
    Local<String *> nameString(cx, name->createString(cx->inHatchery()));
    if (!nameString.get())
        return false;

    // Property not found.  Add an entry.
    if (obj->dict_->addEntry(nameString.get(), defn))
        return true;

    return false;
}


} // namespace VM
} // namespace Whisper
