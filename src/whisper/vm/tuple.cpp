
#include "tuple.hpp"
#include "vm/heap_thing_inlines.hpp"

namespace Whisper {
namespace VM {


Tuple::Tuple() : HeapThing()
{
    uint32_t vals = objectValueCount();
    for (uint32_t i = 0; i < vals; i++)
        valueRef(i) = UndefinedValue();
}

Tuple::Tuple(const Tuple &other) : HeapThing(other)
{
    uint32_t vals = objectValueCount();
    uint32_t otherVals = other.objectValueCount();
    if (vals <= otherVals) {
        for (uint32_t i = 0; i < vals; i++)
            valueRef(i) = other.valueRef(i);
    } else {
        uint32_t i;
        for (i = 0; i < otherVals; i++)
            valueRef(i) = other.valueRef(i);
        for (; i < vals; i++)
            valueRef(i) = UndefinedValue();
    }
}

const Value &
Tuple::element(uint32_t idx) const
{
    return valueRef(idx);
}

Value &
Tuple::element(uint32_t idx)
{
    return valueRef(idx);
}

const Value &
Tuple::operator [](uint32_t idx) const
{
    return valueRef(idx);
}

Value &
Tuple::operator [](uint32_t idx)
{
    return valueRef(idx);
}

uint32_t
Tuple::size() const
{
    return objectValueCount();
}


} // namespace VM
} // namespace Whisper
