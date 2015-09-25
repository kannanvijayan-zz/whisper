
#include "runtime_inlines.hpp"
#include "vm/hash_object.hpp"

namespace Whisper {
namespace VM {


/* static */ void
HashObject::GetDelegates(AllocationContext acx,
                         Handle<HashObject*> obj,
                         MutHandle<Array<Wobject*>*> delegatesOut)
{
    delegatesOut = obj->delegates_;
}

/* static */ bool
HashObject::GetProperty(AllocationContext acx,
                        Handle<HashObject*> obj,
                        Handle<String*> name,
                        MutHandle<PropertyDescriptor> result)
{
    Maybe<uint32_t> maybeIdx = obj->dict_->lookup(name);
    if (!maybeIdx.hasValue())
        return false;

    result = PropertyDescriptor(obj->dict_->value(maybeIdx.value()));
    return true;
}

/* static */ OkResult
HashObject::DefineProperty(AllocationContext acx,
                           Handle<HashObject*> obj,
                           Handle<String*> name,
                           Handle<PropertyDescriptor> defn)
{
    Maybe<uint32_t> maybeIdx = obj->dict_->lookup(name);
    if (maybeIdx.hasValue()) {
        uint32_t idx = maybeIdx.value();
        // Override existing definition.
        WH_ASSERT(name->equals(obj->dict_->name(idx)));
        obj->dict_->setValue(idx, defn);
        return OkVal();
    }

    // Entry needs to be added.  Either define a string
    // or use existing one.
    // Property not found.  Add an entry.
    maybeIdx = obj->dict_->addEntry(name.get(), defn);
    if (maybeIdx.hasValue())
        return OkVal();

    // TODO: Try to enlarge dict and add.
    return ErrorVal();
}


} // namespace VM
} // namespace Whisper
