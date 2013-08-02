
#include "value_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/property_map_thing.hpp"
#include "vm/object.hpp"

#include <algorithm>

namespace Whisper {
namespace VM {


/*static*/ uint32_t
PropertyMapThing::NumInternalSlots(HeapType ht)
{
    switch (ht) {
#define CASE_(type) \
      case HeapType::type: \
        return PropertyMapTypeTraits<HeapType::type>::NumInternalSlots;
    WHISPER_DEFN_PROPMAP_TYPES(CASE_)
#undef CASE_
      default:
        WH_UNREACHABLE("Invalid PropertyMapThing HeapType.");
        return INT32_MAX;
    }
}

PropertyMapThing::PropertyMapThing(Shape *shape, PropertyMapThing *prototype)
  : ShapedHeapThing(shape),
    prototype_(prototype)
{}

bool
PropertyMapThing::isExtensible() const
{
    return !(flags() & PreventExtensions);
}

void
PropertyMapThing::preventExtensions()
{
    WH_ASSERT(isExtensible());
    addFlags(PreventExtensions);
}

uint32_t
PropertyMapThing::numImplicitSlots() const
{
    return BaseImplicitSlots + NumInternalSlots(type());
}

PropertyMapThing *
PropertyMapThing::prototype() const
{
    return prototype_;
}

bool
PropertyMapThing::hasDynamicSlots() const
{
    return dynamicSlots_.hasHeapThing();
}

Tuple *
PropertyMapThing::maybeDynamicSlots() const
{
    return dynamicSlots_;
}

Tuple *
PropertyMapThing::dynamicSlots() const
{
    WH_ASSERT(hasDynamicSlots());
    return dynamicSlots_;
}

uint32_t
PropertyMapThing::numFixedSlots() const
{
    return objectValueCount() - numImplicitSlots();
}

uint32_t
PropertyMapThing::numDynamicSlots() const
{
    // This needs to search the shape lineage for the highest
    // numbered writable dynamic value slot.
    Shape *shape = shape_;
    WH_ASSERT(shape);

    uint32_t maxSlotIndex = 0;
    bool found = false;
    do {
        if (!shape->hasValue() || !shape->isWritable())
            continue;

        ValueShape *valueShape = shape->toValueShape();
        if (!valueShape->isDynamicSlot())
            continue;

        found = true;
        maxSlotIndex = std::max(maxSlotIndex, valueShape->slotIndex());

        shape = shape->maybeParent();

    } while (shape);

    if (!found)
        return 0;

    return maxSlotIndex + 1;
}

uint32_t
PropertyMapThing::numSlots() const
{
    return numFixedSlots() + numDynamicSlots();
}

const Value &
PropertyMapThing::fixedSlotValue(uint32_t idx) const
{
    WH_ASSERT(idx < numFixedSlots());
    return valueRef(idx + numImplicitSlots());
}

const Value &
PropertyMapThing::dynamicSlotValue(uint32_t idx) const
{
    WH_ASSERT(idx < numDynamicSlots());
    return dynamicSlots_->element(idx);
}

const Value &
PropertyMapThing::slotValue(uint32_t idx) const
{
    WH_ASSERT(idx < numSlots());
    if (idx < numFixedSlots())
        return fixedSlotValue(idx);

    return dynamicSlotValue(idx - numFixedSlots());
}


} // namespace VM
} // namespace Whisper
