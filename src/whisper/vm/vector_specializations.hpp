#ifndef WHISPER__VM__VECTOR_SPECIALIZATIONS_HPP
#define WHISPER__VM__VECTOR_SPECIALIZATIONS_HPP

#include "vm/core.hpp"

#include <new>

//
// GC-Specializations for Vector
//
namespace Whisper {
namespace GC {


template <typename T>
struct HeapTraits<VM::Vector<T>>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr GC::AllocFormat Format = GC::AllocFormat::Vector;
    static constexpr bool VarSized = false;
};

template <>
struct AllocFormatTraits<AllocFormat::Vector>
{
    AllocFormatTraits() = delete;
    typedef VM::VectorBase Type;
};

template <>
struct TraceTraits<VM::VectorBase>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;

    static constexpr bool IsLeaf = false;

    typedef VM::VectorBase V_;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const V_ &v,
                     const void *start, const void *end)
    {
        v.contents_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, V_ &v,
                       const void *start, const void *end)
    {
        v.contents_.update(updater, start, end);
    }
};

template <>
struct AllocThingTraits<VM::VectorBase>
{
    AllocThingTraits() = delete;
    static constexpr bool Specialized = true;
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__VECTOR_SPECIALIZATIONS_HPP
