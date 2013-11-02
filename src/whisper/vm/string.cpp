
#include "rooting_inlines.hpp"
#include "vm/string.hpp"
#include "vm/heap_thing_inlines.hpp"

#include <algorithm>

namespace Whisper {
namespace VM {


//
// HeapString
//

HeapString::HeapString()
{}

const HeapThing *
HeapString::toHeapThing() const
{
    return reinterpret_cast<const HeapThing *>(this);
}

HeapThing *
HeapString::toHeapThing()
{
    return reinterpret_cast<HeapThing *>(this);
}

#if defined(ENABLE_DEBUG)
bool
HeapString::isValidString() const
{
    return isLinearString();
}
#endif

bool
HeapString::isLinearString() const
{
    return toHeapThing()->type() == HeapType::LinearString;
}

const LinearString *
HeapString::toLinearString() const
{
    WH_ASSERT(isLinearString());
    return reinterpret_cast<const LinearString *>(this);
}

LinearString *
HeapString::toLinearString()
{
    WH_ASSERT(isLinearString());
    return reinterpret_cast<LinearString *>(this);
}

bool
HeapString::linearize(RunContext *cx, MutHandle<LinearString *> out)
{
    WH_ASSERT(isLinearString());
    out = toLinearString();
    return true;
}

uint32_t
HeapString::length() const
{
    WH_ASSERT(isLinearString());
    return toLinearString()->length();
}

uint16_t
HeapString::getChar(uint32_t idx) const
{
    WH_ASSERT(isLinearString());
    return toLinearString()->getChar(idx);
}

bool
HeapString::fitsImmediate() const
{
    uint32_t len = length();

    if (len <= Value::ImmString16MaxLength)
        return true;

    if (len > Value::ImmString8MaxLength)
        return false;

    // Maybe fits in an 8-bit immediate string.  Check to see if all
    // chars are 8-bit.
    for (uint32_t i = 0; i < len; i++) {
        uint16_t ch = getChar(i);
        if (ch > 0xFFu)
            return false;
    }

    return true;
}

//
// LinearString
//

void
LinearString::initializeFlags(bool interned, Group group)
{
    uint32_t flags = 0;
    if (interned)
        flags |= InternedFlagMask;
    flags |= static_cast<uint32_t>(group) << GroupShift;
    initFlags(flags);
}

uint16_t *
LinearString::writableData()
{
    return recastThis<uint16_t>();
}

LinearString::LinearString(const HeapString *str, bool interned, Group group)
{
    WH_ASSERT(length() == str->length());

    initializeFlags(interned, group);

    // Only LinearString possible for now.
    WH_ASSERT(str->isLinearString());
    const LinearString *linStr = str->toLinearString();

    const uint16_t *data = linStr->data();
    std::copy(data, data + length(), writableData());

}

LinearString::LinearString(const uint8_t *data, bool interned, Group group)
{
    initializeFlags(interned, group);
    std::copy(data, data + length(), writableData());
}

LinearString::LinearString(const uint16_t *data, bool interned, Group group)
{
    initializeFlags(interned, group);
    std::copy(data, data + length(), writableData());
}

const uint16_t *
LinearString::data() const
{
    return recastThis<uint16_t>();
}

bool
LinearString::isInterned() const
{
    return flags() & InternedFlagMask;
}

LinearString::Group
LinearString::group() const
{
    return static_cast<Group>((flags() >> GroupShift) & GroupMask);
}

bool
LinearString::inUnknownGroup() const
{
    return group() == Group::Unknown;
}

bool
LinearString::inIndexGroup() const
{
    return group() == Group::Index;
}

bool
LinearString::inNameGroup() const
{
    return group() == Group::Name;
}

uint32_t
LinearString::length() const
{
    WH_ASSERT(objectSize() % 2 == 0);
    return objectSize() / 2;
}

uint16_t
LinearString::getChar(uint32_t idx) const
{
    WH_ASSERT(idx < length());
    return data()[idx];
}


} // namespace VM
} // namespace Whisper
