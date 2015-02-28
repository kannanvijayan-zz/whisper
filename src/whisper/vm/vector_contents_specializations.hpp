#ifndef WHISPER__VM__VECTOR_CONTENTS_SPECIALIZATIONS_HPP
#define WHISPER__VM__VECTOR_CONTENTS_SPECIALIZATIONS_HPP


#include "vm/core.hpp"

#include <new>

namespace Whisper {
namespace VM {


// Specialize vector for primitive types.
#define DEF_VECTOR_TRAITS_(type, fmtName) \
    template <> \
    struct VectorTraits<type> { \
        VectorTraits() = delete; \
        static const bool Specialized = true; \
        static const GC::AllocFormat VectorContentsFormat = \
            GC::AllocFormat::fmtName; \
    };

DEF_VECTOR_TRAITS_(uint8_t, UntracedThing);
DEF_VECTOR_TRAITS_(uint16_t, UntracedThing);
DEF_VECTOR_TRAITS_(uint32_t, UntracedThing);
DEF_VECTOR_TRAITS_(uint64_t, UntracedThing);
DEF_VECTOR_TRAITS_(int8_t, UntracedThing);
DEF_VECTOR_TRAITS_(int16_t, UntracedThing);
DEF_VECTOR_TRAITS_(int32_t, UntracedThing);
DEF_VECTOR_TRAITS_(int64_t, UntracedThing);
DEF_VECTOR_TRAITS_(float, UntracedThing);
DEF_VECTOR_TRAITS_(double, UntracedThing);
DEF_VECTOR_TRAITS_(GC::AllocThing *, AllocThingPointerVectorContents);

#undef DEF_VECTOR_TRAITS_

// Specialize for general pointer types.
// Treat them by default as vectorcontents of pointers-to-alloc-things.
template <typename P>
struct VectorTraits<P *> {
    static_assert(GC::HeapTraits<P>::Specialized,
                  "Underlying type of pointer is not a heap thing.");
    VectorTraits() = delete;
    static constexpr bool Specialized = true;
    static const GC::AllocFormat VectorContentsFormat =
        GC::AllocFormat::AllocThingPointerVectorContents;
};


} // namespace VM
} // namespace Whisper


// A handy macro for defining VM::VectorTraits<T> and
// GC::AllocFormatTraits<FMT> for a given vector type.
//
// The Macro simply makes VM::VectorTraits<type> specify |format|
// as the AllocFormat of the vector contents, and points
// GC::AllocFormatTraits<format> to VM::VectorContents<type> as the
// traced type.
#define WH_VM__DEF_SIMPLE_VECTOR_TRAITS(type, format) \
  namespace Whisper { \
   namespace VM { \
    template <> struct VectorTraits<type> { \
        static_assert(GC::FieldTraits<type>::Specialized, \
                      "Underlying type is not field-specialized."); \
        VectorTraits() = delete; \
        static constexpr bool Specialized = true; \
        static const GC::AllocFormat VectorContentsFormat = \
            GC::AllocFormat::format; \
    }; \
   } \
   \
   namespace GC { \
    template <> struct AllocFormatTraits<GC::AllocFormat::format> { \
        static_assert(TraceTraits<type>::Specialized, \
                      "Underlying type is not trace-specialized."); \
        AllocFormatTraits() = delete; \
        static constexpr bool Specialized = true; \
        typedef VM::VectorContents<type> Type; \
    }; \
   } \
  }


//
// GC-Specializations for Vector
//
namespace Whisper {
namespace GC {


template <typename T>
struct HeapTraits<VM::VectorContents<T>>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr GC::AllocFormat Format =
        VM::VectorTraits<T>::VectorContentsFormat;
    static constexpr bool VarSized = true;
};

template <>
struct AllocFormatTraits<AllocFormat::AllocThingPointerVectorContents>
{
    AllocFormatTraits() = delete;
    typedef VM::VectorContents<AllocThing *> Type;
};

template <typename T>
struct TraceTraits<VM::VectorContents<T>>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;

    static constexpr bool IsLeaf = TraceTraits<T>::IsLeaf;

    typedef VM::VectorContents<T> Vc_;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const Vc_ &vc,
                     const void *start, const void *end)
    {
        if (IsLeaf)
            return;

        // Scan each pointer in the vc.
        for (uint32_t i = 0; i < vc.length(); i++)
            vc.vals_[i].scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, Vc_ &vc,
                       const void *start, const void *end)
    {
        if (IsLeaf)
            return;

        // Update each pointer in the vc.
        for (uint32_t i = 0; i < vc.length(); i++)
            vc.vals_[i].update(updater, start, end);
    }
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__VECTOR_CONTENTS_SPECIALIZATIONS_HPP
