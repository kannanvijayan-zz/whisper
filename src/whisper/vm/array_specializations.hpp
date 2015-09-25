#ifndef WHISPER__VM__ARRAY_SPECIALIZATIONS_HPP
#define WHISPER__VM__ARRAY_SPECIALIZATIONS_HPP


#include "vm/core.hpp"


// Specialize arrays for primitive types.
#define DEF_PRIM_ARRAY_TRAITS_(type, fmtName) \
  namespace Whisper { \
  namespace VM { \
    template <> \
    struct ArrayTraits<type> { \
        ArrayTraits() = delete; \
        static const bool Specialized = true; \
        static const HeapFormat ArrayFormat = HeapFormat::fmtName; \
    }; \
  } \
  template <> \
  struct HeapFormatTraits<HeapFormat::fmtName> { \
      HeapFormatTraits() = delete; \
      static const bool Specialized = true; \
      typedef VM::Array<type> Type; \
  }; \
  }

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
struct ArrayTraits<P*> {
    static_assert(IsHeapThingType<P>(), "P is not a HeapThing type.");
    ArrayTraits() = delete;
    static constexpr bool Specialized = true;
    static const HeapFormat ArrayFormat = HeapFormat::HeapPointerArray;
};


} // namespace VM


//
// GC-Specializations for Array
//


template <typename T>
struct HeapTraits<VM::Array<T>>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr HeapFormat Format = VM::ArrayTraits<T>::ArrayFormat;
    static constexpr bool VarSized = true;
};

template <>
struct HeapFormatTraits<HeapFormat::HeapPointerArray>
{
    HeapFormatTraits() = delete;
    typedef VM::Array<HeapThing*> Type;
};

template <typename T>
struct TraceTraits<VM::Array<T>>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = TraceTraits<T>::IsLeaf;

    typedef VM::Array<T> Array_;

    template <typename Scanner>
    static void Scan(Scanner& scanner, Array_ const& array,
                     void const* start, void const* end)
    {
        if (!IsLeaf) {
            // Scan each pointer in the array.
            for (uint32_t i = 0; i < array.length(); i++)
                array.vals_[i].scan(scanner, start, end);
        }
    }

    template <typename Updater>
    static void Update(Updater& updater, Array_& array,
                       void const* start, void const* end)
    {
        if (!IsLeaf) {
            // Update each pointer in the array.
            for (uint32_t i = 0; i < array.length(); i++)
                array.vals_[i].update(updater, start, end);
        }
    }
};


} // namespace Whisper


#endif // WHISPER__VM__ARRAY_SPECIALIZATIONS_HPP
