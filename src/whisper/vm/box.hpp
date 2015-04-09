#ifndef WHISPER__VM__BOX_HPP
#define WHISPER__VM__BOX_HPP


#include <cstdio>

#include "vm/core.hpp"
#include "vm/array.hpp"

namespace Whisper {
namespace VM {

//
// A Box wraps either a pointer or immediate value.
//
class Box
{
    friend class TraceTraits<Box>;
  private:
    uint64_t value_;

  public:
    static constexpr uint64_t PointerAlign = 0x8;

    static constexpr uint64_t PointerTagMask = 0x7;
    static constexpr uint64_t PointerTag = 0x0;

    static constexpr uint64_t IntegerTagMask = 0x7;
    static constexpr uint64_t IntegerTag = 0x1;
    static constexpr unsigned IntegerShift = 3;
    static constexpr int64_t IntegerMax =
        std::numeric_limits<int64_t>::max() >> IntegerShift;
    static constexpr int64_t IntegerMin =
        std::numeric_limits<int64_t>::min() >> IntegerShift;

    static constexpr uint64_t InvalidValue = static_cast<uint64_t>(-1);

    static bool IntegerInRange(int64_t ival) {
        return (ival >= IntegerMin) && (ival <= IntegerMax);
    }

  private:
    Box(uint64_t word)
      : value_(word)
    {}

  public:
    Box() : value_(InvalidValue) {}

    bool isInvalid() const {
        return value_ == InvalidValue;
    }
    static Box Invalid() {
        return Box();
    }

    bool isInteger() const {
        return (value_ & IntegerTagMask) == IntegerTag;
    }
    int64_t integer() const {
        return static_cast<int64_t>(value_ >> IntegerShift);
    }
    static Box Integer(int64_t ival) {
        WH_ASSERT(IntegerInRange(ival));
        return Box((static_cast<uint64_t>(ival) << IntegerShift) |
                   IntegerTag);
    }

    bool isPointer() const {
        return (value_ & PointerTagMask) == PointerTag;
    }
    template <typename T>
    T *pointer() const {
        static_assert(PointerTag == 0, "");
        static_assert(IsHeapThingType<T>(), "T is not a HeapThing type.");
        WH_ASSERT(isPointer());
        WH_ASSERT(reinterpret_cast<T *>(value_) != nullptr);
        return reinterpret_cast<T *>(value_);
    }
    template <typename T>
    static Box Pointer(T *ptr) {
        static_assert(IsHeapThingType<T>(), "T is not a HeapThing type.");
        WH_ASSERT(ptr != nullptr);
        WH_ASSERT(IsPtrAligned(ptr, PointerAlign));
        return Box(reinterpret_cast<uint64_t>(ptr));
    }

    void snprint(char *buf, size_t n) const {
        if (isPointer()) {
            snprintf(buf, n, "ptr(%p)", pointer<HeapThing>());
            return;
        }
        if (isInvalid()) {
            snprintf(buf, n, "invalid");
            return;
        }
        WH_UNREACHABLE("Unknown box kind.");
    }

  private:
    template <typename T>
    void setPointer(T *ptr) {
        static_assert(IsHeapThingType<T>(), "T is not a HeapThing type.");
        WH_ASSERT(ptr != nullptr);
        WH_ASSERT(IsPtrAligned(ptr, PointerAlign));
        value_ = reinterpret_cast<uint64_t>(ptr);
    }
};

template <>
struct ArrayTraits<Box>
{
    ArrayTraits() = delete;

    static const bool Specialized = true;
    static const HeapFormat ArrayFormat = HeapFormat::BoxArray;
};


} // namespace VM


//
// GC-Specializations.
//

template <>
struct FieldTraits<VM::Box>
{
    FieldTraits() = delete;
    static constexpr bool Specialized = true;
};

template <>
struct HeapFormatTraits<HeapFormat::BoxArray>
{
    HeapFormatTraits() = delete;
    typedef VM::Array<VM::Box> Type;
};

template <>
struct TraceTraits<VM::Box>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::Box &box,
                     const void *start, const void *end)
    {
        if (!box.isPointer())
            return;
        HeapThing *heapThing = box.pointer<HeapThing>();
        WH_ASSERT(heapThing != nullptr);
        scanner(&(box.value_), heapThing);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::Box &box,
                       const void *start, const void *end)
    {
        if (!box.isPointer())
            return;
        HeapThing *heapThing = box.pointer<HeapThing>();
        WH_ASSERT(heapThing != nullptr);
        HeapThing *updated = updater(&(box.value_), heapThing);
        if (updated != heapThing)
            box.setPointer(updated);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__BOX_HPP
