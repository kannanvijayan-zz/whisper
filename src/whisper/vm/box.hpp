#ifndef WHISPER__VM__BOX_HPP
#define WHISPER__VM__BOX_HPP


#include "vm/core.hpp"
#include "vm/array.hpp"

#include <new>

namespace Whisper {
namespace VM {

//
// A Box wraps either a pointer or immediate value.
//
class Box
{
    friend class GC::TraceTraits<Box>;
  private:
    word_t value_;

  public:
    static constexpr word_t PointerAlign = 0x8;

    static constexpr word_t TagMask = 0x7;
    static constexpr word_t PointerTag = 0x0;

    template <typename T>
    Box(T *ptr) {
        static_assert(GC::HeapTraits<T>::Specialized ||
                      GC::AllocThingTraits<T>::Specialized,
                      "Box constructed with non-AllocThing pointer.");
        WH_ASSERT(ptr != nullptr);
        WH_ASSERT(IsPtrAligned(ptr, PointerAlign));
        value_ = reinterpret_cast<word_t>(ptr);
    }

    bool isPointer() const {
        return (value_ & TagMask) == PointerTag;
    }
    template <typename T>
    T *pointer() const {
        static_assert(GC::HeapTraits<T>::Specialized ||
                      GC::AllocThingTraits<T>::Specialized,
                      "Retreiving non-AllocThing pointer from box.");
        WH_ASSERT(isPointer());
        return reinterpret_cast<T *>(value_);
    }

  private:
    template <typename T>
    void setPointer(T *ptr) {
        static_assert(GC::HeapTraits<T>::Specialized ||
                      GC::AllocThingTraits<T>::Specialized,
                      "Box assigned non-AllocThing pointer.");
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
    static const GC::AllocFormat ArrayFormat = GC::AllocFormat::BoxArray;
};


} // namespace VM
} // namespace Whisper


namespace Whisper {
namespace GC {

    template <>
    struct HeapTraits<VM::Box>
    {
        HeapTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr AllocFormat Format = AllocFormat::Box;
        static constexpr bool VarSized = true;
    };

    template <>
    struct StackTraits<VM::Box>
    {
        StackTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr AllocFormat Format = AllocFormat::Box;
    };

    template <>
    struct FieldTraits<VM::Box>
    {
        FieldTraits() = delete;

        static constexpr bool Specialized = true;
    };

    template <>
    struct AllocFormatTraits<AllocFormat::Box>
    {
        AllocFormatTraits() = delete;
        typedef VM::Box Type;
    };

    template <>
    struct AllocFormatTraits<AllocFormat::BoxArray>
    {
        AllocFormatTraits() = delete;
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
            GC::AllocThing *allocThing = box.pointer<GC::AllocThing>();
            WH_ASSERT(allocThing != nullptr);
            scanner(reinterpret_cast<const void *>(&(box.value_)), allocThing);
        }

        template <typename Updater>
        static void Update(Updater &updater, VM::Box &box,
                           const void *start, const void *end)
        {
            if (!box.isPointer())
                return;
            GC::AllocThing *allocThing = box.pointer<GC::AllocThing>();
            WH_ASSERT(allocThing != nullptr);
            GC::AllocThing *updated = updater(
                reinterpret_cast<void *>(&(box.value_)),
                allocThing);
            if (updated != allocThing)
                box.setPointer(updated);
        }
    };

} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__BOX_HPP
