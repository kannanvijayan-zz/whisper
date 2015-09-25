
#include "runtime_inlines.hpp"
#include "vm/property_dict.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<PropertyDict*>
PropertyDict::Create(AllocationContext acx, uint32_t capacity)
{
    return acx.createSized<PropertyDict>(CalculateSize(capacity), capacity);
}

/* static */ uint32_t
PropertyDict::NameHash(String const* name)
{
    return name->fnvHash();
}

Maybe<uint32_t>
PropertyDict::lookup(String const* name) const
{
    uint32_t length = name->length();

    // Probe for the hash.
    uint32_t hash = NameHash(name);
    for (uint32_t probe = hash % capacity_;
         /* no condition */;
         probe = (probe + 1) % capacity_)
    {
        String* probeName = entries_[probe].name.get();

        // Check for empty entry.
        if (!probeName)
            return Maybe<uint32_t>::None();

        // Check for sentinel entry
        if (probeName == SENTINEL())
            continue;

        if (length != probeName->length())
            continue;

        bool equalSoFar = true;
        String::Cursor checkCursor = name->begin();
        String::Cursor probeCursor = probeName->begin();
        for (uint32_t i = 0; i < length; i++) {
            unic_t checkCh = name->readAdvance(checkCursor);
            unic_t probeCh = probeName->readAdvance(probeCursor);
            equalSoFar = (checkCh == probeCh);
            if (!equalSoFar)
                break;
        }
        if (!equalSoFar)
            continue;

        // Found matching name.
        return Maybe<uint32_t>::Some(probe);
    }
}

Maybe<uint32_t>
PropertyDict::addEntry(String* name, PropertyDescriptor const& descr)
{
    WH_ASSERT(size() <= capacity());
    WH_ASSERT(!lookup(name).hasValue());

    // Check entry can be added.
    if (!canAddEntry())
        return Maybe<uint32_t>::None();

    // Probe for the hash.
    uint32_t hash = NameHash(name);
    for (uint32_t probe = hash % capacity_;
         /* no condition */;
         probe = (probe + 1) % capacity_)
    {
        String* probeName = entries_[probe].name.get();

        // Check for empty or sentinel entry.
        if (probeName && probeName != SENTINEL())
            continue;

        // Empty entry found.  Set it.
        entries_[probe].name.init(name, this);
        entries_[probe].value.init(descr.box(), this);

        return Maybe<uint32_t>::Some(probe);
    }
}


} // namespace VM
} // namespace Whisper
