#ifndef WHISPER__VM__HEAP_THING_HPP
#define WHISPER__VM__HEAP_THING_HPP

#include "common.hpp"
#include "debug.hpp"
#include "vm/heap_type_defn.hpp"

#include <limits>

namespace Whisper {
namespace VM {


//
// Enum of all possible heap thing types.
//
enum class HeapType : uint32_t
{
    INVALID = 0,

#define ENUM_(t) t,
    WHISPER_DEFN_HEAP_TYPES(ENUM_)
#undef ENUM_

    LIMIT
};

inline bool
IsValidHeapType(HeapType ht) {
    return (ht > HeapType::INVALID) && (ht < HeapType::LIMIT);
}


//
// HeapThing
//
// Defines the base structure for all garbage-collected heap things.
// All heap things are composed of a header word, followed by one
// or more payload words.
//
// Pointers to heap things, however, do not point to the header word.
// Instead, pointers reference the first data word.  The header word
// can be accessed by subtracting 8 bytes from the pointer.
//
// A heap thing header word has the following structure:
//
// 64        56        48        40
// 1111-FFFF SSSS-SSSS SSSS-SSSS SSSS-SSSS
//
// 32        24        16        08
// SSSS-SSSS TTTT-TTTT 0000-CCCC CCCC-CCCC
//
// The bitfields are as follows:
//
//  CCCC-CCCC CCCC
//      Holds the card number of the object.  The card number is the
//      distance (in cards) between the start of the object space in the
//      slab, and the address of the card holding the header word of
//      the object.
//
//      It is used to quickly calculate the address of a barrier byte
//      corresponding to a particular word within the object's payload.
//
//  TTTT-TTTT
//      The object's type.  Up to 256 types should be good enough for now,
//      but we have 4 unused bits if we need more.
//
//  SSSS-SSSS SSSS-SSSS SSSS-SSSS SSSS-SSSS
//      The size of the object, in bytes.  The actual amount of space used
//      for the object is this size aligned-up to 8 (64 bits).
//
//      However, as some objects may have a useful notion of a byte-size
//      (e.g. strings), this field actually stores the the string size.
//
//      For normal objects which are composed of values, the size will
//      be a multiple of 8.
//
//  FFFF
//      This 4-bit field has an interpretation that depends on the type.
//      It's basically a small number of "free" bits which a type can
//      use to track information about an object.
//

class HeapThing
{
  protected:
    uint64_t header_ = 0;

    static constexpr uint64_t CardNoBits = 12;
    static constexpr uint64_t CardNoMask = (1ULL << CardNoBits) - 1;
    static constexpr unsigned CardNoShift = 0;

    static constexpr uint64_t TypeBits = 8;
    static constexpr uint64_t TypeMask = (1ULL << TypeBits) - 1;
    static constexpr unsigned TypeShift = 16;

    static constexpr uint64_t SizeBits = 64;
    static constexpr uint64_t SizeMask = UINT32_MAX;
    static constexpr unsigned SizeShift = 24;

    static constexpr uint64_t FlagsBits = 4;
    static constexpr uint64_t FlagsMask = (1ULL << FlagsBits) - 1;
    static constexpr unsigned FlagsShift = 56;

    static constexpr uint64_t Tag = 0xFu;
    static constexpr unsigned TagShift = 60;

    inline HeapThing(HeapType type, uint64_t cardNo, uint64_t size,
                     uint64_t flags)
    {
        WH_ASSERT(IsValidHeapType(type));
        WH_ASSERT(cardNo <= CardNoMask);
        WH_ASSERT(size <= SizeMask);
        WH_ASSERT(flags <= FlagsMask);

        header_ |= Tag << TagShift;
        header_ |= flags << FlagsShift;
        header_ |= size << SizeShift;
        header_ |= static_cast<uint64_t>(type) << TypeShift;
        header_ |= cardNo << CardNoShift;
    }

  public:

    inline uint32_t cardNo() const {
        return (header_ >> CardNoShift) & CardNoMask;
    }

    inline uint32_t type() const {
        return (header_ >> TypeShift) & TypeMask;
    }

    inline uint32_t size() const {
        return (header_ >> SizeShift) & SizeMask;
    }

    inline uint32_t flags() const {
        return (header_ >> FlagsShift) & FlagsMask;
    }
};

//
// HeapThingWrapper establishes a wrapper for payload structs which are
// a subcomponent of heap things.
//
template <typename T, HeapType HT>
class HeapThingWrapper : public HeapThing
{
  private:
    T payload_;

  public:
    template <typename... T_ARGS>
    inline HeapThingWrapper(uint64_t cardNo, uint64_t size, uint64_t flags,
                            T_ARGS... tArgs)
      : HeapThing(HT, cardNo, size, flags),
        payload_(tArgs...)
    {}
};

//
// HeapThingPayload is a base class for payload classes, with protected
// helper methods for accessing the header word.
//
template <typename T, HeapType HT>
class HeapThingPayload
{
  protected:
    inline HeapThingPayload() {};
    inline ~HeapThingPayload() {};

    inline HeapThingWrapper<T, HT> *heapThingWrapper() {
        uint64_t *thisp = static_cast<uint64_t *>(this);
        return static_cast<HeapThingWrapper<T, HT> *>(thisp - 1);
    }

    inline const HeapThingWrapper<T, HT> *heapThingWrapper() const {
        const uint64_t *thisp = static_cast<const uint64_t *>(this);
        return static_cast<const HeapThingWrapper<T, HT> *>(thisp - 1);
    }
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__HEAP_THING_HPP
