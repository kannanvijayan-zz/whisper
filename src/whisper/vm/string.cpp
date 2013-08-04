
#include "vm/string.hpp"
#include "vm/heap_thing_inlines.hpp"

#include <algorithm>

namespace Whisper {
namespace VM {


uint8_t *
HeapString::writableEightBitData()
{
    return recastThis<uint8_t>();
}

uint16_t *
HeapString::writableSixteenBitData()
{
    return recastThis<uint16_t>();
}

HeapString::HeapString(const uint8_t *data)
{
    initFlags(EightBitFlagMask);
    std::copy(data, data + objectSize(), writableEightBitData());
}

HeapString::HeapString(const uint16_t *data)
{
    WH_ASSERT(objectSize() % 2 == 0);
    std::copy(data, data + (objectSize() / 2), writableSixteenBitData());
}

bool
HeapString::isEightBit() const
{
    return flags() & EightBitFlagMask;
}

const uint8_t *
HeapString::eightBitData() const
{
    WH_ASSERT(isEightBit());
    return recastThis<uint8_t>();
}

bool
HeapString::isSixteenBit() const
{
    return !isEightBit();
}

const uint16_t *
HeapString::sixteenBitData() const
{
    WH_ASSERT(isSixteenBit());
    return recastThis<uint16_t>();
}

uint32_t
HeapString::length() const
{
    if (isEightBit())
        return objectSize();
    WH_ASSERT(objectSize() % 2 == 0);
    return objectSize() / 2;
}

uint16_t
HeapString::getChar(uint32_t idx) const
{
    WH_ASSERT(idx < length());
    if (isEightBit())
        return eightBitData()[idx];
    return sixteenBitData()[idx];
}


} // namespace VM
} // namespace Whisper
