#ifndef WHISPER__VM__ARRAY_SPECIALIZATIONS_HPP
#define WHISPER__VM__ARRAY_SPECIALIZATIONS_HPP


#include "vm/core.hpp"

#include <new>


// Specialize arrays for primitive types.
#define DEF_PRIM_ARRAY_TRAITS_(type, fmtName) \
  namespace Whisper { \
  namespace VM { \
    template <> \
    struct ArrayTraits<type> { \
        ArrayTraits() = delete; \
        static const bool Specialized = true; \
        static const GC::AllocFormat ArrayFormat = GC::AllocFormat::fmtName; \
    }; \
  } \
  namespace GC { \
    template <> \
    struct AllocFormatTraits<AllocFormat::fmtName> { \
        AllocFormatTraits() = delete; \
        static const bool Specialized = true; \
        typedef VM::Array<type> Type; \
    }; \
  } \
  } \

DEF_PRIM_ARRAY_TRAITS_(uint8_t, UInt8Array);
DEF_PRIM_ARRAY_TRAITS_(uint16_t, UInt16Array);
DEF_PRIM_ARRAY_TRAITS_(uint32_t, UInt32Array);
DEF_PRIM_ARRAY_TRAITS_(uint64_t, UInt64Array);
DEF_PRIM_ARRAY_TRAITS_(int8_t, Int8Array);
DEF_PRIM_ARRAY_TRAITS_(int16_t, Int16Array);
DEF_PRIM_ARRAY_TRAITS_(int32_t, Int32Array);
DEF_PRIM_ARRAY_TRAITS_(int64_t, Int64Array);
DEF_PRIM_ARRAY_TRAITS_(float, FloatArray);
DEF_PRIM_ARRAY_TRAITS_(double, DoubleArray);

#undef DEF_PRIM_ARRAY_TRAITS_


namespace Whisper {
namespace VM {


// Specialize arrays for general pointer types.
// Treat them by default as arrays of pointers-to-alloc-things.
template <typename P>
struct ArrayTraits<P *> {
    static_assert(GC::HeapTraits<P>::Specialized ||
                  GC::AllocThingTraits<P>::Specialized,
                  "Underlying type of pointer is not a heap thing or "
                  "alloc thing.");
    ArrayTraits() = delete;
    static constexpr bool Specialized = true;
    static const GC::AllocFormat ArrayFormat =
        GC::AllocFormat::AllocThingPointerArray;
};


} // namespace VM
} // namespace Whisper


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
