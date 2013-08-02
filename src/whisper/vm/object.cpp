
#include "vm/object.hpp"
#include "value_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/shape_tree_inlines.hpp"
#include <algorithm>

namespace Whisper {
namespace VM {


Object::Object(Shape *shape, Object *prototype)
  : ShapedHeapThing(shape)
{
    prototype_ = prototype;
    dynamicSlots_ = nullptr;
}

Object *
Object::prototype() const
{
    return prototype_;
}

bool
Object::hasDynamicSlots() const
{
    return dynamicSlots_.hasHeapThing();
}

Tuple *
Object::dynamicSlots() const
{
    return dynamicSlots_;
}

uint32_t
Object::numFixedSlots() const
{
    return objectValueCount() - FixedSlotsStart;
}

uint32_t
Object::numDynamicSlots() const
{
    // This needs to search the shape lineage for the highest
    // numbered, writable, dynamic value slot.
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
Object::numSlots() const
{
    return numFixedSlots() + numDynamicSlots();
}

const Value &
Object::fixedSlotValue(uint32_t idx) const
{
    WH_ASSERT(idx < numFixedSlots());
    return valueRef(idx + FixedSlotsStart);
}

const Value &
Object::dynamicSlotValue(uint32_t idx) const
{
    WH_ASSERT(idx < numDynamicSlots());
    return dynamicSlots_->element(idx);
}

const Value &
Object::slotValue(uint32_t idx) const
{
    WH_ASSERT(idx < numSlots());
    if (idx < numFixedSlots())
        return fixedSlotValue(idx);

    return dynamicSlotValue(idx - numFixedSlots());
}


} // namespace VM
} // namespace Whisper
