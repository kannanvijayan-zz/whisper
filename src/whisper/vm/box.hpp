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
    word_t value_;

  public:
    static constexpr word_t PointerAlign = 0x8;

    static constexpr word_t TagMask = 0x7;
    static constexpr word_t PointerTag = 0x0;
    static constexpr word_t InvalidValue = static_cast<word_t>(-1);

    Box() : value_(InvalidValue) {}

    template <typename T>
    Box(T *ptr) {
        static_assert(IsHeapThingType<T>(), "T is not a HeapThing type.");
        WH_ASSERT(ptr != nullptr);
        WH_ASSERT(IsPtrAligned(ptr, PointerAlign));
        value_ = reinterpret_cast<word_t>(ptr);
    }

    bool isInvalid() const {
        return (value_ & TagMask) == InvalidValue;
    }
    bool isPointer() const {
        return (value_ & TagMask) == PointerTag;
    }
    template <typename T>
    T *pointer() const {
        static_assert(IsHeapThingType<T>(), "T is not a HeapThing type.");
        WH_ASSERT(isPointer());
        WH_ASSERT(reinterpret_cast<T *>(value_) != nullptr);
        return reinterpret_cast<T *>(value_);
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
        value_ = reinterpret_cast<word_t>(ptr);
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


template <>
struct StackTraits<VM::Box>
{
    StackTraits() = delete;
    static constexpr bool Specialized = true;
    static constexpr StackFormat Format = StackFormat::Box;
};

template <>
struct FieldTraits<VM::Box>
{
    FieldTraits() = delete;
    static constexpr bool Specialized = true;
};

template <>
struct StackFormatTraits<StackFormat::Box>
{
    StackFormatTraits() = delete;
    typedef VM::Box Type;
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
