#ifndef WHISPER__VM__HEAP_THING_HPP
#define WHISPER__VM__HEAP_THING_HPP

#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "value.hpp"
#include "vm/heap_type_defn.hpp"

namespace Whisper {
namespace VM {

//
// Pre-declare heap thing classes.
//

#define PREDEC_(t, ...) class t;
    WHISPER_DEFN_HEAP_TYPES(PREDEC_)
#undef PREDEC_

//
// Enum of all possible heap thing types.
//
enum class HeapType : uint32_t
{
    INVALID = 0,

#define ENUM_(t, ...) t,
    WHISPER_DEFN_HEAP_TYPES(ENUM_)
#undef ENUM_

    LIMIT
};

inline bool
IsValidHeapType(HeapType ht) {
    return (ht > HeapType::INVALID) && (ht < HeapType::LIMIT);
}

const char *HeapTypeString(HeapType ht);

void SpewHeapThingSlab(Slab *slab);

template <HeapType HT> struct HeapTypeTraits {};
#define TRAITS_(t, traced) \
    template <> struct HeapTypeTraits<HeapType::t> { \
        static constexpr bool Traced = traced; \
    };
    WHISPER_DEFN_HEAP_TYPES(TRAITS_)
#undef TRAITS_


//
// HeapThingHeader
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
// 0000-FFFF FFFF-SSSS SSSS-SSSS SSSS-SSSS
//
// 32        24        16        08
// SSSS-SSSS SSSS-TTTT TTTT-00CC CCCC-CCCC
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
//  FFFF-FFFF
//      This 8-bit field has an interpretation that depends on the type.
//      It's basically a small number of "free" bits which a type can
//      use to track information about an object.
//

class HeapThingHeader
{
  friend class HeapThing;

  template <typename T>
  friend class HeapThingWrapper;

  private:
    uint64_t header_ = 0;

  public:
    static constexpr uint32_t HeaderSize = sizeof(uint64_t);

    static constexpr uint64_t CardNoBits = 10;
    static constexpr uint64_t CardNoMask = (1ULL << CardNoBits) - 1;
    static constexpr unsigned CardNoShift = 0;

    static constexpr uint64_t TypeBits = 8;
    static constexpr uint64_t TypeMask = (1ULL << TypeBits) - 1;
    static constexpr unsigned TypeShift = 12;

    static constexpr uint64_t SizeBits = 32;
    static constexpr uint64_t SizeMask = UINT32_MAX;
    static constexpr unsigned SizeShift = 20;

    static constexpr uint64_t FlagsBits = 8;
    static constexpr uint64_t FlagsMask = (1ULL << FlagsBits) - 1;
    static constexpr unsigned FlagsShift = 52;

  protected:
    HeapThingHeader(HeapType type, uint32_t cardNo, uint32_t size);

  public:

    uint32_t cardNo() const;

    HeapType type() const;

    uint32_t size() const;

    uint32_t flags() const;

  protected:
    void initFlags(uint32_t fl);
    void addFlags(uint32_t fl);
};

//
// HeapThingWrapper establishes a wrapper for payload structs which are
// a subcomponent of heap things.
//
template <typename T>
class HeapThingWrapper
{
  private:
    HeapThingHeader header_;
    T payload_;

  public:
    template <typename... T_ARGS>
    inline HeapThingWrapper(uint32_t cardNo, uint32_t size, T_ARGS... tArgs);

    inline const HeapThingHeader &header() const;

    inline HeapThingHeader &header();

    inline const HeapThingHeader *headerPointer() const;

    inline HeapThingHeader *headerPointer();

    inline const T &payload() const;

    inline T &payload();

    inline const T *payloadPointer() const;

    inline T *payloadPointer();
};

//
// HeapThing is a base class for heap thing payload classes, with protected
// helper methods for accessing the header.
//
class HeapThing
{
  protected:
    HeapThing();
    ~HeapThing();

    template <typename PtrT>
    inline PtrT *recastThis();

    template <typename PtrT>
    inline const PtrT *recastThis() const;

    HeapThingHeader *header();

    const HeapThingHeader *header() const;

    void initFlags(uint32_t flags);
    void addFlags(uint32_t flags);

    // Write barrier helper
    void noteWrite(void *ptr);

  public:
    uint32_t cardNo() const;

    HeapType type() const;

    uint32_t objectSize() const;

    uint32_t flags() const;

    uint32_t reservedSpace() const;

#define PRED_(t, ...) \
    inline bool is##t() const { \
        return type() == HeapType::t; \
    }
    WHISPER_DEFN_HEAP_TYPES(PRED_)
#undef PRED_

#define CONV_(t, ...) \
    inline const t *to##t() const { \
        WH_ASSERT(is##t()); \
        return reinterpret_cast<const t *>(this);  \
    } \
    inline t *to##t() { \
        WH_ASSERT(is##t()); \
        return reinterpret_cast<t *>(this);  \
    }
    WHISPER_DEFN_HEAP_TYPES(CONV_)
#undef CONV_
};

template <HeapType HT>
class TypedHeapThing
{
  public:
    static constexpr HeapType Type = HT;

  protected:
    inline TypedHeapThing() {}
    inline ~TypedHeapThing() {}
};

// A Value subclass that allows heap things or undefined.
template <typename T, bool Null=false> class HeapThingValue {};

} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__HEAP_THING_HPP
