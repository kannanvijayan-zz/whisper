
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

uint8_t *
LinearString::writableEightBitData()
{
    WH_ASSERT(isEightBit());
    return recastThis<uint8_t>();
}

uint16_t *
LinearString::writableSixteenBitData()
{
    WH_ASSERT(isSixteenBit());
    return recastThis<uint16_t>();
}

LinearString::LinearString(const uint8_t *data)
{
    initFlags(EightBitFlagMask);
    std::copy(data, data + objectSize(), writableEightBitData());
}

LinearString::LinearString(const uint16_t *data)
{
    WH_ASSERT(objectSize() % 2 == 0);
    std::copy(data, data + (objectSize() / 2), writableSixteenBitData());
}

LinearString::LinearString(const uint8_t *data, bool interned, bool propName)
{
    uint32_t flags = EightBitFlagMask;
    if (interned)
        flags |= InternedFlagMask;
    if (propName)
        flags |= PropertyNameFlagMask;
    initFlags(flags);
    std::copy(data, data + objectSize(), writableEightBitData());
}

LinearString::LinearString(const uint16_t *data, bool interned, bool propName)
{
    uint32_t flags = 0;
    if (interned)
        flags |= InternedFlagMask;
    if (propName)
        flags |= PropertyNameFlagMask;
    initFlags(flags);
    std::copy(data, data + objectSize() / 2, writableSixteenBitData());
}

bool
LinearString::isEightBit() const
{
    return flags() & EightBitFlagMask;
}

const uint8_t *
LinearString::eightBitData() const
{
    WH_ASSERT(isEightBit());
    return recastThis<uint8_t>();
}

bool
LinearString::isSixteenBit() const
{
    return !isEightBit();
}

const uint16_t *
LinearString::sixteenBitData() const
{
    WH_ASSERT(isSixteenBit());
    return recastThis<uint16_t>();
}

bool
LinearString::isInterned() const
{
    return flags() & InternedFlagMask;
}

bool
LinearString::isPropertyName() const
{
    return flags() & PropertyNameFlagMask;
}

uint32_t
LinearString::length() const
{
    if (isEightBit())
        return objectSize();
    WH_ASSERT(objectSize() % 2 == 0);
    return objectSize() / 2;
}

uint16_t
LinearString::getChar(uint32_t idx) const
{
    WH_ASSERT(idx < length());
    if (isEightBit())
        return eightBitData()[idx];
    return sixteenBitData()[idx];
}


} // namespace VM
} // namespace Whisper
