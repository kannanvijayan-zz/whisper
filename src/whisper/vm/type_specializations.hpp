#ifndef WHISPER__VM__TYPE_SPECIALIZATIONS_HPP
#define WHISPER__VM__TYPE_SPECIALIZATIONS_HPP

#include "common.hpp"
#include "debug.hpp"
#include "spew.hpp"
#include "gc.hpp"
#include "vm/type.hpp"

#include <new>


//
// GC-Specializations for ValueType
//
namespace Whisper {
namespace GC {


template <>
struct HeapTraits<VM::ValueType>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr AllocFormat Format = AllocFormat::ValueType;

    // All constructor signatures for Array take the length as the
    // first argument.
    template <typename... Args>
    static uint32_t SizeOf(Args... rest) {
        return sizeof(VM::ValueType);
    }
};


template <>
struct StackTraits<VM::ValueType>
{
    StackTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr AllocFormat Format = AllocFormat::ValueType;
};


template <>
struct FieldTraits<VM::ValueType>
{
    FieldTraits() = delete;

    static constexpr bool Specialized = true;
};

template <>
struct AllocFormatTraits<AllocFormat::ValueType>
{
    AllocFormatTraits() = delete;
    typedef VM::ValueType Type;
};

template <>
struct TraceTraits<VM::ValueType>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;

    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::ValueType &type,
                     const void *start, const void *end)
    {
        // For now, no non-primitive types, so nothing to trace.
        // But in general, structure and enum types will be pointers
        // to TypeObjects.
        WH_ASSERT(type.isPrimitive());
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::ValueType &type,
                       const void *start, const void *end)
    {
        // For now, no non-primitive types, so nothing to trace.
        // But in general, structure and enum types will be pointers
        // to TypeObjects.
        WH_ASSERT(type.isPrimitive());
    }
};


} // namespace GC
} // namespace Whisper

WH_VM__DEF_SIMPLE_ARRAY_TRAITS(VM::ValueType, ValueTypeArray);


#endif // WHISPER__VM__TYPE_SPECIALIZATIONS_HPP
