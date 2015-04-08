
#include "runtime_inlines.hpp"
#include "vm/hash_object.hpp"

namespace Whisper {
namespace VM {


/* static */ void
HashObject::GetDelegates(ThreadContext *cx,
                         Handle<HashObject *> obj,
                         MutHandle<Array<Wobject *> *> delegatesOut)
{
    delegatesOut = obj->delegates_;
}

/* static */ bool
HashObject::GetPropertyIndex(Handle<HashObject *> obj,
                             Handle<String *> name,
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
HashObject::GetProperty(ThreadContext *cx,
                        Handle<HashObject *> obj,
                        Handle<String *> name,
                        MutHandle<PropertyDescriptor> result)
{
    uint32_t idx = 0;
    if (!GetPropertyIndex(obj, name, &idx))
        return false;

    result = PropertyDescriptor(obj->dict_->value(idx));
    return true;
}

/* static */ OkResult
HashObject::DefineProperty(ThreadContext *cx,
                           Handle<HashObject *> obj,
                           Handle<String *> name,
                           Handle<PropertyDescriptor> defn)
{
    uint32_t idx = 0;
    if (GetPropertyIndex(obj, name, &idx)) {
        // Override existing definition.
        WH_ASSERT(name->equals(obj->dict_->name(idx)));
        obj->dict_->setValue(idx, defn);
        return OkResult::Ok();
    }

    // Entry needs to be added.  Either define a string
    // or use existing one.
    // Property not found.  Add an entry.
    if (obj->dict_->addEntry(name.get(), defn))
        return OkResult::Ok();

    // TODO: Try to enlarge dict and add.
    return ErrorVal();
}


} // namespace VM
} // namespace Whisper
