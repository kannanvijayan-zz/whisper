#ifndef WHISPER__VM__STRING_HPP
#define WHISPER__VM__STRING_HPP

#include "common.hpp"
#include "debug.hpp"
#include "vm/heap_type_defn.hpp"
#include "vm/heap_thing.hpp"

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
struct HeapString : public HeapThing,
                    public TypedHeapThing<HeapType::HeapString>
{
  public:
    static constexpr uint32_t EightBitFlagMask = 0x1;

  protected:
    uint8_t *writableEightBitData();
    uint16_t *writableSixteenBitData();
    
  public:
    HeapString(const uint8_t *data);
    HeapString(const uint16_t *data);

    bool isEightBit() const;

    const uint8_t *eightBitData() const;

    bool isSixteenBit() const;

    const uint16_t *sixteenBitData() const;

    uint32_t length() const;

    uint16_t getChar(uint32_t idx) const;
};

typedef HeapThingWrapper<HeapString> WrappedHeapString;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__STRING_HPP
