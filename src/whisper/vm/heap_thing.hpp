#ifndef WHISPER__VM__HEAP_THING_HPP
#define WHISPER__VM__HEAP_THING_HPP

#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "value.hpp"
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

#define ENUM_(t, ...) t,
    WHISPER_DEFN_HEAP_TYPES(ENUM_)
#undef ENUM_

    LIMIT
};

inline bool
IsValidHeapType(HeapType ht) {
    return (ht > HeapType::INVALID) && (ht < HeapType::LIMIT);
}

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
// 1111-00FF FFFF-SSSS SSSS-SSSS SSSS-SSSS
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
//  FF-FFFF
//      This 6-bit field has an interpretation that depends on the type.
//      It's basically a small number of "free" bits which a type can
//      use to track information about an object.
//

class HeapThingHeader
{
  friend class UntypedHeapThing;

  protected:
    uint64_t header_ = 0;

    static constexpr uint32_t HeaderSize = sizeof(uint64_t);

    static constexpr uint64_t CardNoBits = 10;
    static constexpr uint64_t CardNoMask = (1ULL << CardNoBits) - 1;
    static constexpr unsigned CardNoShift = 0;

    static constexpr uint64_t TypeBits = 8;
    static constexpr uint64_t TypeMask = (1ULL << TypeBits) - 1;
    static constexpr unsigned TypeShift = 12;

    static constexpr uint64_t SizeBits = 32;
    static constexpr uint64_t SizeMask = UINT32_MAX;
    static constexpr unsigned SizeShift = 16;

    static constexpr uint64_t FlagsBits = 6;
    static constexpr uint64_t FlagsMask = (1ULL << FlagsBits) - 1;
    static constexpr unsigned FlagsShift = 52;

    static constexpr uint64_t Tag = 0xFu;
    static constexpr unsigned TagShift = 60;

    HeapThingHeader(HeapType type, uint32_t cardNo, uint32_t size)
    {
        WH_ASSERT(IsValidHeapType(type));
        WH_ASSERT(cardNo <= CardNoMask);
        WH_ASSERT(size <= SizeMask);

        header_ |= Tag << TagShift;
        header_ |= static_cast<uint64_t>(size) << SizeShift;
        header_ |= static_cast<uint64_t>(type) << TypeShift;
        header_ |= static_cast<uint64_t>(cardNo) << CardNoShift;
    }

  public:

    uint32_t cardNo() const {
        return (header_ >> CardNoShift) & CardNoMask;
    }

    HeapType type() const {
        return static_cast<HeapType>((header_ >> TypeShift) & TypeMask);
    }

    uint32_t size() const {
        return (header_ >> SizeShift) & SizeMask;
    }

    uint32_t flags() const {
        return (header_ >> FlagsShift) & FlagsMask;
    }

  protected:
    void initFlags(uint32_t fl) {
        WH_ASSERT(fl <= FlagsMask);
        WH_ASSERT(flags() == 0);
        header_ |= static_cast<uint64_t>(fl) << FlagsShift;
    }
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
    HeapThingWrapper(uint32_t cardNo, uint32_t size, T_ARGS... tArgs)
      : header_(T::Type, cardNo, size),
        payload_(tArgs...)
    {}

    const HeapThingHeader &header() const {
        return header_;
    }

    HeapThingHeader &header() {
        return header_;
    }

    const HeapThingHeader *headerPointer() const {
        return &header_;
    }

    HeapThingHeader *headerPointer() {
        return &header_;
    }

    const T &payload() const {
        return payload_;
    }

    T &payload() {
        return payload_;
    }

    const T *payloadPointer() const {
        return &payload_;
    }

    T *payloadPointer() {
        return &payload_;
    }
};

//
// HeapThing is a base class for heap thing payload classes, with protected
// helper methods for accessing the header.
//
class UntypedHeapThing
{
  protected:
    UntypedHeapThing() {};
    ~UntypedHeapThing() {};

    template <typename PtrT>
    PtrT *recastThis() {
        return reinterpret_cast<PtrT *>(this);
    }

    template <typename PtrT>
    const PtrT *recastThis() const {
        return reinterpret_cast<const PtrT *>(this);
    }

    HeapThingHeader *header() {
        uint64_t *thisp = recastThis<uint64_t>();
        return reinterpret_cast<HeapThingHeader *>(thisp - 1);
    }

    const HeapThingHeader *header() const {
        const uint64_t *thisp = recastThis<const uint64_t>();
        return reinterpret_cast<const HeapThingHeader *>(thisp - 1);
    }

    void initFlags(uint32_t flags) {
        return header()->initFlags(flags);
    }

    // Write barrier helper
    void noteWrite(void *ptr) {
        // TODO: Add write barrier.
    }
};

template <HeapType HT>
class HeapThing : public UntypedHeapThing
{
  public:
    static constexpr HeapType Type = HT;

  protected:
    HeapThing() {};
    ~HeapThing() {};

  public:
    uint32_t cardNo() const {
        return header()->cardNo();
    }

    HeapType type() const {
        return header()->type();
    }

    uint32_t objectSize() const {
        return header()->size();
    }

    uint32_t objectValueCount() const {
        WH_ASSERT(IsIntAligned<uint32_t>(objectSize(), sizeof(Value)));
        return objectSize() / sizeof(Value);
    }

    uint32_t flags() const {
        return header()->flags();
    }

    uint32_t reservedSpace() const {
        return AlignIntUp<uint32_t>(objectSize(), Slab::AllocAlign);
    }

    template <typename T>
    T *dataPointer(uint32_t offset) {
        uint8_t *ptr = recastThis<uint8_t>() + offset;
        WH_ASSERT(IsPtrAligned(ptr, alignof(T)));
        return reinterpret_cast<T *>(ptr);
    }

    template <typename T>
    const T *dataPointer(uint32_t offset) const {
        const uint8_t *ptr = recastThis<uint8_t>() + offset;
        WH_ASSERT(IsPtrAligned(ptr, alignof(T)));
        return reinterpret_cast<const T *>(ptr);
    }

    template <typename T>
    T &dataRef(uint32_t offset) {
        return *dataPointer<T>(offset);
    }

    template <typename T>
    const T &dataRef(uint32_t offset) const {
        return *dataPointer<T>(offset);
    }

    Value *valuePointer(uint32_t idx) {
        return dataPointer<Value>(idx * 8);
    }

    const Value *valuePointer(uint32_t idx) const {
        return dataPointer<Value>(idx * 8);
    }

    Value &valueRef(uint32_t idx) {
        return dataRef<Value>(idx * 8);
    }

    const Value &valueRef(uint32_t idx) const {
        return dataRef<Value>(idx * 8);
    }
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__HEAP_THING_HPP
