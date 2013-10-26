
#include "rooting_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/tuple.hpp"

namespace Whisper {
namespace VM {


Tuple::Tuple() : HeapThing()
{
    uint32_t vals = size();
    for (uint32_t i = 0; i < vals; i++)
        element(i).set(Value::Undefined(), this);
}

Tuple::Tuple(const Tuple &other) : HeapThing(other)
{
    uint32_t vals = size();
    uint32_t otherVals = other.size();
    if (vals <= otherVals) {
        for (uint32_t i = 0; i < vals; i++)
            set(i, other.element(i));
        return;
    }

    uint32_t i;
    for (i = 0; i < otherVals; i++)
        set(i, other.element(i));
    for (; i < vals; i++)
        set(i, Value::Undefined());
}

Tuple::Tuple(const Value *vals) : HeapThing()
{
    uint32_t count = size();
    for (uint32_t i = 0; i < count; i++)
        element(i).set(vals[i], this);
}

uint32_t
Tuple::size() const
{
    WH_ASSERT(IsIntAligned<uint32_t>(objectSize(), sizeof(Value)));
    return objectSize() / sizeof(Value);
}

Handle<Value>
Tuple::get(uint32_t idx) const
{
    WH_ASSERT(idx < size());
    return element(idx);
}

Handle<Value>
Tuple::operator [](uint32_t idx) const
{
    return get(idx);
}

void
Tuple::set(uint32_t idx, const Value &val)
{
    element(idx).set(val, this);
}

const Heap<Value> &
Tuple::element(uint32_t idx) const
{
    WH_ASSERT(idx < size());
    const Value *thisp = reinterpret_cast<const Value *>(this);
    return reinterpret_cast<const Heap<Value> &>(thisp[idx]);
}

Heap<Value> &
Tuple::element(uint32_t idx)
{
    WH_ASSERT(idx < size());
    Value *thisp = reinterpret_cast<Value *>(this);
    return reinterpret_cast<Heap<Value> &>(thisp[idx]);
}


} // namespace VM
} // namespace Whisper
