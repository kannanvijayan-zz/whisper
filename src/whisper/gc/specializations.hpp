#ifndef WHISPER__GC__SPECIALIZATIONS_HPP
#define WHISPER__GC__SPECIALIZATIONS_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/core.hpp"

#include <functional>

namespace Whisper {
namespace GC {


///////////////////////////////////////////////////////////////////////////////
////
//// Specialization of StackTraits, HeapTraits, FieldTraits and TraceTraits
//// for primitive types.
////
///////////////////////////////////////////////////////////////////////////////

// Primitive-type specializations for trace traits.
#define PRIM_TRACE_TRAITS_DEF_(type, fmtName) \
    template <> \
    struct StackTraits<type> \
    { \
        StackTraits() = delete; \
        static constexpr bool Specialized = true; \
        static constexpr AllocFormat Format = AllocFormat::fmtName; \
    }; \
    template <> \
    struct HeapTraits<type> \
    { \
        HeapTraits() = delete; \
        static constexpr bool Specialized = true; \
        static constexpr AllocFormat Format = AllocFormat::fmtName; \
        static constexpr bool VarSized  = false; \
    }; \
    template <> \
    struct FieldTraits<type> \
    { \
        FieldTraits() = delete; \
        static constexpr bool Specialized = true; \
    }; \
    template <> \
    struct AllocFormatTraits<AllocFormat::fmtName> \
    { \
        AllocFormatTraits() = delete; \
        static constexpr bool Specialized = true; \
        typedef type Type; \
    }; \
    template <> \
    struct TraceTraits<type> : public UntracedTraceTraits<type> {};

    PRIM_TRACE_TRAITS_DEF_(bool, Bool);

    PRIM_TRACE_TRAITS_DEF_(uint8_t, UInt8);
    PRIM_TRACE_TRAITS_DEF_(uint16_t, UInt16);
    PRIM_TRACE_TRAITS_DEF_(uint32_t, UInt32);
    PRIM_TRACE_TRAITS_DEF_(uint64_t, UInt64);

    PRIM_TRACE_TRAITS_DEF_(int8_t, Int8);
    PRIM_TRACE_TRAITS_DEF_(int16_t, Int16);
    PRIM_TRACE_TRAITS_DEF_(int32_t, Int32);
    PRIM_TRACE_TRAITS_DEF_(int64_t, Int64);

    PRIM_TRACE_TRAITS_DEF_(float, Float);
    PRIM_TRACE_TRAITS_DEF_(double, Double);

#undef PRIM_TRACE_TRAITS_DEF_


///////////////////////////////////////////////////////////////////////////////
////
//// Specialization of StackTraits, HeapTraits, FieldTraits and TraceTraits
//// for pointer types.
////
///////////////////////////////////////////////////////////////////////////////

// Specialize AllocThing pointers specially, since AllocThing does not
// have a HeapTraits specialization (and it shouldn't).
template <>
struct StackTraits<AllocThing *>
{
    StackTraits() = delete;
    static constexpr bool Specialized = true;
    static constexpr AllocFormat Format = AllocFormat::AllocThingPointer;
};

template <>
struct HeapTraits<AllocThing *>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr AllocFormat Format = AllocFormat::AllocThingPointer;
    static constexpr bool VarSized = false;
};

template <>
struct FieldTraits<AllocThing *>
{
    FieldTraits() = delete;
    static constexpr bool Specialized = true;
};

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
    static constexpr bool VarSized = false;
};

template <typename P>
struct FieldTraits<P *>
{
    static_assert(HeapTraits<P>::Specialized ||
                  AllocThingTraits<P>::Specialized,
                  "HeapTraits not specialized for underlying type.");

    FieldTraits() = delete;

    static constexpr bool Specialized = true;
};

template <typename P>
struct DerefTraits<P *>
{
    static_assert(HeapTraits<P>::Specialized ||
                  AllocThingTraits<P>::Specialized,
                  "HeapTraits not specialized for underlying type.");

    DerefTraits() = delete;

    typedef P *T_;
    typedef P  Type;

    static inline const Type *Deref(const T_ &ptr) {
        return ptr;
    }
    static inline Type *Deref(T_ &ptr) {
        return ptr;
    }
};

template <>
struct DerefTraits<AllocThing *>
{
    DerefTraits() = delete;

    typedef AllocThing *T_;
    typedef AllocThing  Type;

    static inline const Type *Deref(const T_ &ptr) {
        return ptr;
    }
    static inline Type *Deref(T_ &ptr) {
        return ptr;
    }
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
    static void Scan(Scanner &scanner, const T_ &t,
                     const void *start, const void *end)
    {
        scanner(&t, t);
    }

    template <typename Updater>
    static void Update(Updater &updater, T_ &t,
                       const void *start, const void *end)
    {
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
    static_assert(HeapTraits<P>::Specialized ||
                  AllocThingTraits<P>::Specialized,
                  "HeapTraits or AllocThingTraits not specialized for "
                  "underlying type.");
    typedef P * T_;

    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const T_ &t,
                     const void *start, const void *end)
    {
        if (std::less<const void *>()(&t, start))
            return;
        if (std::greater_equal<const void *>()(&t, end))
            return;
        scanner(&t, reinterpret_cast<AllocThing *>(t));
    }

    template <typename Updater>
    static void Update(Updater &updater, T_ &t,
                       const void *start, const void *end)
    {
        if (std::less<const void *>()(&t, start))
            return;
        if (std::greater_equal<const void *>()(&t, end))
            return;
        AllocThing *tx = updater(&t, reinterpret_cast<AllocThing *>(t));
        if (reinterpret_cast<P *>(tx) != t)
            t = reinterpret_cast<P *>(tx);
    }
};



} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__SPECIALIZATIONS_HPP
