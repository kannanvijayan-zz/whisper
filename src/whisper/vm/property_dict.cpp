
#include "runtime_inlines.hpp"
#include "vm/function.hpp"
#include "vm/property_dict.hpp"

namespace Whisper {
namespace VM {


PropertyDescriptor
PropertyDict::Entry::descriptor() const
{
    uint8_t kindValue = KindBitfield::Const(flags_).value();
    if (kindValue == SLOT_KIND) {
        PropertySlotInfo slotInfo;
        slotInfo.setWritable(SlotIsWritableBitfield::Const(flags_).value());

        return PropertyDescriptor::MakeSlot(ValBox(value), slotInfo);
    }

    WH_ASSERT(kindValue == METHOD_KIND);
    return PropertyDescriptor::MakeMethod(value->pointer<Function>());
}

void
PropertyDict::Entry::init(String* name, PropertyDescriptor const& descr,
                          PropertyDict* holderDict)
{
    this->name.init(name, holderDict);
    initDescriptor(descr, holderDict);
}

void
PropertyDict::Entry::initDescriptor(PropertyDescriptor const& descr,
                                    PropertyDict* holderDict)
{
    flags_ = 0;
    if (descr.isSlot()) {
        KindBitfield(flags_).setValue(SLOT_KIND);
        value.init(descr.slotValue(), holderDict);

        PropertySlotInfo slotInfo = descr.slotInfo();
        SlotIsWritableBitfield(flags_).setValue(slotInfo.isWritable());
    } else {
        KindBitfield(flags_).setValue(METHOD_KIND);
        value.init(Box::Pointer(descr.methodFunction()), holderDict);
    }
}

/* static */ Result<PropertyDict*>
PropertyDict::Create(AllocationContext acx, uint32_t capacity)
{
    return acx.createSized<PropertyDict>(CalculateSize(capacity), capacity);
}

/* static */ Result<PropertyDict*>
PropertyDict::CreateEnlarged(AllocationContext acx,
                             Handle<PropertyDict*> propDict)
{
    uint32_t newCapacity = propDict->capacity() * 2;
    Result<PropertyDict*> newDictResult = Create(acx, newCapacity);
    if (!newDictResult)
        return ErrorVal();

    // Copy entries.
    Local<PropertyDict*> newDict(acx, newDictResult.value());
    for (uint32_t i = 0; i < propDict->capacity(); i++) {
        Entry &ent = propDict->entries_[i];
        if (!ent.name.get() || ent.name.get() == SENTINEL())
            continue;

        newDict->addEntry(ent.name.get(), ent.descriptor());
    }
    return OkVal(newDict.get());
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

PropertyDescriptor
PropertyDict::descriptor(uint32_t idx) const
{
    WH_ASSERT(isValidEntry(idx));
    return entries_[idx].descriptor();
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
        entries_[probe].init(name, descr, this);
        size_++;

        return Maybe<uint32_t>::Some(probe);
    }
}


} // namespace VM
} // namespace Whisper
