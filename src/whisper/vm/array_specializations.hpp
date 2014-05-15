#ifndef WHISPER__VM__ARRAY_SPECIALIZATIONS_HPP
#define WHISPER__VM__ARRAY_SPECIALIZATIONS_HPP

#include "common.hpp"
#include "debug.hpp"
#include "spew.hpp"
#include "gc.hpp"

#include <new>

namespace Whisper {
namespace VM {

template <typename T> class Array;


// Template to annotate types which are used as parameters to
// Array<>, so that we can derive an AllocFormat for the array of
// a particular type.
template <typename T>
struct ArrayTraits
{
    ArrayTraits() = delete;

    // Set to true for all specializations.
    static const bool Specialized = false;

    // Give an AllocFormat for an array of type T.
    //
    // static const GC::AllocFormat ArrayFormat;

    // Give a map-back type T' to map the array format back to
    // VM::Array<T'>
    // typedef T T';
};

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
// GC::AllocFormatTarits<format> to VM::Array<type> as the
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

    // All constructor signatures for Array take the length as the
    // first argument.
    template <typename... Args>
    static uint32_t SizeOf(uint32_t len, Args... rest) {
        return len * sizeof(T);
    }
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
        // Scan each pointer in the array.
        for (uint32_t i = 0; i < array.length(); i++)
            array.vals_[i].scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, Array_ &array,
                       const void *start, const void *end)
    {
        // Update each pointer in the array.
        for (uint32_t i = 0; i < array.length(); i++)
            array.vals_[i].update(updater, start, end);
    }
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__ARRAY_SPECIALIZATIONS_HPP
