#ifndef WHISPER__VM__ARRAY_SPECIALIZATIONS_HPP
#define WHISPER__VM__ARRAY_SPECIALIZATIONS_HPP


#include "vm/core.hpp"

#include <new>

namespace Whisper {
namespace VM {


// Specialize arrays for primitive types.
#define DEF_ARRAY_TRAITS_(type, fmtName) \
    template <> \
    struct ArrayTraits<type> { \
        ArrayTraits() = delete; \
        static const bool Specialized = true; \
        static const GC::AllocFormat ArrayFormat = GC::AllocFormat::fmtName; \
    };

DEF_ARRAY_TRAITS_(uint8_t, UntracedThing);
DEF_ARRAY_TRAITS_(uint16_t, UntracedThing);
DEF_ARRAY_TRAITS_(uint32_t, UntracedThing);
DEF_ARRAY_TRAITS_(uint64_t, UntracedThing);
DEF_ARRAY_TRAITS_(int8_t, UntracedThing);
DEF_ARRAY_TRAITS_(int16_t, UntracedThing);
DEF_ARRAY_TRAITS_(int32_t, UntracedThing);
DEF_ARRAY_TRAITS_(int64_t, UntracedThing);
DEF_ARRAY_TRAITS_(float, UntracedThing);
DEF_ARRAY_TRAITS_(double, UntracedThing);
DEF_ARRAY_TRAITS_(GC::AllocThing *, AllocThingPointerArray);

#undef DEF_ARRAY_TRAITS_

// Specialize arrays for general pointer types.
// Treat them by default as arrays of pointers-to-alloc-things.
template <typename P>
struct ArrayTraits<P *> {
    static_assert(GC::HeapTraits<P>::Specialized,
                  "Underlying type of pointer is not a heap thing.");
    ArrayTraits() = delete;
    static constexpr bool Specialized = true;
    static const GC::AllocFormat ArrayFormat =
        GC::AllocFormat::AllocThingPointerArray;
};


} // namespace VM
} // namespace Whisper


// A handy macro for defining VM::ArrayTraits<T> and
// GC::AllocFormatTraits<FMT> for a given array type.
//
// The Macro simply makes VM::ArrayTraits<type> specify |format|
// as the AllocFormat of the array, and points
// GC::AllocFormatTraits<format> to VM::Array<type> as the
// traced type.
#define WH_VM__DEF_SIMPLE_ARRAY_TRAITS(type, format) \
  namespace Whisper { \
   namespace VM { \
    template <> struct ArrayTraits<type> { \
        static_assert(GC::FieldTraits<type>::Specialized, \
                      "Underlying type is not field-specialized."); \
        ArrayTraits() = delete; \
        static constexpr bool Specialized = true; \
        static const GC::AllocFormat ArrayFormat = GC::AllocFormat::format; \
    }; \
   } \
   \
   namespace GC { \
    template <> struct AllocFormatTraits<GC::AllocFormat::format> { \
        static_assert(TraceTraits<type>::Specialized, \
                      "Underlying type is not trace-specialized."); \
        AllocFormatTraits() = delete; \
        static constexpr bool Specialized = true; \
        typedef VM::Array<type> Type; \
    }; \
   } \
  }


//
// GC-Specializations for Array
//
namespace Whisper {
namespace GC {


template <typename T>
struct HeapTraits<VM::Array<T>>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr GC::AllocFormat Format =
        VM::ArrayTraits<T>::ArrayFormat;
    static constexpr bool VarSized = true;
};

template <>
struct AllocFormatTraits<AllocFormat::AllocThingPointerArray>
{
    AllocFormatTraits() = delete;
    typedef VM::Array<AllocThing *> Type;
};

template <typename T>
struct TraceTraits<VM::Array<T>>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;

    static constexpr bool IsLeaf = TraceTraits<T>::IsLeaf;

    typedef VM::Array<T> Array_;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const Array_ &array,
                     const void *start, const void *end)
    {
        if (IsLeaf)
            return;

        // Scan each pointer in the array.
        for (uint32_t i = 0; i < array.length(); i++)
            array.vals_[i].scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, Array_ &array,
                       const void *start, const void *end)
    {
        if (IsLeaf)
            return;

        // Update each pointer in the array.
        for (uint32_t i = 0; i < array.length(); i++)
            array.vals_[i].update(updater, start, end);
    }
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__ARRAY_SPECIALIZATIONS_HPP
