
#include "vm/wobject.hpp"

namespace Whisper {
namespace VM {


bool
Wobject::lookupPropertyIndex(Handle<PropertyName> name,
                             uint32_t *indexOut)
{
    WH_ASSERT(indexOut);
    for (uint32_t i = 0; i < dict_->size(); i++) {
        if (name->equals(dict_->name(i))) {
            *indexOut = i;
            return true;
        }
    }
    return false;
}

bool
Wobject::lookupProperty(Handle<PropertyName> name,
                        MutHandle<PropertyDescriptor> result)
{
    uint32_t idx = 0;
    if (!lookupPropertyIndex(name, &idx))
        return false;

    result = PropertyDescriptor(dict_->value(idx));
    return true;
}

bool
Wobject::defineProperty(RunContext *cx,
                        Handle<PropertyName> name,
                        Handle<PropertyDescriptor> defn)
{
    uint32_t idx = 0;
    if (lookupPropertyIndex(name, &idx)) {
        // Override existing definition.
        WH_ASSERT(name->equals(dict_->name(idx)));
        dict_->setValue(idx, defn);
        return true;
    }

    // Entry needs to be added.  Either define a string
    // or use existing one.
    Local<String *> nameString(cx, name->createString(cx->inHatchery()));
    if (!nameString.get())
        return false;

    // Property not found.  Add an entry.
    if (dict_->addEntry(nameString.get(), defn))
        return true;

    return false;
}


} // namespace VM
} // namespace Whisper
