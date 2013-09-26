#ifndef WHISPER__RUNTIME__HEAP_THING_HPP
#define WHISPER__RUNTIME__HEAP_THING_HPP

#include "common.hpp"
#include "debug.hpp"

namespace Whisper {
namespace Runtime {


#define WHISPER_DEFN_HEAP_TYPES(_)      \
    _(String)                           \
    _(Module)                           \
    _(ModuleEntry)

enum class HeapType
{
    INVALID = 0,
#define ENUM_(t)    t,
    WHISPER_DEFN_HEAP_TYPES(ENUM_)
#undef ENUM_
    LIMIT
};

bool IsValidHeapType(HeapType type);
const char *HeapTypeString(HeapType type);


//
// The header word for a heap thing is composed of a 32-bit word:
//
//      CCCC CCCC CCCC 0000 0000 0000 TTTT TTTT
//        28   24   20   16   12    8    4    0
//
// CCCC CCCC CCCC
//  Card number.  Identifies the card in which this exists.
//
// TTTT TTTT
//  The type of the object.  The size of an object can be determind
//  using its type.
//

class HeapThingHeader
{
  public:
    static constexpr unsigned CARD_BITS = 12;
    static constexpr unsigned CARD_SHIFT = 20;
    static constexpr uint32_t MAX_CARD = 0xFFF;

    static constexpr unsigned TYPE_BITS = 8;
    static constexpr unsigned TYPE_SHIFT = 0;
    static constexpr uint32_t MAX_TYPE = 0xFF;

  private:
    word_t data_;

    static inline uint32_t MakeData(uint32_t card, HeapType type)
    {
        WH_ASSERT(card <= MAX_CARD);
        WH_ASSERT(IsValidHeapType(type));

        return (card << CARD_SHIFT) |
               (static_cast<uint32_t>(type) << TYPE_SHIFT);
    }

  public:
    HeapThingHeader(uint32_t card, HeapType type)
      : data_(MakeData(card, type))
    {}

    inline uint32_t card() const {
        return (data_ >> CARD_SHIFT) & MAX_CARD;
    }

    inline HeapType type() const {
        return static_cast<HeapType>((data_ >> TYPE_SHIFT) & MAX_TYPE);
    }
};

class HeapThing
{
  protected:
    inline HeapThing() {}

  public:
    inline const HeapThingHeader *heapThingHeader() const {
        return reinterpret_cast<const HeapThingHeader *>(this) - 1;
    }
};


//
// The following class should be specialized for every terminal subtype
// of HeapThing, and endowed with the following fields:
//
//      HeapType HEAP_TYPE  - the HeapType value for that type.
//      bool     TERMINAL   - If true, objects of this type are not traced.
//
template <typename T>
class HeapThingTraits
{
    // static constexpr HeapType HEAP_TYPE = HeapType::...;
};


} // namespace Runtime
} // namespace Whisper

#endif // WHISPER__RUNTIME__HEAP_THING_HPP
