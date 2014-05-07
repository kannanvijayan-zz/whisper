#ifndef WHISPER__GC__SPECIALIZATIONS_HPP
#define WHISPER__GC__SPECIALIZATIONS_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gcx/core.hpp"

#include <functional>

namespace Whisper {
namespace GC {

///////////////////////////////////////////////////////////////////////////////
////
//// Specialization of UntracedThing format.
////
//// AllocFormat::UntracedThing is handy for use by structures that want
//// to be allocated on heap and managed by the gc, but not be traced at all.
////
///////////////////////////////////////////////////////////////////////////////

template <>
struct AllocFormatTraits<AllocFormat::UntracedThing>
{
    AllocFormatTraits() = delete;

    typedef uint32_t Type;
};

///////////////////////////////////////////////////////////////////////////////
////
//// Specialization of StackTraits, HeapTraits, FieldTraits and TraceTraits
//// for primitive types.
////
///////////////////////////////////////////////////////////////////////////////

// Primitive-type specializations for trace traits.
#define PRIM_TRACE_TRAITS_DEF_(type) \
    template <> \
    struct StackTraits<type> \
    { \
        StackTraits() = delete; \
        static constexpr bool Specialized = true; \
        static constexpr AllocFormat Format = AllocFormat::UntracedThing; \
    }; \
    template <> \
    struct HeapTraits<type> \
    { \
        HeapTraits() = delete; \
        static constexpr bool Specialized = true; \
        static constexpr AllocFormat Format = AllocFormat::UntracedThing; \
        \
        template <typename... Args> \
        static uint32_t CalculateSize(Args... args) { \
            return sizeof(type); \
        } \
    }; \
    template <> \
    struct FieldTraits<type> \
    { \
        FieldTraits() = delete; \
        static constexpr bool Specialized = true; \
    }; \
    template <> \
    struct TraceTraits<type> \
    { \
        typedef type T_; \
        \
        TraceTraits() = delete; \
        static constexpr bool Specialized = true; \
        static constexpr bool IsLeaf = true; \
        \
        template <typename Scanner> \
        static void Scan(Scanner &, const T_ &, void *, void *) {} \
        \
        template <typename Updater> \
        static void Update(Updater &, T_ &, void *, void *) {} \
    }

    PRIM_TRACE_TRAITS_DEF_(bool);

    PRIM_TRACE_TRAITS_DEF_(uint8_t);
    PRIM_TRACE_TRAITS_DEF_(uint16_t);
    PRIM_TRACE_TRAITS_DEF_(uint32_t);
    PRIM_TRACE_TRAITS_DEF_(uint64_t);

    PRIM_TRACE_TRAITS_DEF_(int8_t);
    PRIM_TRACE_TRAITS_DEF_(int16_t);
    PRIM_TRACE_TRAITS_DEF_(int32_t);
    PRIM_TRACE_TRAITS_DEF_(int64_t);

    PRIM_TRACE_TRAITS_DEF_(float);
    PRIM_TRACE_TRAITS_DEF_(double);

#undef PRIM_TRACE_TRAITS_DEF_


///////////////////////////////////////////////////////////////////////////////
////
//// Specialization of StackTraits, HeapTraits, FieldTraits and TraceTraits
//// for pointer types.
////
///////////////////////////////////////////////////////////////////////////////

// By default pointers are expected to point to structures for which
// HeapTraits is defined.

template <typename P>
struct StackTraits<P *>
{
    static_assert(HeapTraits<P>::Specialized,
                  "HeapTraits not specialized for underlying type.");

    StackTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr AllocFormat Format = AllocFormat::AllocThingPointer;
};

template <typename P>
struct HeapTraits<P *>
{
    static_assert(HeapTraits<P>::Specialized,
                  "HeapTraits not specialized for underlying type.");

    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr AllocFormat Format = AllocFormat::AllocThingPointer;

    template <typename... Args>
    static uint32_t CalculateSize(Args... args) {
        return sizeof(P *);
    }
};

template <typename P>
struct FieldTraits<P *>
{
    static_assert(HeapTraits<P>::Specialized,
                  "HeapTraits not specialized for underlying type.");

    FieldTraits() = delete;

    static constexpr bool Specialized = true;
};


//
// Specialize AllocFormatTraits for AllocThingPointer.
//
// This just maps AllocThingPointer to the type |AllocThing *| for
// tracing.
//
template <>
struct AllocFormatTraits<AllocFormat::AllocThingPointer>
{
    AllocFormatTraits() = delete;

    typedef AllocThing *Type;
    static constexpr bool Traced = true;
};

//
// Specialize AllocThing * for TraceTraits
//
template <>
struct TraceTraits<AllocThing *>
{
    typedef AllocThing * T_;

    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const T_ &t, void *start, void *end) {
        if (std::less<const void *>(&t, start))
            return;
        if (std::greater_equal<const void *>(&t, end))
            return;
        scanner(&t, t);
    }

    template <typename Updater>
    static void Update(Updater &updater, T_ &t, void *start, void *end) {
        if (std::less<void *>(&t, start))
            return;
        if (std::greater_equal<void *>(&t, end))
            return;
        AllocThing *tx = updater(&t, t);
        if (tx != t)
            t = tx;
    }
};

//
// Specialize other pointers for trace-traits, so that Field<P *>
// specializations can use them.
//
template <typename P>
struct TraceTraits<P *>
{
    // Traced pointers are assumed to be pointers to heap-things by default.
    static_assert(HeapTraits<P>::Specialized,
                  "HeapTraits not specialized for underlying type.");
    typedef P * T_;

    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const T_ &t, void *start, void *end) {
        if (std::less<const void *>(&t, start))
            return;
        if (std::greater_equal<const void *>(&t, end))
            return;
        scanner(&t, reinterpret_cast<AllocThing *>(t));
    }

    template <typename Updater>
    static void Update(Updater &updater, T_ &t, void *start, void *end) {
        if (std::less<void *>(&t, start))
            return;
        if (std::greater_equal<void *>(&t, end))
            return;
        AllocThing *tx = updater(&t, reinterpret_cast<AllocThing *>(t));
        if (tx != t)
            t = reinterpret_cast<P *>(tx);
    }
};


///////////////////////////////////////////////////////////////////////////////
////
//// Helper to scan a pointer given an AllocFormat describing its contents.
////
///////////////////////////////////////////////////////////////////////////////


template <typename Scanner>
void
GcScanAllocFormat(AllocFormat fmt, const void *ptr,
                  Scanner &scanner, void *start, void *end)
{
    WH_ASSERT(ptr != nullptr);
    switch (fmt) {
#define SWITCH_(fmtName) \
    case AllocFormat::fmtName: {\
        constexpr AllocFormat FMT = AllocFormat::fmtName; \
        typedef AllocFormatTraits<AllocFormat::fmtName>::Type TRACE_TYPE; \
        const TRACE_TYPE &tref = \
            *reinterpret_cast<const TRACE_TYPE *>(ptr); \
        TraceTraits<TRACE_TYPE>::Scan(scanner, tref, start, end); \
        break; \
    }
      WHISPER_DEFN_GC_ALLOC_FORMATS(SWITCH_)
#undef SWITCH_
      default:
        WH_ASSERT(!!!"BAD AllocFormat");
        break;
    }
}

template <typename Updater>
void
GcUpdateAllocFormat(AllocFormat fmt, const void *ptr,
                    Updater &updater, void *start, void *end)
{
    WH_ASSERT(ptr != nullptr);
    switch (fmt) {
#define SWITCH_(fmtName) \
    case AllocFormat::fmtName: {\
        constexpr AllocFormat FMT = AllocFormat::fmtName; \
        typedef AllocFormatTraits<FMT>::Type TRACE_TYPE; \
        TRACE_TYPE &tref = \
            *reinterpret_cast<const TRACE_TYPE *>(ptr); \
        TraceTraits<TRACE_TYPE>::Update(updater, tref, start, end); \
        break; \
    }
      WHISPER_DEFN_GC_ALLOC_FORMATS(SWITCH_)
#undef SWITCH_
      default:
        WH_ASSERT(!!!"BAD AllocFormat");
        break;
    }
}



} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__SPECIALIZATIONS_HPP
