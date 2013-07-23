#ifndef WHISPER__VM__STRING_HPP
#define WHISPER__VM__STRING_HPP

#include "common.hpp"
#include "debug.hpp"
#include "vm/heap_type_defn.hpp"
#include "vm/heap_thing.hpp"

#include <limits>
#include <algorithm>

namespace Whisper {
namespace VM {


//
// String objects can be composed of 8 or 16 bit characters.
// The character class is identified by the low-bit flag on
// the string.
//
//      +-----------------------+
//      | Header                |
//      +-----------------------+
//      | String Data           |
//      | ...                   |
//      | ...                   |
//      +-----------------------+
//
struct HeapString : public HeapThing<HeapType::HeapString>
{
  public:
    static constexpr uint32_t EightBitFlagMask = 0x1;

  protected:
    uint8_t *eightBitData() {
        return recastThis<uint8_t>();
    }
    const uint8_t *eightBitData() const {
        return recastThis<uint8_t>();
    }

    uint16_t *sixteenBitData() {
        return recastThis<uint16_t>();
    }
    const uint16_t *sixteenBitData() const {
        return recastThis<uint16_t>();
    }
    
  public:
    HeapString(const uint8_t *data) {
        initFlags(EightBitFlagMask);
        std::copy(data, data + objectSize(), eightBitData());
    }
    HeapString(const uint16_t *data) {
        WH_ASSERT(objectSize() % 2 == 0);
        std::copy(data, data + (objectSize() / 2), sixteenBitData());
    }

    inline bool isEightBit() const {
        return flags() & EightBitFlagMask;
    }

    inline bool isSixteenBit() const {
        return !isEightBit();
    }

    inline uint32_t length() const {
        if (isEightBit())
            return objectSize();
        return objectSize() / 2;
    }

    inline uint16_t getChar(uint32_t idx) const {
        WH_ASSERT(idx < length());
        if (isEightBit())
            return eightBitData()[idx];
        return sixteenBitData()[idx];
    }
};

typedef HeapThingWrapper<HeapString> WrappedHeapString;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__STRING_HPP
