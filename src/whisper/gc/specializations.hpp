#ifndef WHISPER__GC__SPECIALIZATIONS_HPP
#define WHISPER__GC__SPECIALIZATIONS_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/core.hpp"

#include <functional>

namespace Whisper {

///////////////////////////////////////////////////////////////////////////////
////
//// Specialization of UntracedThing format.
////
//// AllocFormat::UntracedThing is a handy AllocFormat for use by
//// structures that want to be allocated but do not need to be traced
//// at all.
////
///////////////////////////////////////////////////////////////////////////////

template <>
struct AllocFormatTraits<AllocFormat::UntracedThing>
{
    AllocFormatTraits() = delete;

    struct T_ {};

    typedef T_ TYPE;
    static constexpr bool TRACED = false;
};

template <>
struct TraceTraits<AllocFormatTraits<AllocFormat::UntracedThing>::T_>
{
    TraceTraits() = delete;

    typedef AllocFormatTraits<AllocFormat::UntracedThing>::T_ T_;

    static constexpr bool SPECIALIZED = true;

    template <typename Scanner>
    static void SCAN(Scanner &scanner, const T_ &t, void *start, void *end)
    {}

    template <typename Updater>
    static void UPDATE(Updater &updater, T_ &t, void *start, void *end)
    {}
};

///////////////////////////////////////////////////////////////////////////////
////
//// Specialization of AllocTraits for primitive values.
////
//// This allows usage of primitives with Local, Field, and Handle wrappers.
////
///////////////////////////////////////////////////////////////////////////////

// By default pointers are expected to point to structures for which
// AllocTraits is defined (i.e. pointers to traced things)

//
// All raw pointers handled by the system are expected to be pointers
// to types which define AllocTraits, and traceable as such.
//
// By specializing AllocTraits for |P *| where AllocTraits<P> is defined,
// we enable the allocation of pointer-cells on the stack and heap.
//
#define PRIM_ALLOC_TRAITS_DEF_(type) \
    template <> \
    struct AllocTraits<type> { \
        AllocTraits() = delete; \
        static constexpr bool SPECIALIZED = true; \
        static constexpr AllocFormat FORMAT = AllocFormat::UntracedThing; \
        typedef type DEREF_TYPE; \
        static DEREF_TYPE *DEREF(type &t) { return &t; } \
        static const DEREF_TYPE *DEREF(const type &t) { return &t; } \
    };
    PRIM_ALLOC_TRAITS_DEF_(bool)
    PRIM_ALLOC_TRAITS_DEF_(uint8_t)
    PRIM_ALLOC_TRAITS_DEF_(int8_t)
    PRIM_ALLOC_TRAITS_DEF_(uint16_t)
    PRIM_ALLOC_TRAITS_DEF_(int16_t)
    PRIM_ALLOC_TRAITS_DEF_(uint32_t)
    PRIM_ALLOC_TRAITS_DEF_(int32_t)
    PRIM_ALLOC_TRAITS_DEF_(uint64_t)
    PRIM_ALLOC_TRAITS_DEF_(int64_t)
    PRIM_ALLOC_TRAITS_DEF_(float)
    PRIM_ALLOC_TRAITS_DEF_(double)
#undef PRIM_ALLOC_TRAITS_DEF_


///////////////////////////////////////////////////////////////////////////////
////
//// Specialization of AllocTraits for Pointers.
////
//// This allows usage of pointers with Local, Field, and Handle wrappers.
////
///////////////////////////////////////////////////////////////////////////////

// By default pointers are expected to point to structures for which
// AllocTraits is defined (i.e. pointers to traced things)

//
// All raw pointers handled by the system are expected to be pointers
// to types which define AllocTraits, and traceable as such.
//
// By specializing AllocTraits for |P *| where AllocTraits<P> is defined,
// we enable the allocation of pointer-cells on the stack and heap.
//
template <typename P>
struct AllocTraits<P *>
{
    static_assert(AllocTraits<P>::SPECIALIZED, "P is not an AllocThing.");

    AllocTraits() = delete;

    static constexpr bool SPECIALIZED = true;
    static constexpr AllocFormat FORMAT = AllocFormat::TracedPointer;

    typedef P * T_;
    typedef P DEREF_TYPE;
    static DEREF_TYPE *DEREF(T_ &t) {
        return t;
    }
    static const DEREF_TYPE *DEREF(const T_ &t) {
        return t;
    }
};

//
// Specialize AllocFormatTraits for AllocThingPointer
//
template <>
struct AllocFormatTraits<AllocFormat::TracedPointer>
{
    static constexpr AllocFormat FMT = AllocFormat::TracedPointer;

    AllocFormatTraits() = delete;

    // All specializations for FMT must define TYPE to be the
    // static type for the format.
    typedef AllocThing *TYPE;

    static constexpr bool TRACED = true;
};

//
// Specialize AllocThing * for TraceTraits
//
template <>
struct TraceTraits<AllocThing *>
{
    typedef AllocThing * T_;

    TraceTraits() = delete;

    static constexpr bool SPECIALIZED = true;

    template <typename Scanner>
    static void SCAN(Scanner &scanner, const T_ &t, void *start, void *end) {
        if (std::less<const void *>(&t, start))
            return;
        if (std::greater_equal<const void *>(&t, end))
            return;
        scanner(&t, t);
    }

    template <typename Updater>
    static void UPDATE(Updater &updater, T_ &t, void *start, void *end) {
        if (std::less<void *>(&t, start))
            return;
        if (std::greater_equal<void *>(&t, end))
            return;
        AllocThing *tx = updater(&t, t);
        if (tx != t)
            t = tx;
    }
};


///////////////////////////////////////////////////////////////////////////////
////
//// Helper to scan a pointer given an AllocFormat describing its contents.
////
///////////////////////////////////////////////////////////////////////////////


template <typename SCANNER>
void
GcScanAllocFormat(AllocFormat fmt, const void *ptr,
                  SCANNER &scanner, void *start, void *end)
{
    WH_ASSERT(ptr != nullptr);
    switch (fmt) {
#define SWITCH_(fmtName) \
    case AllocFormat::fmtName: {\
        constexpr AllocFormat FMT = AllocFormat::fmtName; \
        constexpr bool TRACE = AllocFormatTraits<FMT>::TRACED; \
        if (AllocFormatTraits<FMT>::TRACED) { \
            typedef AllocFormatTraits<FMT>::TYPE TRACE_TYPE; \
            const TRACE_TYPE &tptr = \
                *reinterpret_cast<const TRACE_TYPE *>(ptr); \
            TraceTraits<TRACE_TYPE>::SCAN(scanner, tptr, start, end); \
        } \
        break; \
    }
      WHISPER_DEFN_GC_ALLOC_FORMATS(SWITCH_)
#undef SWITCH_
      default:
        WH_ASSERT(!!!"BAD AllocFormat");
        break;
    }
}

template <typename UPDATER>
void
GcUpdateAllocFormat(AllocFormat fmt, const void *ptr,
                    UPDATER &updater, void *start, void *end)
{
    WH_ASSERT(ptr != nullptr);
    switch (fmt) {
#define SWITCH_(fmtName) \
    case AllocFormat::fmtName: {\
        constexpr AllocFormat FMT = AllocFormat::fmtName; \
        constexpr bool TRACE = AllocFormatTraits<FMT>::TRACED; \
        if (AllocFormatTraits<FMT>::TRACED) { \
            typedef AllocFormatTraits<FMT>::TYPE TRACE_TYPE; \
            const TRACE_TYPE &tptr = \
                *reinterpret_cast<const TRACE_TYPE *>(ptr); \
            TraceTraits<TRACE_TYPE>::UPDATE(updater, tptr, start, end); \
        } \
        break; \
    }
      WHISPER_DEFN_GC_ALLOC_FORMATS(SWITCH_)
#undef SWITCH_
      default:
        WH_ASSERT(!!!"BAD AllocFormat");
        break;
    }
}



} // namespace Whisper

#endif // WHISPER__GC__SPECIALIZATIONS_HPP
