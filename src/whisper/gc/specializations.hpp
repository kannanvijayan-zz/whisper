#ifndef WHISPER__GC__SPECIALIZATIONS_HPP
#define WHISPER__GC__SPECIALIZATIONS_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/tracing.hpp"
#include "gc/field.hpp"

#include <functional>

namespace Whisper {


///////////////////////////////////////////////////////////////////////////////
////
//// FieldTraits and TraceTraits specializations for primitive types.
////
///////////////////////////////////////////////////////////////////////////////

#define DEF_PRIM_TRAITS_(type) \
    template <> struct FieldTraits<type> { \
        FieldTraits() = delete; \
        static constexpr bool Specialized = true; \
    }; \
    template <> struct TraceTraits<type> \
        : public UntracedTraceTraits<type> {}

DEF_PRIM_TRAITS_(bool);
DEF_PRIM_TRAITS_(uint8_t);
DEF_PRIM_TRAITS_(uint16_t);
DEF_PRIM_TRAITS_(uint32_t);
DEF_PRIM_TRAITS_(uint64_t);
DEF_PRIM_TRAITS_(int8_t);
DEF_PRIM_TRAITS_(int16_t);
DEF_PRIM_TRAITS_(int32_t);
DEF_PRIM_TRAITS_(int64_t);
DEF_PRIM_TRAITS_(float);
DEF_PRIM_TRAITS_(double);

#undef DEF_PRIM_TRAITS_


///////////////////////////////////////////////////////////////////////////////
////
//// Specialization of StackTraits, FieldTraits and TraceTraits
//// for pointer types.
////
///////////////////////////////////////////////////////////////////////////////

// By default pointers are expected to point to HeapThing structures.
template <typename P>
struct StackTraits<P*>
{
    static_assert(IsHeapThingType<P>(), "P is not a HeapThingType.");
    StackTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr StackFormat Format = StackFormat::HeapPointer;
};

template <typename P>
struct FieldTraits<P*>
{
    static_assert(IsHeapThingType<P>(), "P is not a HeapThingType.");
    FieldTraits() = delete;

    static constexpr bool Specialized = true;
};

template <typename P>
struct DerefTraits<P*>
{
    static_assert(IsHeapThingType<P>(), "P is not a HeapThingType.");
    DerefTraits() = delete;

    typedef P* T_;

    // Regardless of whether pointer value itself is const,
    // the deref type is underlying pointed-to-type without
    // any const modifiers.
    typedef P  Type;
    typedef P  ConstType;

    static inline ConstType* Deref(T_ const& ptr) {
        return ptr;
    }
    static inline Type* Deref(T_& ptr) {
        return ptr;
    }
};


//
// Specialize StackFormatTraits for heap pointers.
//
// This just maps HeapPointer to the type |HeapThing*| for
// tracing.
//
template <>
struct StackFormatTraits<StackFormat::HeapPointer>
{
    StackFormatTraits() = delete;
    typedef HeapThing* Type;
};

//
// Specialize HeapThing* for TraceTraits
//
template <>
struct TraceTraits<HeapThing*>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    typedef HeapThing* T_;

    template <typename Scanner>
    static void Scan(Scanner& scanner, T_ const& t,
                     void const* start, void const* end)
    {
        if (!t)
            return;
        scanner(&t, t);
    }

    template <typename Updater>
    static void Update(Updater& updater, T_& t,
                       void const* start, void const* end)
    {
        if (!t)
            return;
        HeapThing* ht = updater(&t, t);
        if (ht != t)
            t = ht;
    }
};

//
// Specialize other pointers for trace-traits, so that Field<P*>
// specializations can use them.
//
template <typename P>
struct TraceTraits<P*>
{
    // Traced pointers are assumed to be pointers to heap-things by default.
    static_assert(IsHeapThingType<P>(), "P is not a HeapThing type.");
    TraceTraits() = delete;

    static constexpr bool Specialized = true;

    typedef P* T_;

    template <typename Scanner>
    static void Scan(Scanner& scanner, T_ const& t,
                     void const* start, void const* end)
    {
        if (!t)
            return;
        scanner(&t, HeapThing::From(t));
    }

    template <typename Updater>
    static void Update(Updater& updater, T_& t,
                       void const* start, void const* end)
    {
        if (!t)
            return;
        HeapThing* ht = updater(&t, HeapThing::From(t));
        if (ht->to<P>() != t)
            t = ht->to<P>();
    }
};


} // namespace Whisper

#endif // WHISPER__GC__SPECIALIZATIONS_HPP
